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


_JCC = ("jmp", "je", "jne", "jz", "jnz", "ja", "jae", "jb", "jbe", "jg", "jge",
        "jl", "jle", "js", "jns", "jo", "jno", "jp", "jnp", "jcxz", "jecxz")


def blocks(text: str) -> str:
    """IDA-style basic-block view of one side's disasm: split at branch targets,
    show each block's in-edges, branch destinations as block labels, back-edges
    marked as loops and the shared ret tail(s) called out. Structure recovery
    aid: 'jcc -> @tail' runs = a nested single-exit source shape."""
    import re as _re
    row = _re.compile(r"^\s*([0-9a-f]+):\s+(?:[0-9a-f]{2}\s)+\s*(\S.*?)\s*$")
    insns = []          # (addr, text)
    for ln in text.splitlines():
        m = row.match(ln)
        if not m:
            continue
        t = _re.sub(r"[ \t]+", " ", m.group(2))
        if _re.fullmatch(r"(?:[0-9a-f]{2} ?)+", t):
            continue    # byte-wrap continuation line
        insns.append((int(m.group(1), 16), t))
    if not insns:
        return "(no instruction rows found)\n"
    addrs = {a for a, _ in insns}
    lo, hi = insns[0][0], insns[-1][0]

    def branch_target(t: str):
        m = _re.match(r"(\w+) (0x[0-9a-f]+)", t)
        if m and (m.group(1) in _JCC or m.group(1).startswith("loop")):
            tgt = int(m.group(2), 16)
            return tgt if lo <= tgt <= hi and tgt in addrs else None
        return None

    leaders, edges = {lo}, {}   # edges: src addr -> [dst addrs]
    for i, (a, t) in enumerate(insns):
        op = t.split()[0]
        tgt = branch_target(t)
        nxt = insns[i + 1][0] if i + 1 < len(insns) else None
        if tgt is not None:
            leaders.add(tgt)
            edges.setdefault(a, []).append(tgt)
            if nxt and op != "jmp":
                leaders.add(nxt)
                edges.setdefault(a, []).append(nxt)
        elif op == "jmp" or op.startswith("ret"):
            if nxt:
                leaders.add(nxt)
        elif op in _JCC and nxt:   # jcc with out-of-range/unparsed target
            leaders.add(nxt)
    order = sorted(leaders)
    blk_of = {}
    for a, _ in insns:
        while order and blk_of.get(a) is None:
            import bisect
            blk_of[a] = order[bisect.bisect_right(order, a) - 1]
            break
    preds = {}
    for i, (a, t) in enumerate(insns):
        for d in edges.get(a, []):
            preds.setdefault(d, []).append(a)
        # fallthrough into a leader from a non-branching insn
        nxt = insns[i + 1][0] if i + 1 < len(insns) else None
        op = t.split()[0]
        if (nxt in leaders and branch_target(t) is None
                and op != "jmp" and not op.startswith("ret")):
            preds.setdefault(nxt, []).append(a)

    out = []
    wid = len(f"{hi:x}")
    for i, (a, t) in enumerate(insns):
        if a in leaders:
            ins = preds.get(a, [])
            tag = ""
            if any(p > a for p in ins):
                tag += "  <=== LOOP HEAD"
            first = t.split()[0]
            if first.startswith("ret") or (i + 1 < len(insns)
                                           and len(ins) > 2
                                           and any(x[1].startswith("ret")
                                                   for x in insns[i:i + 8])):
                if len(ins) > 2:
                    tag += f"  <=== COMMON TAIL ({len(ins)} in-edges)"
            src = ", ".join(f"@{p:x}" for p in sorted(ins)) or "entry"
            out.append("")
            out.append(f"block @{a:0{wid}x}:   in: {src}{tag}")
        tgt = branch_target(t)
        arrow = ""
        if tgt is not None:
            arrow = f"   -> @{tgt:0{wid}x}" + ("  ^loop" if tgt <= a else "")
        out.append(f"  {a:0{wid}x}:  {t}{arrow}")
    return "\n".join(out) + "\n"


def _mask_insn(ln: str) -> str:
    """One instruction -> the same normalized/masked spelling `norm` produces
    (case/space unify, absolute addresses -> <addr>, branch targets -> <tgt>)."""
    import re as _re
    ln = _re.sub(r"[ \t]+", " ", ln.strip().lower())
    ln = _re.sub(r"0x[0-9a-f]{6,8}\b", "<addr>", ln)
    ln = _re.sub(r" ?([,+*]) ?", r"\1", ln)
    ln = ln.replace("ds:", "")
    ln = _re.sub(r"\bptr (<addr>|0x[0-9a-f]+)(?![\w\]])", r"ptr [\1]", ln)
    ln = _re.sub(r"\[(0x[0-9a-f]+|<addr>)\]", "[<addr>]", ln)
    if not ln.startswith(("push", "j", "call", "loop")):
        ln = _re.sub(r"(?<=[ ,])<addr>(?=,|$)", "[<addr>]", ln)
    ln = _re.sub(r"(dword|word|byte) ptr \[<addr>\]", "[<addr>]", ln)
    ln = _re.sub(r"^((?:j[a-z]{1,3}|call|loop\w*) )(0x[0-9a-f]+|<addr>)( <[^>]*>)?$",
                 r"\1<tgt>", ln)
    return ln


def _cfg(text: str):
    """Parse one side's disasm into an ordered CFG: [(addr, [masked insns],
    term)] where term describes the block's exit as block INDICES so base
    (obj offsets) and target (RVAs) become comparable: ('jcc B4 | fall B2'),
    ('jmp B9'), ('ret'), ('fall B3')."""
    import re as _re
    row = _re.compile(r"^\s*([0-9a-f]+):\s+(?:[0-9a-f]{2}\s)+\s*(\S.*?)\s*$")
    insns = []
    for ln in text.splitlines():
        m = row.match(ln)
        if not m:
            continue
        t = _re.sub(r"[ \t]+", " ", m.group(2))
        if _re.fullmatch(r"(?:[0-9a-f]{2} ?)+", t):
            continue
        insns.append((int(m.group(1), 16), t))
    if not insns:
        return []
    addrs = {a for a, _ in insns}
    lo, hi = insns[0][0], insns[-1][0]

    def btgt(t):
        m = _re.match(r"(\w+) (0x[0-9a-f]+)", t)
        if m and (m.group(1) in _JCC or m.group(1).startswith("loop")):
            tgt = int(m.group(2), 16)
            return tgt if lo <= tgt <= hi and tgt in addrs else None
        return None

    leaders = {lo}
    for i, (a, t) in enumerate(insns):
        op, tgt = t.split()[0], btgt(t)
        nxt = insns[i + 1][0] if i + 1 < len(insns) else None
        if tgt is not None:
            leaders.add(tgt)
            if nxt and op != "jmp":
                leaders.add(nxt)
        elif (op == "jmp" or op.startswith("ret") or op in _JCC) and nxt:
            leaders.add(nxt)
    order = sorted(leaders)
    bidx = {a: i for i, a in enumerate(order)}

    def blk_of(a):
        import bisect
        return bidx[order[bisect.bisect_right(order, a) - 1]]

    blocks = [[a, [], None] for a in order]
    for i, (a, t) in enumerate(insns):
        b = blocks[blk_of(a)]
        op, tgt = t.split()[0], btgt(t)
        nxt = insns[i + 1][0] if i + 1 < len(insns) else None
        last_of_block = nxt is None or nxt in bidx
        b[1].append(_mask_insn(t))
        if not last_of_block:
            continue
        if tgt is not None:
            dst = f"B{blk_of(tgt)}" + ("^" if tgt <= a else "")
            b[2] = (f"jmp {dst}" if op == "jmp"
                    else f"jcc {dst} | fall B{blk_of(nxt)}" if nxt is not None
                    else f"jcc {dst}")
        elif op.startswith("ret"):
            b[2] = "ret"
        elif op == "jmp":
            b[2] = "jmp <ext>"
        else:
            b[2] = f"fall B{blk_of(nxt)}" if nxt is not None else "end"
    return [(a, body, term) for a, body, term in blocks]


def blocks_diff(base_raw: str, tgt_raw: str) -> str:
    """Block-aligned CFG diff: says whether the FLOW SHAPE matches and which
    blocks' bodies differ. Blocks become comparable units (branch targets
    rewritten to block indices); equal blocks align via SequenceMatcher,
    replace-runs pair up index-wise (INSERT/DELETE = real flow divergence)."""
    import difflib
    b, t = _cfg(base_raw), _cfg(tgt_raw)
    bs = ["\n".join(body + [term or ""]) for _, body, term in b]
    ts = ["\n".join(body + [term or ""]) for _, body, term in t]
    bflow = [x[2] or "" for x in b]
    tflow = [x[2] or "" for x in t]
    out = [f"[block diff: base {len(b)} blocks vs target {len(t)} blocks; "
           f"flow {'SAME' if bflow == tflow else 'DIFFERS'}]"]
    if bflow != tflow:
        # where does the CFG SHAPE first split?  Compare the branch KIND
        # (jcc/jmp/ret/fall) + direction, not absolute block indices - a
        # single inserted block shifts every later index without changing
        # the skeleton.
        def kind(term, at):
            import re as _re
            parts = []
            for tok in _re.findall(r"(jcc|jmp|ret|fall|end)( B(\d+)(\^?))?",
                                   term or ""):
                if tok[2]:
                    parts.append(tok[0] + ("^" if tok[3] else
                                           ">" if int(tok[2]) > at else "<"))
                else:
                    parts.append(tok[0])
            return " ".join(parts)
        bk = [kind(x, i) for i, x in enumerate(bflow)]
        tk = [kind(x, i) for i, x in enumerate(tflow)]
        k = next((i for i, (x, y) in enumerate(zip(bk, tk)) if x != y), None)
        if k is not None:
            out.append(f"[skeleton diverges at B{k}: base [{bflow[k]}]  vs  "
                       f"target [{tflow[k]}]  (branch kinds identical before)]")
        elif len(bk) != len(tk):
            out.append(f"[same branch skeleton for the shared {min(len(bk), len(tk))} "
                       f"blocks; {'base' if len(bk) > len(tk) else 'target'} has "
                       f"{abs(len(bk) - len(tk))} extra block(s)]")
        else:
            out.append("[same branch-kind skeleton; only block-index targets differ "
                       "(an inserted/moved block shifts later indices)]")
    sm = difflib.SequenceMatcher(a=bs, b=ts, autojunk=False)
    ndiff = 0
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag == "equal":
            for k in range(i2 - i1):
                bi, tj = i1 + k, j1 + k
                out.append(f"  B{tj} @{b[bi][0]:x}/@{t[tj][0]:x}  == "
                           f"({len(t[tj][1])} insns)  [{t[tj][2]}]")
            continue
        for k in range(max(i2 - i1, j2 - j1)):
            bi = i1 + k if i1 + k < i2 else None
            tj = j1 + k if j1 + k < j2 else None
            ndiff += 1
            if bi is not None and tj is not None:
                out.append(f"  B{tj} @{b[bi][0]:x}/@{t[tj][0]:x}  DIFFERS:")
                for ln in difflib.unified_diff(
                        b[bi][1] + [b[bi][2] or ""],
                        t[tj][1] + [t[tj][2] or ""], lineterm="", n=2):
                    if ln.startswith(("---", "+++", "@@")):
                        continue
                    out.append("      " + ln)
            elif bi is not None:
                out.append(f"  --  @{b[bi][0]:x}  BASE-ONLY block "
                           f"({len(b[bi][1])} insns)  [{b[bi][2]}]:")
                for ln in b[bi][1][:6]:
                    out.append(f"      -{ln}")
            else:
                out.append(f"  B{tj} @{t[tj][0]:x}  TARGET-ONLY block "
                           f"({len(t[tj][1])} insns)  [{t[tj][2]}]:")
                for ln in t[tj][1][:6]:
                    out.append(f"      +{ln}")
    out.append(f"[{ndiff} block(s) differ]" if ndiff
               else "[all aligned blocks identical]")
    return "\n".join(out) + "\n"


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
    rc = call_main("gruntz.core.cc_wrap",
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
            from gruntz.core import codeview
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
    if getattr(args, "blocks", False):
        if args.diff:
            print(f"[block diff: BASE (compiled) vs TARGET (retail) @ {args.rva}]")
            out = blocks_diff(base_text(args.rva), target_text(args.rva))
            print(out, end="")
            sys.exit(0 if "flow SAME]" in out and "0 block" in out else 1)
        side = "BASE (compiled)" if args.base else "TARGET (retail)"
        text = base_text(args.rva) if args.base else target_text(args.rva)
        print(f"[basic blocks: {side} @ {args.rva}]")
        print(blocks(text), end="")
        sys.exit(0)
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
