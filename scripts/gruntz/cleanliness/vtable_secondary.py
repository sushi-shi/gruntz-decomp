#!/usr/bin/env python3
"""Verify that every catalogued multiple-inheritance vtable name is delink-bound."""
from __future__ import annotations

import sys

from gruntz.core import vtable_catalog
from gruntz.core.class_meta import REPO

SYMS = REPO / "build" / "gen" / "symbol_names.csv"


def symbol_names() -> tuple[dict[int, str], set[str]] | tuple[None, None]:
    if not SYMS.exists():
        return None, None
    by_rva, names = {}, set()
    for line in SYMS.read_text().splitlines():
        parts = line.split(",")
        if len(parts) >= 2 and parts[0].startswith("0x"):
            by_rva[int(parts[0], 16)] = parts[1]
            names.add(parts[1])
    return by_rva, names


def main() -> int:
    rows = [row for row in vtable_catalog.game_rows() if row["kind"] == "secondary"]
    if "--list" in sys.argv:
        for row in rows:
            print(f"0x{row['rva']:06x}  {row['name']}")
        return 0

    by_rva, names = symbol_names()
    if names is None:
        print("secondary-vtable coverage: WARN symbol_names.csv absent; run gruntz build")
        return 0
    unbound = [row for row in rows if row["name"] not in names]
    if unbound:
        print("secondary-vtable coverage: catalog names not bound in symbol_names.csv:",
              file=sys.stderr)
        for row in unbound:
            actual = by_rva.get(row["rva"], "unnamed")
            print(f"  {row['name']} at 0x{row['rva']:06x} (currently {actual})",
                  file=sys.stderr)
        return 1
    print(f"secondary-vtable coverage: all {len(rows)} catalogued MI names bound")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
