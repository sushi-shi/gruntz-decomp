"""Enumerate reconstruction targets that have no source function claim.

`(unmatched)` is every carved retail `.text` function classified as a genuine
reconstruction target that no `src/` TU has started. `status.py` counts them;
this tool enumerates the same shared set.

The reported TU is only a navigation hint from neighboring claimed code symbols.
RVA proximity does not establish ownership; confirm it from semantic evidence
before adding a reconstruction to that TU.

Usage (from a `nix develop .#build` shell):
  python -m gruntz.audit.unmatched_attribute                 # grouped worklist
  python -m gruntz.audit.unmatched_attribute --bands 4       # + N disjoint RVA bands
  python -m gruntz.audit.unmatched_attribute --tsv out.tsv   # write machine-readable TSV

The tool self-checks its enumeration against `status.engine_universe()`; both
consumers use the same classifier and must agree exactly.
"""

from __future__ import annotations

import argparse
import bisect
import csv
import sys
from pathlib import Path

from gruntz.core.function_universe import classify as classify_function_universe

REPO = Path(__file__).resolve().parents[3]
FUNCS_CSV = REPO / "config/retail/functions.tsv"
SYM_CSV = REPO / "build/gen/symbol_names.csv"


def _load_code_symbols():
    """Return source function claims as sorted ``(rva, unit)`` hints."""
    code_syms: list[tuple[int, str]] = []
    if not SYM_CSV.is_file():
        return code_syms
    with open(SYM_CSV) as f:
        rdr = csv.DictReader(f)
        for r in rdr:
            head = (r.get("rva") or "").strip()
            if not head.lower().startswith("0x"):
                continue
            try:
                rva = int(head, 16)
            except ValueError:
                continue
            if (r.get("kind") or "func").strip() == "func":
                unit = (r.get("unit") or "").strip()
                if unit:
                    code_syms.append((rva, unit))
    code_syms.sort()
    return code_syms


def unmatched_targets():
    """The shared universe's genuine targets with no source function claim."""
    _rows, meta = classify_function_universe(REPO)
    code_syms = _load_code_symbols()
    out = [(row["rva"], row["size"], row["retail_name"])
           for row in meta["unmatched"]]
    return out, code_syms


def attribute(rva, code_syms):
    """Return a candidate unit and explicitly non-authoritative evidence."""
    if not code_syms:
        return ("?", "none", "no symbol map")
    rvas = [r for r, _u in code_syms]
    i = bisect.bisect_left(rvas, rva)
    below = code_syms[i - 1] if i > 0 else None
    above = code_syms[i] if i < len(code_syms) else None
    if below and above:
        if below[1] == above[1]:
            return (below[1], "hint", f"same-unit neighbors {below[1]} "
                    f"({below[0]:#08x}<{rva:#08x}<{above[0]:#08x})")
        db, da = rva - below[0], above[0] - rva
        near = below if db <= da else above
        return (near[1], "hint", f"between {below[1]}@{below[0]:#08x} and "
                f"{above[1]}@{above[0]:#08x}; nearer={near[1]} (+{min(db, da):#x})")
    only = below or above
    return (only[1], "hint", f"edge; nearest {only[1]}@{only[0]:#08x}")


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--bands", type=int, default=0,
                    help="also print N disjoint RVA bands (roughly equal counts) for fan-out")
    ap.add_argument("--tsv", type=str, default="",
                    help="write a machine-readable TSV (rva,size,name,owner,confidence,evidence)")
    args = ap.parse_args(argv)

    if not FUNCS_CSV.is_file():
        print("error: tracked retail function inventory is missing.", file=sys.stderr)
        return 1

    targets, code_syms = unmatched_targets()
    rowset = []
    for rva, sz, name in targets:
        owner, conf, ev = attribute(rva, code_syms)
        rowset.append((rva, sz, name, owner, conf, ev))

    # Self-check against status.py's explicit RVA-based unmatched set.
    try:
        from gruntz.match import status
        eng = status.engine_universe()
        status_un = eng["unmatched_fn"] if eng else None
        if status_un is not None:
            gap = abs(status_un - len(rowset))
            note = "OK" if gap == 0 else "DRIFT — classifier consumers disagree"
            print(f"# self-check: status (unmatched)={status_un}, "
                  f"enumerated={len(rowset)} (gap {gap}: {note})\n")
    except Exception as e:
        print(f"# (self-check skipped: {e})\n")

    by_owner: dict[str, list] = {}
    for row in rowset:
        by_owner.setdefault(row[3], []).append(row)

    print(f"# (unmatched) targets: {len(rowset)}  across {len(by_owner)} owner TU(s)\n")
    for owner in sorted(by_owner, key=lambda o: -len(by_owner[o])):
        items = sorted(by_owner[owner])
        print(f"## {owner:28} {len(items):3} fn(s)  (neighborhood hints only)")
        for rva, sz, name, _o, conf, ev in items:
            print(f"    {rva:#08x}  {sz:5}B  {name}  [{conf}: {ev}]")
        print()

    if args.bands > 0:
        rowset.sort()
        n = len(rowset)
        per = (n + args.bands - 1) // args.bands
        print("=" * 60)
        print(f"# {args.bands} disjoint RVA bands (~{per} fns each) for fan-out:\n")
        for b in range(args.bands):
            chunk = rowset[b * per:(b + 1) * per]
            if not chunk:
                continue
            lo, hi = chunk[0][0], chunk[-1][0]
            owners = sorted({r[3] for r in chunk})
            print(f"BAND {b + 1}: {lo:#08x}..{hi:#08x}  {len(chunk)} fns  "
                  f"owners: {', '.join(owners)}")

    if args.tsv:
        with open(args.tsv, "w") as f:
            f.write("rva\tsize\tname\towner\tconfidence\tevidence\n")
            for rva, sz, name, owner, conf, ev in sorted(rowset):
                f.write(f"{rva:#08x}\t{sz}\t{name}\t{owner}\t{conf}\t{ev}\n")
        print(f"\nwrote {args.tsv}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
