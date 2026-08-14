"""Structural validation of the retail vtable catalogs.

``--assert-unique`` (the only mode) validates both catalogs: unique name/RVA
rows, a legal kind on every game row, sanctioned units, and no game/library
overlap. Per-class completeness is structural now: every retail vtable found by
vtable_scan must be covered (gruntz.cleanliness.vtable_coverage), every covered
one bound/modelled (vtable_owner, vtable_virtuality, vtable_slot_binding) - so
the retired VTBL_ABSENT source marker and its completeness gate have no role
left; a class whose vtable retail never emitted simply has no catalog row.
"""
from __future__ import annotations

import sys

from gruntz.core import vtable_catalog


def assert_unique_vtbls() -> int:
    """Validate unique name/RVA rows and catalog kinds.

    A primary and its through-base name may deliberately share an RVA.
    """
    game = vtable_catalog.game_rows()
    library = vtable_catalog.library_rows()
    errors = vtable_catalog.validate(game) + vtable_catalog.validate(library)
    overlap = {row["rva"] for row in game} & {row["rva"] for row in library}
    errors.extend(f"RVA 0x{rva:06x} appears in both game and library catalogs"
                  for rva in sorted(overlap))
    if not errors:
        print("vtable-catalog: OK - names, RVAs, and kinds are structurally valid")
        return 0
    print(f"vtable-catalog: FATAL - {len(errors)} structural error(s):", file=sys.stderr)
    for error in errors:
        print(f"  {error}", file=sys.stderr)
    return 1


if __name__ == "__main__":
    import argparse
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--assert-unique", action="store_true",
                    help="validate the vtable catalogs (the only mode)")
    ap.parse_args()
    raise SystemExit(assert_unique_vtbls())
