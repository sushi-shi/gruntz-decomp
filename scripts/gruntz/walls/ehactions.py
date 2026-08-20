"""gruntz.walls.ehactions - the /GX unwind ACTION SEQUENCE of one parent.

An MSVC 5.0 unwind funclet is two instructions:

    lea ecx,[esp+N]         the object slot it destroys
    jmp <dtor>              the destructor it runs

MEASURED, both forms occur and both mean the same thing:

    mov eax,[ebp-0x30] / push eax / call <dtor> / pop ecx / ret   (11 B)
    lea ecx,<slot>     / jmp <dtor>                               (thiscall)

The (object slot, destructor) pair IS the unwind action, and the ordered list
of pairs is the only thing about a function's EH tail that carries meaning.
This module decodes every `__ehunwind$<parent>$<n>` funclet on both sides of
the normalized pair and diffs the two ACTION SEQUENCES.

WHY THE COUNT IS THE WRONG QUESTION. A funclet exists per out-of-line
destructible temporary, so the funclet COUNT is a readout of the
ctor-inlining boundary, not of correctness: inline one more constructor and a
temporary stops needing its own cleanup entry. Lane B closed three parents
this way, matching the count arithmetically parent-by-parent - and then found
all 18 funclets' ACTIONS equal across the pair. A count delta on its own is
an inline-boundary observation; a differing object slot or a differing dtor
symbol is an unwind-action defect.

ZERO-EXTENT INTERIOR LABELS - do not re-diagnose "unpaired" as missing code.
The funclet labels are INTERIOR and carry `physical_size = 0` on both sides,
and they are not even named the same way (measured on sbi_rectonly,
BuildTabzDialog):

    base    COFF name `$L44255`, `$L44256`, ... - cl's own LOCAL labels, at
            an 11-byte stride; `__ehunwind$<parent>$<n>` is the NORMALIZER's
            canonical name for them, not a COFF name. Found via symbols.tsv.
    target  COFF name `__ehunwind$<parent>$<n>` for real, at a 12-byte
            stride - but symbols.tsv reports `section_ordinal = 0` and
            prefixes the canonical name `$dup$`.

So the compare report lists them UNPAIRED and symbols.tsv looks like the
target has no defining section. Both are labelling properties of zero-extent
interior symbols, NOT an absent funclet - the target COFF does define them
(sbi_rectonly section 210). This module takes the COFF definition when there
is one and falls back to the normalizer's canonical map when there is not.

    gruntz walls ehactions <rva|name> [--raw]

`--raw` additionally dumps any section that does define the `__ehunwind$`
labels, with its relocations - lane B's unwind.py view, for the hand-audit
when a decode gives up.
"""

from __future__ import annotations

import argparse
import re
from difflib import SequenceMatcher

from gruntz.core.paths import BUILD
from gruntz.delink.coffx import Obj
from gruntz.tool import objdump
from gruntz.walls.diagnose import _locate

NORM = BUILD / "objdiff/compare-new"

FUNCLET = "__ehunwind$"
#: both funclet encodings: the cdecl `mov eax,<slot>; push; call` and the
#: thiscall `lea ecx,<slot>; jmp`.
SLOT = re.compile(r"^(mov\s+eax,\S*\s*PTR\s*|lea\s+ecx,)"
                  r"\[(\S+?)([+-]0x[0-9a-f]+)?\]")
TRANSFER = re.compile(r"^(call|jmp)\b")


def _obj(side: str, unit: str) -> Obj:
    if side == "base":
        return Obj(NORM / "base" / f"{unit}.obj")
    path = NORM / "target" / f"{unit}.c.obj"
    if not path.exists():
        path = NORM / "target" / f"{unit}.obj"
    return Obj(path)


def _canonical_map(side: str, unit: str, parent: str):
    """{funclet index: (section ordinal, offset)} from the normalizer's
    symbols.tsv - the only place the base side's `$L<n>` labels carry their
    `__ehunwind$` identity."""
    import csv
    path = NORM / side / f"{unit}.symbols.tsv"
    if not path.exists():
        return {}
    want = f"{FUNCLET}{parent}$"
    out = {}
    with open(path) as fh:
        for row in csv.DictReader(fh, delimiter="\t"):
            name = row["canonical_name"]
            if name.startswith("$dup$"):
                name = name[len("$dup$"):]
            if not name.startswith(want):
                continue
            tail = name.rsplit("$", 1)[-1]
            if not tail.isdigit():
                continue
            sec = int(row["section_ordinal"] or 0)
            if sec:
                out[int(tail)] = (sec, int(row["section_offset"], 16))
    return out


def funclets(obj: Obj, side: str, unit: str, parent: str):
    """[(index, bytes, relocations)] for one parent, in funclet order.

    The labels are zero-extent, so each funclet is cut at the NEXT defined
    member of its section.
    """
    want = f"{FUNCLET}{parent}$"
    located: dict[int, tuple[int, int]] = {}
    for secnum in range(1, obj.nsec + 1):
        for value, name, _scl in obj.section_members(secnum):
            if name.startswith(want) and name.rsplit("$", 1)[-1].isdigit():
                located[int(name.rsplit("$", 1)[-1])] = (secnum, value)
    if not located:
        located = _canonical_map(side, unit, parent)

    out = []
    for idx in sorted(located):
        secnum, value = located[idx]
        payload = obj.section_payload(secnum)
        bounds = sorted({v for v, _n, _s in obj.section_members(secnum)}
                        | {len(payload)})
        end = next((b for b in bounds if b > value), len(payload))
        rel = {o - value: t for o, t in obj.relocations(secnum).items()
               if value <= o < end}
        out.append((idx, payload[value:end], rel))
    return out


def action(body: bytes, rel: dict[int, str]) -> str:
    """`<object slot> -> <dtor>` for one funclet, either encoding."""
    slot, dtor = None, None
    for line in objdump.disassemble(body, vma=0).splitlines():
        if ":\t" not in line:
            continue
        head, rest = line.split(":\t", 1)
        try:
            addr = int(head.strip(), 16)
        except ValueError:
            continue
        parts = rest.split("\t")
        asm = " ".join((parts[-1] if len(parts) > 1 else parts[0]).split())
        nbytes = len((parts[0] if len(parts) > 1 else "").split())
        m = SLOT.match(asm)
        if m and slot is None:
            slot = f"[{m.group(2)}{m.group(3) or '+0x0'}]"
        if TRANSFER.match(asm) and dtor is None:
            dtor = next((t for o, t in rel.items()
                         if addr <= o < addr + max(nbytes, 1)), None)
    if slot is None and dtor is None:
        return "?"
    return f"{slot or '(no slot)'} -> {dtor or '(unrelocated)'}"


def classify(base: list[str], target: list[str], ops) -> str:
    """"count" when every divergence is a pure insertion/deletion whose
    actions the two sides already share, else "action".

    The distinction is the whole doctrine: a funclet COUNT delta is the
    ctor-inlining boundary (one fewer out-of-line destructible temporary), a
    changed object slot or destructor is a real unwind defect.
    """
    common = set(base) & set(target)
    for op, i1, i2, j1, j2 in ops:
        if op == "replace":
            return "action"
        extra = base[i1:i2] + target[j1:j2]
        if any(a not in common for a in extra):
            return "action"
    return "count"


def report(token: str, raw: bool) -> int:
    b, why = _locate(token)
    if b is None:
        raise SystemExit(f"[ehactions] {why}")
    acts = {}
    for side in ("base", "target"):
        obj = _obj(side, b.unit)
        if raw:
            _dump_raw(side, obj, b.name)
        acts[side] = [action(body, rel) for _i, body, rel
                      in funclets(obj, side, b.unit, b.name)]

    print(f"{b.name}  0x{b.rva:06x}  [{b.unit}]")
    print(f"funclets: base {len(acts['base'])} target {len(acts['target'])}"
          + ("   (a COUNT delta is the out-of-line destructible-temporary "
             "count = the ctor-inlining boundary)"
             if len(acts["base"]) != len(acts["target"]) else ""))
    for side in ("base", "target"):
        print(f"== {side} action sequence")
        for i, a in enumerate(acts[side]):
            print(f"   ${i:<3} {a}")
        if not acts[side]:
            print("   (none)")
    sm = SequenceMatcher(None, acts["base"], acts["target"], autojunk=False)
    ops = [o for o in sm.get_opcodes() if o[0] != "equal"]
    if not ops:
        print("== ACTIONS EQUAL (object slot + dtor identity, in order)")
        return 0
    if classify(acts["base"], acts["target"], ops) == "count":
        print(f"== COUNT-ONLY: {len(ops)} pure insertion(s)/deletion(s), every "
              f"extra funclet repeating an action both sides already run.\n"
              f"   That is the out-of-line destructible-temporary count = the "
              f"ctor-inlining boundary, NOT an unwind defect.")
    else:
        print(f"== ACTIONS DIFFER: {len(ops)} divergence(s) - a differing "
              f"object slot or dtor IS an unwind defect")
    for op, i1, i2, j1, j2 in ops:
        print(f"   {op} base[{i1}:{i2}] target[{j1}:{j2}]")
        for x in acts["base"][i1:i2][:8]:
            print(f"      B {x}")
        for x in acts["target"][j1:j2][:8]:
            print(f"      T {x}")
    return 0


def _dump_raw(side: str, obj: Obj, parent: str) -> None:
    want = f"{FUNCLET}{parent}$"
    for secnum in range(1, obj.nsec + 1):
        members = obj.section_members(secnum)
        if not any(n.startswith(want) for _v, n, _s in members):
            continue
        payload = obj.section_payload(secnum)
        rel = obj.relocations(secnum)
        names: dict[int, list[str]] = {}
        for value, name, _scl in members:
            names.setdefault(value, []).append(name)
        print(f"== {side} section {secnum}: {len(payload)} B, "
              f"{len(rel)} relocs")
        for line in objdump.disassemble(payload, vma=0).splitlines():
            if ":\t" not in line:
                continue
            addr = int(line.split(":\t", 1)[0].strip(), 16)
            note = next((f"   ; -> {t}" for o, t in rel.items()
                         if addr <= o < addr + 8), "")
            tag = "".join(f"  <{n}>" for n in names.get(addr, []))
            print(" ".join(line.split())[:96] + note + tag)


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="gruntz walls ehactions",
                                 description=__doc__.split("\n\n")[0])
    ap.add_argument("token", help="hex rva, mangled name, or CClass::Member")
    ap.add_argument("--raw", action="store_true",
                    help="also dump the whole funclet section with relocs")
    args = ap.parse_args(argv)
    return report(args.token, args.raw)


if __name__ == "__main__":
    raise SystemExit(main())
