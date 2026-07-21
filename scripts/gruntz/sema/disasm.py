"""gruntz.sema.disasm - `gruntz sema disasm`: TARGET (retail, default) /
--base (compiled) / --rich (base + /Z7 source lines) / --diff (base vs target,
rc=1 if differs) / --lite (asm only).

Target side: gruntz.sema.dump_target (delinked bytes + relocs). Base side:
llvm-objdump over the unit's base obj; --rich recompiles the unit `/Z7`
(codegen-neutral CodeView) and interleaves each instruction's source line.
"""
import subprocess
import sys

from gruntz.sema._common import (GEN_NAMES, REPO, call_main, csv_find, die,
                                 flags_for, units)


def _capture(cmd: list) -> str:
    """Run an external disasm producer (llvm-objdump), return stdout (stderr
    passes through)."""
    res = subprocess.run(cmd, cwd=str(REPO), capture_output=True, text=True)
    sys.stderr.write(res.stderr)
    return res.stdout


def target_text(rva: str) -> str:
    from gruntz.core import get_context
    from gruntz.sema import dump_target
    return dump_target.dump_text(get_context(), rva) + "\n"


def base_text(rva: str) -> str:
    """The CURRENT compiled asm: the fn's symbol disassembled out of its unit's
    base obj (what objdiff compares against retail)."""
    try:
        n = int(rva, 16)
    except ValueError:
        die(f"'{rva}' is not a hex RVA (--base needs an RVA)")
    claim = csv_find(GEN_NAMES, n)
    if not claim:
        die("no src claim at this RVA - base disasm needs a reconstructed fn "
            "(check `gruntz sema rva`)")
    obj = REPO / "build" / "objdiff" / "base" / (claim["unit"] + ".obj")
    if not obj.is_file():
        die(f"{obj.relative_to(REPO)} missing - run `gruntz build` first")
    out = _capture(["llvm-objdump", "-dr", "--x86-asm-syntax=intel",
                    f"--disassemble-symbols={claim['name']}", str(obj)])
    return f"{claim['name']}  [{claim['unit']}]\n" + out


_DISASM_ROW = None  # compiled lazily


def lite(text: str) -> str:
    """Only the asm: drop addresses, byte columns, reloc blocks; keep title lines."""
    import re as _re
    global _DISASM_ROW
    if _DISASM_ROW is None:
        _DISASM_ROW = _re.compile(r"^\s*[0-9a-f]+:\s+((?:[0-9a-f]{2}\s)+)\s*(\S.*)$")
    keep = []
    for ln in text.splitlines():
        m = _DISASM_ROW.match(ln)
        if m:
            keep.append("    " + m.group(2).strip())
        elif " @ RVA " in ln or ln.rstrip().endswith(">:") or "  [" in ln[:1]:
            keep.append(ln)
        elif ln.startswith(("CState", "?")) and ln.rstrip().endswith("]"):
            keep.append(ln)
    return "\n".join(keep) + "\n"


def norm(text: str) -> list:
    """Lite + case/whitespace-unify + mask absolute-address immediates for --diff.
    base (llvm-objdump, 'dword ptr') and target (dump_target, 'DWORD PTR') disagree
    on case and spacing - lowercase + collapse runs so only real diffs survive."""
    import re as _re
    # reloc-aware pre-pass (base side only): llvm-objdump -dr emits IMAGE_REL_ lines
    # after the owning insn - mask that insn's placeholder imm (often 0x0) as <addr>
    raw = text.splitlines()
    for i, ln in enumerate(raw):
        if "IMAGE_REL_I386_" not in ln:
            continue
        for j in range(i - 1, -1, -1):
            if "IMAGE_REL_I386_" in raw[j] or not _re.search(r"0x[0-9a-f]+", raw[j]):
                continue
            m = _re.search(r":\s+(?:[0-9a-f]{2} )+\s*([a-z]\w*)", raw[j])
            if m and _re.fullmatch(r"call|jmp|j[a-z]{1,2}|loop\w*", m.group(1)):
                break  # rel32 target - the <tgt> rule owns these
            # DIR32 on a memory disp32 (pure-absolute bracket) beats an imm32 guess
            raw[j], n = _re.subn(r"\[0x[0-9a-f]+\]", "[<addr>]", raw[j], count=1)
            if not n:
                raw[j] = _re.sub(r"0x[0-9a-f]+(?=[^x]*$)", "<addr>", raw[j], count=1)
            break
    text = "\n".join(raw)
    lines = []
    for ln in lite(text).splitlines():
        if not ln.startswith("    "):
            continue  # instructions only in the diff body
        ln = _re.sub(r"[ \t]+", " ", ln.strip().lower())
        if _re.fullmatch(r"(?:[0-9a-f]{2} )*[0-9a-f]{2}", ln):
            continue  # byte-dump continuation of a long insn (dump_target wrap)
        ln = _re.sub(r"0x[0-9a-f]{6,8}\b", "<addr>", ln)
        ln = _re.sub(r" ?([,+*]) ?", r"\1", ln)   # 'ebp, ecx'/'esp + 0xc' -> tight
        ln = ln.replace("ds:", "")                 # default-segment prefix (dump_target)
        ln = _re.sub(r"\bptr (<addr>|0x[0-9a-f]+)(?![\w\]])", r"ptr [\1]", ln)  # bare -> bracketed
        ln = _re.sub(r"\[(0x[0-9a-f]+|<addr>)\]", "[<addr>]", ln)  # absolute mem ref
        # bare <addr> as a mov-class operand is a memory ref (dump_target drops brackets)
        ln = _re.sub(r"(?<=[ ,])<addr>(?=,|$)", "[<addr>]",
                     ln) if not ln.startswith(("push", "j", "call", "loop")) else ln
        ln = _re.sub(r"(dword|word|byte) ptr \[<addr>\]", "[<addr>]", ln)
        # direct jump/call targets: base prints rel+symbol, target prints absolute
        ln = _re.sub(r"^((?:j[a-z]{1,3}|call|loop\w*) )(0x[0-9a-f]+|<addr>)( <[^>]*>)?$",
                     r"\1<tgt>", ln)
        lines.append(ln)
    while lines and lines[-1] == "nop":
        lines.pop()  # COMDAT alignment padding (base only; absent in delinked target)
    return lines


def _debug_obj_for(unit: str, source: str, flags: list):
    """build/debug/<unit>.obj compiled `<flags> /Z7` (codegen-neutral CodeView),
    cached on source mtime - same artifact harvest_locals.py builds. Path or None."""
    obj = REPO / "build" / "debug" / f"{unit}.obj"
    src = REPO / source
    if not src.is_file():
        return None
    if obj.is_file() and obj.stat().st_mtime >= src.stat().st_mtime:
        return obj  # fresh
    obj.parent.mkdir(parents=True, exist_ok=True)
    # in-process (sema spawns no python child); cc_wrap itself runs `wine cl`.
    rc = call_main("gruntz.build.cc_wrap",
                   ["--out", str(obj), "--src", str(src), "--", *flags, "/Z7"])
    if rc != 0 or not obj.is_file():
        sys.stderr.write(f"[--rich] /Z7 compile of {unit} failed (wine/cl missing?); "
                         f"showing bare asm.\n")
        return None
    return obj


def rich(rva: str, want_lite: bool) -> str:
    """BASE disasm interleaved with the /Z7 CodeView source lines it came from:
    each mapped code offset prints its source statement (flush-left) above the
    instruction(s) it lowered to. Shows which statements survive /O2 and which
    got folded (a run of instructions under one line = merged; a source line
    that never appears = optimized away)."""
    try:
        n = int(rva, 16)
    except ValueError:
        die(f"'{rva}' is not a hex RVA (--rich needs an RVA)")
    claim = csv_find(GEN_NAMES, n)
    if not claim:
        die("no src claim at this RVA - --rich needs a reconstructed fn "
            "(check `gruntz sema rva`)")
    unit, name = claim["unit"], claim["name"]
    udef = next((u for u in units() if u.get("unit") == unit), None)
    source = (udef or {}).get("source", "")
    # line map from the (fresh) /Z7 debug obj; degrade to bare disasm if absent.
    linemap, bf = {}, None
    if udef and source.startswith("src/"):
        dbg = _debug_obj_for(unit, source, flags_for(udef))
        if dbg is not None:
            from gruntz.build import codeview
            info = codeview.parse_lines(str(dbg)).get(name)
            if info:
                linemap, bf = info["lines"], info["bf"]
    src_path = REPO / source
    src_lines = (src_path.read_text(errors="replace").splitlines()
                 if src_path.is_file() else None)

    def src_text(lineno: int) -> str:
        if src_lines and 1 <= lineno <= len(src_lines):
            return src_lines[lineno - 1].rstrip() or f"{source}:{lineno}"
        return f"{source}:{lineno}"

    try:
        size = int(claim.get("size", "0") or "0", 16)
    except ValueError:
        size = 0
    import re as _re
    row = _re.compile(r"^(\s*)([0-9a-f]+):\s+((?:[0-9a-f]{2}\s)+)\s*(\S.*)$")
    out = [f"{name}  [{unit}]",
           f"('NNNNN| code' = {source} source line; indented = asm)"]
    if bf is None:
        out[-1] = "(no /Z7 line info for this fn - bare asm)"
    current = None
    for ln in base_text(rva).splitlines():
        m = row.match(ln)
        if not m:
            if "IMAGE_REL" in ln and not want_lite:
                out.append(ln)  # reloc annotation - attaches to the instr above
            continue  # else drop llvm-objdump boilerplate; keep the rich view clean
        off = int(m.group(2), 16)
        if size and off >= size:
            break  # trailing COMDAT padding (nops) past the function
        want = linemap.get(off, bf if current is None else current)
        if want is not None and want != current:
            # 'NNN|' gutter keeps source unmistakable from asm - indented C++
            # and --lite's bare asm are otherwise visually identical
            out.append(f"{want:5d}| {src_text(want)}")
            current = want
        out.append("      " + m.group(4).strip() if want_lite else ln)
    return "\n".join(out) + "\n"


def run(args) -> None:
    if getattr(args, "rich", False):
        if args.target:
            die("--rich is BASE-only (retail GRUNTZ.EXE carries no line info); "
                "drop --target")
        if args.diff:
            die("--rich does not combine with --diff (rich is a single-side view)")
        if not args.base:
            print("[--rich implies --base: source lines come from the /Z7 debug "
                  "build of your compiled obj]")
        print(rich(args.rva, args.lite), end="")
        sys.exit(0)
    if args.diff:
        import difflib
        base = norm(base_text(args.rva))
        tgt = norm(target_text(args.rva))
        if base == tgt:
            print(f"identical asm ({len(tgt)} instruction(s); addresses/relocs masked)")
            sys.exit(0)
        print(f"[diff: BASE (compiled) vs TARGET (retail) @ {args.rva}; "
              "addresses masked as <addr>]")
        print("[caveat: base prints reloc-site immediates as their placeholder "
              "(e.g. 'push 0x0') where target shows the resolved '<addr>' - such "
              "lone pairs are usually NOT real diffs; objdiff is reloc-aware truth]")
        for ln in difflib.unified_diff(base, tgt, "base", "target", lineterm=""):
            print(ln)
        sys.exit(1)
    if args.base:
        print("[disasm source: BASE - your compiled obj (build/objdiff/base)]")
        text = base_text(args.rva)
    else:
        print("[disasm source: TARGET - retail GRUNTZ.EXE (delinked bytes + relocs)]")
        text = target_text(args.rva)
    print(lite(text) if args.lite else text, end="")
    sys.exit(0)
