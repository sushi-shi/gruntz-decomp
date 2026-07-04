#!/usr/bin/env python3
"""verify_unique_names - FATAL build gate: one mangled FUNCTION name = one RVA.

Linker theorem: MSVC5 keeps ONE COMDAT copy per symbol name, so N retail bodies
at N RVAs means the original link had N DISTINCT names. A reconstruction that
emits the SAME mangled fn name at two RVAs (in any two units) therefore
contradicts the binary - it is a mis-model (the surface-family "5 copies of one
dtor" premise died exactly this way: the five were five distinct classes). It
also breaks name->RVA injectivity for every reverse lookup (sema, Ghidra
navigation, xref-by-name).

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
    total = 0
    for r in csv.DictReader(path.open()):
        if r.get("kind") != "func":
            continue
        total += 1
        by_name[r["name"]].add((r["rva"], r["unit"]))
    dups = {n: sites for n, sites in by_name.items()
            if len({rva for rva, _ in sites}) > 1}
    if dups:
        print(f"name-injectivity: FATAL - {len(dups)} mangled fn name(s) at >1 RVA "
              "(one name = one retail RVA; a persistent duplicate is a mis-model):")
        for name, sites in sorted(dups.items()):
            locs = "  ".join(f"{rva} [{unit}]" for rva, unit in sorted(sites))
            print(f"  {name}\n      {locs}")
        return 1
    print(f"name-injectivity: OK - {total} fn claims, every mangled name at exactly one RVA.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
