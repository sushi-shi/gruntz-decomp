"""gruntz.walls.patterns - image-wide sweeps for compiler mechanisms that
closed a wall once.

Each sweep reads every unit's normalized comparison pair (the bodies objdiff
scored: build/objdiff/compare-new/{base,target}), joins it with
config/match_baseline.tsv (MAX/HIST) and the unit's profile in
config/units.toml, and prints a scored candidate list. Only functions with a
ledger row are read: an unscored name can pair two unrelated bodies. Nothing
here edits source or profiles.

  gruntz walls patterns cc [--unit U] [--limit N] [--json]
        Calling conventions. Per function: the bytes each return pops
        (`ret N`) and whether ecx/edx are live on entry, ours vs retail. Per
        callee: the stack each paired call site releases right after the call
        (`add esp,N` / `pop ecx`), ours vs retail. A disagreement means the
        declaration's convention (an explicit keyword or WINAPI-style macro,
        static vs non-static member, a member vs a free function) differs
        from the one retail compiled. Rows list explicit spellings in the
        source. cl 5.0 at /O2 also defers and merges the cleanup of
        consecutive calls, so a callee with both agreeing and differing sites
        is usually a call-schedule residue, not a convention.

  gruntz walls patterns flags [--unit U] [--limit N] [--json]
        x87 signatures per profile: float temporaries stored to a frame slot
        and reloaded at once (`fstp [ebp-x]; fld [ebp-x]`, the /Op shape),
        inline `fabs` vs calls to _fabs, and __ftol calls. Every profile is
        /O2 without /Op; a body set where retail stores and reloads where we
        do not is the evidence a profile control (--try) can test.
  gruntz walls patterns flags --try UNIT[,UNIT] | --profile NAME
                              [--drop FLAG] [--add FLAG] [--jobs N]
        A/B control: compile the units with the changed profile into
        build/patterns/flags/, normalize them against the delinked targets
        the build uses, score them with objdiff, and print every function
        whose score moves against the current report.

  gruntz walls patterns inline [--unit U] [--limit N] [--json]
        Call-set asymmetries aggregated per callee: calls we emit where the
        paired retail body has fewer (retail expanded the callee) and calls
        retail emits where ours has fewer (we expanded it). /O2 is /Ob1 on
        cl 5.0: declared-inline bodies and instantiated template members
        expand (gruntz.walls.inline_model), so a callee whose asymmetry
        recurs across callers points at its declaration, not at one caller.

  gruntz walls patterns loops [--unit U] [--limit N] [--json]
        Loop shapes of non-exact functions: unconditional and conditional
        back edges, ours vs retail. A rotated loop closes with a conditional
        back edge after one test before entry; an unrotated one jumps back to
        its head (for/while vs do-while, or a condition the optimizer could
        not duplicate).

  gruntz walls patterns ctors [--unit U] [--limit N] [--json]
        Store order. Non-exact functions where both sides emit the same
        stores through one base register in another order, ranked by the
        share of the difference those blocks explain. Usual sources: member
        initializers vs body assignments (a vptr store moves relative to
        them), default arguments vs a separate constructor, another
        constructor expanded at the site, or a later store to the same member
        that kills the constructor's.

`gap` is 100 - MAX; rows sort by evidence weight, then gap. `--limit 0`
prints every row.
"""

from __future__ import annotations

import argparse
import json
import re
import shutil
import subprocess
import sys
from collections import Counter, defaultdict
from dataclasses import dataclass

from gruntz.core.paths import BUILD, REPO
from gruntz.walls.pairscan import COMPGEN, DIR32, REL32, canon

CALL = 0xE8
#: Registers that carry arguments: ecx (thiscall, fastcall first), edx
#: (fastcall second).
_REG_FAMILY = {"ecx": "ecx", "cx": "ecx", "cl": "ecx", "ch": "ecx",
               "edx": "edx", "dx": "edx", "dl": "edx", "dh": "edx"}
_REG = re.compile(r"\b(ecx|cx|cl|ch|edx|dx|dl|dh)\b")
CONVENTIONS = ("__cdecl", "__stdcall", "__fastcall", "__thiscall")
#: Separates bodies in one batched objdump run: longer than any instruction,
#: so a body that ends mid-instruction cannot desynchronize the next one.
_SEPARATOR = b"\x90" * 16
WORK = BUILD / "patterns"


# --- pairing ---------------------------------------------------------------------

@dataclass
class Function:
    unit: str
    name: str
    profile: str
    flags: tuple
    ours: bytes
    ours_relocs: dict
    retail: bytes
    retail_relocs: dict
    best: float
    hist: float
    rva: int | None
    ours_insns: dict | None = None
    retail_insns: dict | None = None

    @property
    def gap(self) -> float:
        return round(100.0 - self.best, 4)


def ob1(flags) -> bool:
    """True when only declared-inline bodies (and, on cl 5.0, instantiated
    template members) expand: /O1 or /O2 without /Ob2."""
    flags = set(flags)
    return bool(flags & {"/O1", "/O2"}) and "/Ob2" not in flags


def unit_profiles() -> dict[str, tuple[str, tuple]]:
    """{unit: (profile name, flags)} from config/units.toml."""
    from gruntz import manifest
    profiles = manifest.flag_profiles()
    return {u["unit"]: (u["flags"], tuple(profiles.get(u["flags"], ())))
            for u in manifest.units()}


def _window(obj, secnum: int, lo: int, hi: int) -> tuple[bytes, dict]:
    """(body, {offset: (canonical referent, type, addend)}) of one window."""
    from gruntz.walls.pairscan import fn_relocs, function_body
    body = function_body(obj, secnum, lo, hi)
    relocs = {off - lo: (canon(name), typ, addend)
              for off, name, typ, addend in fn_relocs(obj, secnum, lo, hi)
              if off - lo < len(body)}
    return body, relocs


def functions(units: set[str] | None = None):
    """Every ledger-scored function present in both normalized objects of
    its unit, decoded in one objdump run per unit side."""
    from gruntz.delink.coffx import Obj
    from gruntz.verify.baseline import load as load_ledger
    from gruntz.walls import pairscan
    ledger = load_ledger()
    profiles = unit_profiles()
    for unit, (base, target) in sorted(pairscan.require_pairs(units).items()):
        try:
            bobj, tobj = Obj(base), Obj(target)
        except (ValueError, OSError):
            continue
        bf, tf = pairscan.functions(bobj), pairscan.functions(tobj)
        profile, flags = profiles.get(unit, ("?", ()))
        found = []
        for name in sorted(bf.keys() & tf.keys()):
            row = ledger.get((unit, name))
            if row is None or COMPGEN.match(name):
                continue
            ours, ours_relocs = _window(bobj, *bf[name])
            retail, retail_relocs = _window(tobj, *tf[name])
            found.append(Function(unit, name, profile, flags, ours, ours_relocs,
                                  retail, retail_relocs, row["best"], row["hist"],
                                  row["addr"]))
        if found:
            ours = decode_many([(f.ours, f.ours_relocs, f.name) for f in found])
            theirs = decode_many([(f.retail, f.retail_relocs, f.name) for f in found])
            for f, a, b in zip(found, ours, theirs):
                f.ours_insns, f.retail_insns = a, b
        yield from found


# --- disassembly -----------------------------------------------------------------

@dataclass(frozen=True)
class Insn:
    at: int
    size: int
    mnemonic: str
    ops: str


def _objdump_lines(blob: bytes):
    """(offset, mnemonic, operands) for every line of `blob`; the mnemonic
    is None where objdump could not decode an instruction."""
    from gruntz.tool import objdump
    for line in objdump.disassemble(blob).splitlines():
        if ":\t" not in line:
            continue
        head, rest = line.split(":\t", 1)
        parts = rest.split("\t")
        if len(parts) < 2 or not parts[-1].strip():
            continue
        try:
            at = int(head.strip(), 16)
        except ValueError:
            continue
        text = parts[-1].strip()
        if text.startswith((".", "(bad)")):
            yield at, None, ""           # a truncated or undecodable byte
            continue
        mnemonic, _, ops = text.partition(" ")
        ops = ops.strip()
        if mnemonic in ("rep", "repz", "repnz", "repe", "repne", "lock") and ops:
            nxt, _, ops = ops.partition(" ")
            mnemonic, ops = f"{mnemonic} {nxt}", ops.strip()
        yield at, mnemonic, ops


def linear(blob: bytes) -> dict[int, Insn]:
    """{offset: Insn} of a linear sweep over one body."""
    rows = list(_objdump_lines(blob))
    out = {}
    for i, (at, mnemonic, ops) in enumerate(rows):
        end = rows[i + 1][0] if i + 1 < len(rows) else len(blob)
        if mnemonic is not None:
            out[at] = Insn(at, end - at, mnemonic, ops)
    return out


def branch_target(insn: Insn) -> int | None:
    if not (insn.mnemonic.startswith("j") or insn.mnemonic.startswith("loop")):
        return None
    return int(insn.ops, 16) if re.fullmatch(r"0x[0-9a-f]+", insn.ops) else None


def _terminal(insn: Insn) -> bool:
    return insn.mnemonic.startswith("ret") or insn.mnemonic in ("jmp", "int3", "hlt")


def reachable(insns: dict[int, Insn], roots) -> dict[int, Insn]:
    """The instructions reached from `roots` through fall-through and direct
    branches: switch tables and padding after the code are never kept."""
    out, work = {}, list(roots)
    while work:
        at = work.pop()
        while at in insns and at not in out:
            insn = insns[at]
            out[at] = insn
            target = branch_target(insn)
            if target is not None:
                work.append(target)
            if _terminal(insn):
                break
            at += insn.size
    return out


def _self_entries(relocs: dict, own: str, size: int) -> dict[int, int]:
    """{site: addend} of DIR32 relocations naming the function itself: jump
    table entries, and the table operands of the instructions that index
    them."""
    me = canon(own)
    return {off: addend for off, (name, typ, addend) in relocs.items()
            if typ == DIR32 and name == me and 0 <= addend < size}


def walk(insns: dict[int, Insn], relocs: dict | None = None,
         own: str | None = None, size: int = 0) -> dict[int, Insn]:
    """The code of one body: reached from offset 0 and from every case target
    of the function's own jump tables. cl 5.0 emits every table after the
    code, so the code ends at the lowest table a reached instruction indexes
    (its operand is a forward self-relocation); entries are the
    self-relocations past that point."""
    entries = _self_entries(relocs, own, size) if relocs and own else {}
    out = reachable(insns, {0})
    covered = {at + k for at, insn in out.items() for k in range(insn.size)}
    tables = [addend for site, addend in entries.items()
              if site in covered and addend > site]
    if not tables:
        return out
    end = min(tables)
    out.update(reachable(insns, {addend for site, addend in entries.items()
                                 if site >= end and addend < end}))
    return {at: insn for at, insn in out.items() if at < end}


def decode(body: bytes, relocs: dict | None = None, own: str | None = None) -> dict[int, Insn]:
    """{offset: Insn} of one body's code (see `walk`)."""
    return walk(linear(body), relocs, own, len(body))


def decode_many(items: list[tuple[bytes, dict, str]]) -> list[dict[int, Insn]]:
    """`decode` over many bodies with one objdump run."""
    blob, starts = bytearray(), []
    for body, _relocs, _own in items:
        starts.append(len(blob))
        blob += body + _SEPARATOR
    lines = list(_objdump_lines(bytes(blob)))
    out = []
    index = 0
    for (body, relocs, own), start in zip(items, starts):
        while index < len(lines) and lines[index][0] < start:
            index += 1
        rows = []
        while index < len(lines) and lines[index][0] < start + len(body):
            rows.append(lines[index])
            index += 1
        following = lines[index][0] if index < len(lines) else len(blob)
        insns = {}
        for i, (at, mnemonic, ops) in enumerate(rows):
            end = rows[i + 1][0] if i + 1 < len(rows) else following
            if end > start + len(body):
                break                    # runs into the separator: truncated
            if mnemonic is None:
                continue
            if mnemonic.startswith(("j", "loop", "call")) \
                    and re.fullmatch(r"0x[0-9a-f]+", ops):
                ops = f"0x{int(ops, 16) - start:x}"
            insns[at - start] = Insn(at - start, end - at, mnemonic, ops)
        out.append(walk(insns, relocs, own, len(body)))
    return out


def _families(text: str) -> set[str]:
    return {_REG_FAMILY[m] for m in _REG.findall(text)}


def _operands(ops: str) -> list[str]:
    return [o.strip() for o in ops.split(",")] if ops else []


def use_def(insn: Insn) -> tuple[set, set]:
    """(argument registers read, argument registers written) by one
    instruction, as register families; memory addressing is always a read."""
    m, ops = insn.mnemonic, _operands(insn.ops)
    if m == "call":
        return set(), {"ecx", "edx"}
    if m == "jecxz":
        return {"ecx"}, set()
    if m.startswith(("rep", "loop")):
        return {"ecx"}, {"ecx"}
    if m.startswith("ret") or m in ("nop", "int3", "hlt") or m.startswith("j"):
        return (_families(insn.ops) if m == "jmp" else set()), set()
    if m == "push" and insn.ops in ("ecx", "edx"):
        return set(), set()            # `push ecx` reserves a frame slot
    if m == "cdq":
        return set(), {"edx"}
    if m in ("mul", "div", "idiv") or (m == "imul" and len(ops) == 1):
        use = _families(insn.ops) | ({"edx"} if m in ("div", "idiv") else set())
        return use, {"edx"}
    if m in ("in", "out"):
        return _families(insn.ops), set()
    use = set()
    for op in ops:
        for inner in re.findall(r"\[([^\]]*)\]", op):
            use |= _families(inner)
    dest = ops[0] if ops else ""
    dest_reg = _REG_FAMILY.get(dest) if "[" not in dest else None
    sources = ops[1:] if len(ops) > 1 else []
    for op in sources:
        if "[" not in op:
            use |= _families(op)
    if dest_reg is None:
        return use | (_families(dest) if "[" not in dest else set()), set()
    if m in ("xor", "sub", "sbb") and len(ops) == 2 and ops[0] == ops[1]:
        return use - {dest_reg}, {dest_reg}
    if m in ("mov", "movzx", "movsx", "lea", "pop") or m.startswith("set") \
            or (m == "imul" and len(ops) == 3):
        return use, {dest_reg}
    if m in ("cmp", "test", "push", "bt"):
        return use | {dest_reg}, set()
    if m == "xchg":
        return use | {dest_reg}, {dest_reg} | _families(",".join(sources))
    return use | {dest_reg}, {dest_reg}


def _successors(insns: dict[int, Insn], at: int) -> list[int]:
    insn = insns[at]
    if insn.mnemonic.startswith("ret") or insn.mnemonic in ("int3", "hlt"):
        return []
    target = branch_target(insn)
    if insn.mnemonic == "jmp":
        return [target] if target in insns else []
    succ = [at + insn.size] if at + insn.size in insns else []
    if target is not None and target in insns:
        succ.append(target)
    return succ


def signature(insns: dict[int, Insn]) -> tuple[frozenset, frozenset]:
    """(bytes popped by each return, argument registers live on entry)."""
    pops = {int(i.ops, 16) if i.ops else 0
            for i in insns.values() if i.mnemonic.startswith("ret")}
    live: dict[int, set] = {at: set() for at in insns}
    order = sorted(insns, reverse=True)
    changed = True
    while changed:
        changed = False
        for at in order:
            use, defs = use_def(insns[at])
            out = set().union(*(live[s] for s in _successors(insns, at)))
            new = use | (out - defs)
            if new != live[at]:
                live[at] = new
                changed = True
    return frozenset(pops), frozenset(live.get(0, set()))


# --- names -----------------------------------------------------------------------

def undname(names) -> dict[str, str]:
    """{mangled: demangled text} for C++ names, one batched llvm-undname."""
    cpp = sorted({n for n in names if n.startswith("?")})
    if not cpp or shutil.which("llvm-undname") is None:
        return {}
    proc = subprocess.run(["llvm-undname"], input="\n".join(cpp) + "\n",
                          capture_output=True, text=True)
    out = {}
    for block in proc.stdout.split("\n\n"):
        lines = block.splitlines()
        if len(lines) >= 2 and not lines[1].startswith("error"):
            out[lines[0]] = lines[1]
    return out


def convention(name: str, demangled: dict) -> str | None:
    text = demangled.get(name)
    if text:
        return next((cc for cc in CONVENTIONS if cc in text), None)
    if name.startswith("@") and re.search(r"@\d+$", name):
        return "__fastcall"
    if name.startswith("_") and re.search(r"@\d+$", name):
        return "__stdcall"
    if name.startswith("_"):
        return "__cdecl"
    return None


def qualified(text: str) -> str:
    """'public: int __thiscall zDArray<CString>::Get(int)' ->
    'zDArray<CString>::Get': the declarator between the calling convention
    and its parameter list (template arguments kept whole)."""
    cc = re.search(r"\b(?:__cdecl|__stdcall|__fastcall|__thiscall|__pascal)\s+", text)
    if cc is None:
        return text
    depth = 0
    for i in range(cc.end(), len(text)):
        ch = text[i]
        if ch == "<":
            depth += 1
        elif ch == ">":
            depth -= 1
        elif ch == "(" and depth == 0:
            return text[cc.end():i].strip()
    return text[cc.end():].strip()


def short_name(name: str, demangled: dict) -> str:
    text = demangled.get(name)
    return qualified(text) if text else name


def _bare(qualified: str) -> str:
    return qualified.split("(")[0].split("::")[-1].strip()


#: Explicit convention spellings: the keywords and the SDK macros over them.
_EXPLICIT = re.compile(
    r"\b(__fastcall|__cdecl|__stdcall|__thiscall|WINAPI|CALLBACK|APIENTRY|PASCAL)"
    r"\s+(?:[A-Za-z_]\w*::)*(~?[A-Za-z_]\w*|operator\s*\S+?)\s*\(")


def explicit_declarations(roots=("src", "include")) -> dict[str, list[str]]:
    """{identifier: [file:line ...]} for every declaration in the tree that
    spells its calling convention."""
    out: dict[str, list[str]] = defaultdict(list)
    for top in roots:
        for path in sorted((REPO / top).rglob("*")):
            if path.suffix.lower() not in (".h", ".hpp", ".c", ".cpp", ".inl"):
                continue
            rel = path.relative_to(REPO).as_posix()
            for number, line in enumerate(path.read_text(errors="replace").splitlines(), 1):
                for match in _EXPLICIT.finditer(line):
                    out[match.group(2)].append(f"{rel}:{number}")
    return out


def _rva(fn: Function) -> str:
    return f"0x{fn.rva:06x}" if fn.rva is not None else "-"


def _limit(rows: list, limit: int | None) -> list:
    return rows[:limit] if limit else rows


# --- cc --------------------------------------------------------------------------

def cleanup_after(body: bytes, end: int) -> int:
    """Bytes a caller releases right after a call ending at `end`."""
    if body[end:end + 2] == b"\x83\xc4" and end + 2 < len(body):
        return body[end + 2]
    if body[end:end + 2] == b"\x81\xc4":
        return int.from_bytes(body[end + 2:end + 6], "little")
    if body[end:end + 1] == b"\x59":
        return 8 if body[end + 1:end + 2] == b"\x59" else 4
    return 0


def _direct_calls(body: bytes, relocs: dict):
    for site in sorted(relocs):
        name, typ, _addend = relocs[site]
        if typ == REL32 and site >= 1 and body[site - 1] == CALL:
            yield site, name


def call_sites(body: bytes, relocs: dict) -> dict[str, list[int]]:
    """{callee: [cleanup bytes after each direct call, in body order]}."""
    sites = defaultdict(list)
    for site, name in _direct_calls(body, relocs):
        sites[name].append(cleanup_after(body, site + 4))
    return dict(sites)


def sweep_cc(fns: list[Function], *, limit=None) -> dict:
    callee_rows: dict[str, dict] = {}
    function_rows = []
    for fn in fns:
        ours, theirs = signature(fn.ours_insns), signature(fn.retail_insns)
        if ours != theirs and fn.gap > 0:
            function_rows.append({
                "unit": fn.unit, "symbol": fn.name, "rva": _rva(fn),
                "max": fn.best, "hist": fn.hist, "gap": fn.gap,
                "ours_pops": sorted(ours[0]), "retail_pops": sorted(theirs[0]),
                "ours_live": sorted(ours[1]), "retail_live": sorted(theirs[1]),
                "weight": 2 * (ours[0] != theirs[0]) + (ours[1] != theirs[1]),
            })
        a = call_sites(fn.ours, fn.ours_relocs)
        b = call_sites(fn.retail, fn.retail_relocs)
        for callee in a.keys() & b.keys():
            if len(a[callee]) != len(b[callee]):
                continue                  # the sites do not pair one to one
            row = callee_rows.setdefault(callee, {
                "symbol": callee, "agree": 0, "differ": 0, "examples": []})
            for x, y in zip(a[callee], b[callee]):
                if x == y:
                    row["agree"] += 1
                    continue
                row["differ"] += 1
                if len(row["examples"]) < 4:
                    row["examples"].append({"unit": fn.unit, "symbol": fn.name,
                                            "rva": _rva(fn), "max": fn.best,
                                            "ours": x, "retail": y})
    callees = [r for r in callee_rows.values() if r["differ"]]
    demangled = undname({r["symbol"] for r in function_rows}
                        | {r["symbol"] for r in callees}
                        | {e["symbol"] for r in callees for e in r["examples"]})
    explicit = explicit_declarations()
    for r in function_rows:
        r["function"] = short_name(r["symbol"], demangled)
        r["convention"] = convention(r["symbol"], demangled)
        r["explicit"] = explicit.get(_bare(r["function"]), [])
    for r in callees:
        r["callee"] = short_name(r["symbol"], demangled)
        r["convention"] = convention(r["symbol"], demangled)
        r["explicit"] = explicit.get(_bare(r["callee"]), [])
        r["share"] = round(r["differ"] / (r["differ"] + r["agree"]), 3)
        for e in r["examples"]:
            e["function"] = short_name(e["symbol"], demangled)
    callees.sort(key=lambda r: (-r["share"], -r["differ"]))
    function_rows.sort(key=lambda r: (-r["weight"], -r["gap"]))
    return {"functions": _limit(function_rows, limit), "callees": _limit(callees, limit)}


def render_cc(result) -> None:
    rows = result["functions"]
    print(f"== non-exact functions whose entry signature differs ({len(rows)}): "
          "pops / live-in argument registers, ours vs retail")
    for r in rows:
        print(f"  [{r['weight']}] {r['rva']:>8} {r['unit']:22} MAX {r['max']:7.3f} "
              f"{r['convention'] or '?':10} {r['function']}\n"
              f"        pops {r['ours_pops']} / {r['retail_pops']}; "
              f"live {r['ours_live']} / {r['retail_live']}"
              + (f"; explicit at {', '.join(r['explicit'][:2])}" if r["explicit"] else ""))
    rows = result["callees"]
    print(f"\n== callees whose caller cleanup differs at paired sites ({len(rows)})")
    for r in rows:
        print(f"  differ {r['differ']:3} agree {r['agree']:3} "
              f"{r['convention'] or '?':10} {r['callee']}"
              + (f"  [explicit {r['explicit'][0]}]" if r["explicit"] else ""))
        for e in r["examples"]:
            print(f"        {e['rva']:>8} {e['unit']}:{e['function']} "
                  f"(MAX {e['max']:.2f}) ours {e['ours']} retail {e['retail']}")


# --- flags (x87) -------------------------------------------------------------------

_FRAME_SLOT = re.compile(r"\[(?:ebp|esp)(?:[+-]0x[0-9a-f]+)?\]")
X87_KEYS = ("reloads", "fabs", "fabs_calls", "ftol_calls")


def x87_profile(insns: dict[int, Insn], body: bytes, relocs: dict) -> dict | None:
    """x87 shape counts of one body, or None when it uses no x87 code."""
    rows = [insns[at] for at in sorted(insns)]
    if not any(i.mnemonic.startswith("f") for i in rows):
        return None
    reloads = 0
    for i, insn in enumerate(rows):
        slot = _FRAME_SLOT.search(insn.ops)
        if insn.mnemonic in ("fst", "fstp") and slot and i + 1 < len(rows):
            nxt = rows[i + 1]
            if nxt.mnemonic.startswith("fld") and slot.group(0) in nxt.ops:
                reloads += 1
    calls = Counter(name for _site, name in _direct_calls(body, relocs))
    return {"reloads": reloads,
            "fabs": sum(1 for i in rows if i.mnemonic == "fabs"),
            "fabs_calls": calls["_fabs"],
            "ftol_calls": calls["__ftol"],
            "x87": sum(1 for i in rows if i.mnemonic.startswith("f"))}


def sweep_flags(fns: list[Function], *, limit=None) -> dict:
    rows = []
    for fn in fns:
        a = x87_profile(fn.ours_insns, fn.ours, fn.ours_relocs)
        b = x87_profile(fn.retail_insns, fn.retail, fn.retail_relocs)
        if a is None and b is None:
            continue
        a, b = a or {}, b or {}
        rows.append({"unit": fn.unit, "symbol": fn.name, "rva": _rva(fn),
                     "profile": fn.profile, "op": "/Op" in fn.flags,
                     "max": fn.best, "gap": fn.gap, "ours": a, "retail": b,
                     "differs": [k for k in X87_KEYS if a.get(k, 0) != b.get(k, 0)]})
    summary: dict[str, Counter] = defaultdict(Counter)
    for r in rows:
        counts = summary[r["profile"]]
        counts["float functions"] += 1
        counts["exact"] += r["gap"] == 0
        for k in r["differs"]:
            more = r["ours"].get(k, 0) > r["retail"].get(k, 0)
            counts[f"{k} {'ours>retail' if more else 'ours<retail'}"] += 1
    candidates = sorted((r for r in rows if r["differs"]),
                        key=lambda r: (-len(r["differs"]), -r["gap"]))
    demangled = undname({r["symbol"] for r in candidates})
    for r in candidates:
        r["function"] = short_name(r["symbol"], demangled)
    units = defaultdict(Counter)
    for r in candidates:
        if "reloads" in r["differs"]:
            more = r["ours"].get("reloads", 0) > r["retail"].get("reloads", 0)
            units[r["unit"]]["ours>retail" if more else "ours<retail"] += 1
    return {"summary": {k: dict(v) for k, v in sorted(summary.items())},
            "reload_units": {u: dict(c) for u, c in
                             sorted(units.items(), key=lambda kv: -sum(kv[1].values()))},
            "candidates": _limit(candidates, limit)}


def render_flags(result) -> None:
    print("== float-using functions per profile (ours vs retail x87 shape)")
    for key, counts in result["summary"].items():
        print(f"  {key}: " + ", ".join(f"{k} {v}" for k, v in sorted(counts.items())))
    if result["reload_units"]:
        print("\n== units whose store/reload counts differ (functions)")
        for unit, counts in list(result["reload_units"].items())[:20]:
            print(f"  {unit:28} " + ", ".join(f"{k} {v}" for k, v in sorted(counts.items())))
    rows = result["candidates"]
    print(f"\n== functions whose x87 signature differs ({len(rows)}), ours/retail")
    for r in rows:
        keys = ", ".join(f"{k} {r['ours'].get(k, 0)}/{r['retail'].get(k, 0)}"
                         for k in r["differs"])
        print(f"  {r['rva']:>8} {r['unit']:22} MAX {r['max']:7.3f} "
              f"{r['profile']:14} {r['function']}  [{keys}]")


def try_flags(units: list[str], drop: list[str], add: list[str], *,
              jobs: int = 4) -> list[dict]:
    """Compile each unit with its profile changed, normalize and score it as
    the build does, and return the functions whose score moves."""
    from concurrent.futures import ThreadPoolExecutor

    from gruntz import graph, manifest
    from gruntz.compare import normalize, project
    from gruntz.tool import ToolError, cl, objdiff
    from gruntz.verify import scores
    from gruntz.verify.baseline import load as load_ledger

    by_unit = manifest.by_unit()
    profiles = manifest.flag_profiles()
    target_dir = REPO / graph.TARGET_DIR
    work = WORK / "flags"
    shutil.rmtree(work, ignore_errors=True)
    base_dir, compare_dir = work / "base", work / "compare"
    base_dir.mkdir(parents=True)

    def one(unit):
        spec = by_unit[unit]
        flags = [f for f in profiles[spec["flags"]] if f not in drop] + list(add)
        try:
            cl.compile(REPO / spec["source"], base_dir / f"{unit}.obj", flags)
        except ToolError as exc:
            return unit, str(exc)
        return unit, None

    errors = []
    with ThreadPoolExecutor(max_workers=jobs) as pool:
        for unit, error in pool.map(one, units):
            if error:
                errors.append({"unit": unit, "error": error})
    built = [u for u in units if (base_dir / f"{u}.obj").is_file()
             and normalize.target_object(target_dir, u) is not None]
    if not built:
        return errors
    normalize.normalize(base_dir, target_dir, compare_dir, built, force=True, quiet=True)
    project.project(built, target_dir, compare_dir)
    after = scores.functions(scores.load(
        objdiff.report(compare_dir, compare_dir / "report.json")))
    before = scores.functions(scores.load())
    ledger = load_ledger()
    rows = list(errors)
    for (unit, name), pct in sorted(after.items()):
        old = before.get((unit, name))
        if old is not None and abs(pct - old) <= 1e-6:
            continue
        row = ledger.get((unit, name))
        rows.append({"unit": unit, "symbol": name, "before": old, "after": pct,
                     "max": row["best"] if row else None,
                     "rva": f"0x{row['addr']:06x}" if row and row["addr"] is not None else "-"})
    demangled = undname({r["symbol"] for r in rows if "symbol" in r})
    for r in rows:
        if "symbol" in r:
            r["function"] = short_name(r["symbol"], demangled)
    return rows


def render_try(rows) -> None:
    moved = [r for r in rows if "symbol" in r and r["before"] is not None]
    better = sum(1 for r in moved if r["after"] > r["before"])
    worse = sum(1 for r in moved if r["after"] < r["before"])
    new = sum(1 for r in rows if "symbol" in r and r["before"] is None)
    print(f"== {len(moved)} function(s) move: {better} better, {worse} worse"
          + (f"; {new} unscored before" if new else ""))
    for r in sorted(rows, key=lambda r: ("symbol" in r, (r.get("after") or 0)
                                         - (r.get("before") or 0))):
        if "error" in r:
            print(f"  {r['unit']}: compile failed\n{r['error']}")
            continue
        b = "-" if r["before"] is None else f"{r['before']:.4f}"
        m = "-" if r["max"] is None else f"{r['max']:.4f}"
        print(f"  {r['rva']:>8} {r['unit']:22} {b:>9} -> {r['after']:8.4f} "
              f"(MAX {m})  {r['function']}")


# --- inline ----------------------------------------------------------------------

def call_sequence(body: bytes, relocs: dict, own: str | None = None) -> list[str]:
    """Direct callees in body order. A self-call resolves inside the
    delinked section without a relocation, so recursion is left out."""
    me = canon(own) if own else None
    return [name for _site, name in _direct_calls(body, relocs) if name != me]


def substitutions(ours: list[str], retail: list[str]) -> list[tuple[str, str]]:
    """(our callee, retail callee) at aligned call sites where each side
    calls something the other side calls fewer times: one callee replaced
    by another (an identity or object-type question), not an expansion.
    Sites align by difflib over the two call sequences; only equal-length
    replaced runs pair."""
    import difflib
    a, b = Counter(ours), Counter(retail)
    ours_extra, retail_extra = a - b, b - a
    out = []
    matcher = difflib.SequenceMatcher(None, ours, retail, autojunk=False)
    for tag, i1, i2, j1, j2 in matcher.get_opcodes():
        if tag != "replace" or i2 - i1 != j2 - j1:
            continue
        for x, y in zip(ours[i1:i2], retail[j1:j2]):
            if ours_extra[x] > 0 and retail_extra[y] > 0:
                out.append((x, y))
                ours_extra[x] -= 1
                retail_extra[y] -= 1
    return out


def sweep_inline(fns: list[Function], *, all_units=False, limit=None) -> dict:
    per_callee: dict[str, dict] = {}
    swaps: dict[tuple, dict] = {}
    for fn in fns:
        if not all_units and not ob1(fn.flags):
            continue
        sa = call_sequence(fn.ours, fn.ours_relocs, fn.name)
        sb = call_sequence(fn.retail, fn.retail_relocs, fn.name)
        a, b = Counter(sa), Counter(sb)
        for x, y in substitutions(sa, sb):
            a[x] -= 1
            b[y] -= 1
            row = swaps.setdefault((x, y), {"ours": x, "retail": y, "sites": 0,
                                            "callers": {}})
            row["sites"] += 1
            row["callers"].setdefault(fn.name, {"unit": fn.unit, "symbol": fn.name,
                                                "rva": _rva(fn), "max": fn.best})
        for callee in a.keys() | b.keys():
            extra = a[callee] - b[callee]
            row = per_callee.setdefault(callee, {
                "symbol": callee, "we_call_retail_expands": 0,
                "retail_calls_we_expand": 0, "both_call": 0, "callers": []})
            row["both_call"] += min(a[callee], b[callee])
            if extra > 0:
                row["we_call_retail_expands"] += extra
            elif extra < 0:
                row["retail_calls_we_expand"] -= extra
            if extra:
                row["callers"].append({"unit": fn.unit, "symbol": fn.name,
                                       "rva": _rva(fn), "max": fn.best, "delta": extra})
    rows = [r for r in per_callee.values()
            if r["we_call_retail_expands"] or r["retail_calls_we_expand"]]
    for r in rows:
        r["weight"] = r["we_call_retail_expands"] + r["retail_calls_we_expand"]
        r["callers"].sort(key=lambda c: c["max"])
        r["callers"] = r["callers"][:6]
    rows.sort(key=lambda r: (-r["weight"], r["symbol"]))
    rows = _limit(rows, limit)
    swapped = sorted(swaps.values(), key=lambda r: (-r["sites"], r["ours"]))
    for r in swapped:
        r["callers"] = sorted(r["callers"].values(), key=lambda c: c["max"])
    swapped = _limit(swapped, limit)
    demangled = undname({r["symbol"] for r in rows}
                        | {c["symbol"] for r in rows + swapped for c in r["callers"]}
                        | {r["ours"] for r in swapped} | {r["retail"] for r in swapped})
    for r in rows:
        r["callee"] = short_name(r["symbol"], demangled)
    for r in swapped:
        r["ours_callee"] = short_name(r["ours"], demangled)
        r["retail_callee"] = short_name(r["retail"], demangled)
    for r in rows + swapped:
        for c in r["callers"]:
            c["function"] = short_name(c["symbol"], demangled)
    return {"substitutions": swapped, "callees": rows}


def render_inline(result) -> None:
    rows = result["substitutions"]
    print(f"== callee substitutions at aligned call sites ({len(rows)}): "
          "identity/object-type leads, not inlining")
    for r in rows:
        print(f"  {r['sites']:3} site(s)  ours {r['ours_callee']}  ->  retail {r['retail_callee']}")
        for c in r["callers"][:4]:
            print(f"        {c['rva']:>8} {c['unit']}:{c['function']} (MAX {c['max']:.2f})")
    print()
    rows = result["callees"]
    print(f"== callees whose call/expansion differs ({len(rows)}); "
          "+N = we call where retail expands, -N = retail calls where we expand")
    for r in rows:
        print(f"  +{r['we_call_retail_expands']:<3} -{r['retail_calls_we_expand']:<3} "
              f"both {r['both_call']:<4} {r['callee']}")
        for c in r["callers"]:
            print(f"        {c['rva']:>8} {c['unit']}:{c['function']} "
                  f"(MAX {c['max']:.2f}) {c['delta']:+d}")


# --- loops -----------------------------------------------------------------------

def loop_shape(insns: dict[int, Insn]) -> dict:
    shape = Counter()
    for at, insn in insns.items():
        target = branch_target(insn)
        if target is None:
            continue
        kind = "jmp" if insn.mnemonic == "jmp" else "jcc"
        shape[f"{kind}_back" if target <= at else f"{kind}_fwd"] += 1
    shape["ret"] = sum(1 for i in insns.values() if i.mnemonic.startswith("ret"))
    return dict(shape)


def sweep_loops(fns: list[Function], *, limit=None) -> dict:
    rows = []
    for fn in fns:
        if fn.gap <= 0:
            continue
        a, b = loop_shape(fn.ours_insns), loop_shape(fn.retail_insns)
        uncond = a.get("jmp_back", 0) - b.get("jmp_back", 0)
        cond = a.get("jcc_back", 0) - b.get("jcc_back", 0)
        if not uncond and not cond:
            continue
        rotation = (uncond > 0 > cond) or (uncond < 0 < cond)
        rows.append({"unit": fn.unit, "symbol": fn.name, "rva": _rva(fn),
                     "gap": fn.gap, "max": fn.best, "ours": a, "retail": b,
                     "kind": ("retail rotates, ours does not" if uncond > 0 > cond else
                              "ours rotates, retail does not" if rotation else
                              "back-edge count differs"),
                     "weight": 2 * rotation + 1})
    rows.sort(key=lambda r: (-r["weight"], -r["gap"]))
    rows = _limit(rows, limit)
    demangled = undname({r["symbol"] for r in rows})
    for r in rows:
        r["function"] = short_name(r["symbol"], demangled)
    return {"functions": rows}


def render_loops(result) -> None:
    rows = result["functions"]
    print(f"== non-exact functions whose back edges differ ({len(rows)}); "
          "jmp/jcc back, ours/retail")
    for r in rows:
        a, b = r["ours"], r["retail"]
        print(f"  [{r['weight']}] {r['rva']:>8} {r['unit']:22} MAX {r['max']:7.3f} "
              f"jmp {a.get('jmp_back', 0)}/{b.get('jmp_back', 0)} "
              f"jcc {a.get('jcc_back', 0)}/{b.get('jcc_back', 0)}  "
              f"{r['kind']}: {r['function']}")


# --- ctors -----------------------------------------------------------------------

_ADDRESS = re.compile(r"0x[0-9a-f]{5,}")
_STORE = re.compile(r"^mov (?:BYTE|WORD|DWORD) PTR \[(?P<base>[a-z]+)")


def masked_rows(insns: dict[int, Insn]) -> list[str]:
    """Instructions in address order, addresses and branch targets masked."""
    rows = []
    for at in sorted(insns):
        insn = insns[at]
        ops = "@" if branch_target(insn) is not None or insn.mnemonic == "call" \
            else _ADDRESS.sub("A", insn.ops)
        rows.append(f"{insn.mnemonic} {ops}".strip())
    return rows


def store_permutations(a: list[str], b: list[str], gap: int = 3) -> dict:
    """Regions where both row lists hold the same rows in another order and
    every displaced row is a store through one base register. Edits at most
    `gap` equal rows apart form one region (difflib reports a swap as a
    delete plus an insert around an equal row)."""
    import difflib
    ops = difflib.SequenceMatcher(None, a, b, autojunk=False).get_opcodes()
    regions, current = [], None
    for tag, i1, i2, j1, j2 in ops:
        if tag == "equal":
            if current and i2 - i1 <= gap:
                current["tail"] = (i2, j2)
            elif current:
                regions.append(current)
                current = None
            continue
        if current is None:
            current = {"start": (i1, j1)}
        current["tail"] = (i2, j2)
    if current:
        regions.append(current)
    differing = sum(max(i2 - i1, j2 - j1) for t, i1, i2, j1, j2 in ops if t != "equal")
    blocks = []
    for region in regions:
        (i1, j1), (i2, j2) = region["start"], region["tail"]
        left, right = a[i1:i2], b[j1:j2]
        if left == right or sorted(left) != sorted(right):
            continue
        moved = [x for x, y in zip(left, right) if x != y]
        stores = [_STORE.match(row) for row in moved]
        if moved and all(stores) and len({m.group("base") for m in stores}) == 1:
            blocks.append({"ours": left, "retail": right, "rows": len(moved)})
    return {"blocks": blocks, "permuted": sum(x["rows"] for x in blocks),
            "differing": differing}


def sweep_ctors(fns: list[Function], *, limit=None) -> dict:
    rows = []
    for fn in fns:
        if fn.gap <= 0:
            continue
        found = store_permutations(masked_rows(fn.ours_insns), masked_rows(fn.retail_insns))
        if not found["blocks"]:
            continue
        share = found["permuted"] / max(found["differing"], 1)
        rows.append({"unit": fn.unit, "symbol": fn.name, "rva": _rva(fn), "gap": fn.gap,
                     "max": fn.best, "share": round(share, 3), **found})
    rows.sort(key=lambda r: (-r["share"], r["gap"]))
    rows = _limit(rows, limit)
    demangled = undname({r["symbol"] for r in rows})
    for r in rows:
        r["function"] = short_name(r["symbol"], demangled)
    return {"functions": rows}


def render_ctors(result) -> None:
    rows = result["functions"]
    print(f"== non-exact functions with same-base store permutations ({len(rows)}); "
          "share = permuted rows / differing rows")
    for r in rows:
        print(f"  {r['rva']:>8} {r['unit']:22} MAX {r['max']:7.3f} share {r['share']:.2f} "
              f"({r['permuted']}/{r['differing']})  {r['function']}")
        for block in r["blocks"][:2]:
            print(f"        ours   {' ; '.join(block['ours'])}\n"
                  f"        retail {' ; '.join(block['retail'])}")


# --- CLI -------------------------------------------------------------------------

def _parser() -> argparse.ArgumentParser:
    ap = argparse.ArgumentParser(prog="gruntz walls patterns", description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    for name, text in (("cc", "entry signatures and caller cleanup"),
                       ("flags", "x87 profile signatures, or an A/B compile with --try"),
                       ("inline", "call-set asymmetries per callee"),
                       ("loops", "back-edge shapes of non-exact functions"),
                       ("ctors", "store-order permutations (constructor shape)")):
        p = sub.add_parser(name, help=text)
        p.add_argument("--unit", help="restrict to one unit of config/units.toml")
        p.add_argument("--json", action="store_true")
        p.add_argument("--limit", type=int, default=40, help="rows per list (0: all)")
        if name == "inline":
            p.add_argument("--all-units", action="store_true",
                           help="include units that are not /Ob1")
        if name == "flags":
            p.add_argument("--try", dest="try_units", metavar="UNIT[,UNIT]",
                           help="A/B compile these units with the changed profile")
            p.add_argument("--profile", help="with --try: every unit of this profile")
            p.add_argument("--drop", action="append", default=[],
                           help="with --try: a flag to remove")
            p.add_argument("--add", action="append", default=[],
                           help="with --try: a flag to add")
            p.add_argument("--jobs", type=int, default=4)
    return ap


def main(argv=None) -> int:
    args = _parser().parse_args(argv)
    from gruntz.walls import check_unit
    check_unit(args.unit)
    if args.cmd == "flags" and (args.try_units or args.profile):
        from gruntz import manifest
        units = [u for u in (args.try_units or "").split(",") if u]
        for unit in units:
            check_unit(unit)
        if args.profile:
            if args.profile not in manifest.flag_profiles():
                print(f"[patterns] unknown profile {args.profile!r}", file=sys.stderr)
                return 2
            units += [u["unit"] for u in manifest.units()
                      if u["flags"] == args.profile and u["unit"] not in units]
        if not (args.drop or args.add):
            print("[patterns] --try needs --drop or --add", file=sys.stderr)
            return 2
        rows = try_flags(units, args.drop, args.add, jobs=args.jobs)
        print(json.dumps(rows, indent=1)) if args.json else render_try(rows)
        return 0
    fns = list(functions({args.unit} if args.unit else None))
    limit = args.limit or None
    if args.cmd == "cc":
        result, render = sweep_cc(fns, limit=limit), render_cc
    elif args.cmd == "flags":
        result, render = sweep_flags(fns, limit=limit), render_flags
    elif args.cmd == "inline":
        result, render = sweep_inline(fns, all_units=args.all_units, limit=limit), render_inline
    elif args.cmd == "ctors":
        result, render = sweep_ctors(fns, limit=limit), render_ctors
    else:
        result, render = sweep_loops(fns, limit=limit), render_loops
    if args.json:
        print(json.dumps(result, indent=1, default=list))
    else:
        render(result)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
