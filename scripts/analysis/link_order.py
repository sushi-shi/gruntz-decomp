#!/usr/bin/env python3
"""link_order.py - what the linker layout tells us about the retail build order.

Cross-references three things:
  * build/gen/symbol_names.csv  - retail RVA -> name -> unit, for matched funcs.
  * a candidate link .map        - OUR link's RVA + source object per symbol
                                   (from `gruntz link`; intra-object order == the
                                   compiler's source-definition order).
  * (implicitly) the retail RVA order of the same functions.

and reports the three layout facts that drive matching (see
docs/link-order-investigation.md):

  1. INTRA-TU ORDER. For each TU, is our candidate-link function order the same as
     retail's? They differ iff our src/ defines functions in a different order than
     the original .cpp did - because MSVC emits COMDATs in source order. So a
     mismatch here is a concrete TODO: reorder that .cpp's function definitions to
     retail-RVA order. (zlib TUs already match; they are faithful copies.)

  2. CROSS-TU ORDER == LINK ORDER. Retail .text is a sequence of contiguous,
     mostly-disjoint per-object blocks. Sorting TUs by their retail address block
     recovers the order the objects were fed to the linker - the build order we
     want to replicate.

  3. CONFLATED UNITS. A `unit` whose retail functions split into two well-separated
     address clusters most likely conflates two real TUs (e.g. an engine base class
     + its game-side glue) and should be split.

Usage:
    python3 scripts/analysis/link_order.py \
        --map build/exe/GRUNTZ.candidate.map --names build/gen/symbol_names.csv
"""

import argparse
import csv
import re
from collections import defaultdict
from pathlib import Path

# Two MATCHED functions of one unit are normally separated by the unit's own
# not-yet-matched functions, so small/medium gaps are expected. Only a gap this
# large means the unit's code occupies two genuinely distant regions of .text -
# the signature of a `unit` that conflates two real TUs. (Sparse matching means
# we can't recover exact TU block boundaries, only that the bulk of a unit sits
# in one region.)
REGION_GAP = 0x40000

# A "Publics by Value" .text line:
#   0001:00002d60   ??_ECBattlezDlg@@UAEPAXI@Z  00403d60 f i dialogs.obj
# fields: seg:offset  name  rva  'f' [extra flag letters...]  object
# (mangled names carry no spaces, so the object is always the LAST token).
MAP_SEG_RE = re.compile(r"0001:[0-9a-f]+$")
RVA_RE = re.compile(r"[0-9a-f]{8}$")


def load_retail(names_csv: Path):
    """name -> (rva, unit) and unit -> [(rva, name)], matched funcs only."""
    by_name, by_unit = {}, defaultdict(list)
    with names_csv.open() as f:
        for r in csv.DictReader(ln for ln in f if not ln.lstrip().startswith("#")):
            if (r.get("kind") or "func") != "func":
                continue
            rva = int(r["rva"], 16)
            by_name[r["name"]] = (rva, r["unit"])
            by_unit[r["unit"]].append((rva, r["name"]))
    for u in by_unit:
        by_unit[u].sort()
    return by_name, by_unit


def load_candidate_order(map_path: Path):
    """object -> [name,...] in OUR link's .text order (== source-definition order)."""
    funcs = []
    for ln in map_path.read_text(errors="replace").splitlines():
        p = ln.split()
        if (len(p) >= 5 and MAP_SEG_RE.match(p[0]) and RVA_RE.match(p[2])
                and p[3] == "f"):
            funcs.append((int(p[2], 16), p[1], p[-1]))
    funcs.sort()
    by_obj = defaultdict(list)
    for _, name, obj in funcs:
        by_obj[obj].append(name)
    return by_obj


def regions(rvas, gap=REGION_GAP):
    """Split sorted RVAs into runs separated by > gap. Returns [(lo, hi, count)]."""
    out, run = [], []
    for rva in rvas:
        if run and rva - run[-1] > gap:
            out.append((run[0], run[-1], len(run)))
            run = []
        run.append(rva)
    if run:
        out.append((run[0], run[-1], len(run)))
    return out


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--map", default="build/exe/GRUNTZ.candidate.map")
    ap.add_argument("--names", default="build/gen/symbol_names.csv")
    ap.add_argument("--skip-unit", action="append", default=["engine_label_stubs"],
                    help="units to exclude from the cross-TU order (catch-all backlogs).")
    args = ap.parse_args()

    by_name, by_unit = load_retail(Path(args.names))
    cand = load_candidate_order(Path(args.map)) if Path(args.map).exists() else {}

    # ---- 1. intra-TU order: our candidate-link order vs retail order ----------
    print("=" * 72)
    print("1. INTRA-TU ORDER  (our link order vs retail; mismatch => reorder .cpp)")
    print("=" * 72)
    exact = total = 0
    todo = []
    for obj in sorted(cand):
        unit = obj[:-4] if obj.endswith(".obj") else obj
        our = [n for n in cand[obj] if n in by_name]
        if len(our) < 2:
            continue
        ret_order = sorted(our, key=lambda n: by_name[n][0])
        pos = {n: i for i, n in enumerate(ret_order)}
        inv = sum(1 for i in range(len(our) - 1) if pos[our[i]] > pos[our[i + 1]])
        total += 1
        ok = our == ret_order
        exact += ok
        if not ok:
            todo.append((unit, len(our), inv))
        print(f"  {unit:24} n={len(our):3d}  {'OK' if ok else 'REORDER':>7}"
              f"  ({inv} adjacent inversion{'s' if inv != 1 else ''})")
    print(f"\n  {exact}/{total} TUs already in retail source order.")
    if todo:
        print("  reorder these .cpp function definitions to retail-RVA order:")
        for unit, n, inv in sorted(todo, key=lambda t: -t[2]):
            print(f"     {unit:24} {n:3d} funcs, {inv} inversions")

    # ---- 2 & 3. cross-TU order + conflated-unit detection ---------------------
    print("\n" + "=" * 72)
    print("2. CROSS-TU ORDER == inferred object LINK ORDER (units by min retail RVA)")
    print(f"   3. units spanning >1 region (gap>{REGION_GAP:#x}) likely CONFLATE 2 TUs")
    print("=" * 72)
    skip = set(args.skip_unit)
    rows = []  # (min_rva, unit, max_rva, n, n_regions)
    for unit, fs in by_unit.items():
        if unit in skip:
            continue
        rvas = [rva for rva, _ in fs]
        rows.append((rvas[0], unit, rvas[-1], len(rvas), len(regions(rvas))))
    rows.sort()
    print(f"  {'#':>3} {'min-rva':>9} {'max-rva':>9} {'span':>8} {'n':>3}  unit")
    for i, (lo, unit, hi, n, nreg) in enumerate(rows):
        tag = f"   <- {nreg} regions (conflated TU?)" if nreg > 1 else ""
        print(f"  {i:3d} {lo:9x} {hi:9x} {hi - lo:8x} {n:3d}  {unit}{tag}")

    multi = sorted(u for _, u, _, _, nreg in rows if nreg > 1)
    if multi:
        print(f"\n  conflated-TU candidates (code in >1 distant region of .text, "
              f"gap>{REGION_GAP:#x}): {', '.join(multi)}")
    print("\n  NOTE: matching is sparse, so a unit's matched funcs are interspersed")
    print("  with its not-yet-matched ones; the min-rva ordering above is the")
    print("  inferred object link order. Exact block boundaries need fuller coverage.")


if __name__ == "__main__":
    main()
