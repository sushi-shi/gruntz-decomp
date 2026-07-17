#!/usr/bin/env python3
"""verify_unique_names - FATAL build gate: one mangled FUNCTION name = one RVA, and
one stretch of retail .text = one claim.

Linker theorem: MSVC5 keeps ONE COMDAT copy per symbol name, so N retail bodies
at N RVAs means the original link had N DISTINCT names. A reconstruction that
emits the SAME mangled fn name at two RVAs (in any two units) therefore
contradicts the binary - it is a mis-model (the surface-family "5 copies of one
dtor" premise died exactly this way: the five were five distinct classes). It
also breaks name->RVA injectivity for every reverse lookup (sema, Ghidra
navigation, xref-by-name).

SECOND CHECK - overlapping RANGES (added 2026-07-17, ML1). Name-injectivity alone
enforces one RVA per NAME; it never looks at EXTENTS. So two different names could
claim overlapping [rva, rva+size) stretches and both get scored, indefinitely:

  * `?Serialize@CMapMgr@@QAEHHHHH@Z` RVA(0x9356c, 0x38) sat ENTIRELY INSIDE
    `?BroadcastCmd@CGruntzMgr@@QAEHHHHH@Z` RVA(0x93460, 0x15c). It was not a function
    at all - it was BroadcastCmd's tail, carved out by a trace and modelled as its own
    method, then parked @early-stop at ~38% behind a fabricated ABI note explaining why
    "no source form" could match it. Nothing referenced it. Deleting it RAISED the
    tracked metric. That is what an unchecked extent buys.
  * The first run of this check then found two more, both over-declared sizes:
    LaunchWebBrowser 0x264->0x170 (swallowed its own padding + a whole real function),
    CMoviePlayer::InitMode 0x14e->0x120 (ran 0x2e into Teardown).

An overlap is always a defect - MSVC5 has no /OPT:ICF, so no two functions share bytes.
It is either a wrong size arg (usually) or a phantom claim on a fragment of a real
function (worse). Rows with no size are skipped: they cannot be checked, and a missing
size is not evidence of an overlap.

Reads build/gen/symbol_names.csv (kind == func); exits 1 printing every
duplicate. DATA globals are exempt here (multi-TU extern pins of one object are
legitimate and separately warned by the labels step).

Run standalone: python -m gruntz.match.verify_unique_names [--csv PATH]
"""
from __future__ import annotations

import collections
import csv
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
DEFAULT = REPO / "build" / "gen" / "symbol_names.csv"


def main() -> int:
    path = DEFAULT
    if "--csv" in sys.argv:
        path = Path(sys.argv[sys.argv.index("--csv") + 1])
    if not path.is_file():
        print(f"verify_unique_names: {path} missing - the labels step must run "
              "before this gate (never vacuous).", file=sys.stderr)
        return 1
    by_name: dict = collections.defaultdict(set)
    extents: list[tuple[int, int, str, str]] = []  # (rva, size, name, unit)
    total = 0
    for r in csv.DictReader(path.open()):
        if r.get("kind") != "func":
            continue
        total += 1
        by_name[r["name"]].add((r["rva"], r["unit"]))
        try:  # size is optional (an @rva-symbol pin may carry none) -> uncheckable
            extents.append((int(r["rva"], 16), int(r["size"], 16), r["name"], r["unit"]))
        except (KeyError, ValueError, TypeError):
            pass
    dups = {n: sites for n, sites in by_name.items()
            if len({rva for rva, _ in sites}) > 1}
    if dups:
        print(f"name-injectivity: FATAL - {len(dups)} mangled fn name(s) at >1 RVA "
              "(one name = one retail RVA; a persistent duplicate is a mis-model):")
        for name, sites in sorted(dups.items()):
            locs = "  ".join(f"{rva} [{unit}]" for rva, unit in sorted(sites))
            print(f"  {name}\n      {locs}")
        return 1

    overlaps = _overlaps(extents)
    if overlaps:
        print(f"claim-extents: FATAL - {len(overlaps)} overlapping RVA range(s). MSVC5 has "
              "no /OPT:ICF, so two functions never share bytes: each is a wrong size arg, "
              "or a phantom claim on a fragment of the function above it.")
        for a, b in overlaps:
            kind = "CONTAINS" if a[0] + a[1] >= b[0] + b[1] else "overlaps"
            print(f"  0x{a[0]:06x}..0x{a[0]+a[1]:06x} [{a[3]}] {a[2]}\n"
                  f"      {kind} 0x{b[0]:06x}..0x{b[0]+b[1]:06x} [{b[3]}] {b[2]}\n"
                  f"      -> shrink the first claim's RVA() size to its real end, or (if it "
                  f"is not a function) delete it")
        return 1

    print(f"name-injectivity: OK - {total} fn claims, every mangled name at exactly one RVA.")
    print(f"claim-extents: OK - {len(extents)} sized claim(s), no overlapping RVA ranges "
          f"({total - len(extents)} unsized, not checkable).")
    return 0


def _overlaps(extents):
    """Every adjacent pair whose [rva, rva+size) ranges intersect. Sorting by rva makes
    adjacent comparison sufficient for the FIRST overlap of each run, which is what a
    gate needs - it names the offender, and the fix re-runs the check."""
    out = []
    rows = sorted(extents)
    for a, b in zip(rows, rows[1:]):
        if a[0] + a[1] > b[0]:
            out.append((a, b))
    return out


if __name__ == "__main__":
    raise SystemExit(main())
