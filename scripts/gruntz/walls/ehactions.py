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

WHY THE COUNT IS THE WRONG QUESTION. A funclet exists per live cleanup state,
so the funclet COUNT is partly a readout of ctor/dtor inlining, not of source
correctness. Inline one more constructor and a temporary can stop needing its
own cleanup entry. Lane B closed three parents this way, matching the count
arithmetically parent-by-parent, and then found all 18 funclets' ACTIONS equal.

The converse is not valid either: a differing action shape is a structural
signal, not automatic proof of an authored cleanup defect. ChangeState is the
negative control. Retail calls the CMoviePlayer member CArray ctor/dtor while
base expands them; both sides have eleven funclets, but retail consequently
has a preceding-member CFecFile cleanup during construction and decomposes an
inlined CFecFile dtor through a saved receiver. Adjudicate action differences
against constructor/destructor inline boundaries and the parent state map.

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
    gruntz walls ehactions --census [--limit N] [--json]

`--raw` additionally dumps any section that does define the `__ehunwind$`
labels, with its relocations - lane B's unwind.py view, for the hand-audit
when a decode gives up.

CENSUS - the whole sub-100 EH band, grouped by PARENT.

The EH band is a third of the sub-100 queue and no other sieve reaches it,
because a funclet has no rva in the Model and no `__ehunwind$` symbol in our
own COFF. It pairs BY CONSTRUCTION anyway: the base side's funclets get their
own `.text` COMDAT carrying cl's local labels, and `<unit>.symbols.tsv` maps
each to its canonical `__ehunwind$<parent>$<n>` name and (section, offset).
`--census` walks the sub-100 rows, groups them by parent, decodes both action
sequences and names what differs. The 2026-08-23 run, 222 rows over 42
parents:

    slot-shift   28 parents  155 rows  same dtors IN ORDER, different frame
                                       displacement - a second readout of the
                                       PARENT's frame layout, not work of its
                                       own. Closes when the parent closes.
    count         9 parents   56 rows  the funclet count differs: the ctor/dtor
                                       inline boundary (see
                                       repeated-container-call-is-an-inline-member.md)
    dtor-identity 5 parents   11 rows  the ordered destructor list differs -
                                       the only structural bucket

So 222 opaque rows are really 42 questions, of which 14 are actionable. Four
of the five dtor-identity parents are a destructor-NAMING question rather than
a cleanup one - the object slot and the sequence position are identical and
only the referent's name differs: our `??1CGruntCoordList@@UAE@XZ` against
retail's `??1CPtrList@@UAE@XZ` at CGrunt+0x31c (three funclets), and our
`??1CTileImageSet@@UAE@XZ` against retail's `??1CObject@@UAE@XZ` in
`~CImageSet3`. An EMPTY derived destructor is byte-identical to its base's, so
the link kept one copy under whichever name came first
(identical-derived-dtor-comdat-is-named-by-link-order.md). Do NOT re-model a
class from a funclet referent - the unwind action can only report the name the
LINK kept. The fifth is `CGruntzMgr::ChangeState`, the documented negative
control above.
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
SLOT = re.compile(r"^(mov|lea)\s+(eax|ecx),"
                  r"(?:\S+\s+PTR\s*)?\[([^]]+)\]")
ADJUST = re.compile(r"^add\s+(eax|ecx),(0x[0-9a-f]+)")
TRANSFER = re.compile(r"^(call|jmp)\b")


def _obj(side: str, unit: str) -> Obj:
    if side == "base":
        return Obj(NORM / "base" / f"{unit}.obj")
    path = NORM / "target" / f"{unit}.c.obj"
    if not path.exists():
        path = NORM / "target" / f"{unit}.obj"
    return Obj(path)


def _canonical_rows(side: str, unit: str):
    """[(canonical name, section ordinal, offset)] for every `__eh*` symbol -
    the only place the base side's `$L<n>` labels carry their identity."""
    import csv
    path = NORM / side / f"{unit}.symbols.tsv"
    key = str(path)                    # NOT (side, unit): NORM is patchable
    if key in _CANON_CACHE:
        return _CANON_CACHE[key]
    out = []
    if path.exists():
        with open(path) as fh:
            for row in csv.DictReader(fh, delimiter="\t"):
                name = row["canonical_name"]
                if name.startswith("$dup$"):
                    name = name[len("$dup$"):]
                sec = int(row["section_ordinal"] or 0)
                if sec and name.startswith("__eh"):
                    out.append((name, sec, int(row["section_offset"], 16)))
    _CANON_CACHE[key] = out
    return out


_CANON_CACHE: dict = {}


def _canonical_map(side: str, unit: str, parent: str):
    """{funclet index: (section ordinal, offset)} for one parent."""
    want = f"{FUNCLET}{parent}$"
    out = {}
    for name, sec, off in _canonical_rows(side, unit):
        if not name.startswith(want):
            continue
        tail = name.rsplit("$", 1)[-1]
        if tail.isdigit():
            out[int(tail)] = (sec, off)
    return out


def funclets(obj: Obj, side: str, unit: str, parent: str):
    """[(index, bytes, relocations)] for one parent, in funclet order.

    The labels are zero-extent, so each funclet is cut at the NEXT defined
    member of its section. `section_members` does not list cl's `$L<n>` local
    labels, so the CANONICAL offsets are unioned into the boundary set: with
    the COFF members alone one funclet's slice runs to the end of the whole
    unwind COMDAT.
    """
    want = f"{FUNCLET}{parent}$"
    located: dict[int, tuple[int, int]] = {}
    for secnum in range(1, obj.nsec + 1):
        for value, name, _scl in obj.section_members(secnum):
            if name.startswith(want) and name.rsplit("$", 1)[-1].isdigit():
                located[int(name.rsplit("$", 1)[-1])] = (secnum, value)
    if not located:
        located = _canonical_map(side, unit, parent)
    extra: dict[int, set[int]] = {}
    for _n, sec, off in _canonical_rows(side, unit):
        extra.setdefault(sec, set()).add(off)

    out = []
    for idx in sorted(located):
        secnum, value = located[idx]
        payload = obj.section_payload(secnum)
        bounds = sorted({v for v, _n, _s in obj.section_members(secnum)}
                        | extra.get(secnum, set()) | {len(payload)})
        end = next((b for b in bounds if b > value), len(payload))
        rel = {o - value: t for o, t in obj.relocations(secnum).items()
               if value <= o < end}
        out.append((idx, payload[value:end], rel))
    return out


def action(body: bytes, rel: dict[int, str]) -> str:
    """`<object slot> -> <dtor>` for one funclet, either encoding."""
    slot, slot_reg, indirect, adjust, dtor = None, None, False, 0, None
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
            indirect = m.group(1) == "mov"
            slot_reg = m.group(2)
            slot = f"[{m.group(3)}]"
        m = ADJUST.match(asm)
        if m and slot is not None and m.group(1) == slot_reg:
            adjust += int(m.group(2), 16)
        if TRANSFER.match(asm) and dtor is None:
            dtor = next((t for o, t in rel.items()
                         if addr <= o < addr + max(nbytes, 1)), None)
            break
    if slot is None and dtor is None:
        return "?"
    if slot is not None:
        slot = ("*" if indirect else "") + slot
        if adjust:
            slot += f"+0x{adjust:x}"
    return f"{slot or '(no slot)'} -> {dtor or '(unrelocated)'}"


def classify(base: list[str], target: list[str], ops) -> str:
    """"count" for repeated-action insertions/deletions, else "shape".

    Neither result decides source correctness. ``count`` is commonly an
    inline-boundary delta; ``shape`` requires constructor/destructor and state-
    map adjudication before it can be called an authored cleanup defect.
    """
    common = set(base) & set(target)
    for op, i1, i2, j1, j2 in ops:
        if op == "replace":
            return "shape"
        extra = base[i1:i2] + target[j1:j2]
        if any(a not in common for a in extra):
            return "shape"
    return "count"


#: `__ehunwind$<parent>$<n>` / `__ehreg$<parent>`: the parent is what groups a
#: census row, because the funclet INDEX is per-side address order and shifts
#: whenever one side has a funclet the other does not.
BAND_ROW = re.compile(r"^__eh(?:unwind|reg)\$(?P<parent>.*?)(?:\$\d+)?$")

_OBJ_CACHE: dict = {}


def _cached_obj(side: str, unit: str) -> Obj:
    key = (side, unit)
    if key not in _OBJ_CACHE:
        _OBJ_CACHE[key] = _obj(side, unit)
    return _OBJ_CACHE[key]


def _dtors(seq):
    return [a.split(" -> ")[-1] for a in seq]


def _slots(seq):
    return [a.split(" -> ")[0] for a in seq]


def census_verdict(base: list[str], target: list[str]) -> str:
    """What the two action sequences of ONE parent actually differ in."""
    if not base and not target:
        return "empty"
    if _dtors(base) != _dtors(target):
        return "count" if len(base) != len(target) else "dtor-identity"
    if _slots(base) != _slots(target):
        return "slot-shift"
    return "equal"


def census(limit: int, as_json: bool) -> int:
    from collections import Counter, defaultdict
    from gruntz.walls.inventory import build
    rows = [r for r in build(None, 100.0, False)
            if r["symbol"].startswith("__ehunwind$")
            or r["symbol"].startswith("__ehreg$")]
    parents = defaultdict(list)
    for r in rows:
        m = BAND_ROW.match(r["symbol"])
        parents[(r["unit"], m.group("parent"))].append(r)

    out, tally, tally_rows = [], Counter(), Counter()
    for (unit, parent), rs in sorted(parents.items()):
        rec = {"unit": unit, "parent": parent, "rows": len(rs),
               "worst": min(x["cur"] for x in rs)}
        try:
            acts = {side: [action(body, rel) for _i, body, rel in
                           funclets(_cached_obj(side, unit), side, unit, parent)]
                    for side in ("base", "target")}
        except BaseException as err:
            rec["verdict"] = "error"
            rec["why"] = str(err)[:110]
            tally["error"] += 1
            tally_rows["error"] += len(rs)
            out.append(rec)
            continue
        rec["n_base"], rec["n_target"] = len(acts["base"]), len(acts["target"])
        rec["base"], rec["target"] = acts["base"], acts["target"]
        rec["verdict"] = census_verdict(acts["base"], acts["target"])
        tally[rec["verdict"]] += 1
        tally_rows[rec["verdict"]] += len(rs)
        out.append(rec)

    if as_json:
        import json
        json.dump(out, __import__("sys").stdout)
        return 0
    print(f"sub-100 EH-band rows: {len(rows)}  over {len(parents)} parent(s)")
    print("  a funclet is a readout of its PARENT, so the parent is the unit "
          "of work.\n")
    meaning = {
        "slot-shift": "same dtors IN ORDER, different frame slot - the "
                      "PARENT's frame layout",
        "count": "the funclet count differs - the ctor/dtor inline boundary",
        "dtor-identity": "the ordered destructor list differs - structural",
        "equal": "actions identical (the row is byte padding or a referent)",
        "empty": "neither side has a funclet",
        "error": "could not decode",
    }
    for k, n in tally.most_common():
        print(f"  {n:3d} parents {tally_rows[k]:4d} rows  {k:<14} "
              f"{meaning.get(k, '')}")
    print()
    order = {"dtor-identity": 0, "count": 1, "error": 2, "slot-shift": 3}
    for rec in sorted(out, key=lambda r: (order.get(r["verdict"], 9),
                                          -r["rows"]))[:limit]:
        print(f"{rec['verdict']:<14} rows={rec['rows']:<3} "
              f"worst={rec['worst']:6.2f} "
              f"n={rec.get('n_base', '?')}/{rec.get('n_target', '?')}  "
              f"{rec['unit']}/{rec['parent'][:64]}")
        if rec["verdict"] not in ("dtor-identity", "count"):
            continue
        sm = SequenceMatcher(None, _dtors(rec["base"]), _dtors(rec["target"]),
                             autojunk=False)
        for op, i1, i2, j1, j2 in sm.get_opcodes():
            if op == "equal":
                continue
            for x in rec["base"][i1:i2][:4]:
                print(f"      B {x}")
            for x in rec["target"][j1:j2][:4]:
                print(f"      T {x}")
    return 0


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
        print(f"== ACTION SHAPE DIFFERS: {len(ops)} divergence(s) - adjudicate "
              f"ctor/dtor inline boundaries and the parent state map before "
              f"calling this an authored cleanup defect")
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
    ap.add_argument("token", nargs="?",
                    help="hex rva, mangled name, or CClass::Member")
    ap.add_argument("--raw", action="store_true",
                    help="also dump the whole funclet section with relocs")
    ap.add_argument("--census", action="store_true",
                    help="the whole sub-100 EH band, grouped by parent and "
                         "classified: slot-shift / count / dtor-identity")
    ap.add_argument("--limit", type=int, default=60)
    ap.add_argument("--json", action="store_true")
    args = ap.parse_args(argv)
    if args.census:
        return census(args.limit, args.json)
    if not args.token:
        ap.error("a token is required unless --census is given")
    return report(args.token, args.raw)


if __name__ == "__main__":
    raise SystemExit(main())
