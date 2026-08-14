#!/usr/bin/env python3
"""Class-vtable catalog completeness check: every class that HAS a virtual table
must be CATALOGUED, so its delinked vtable datum can be named (single source of
truth for the vtable set).

A class "has a vtable" when ANY of:
  * it has a simple ``??_7<Name>@@6B@`` entry in a retail vtable catalog, OR
  * its body declares a real C++ ``virtual``, OR
  * its body carries a manual vtable stamp (``&...Vtbl`` / ``&..._vftable`` /
    an ``m_vtbl`` / ``m_vptr`` field - the WAP-engine hand-rolled-vtable idiom).

It is "catalogued" (NOT a violator) when ANY of:
  * it has a row in ``data_vtables.tsv`` or ``data_static_libs.tsv``, OR
  * it uses a manual ``&...Vtbl`` stamp - the vtable datum is already named through
    the older ``DATA(g_*Vtbl)`` global binding (a second catalog row would collide on
    that rva; the sweep migrates these, it does not double-bind them), OR
  * its RTTI vtable rva is already a row in build/gen/symbol_names.csv (named via a
    real polymorphic class / the auto ??_7 path).

So the violator worklist is exactly the vtable-bearing classes with NO catalog
entry of any kind. Prints them and
exits nonzero if any. Runnable as ``python -m gruntz.cleanliness.class_vtables``.
(NB: cross-file manual stamps - a ctor in a .cpp that stamps a vtable declared in
another TU - are not seen here; this check reads only each class's own body. And
MFC/CRT library classes may surface; skip those in the sweep per the
game-not-library policy.)
"""
from __future__ import annotations

import re
import sys
from pathlib import Path
from collections import defaultdict

from gruntz.core import vtable_catalog

from gruntz.core.class_meta import (
    MANUAL_STAMP_RE,
    REPO,
    iter_class_defs,
    rel,
    rtti_vtables,
    vtbl_absent_names,
    vtbl_annotated_names,
    vtbl_annotations,
)
def present_rvas():
    """RVAs already carrying a row in build/gen/symbol_names.csv (any kind)."""
    out = set()
    path = REPO / "build" / "gen" / "symbol_names.csv"
    if not path.exists():
        return out
    for ln in path.read_text().splitlines():
        p = ln.split(",", 1)
        if p and p[0].startswith("0x"):
            try:
                out.add(int(p[0], 16))
            except ValueError:
                pass
    return out


def library_vtable_rvas():
    """RVAs of statically linked library vtables. Statically linked, so
    present_rvas() (our emitted base objs) misses them; a class whose RTTI vtable is here IS
    catalogued - we model it as a minimal view, never reconstruct the library class."""
    return {row["rva"] for row in vtable_catalog.library_rows()}


def vtbl_rva_collisions():
    """Compatibility API: structural catalog errors keyed for the old gate."""
    return {i: [(error, vtable_catalog.GAME, 0)]
            for i, error in enumerate(vtable_catalog.validate(vtable_catalog.game_rows()), 1)}


def absent_emitted_in_base(absent_names):
    """[(class, obj)] for every VTBL_ABSENT class whose ??_7 our OWN base objs
    nonetheless DEFINE. Retail provably lacks these vtables, so a base-side
    emission means a ctor/dtor stamp SURVIVED that retail never had - a wrong
    base in the hierarchy (e.g. deriving the grand-base directly where retail
    derives an intermediate whose stamp is the one that lives). Codegen truth
    the source-side census cannot see; found 2026-07-22 (??_7CWapObj emitted by
    ??0CGameLevel / ??0CDDrawWorkerHost while both really derive CLoadable)."""
    if not absent_names:
        return []
    import subprocess
    from gruntz.cleanliness.view_debt import _current_objs
    pats = {f"??_7{n}@@6B@": n for n in absent_names}
    hits = []
    for o in _current_objs():
        out = subprocess.run(["llvm-nm", o], capture_output=True, text=True).stdout
        for ln in out.splitlines():
            p = ln.split()
            if len(p) == 3 and p[1] != "U" and p[2] in pats:
                hits.append((pats[p[2]], o))
    return hits


def _ob1_acks():
    """{(class, unit)} byte-proven /Ob1 readouts - see the TSV's own header.

    Deliberately narrow: an ack names ONE class in ONE unit, so the same class
    emitted anywhere else still fails, and it is REPORTED (not hidden) so the
    row stays visible in every full-tier run."""
    f = REPO / "config/cleanliness/vtbl-absent-ob1-ack.tsv"
    out = set()
    if not f.is_file():
        return out
    for ln in f.read_text().splitlines():
        if not ln.strip() or ln.lstrip().startswith("#"):
            continue
        parts = ln.split("\t")
        if len(parts) >= 3 and parts[2].strip():
            out.add((parts[0].strip(), parts[1].strip()))
    return out


def main() -> int:
    # Report structural catalog errors alongside completeness failures.
    collisions = vtbl_rva_collisions()
    if collisions:
        print(
            f"vtable-catalog errors: {len(collisions)} invalid row(s):",
            file=sys.stderr,
        )
        for rva, sites in sorted(collisions.items(), key=lambda kv: -len(kv[1])):
            names = ", ".join(sorted(n for n, _p, _l in sites))
            print(f"  0x{rva:08x}  <- {len(sites)} classes: {names}", file=sys.stderr)

    rtti = rtti_vtables()
    have_csv = (REPO / "build" / "gen" / "symbol_names.csv").exists()
    present = present_rvas()
    lib_rvas = library_vtable_rvas()
    vtbl_ann = vtbl_annotated_names()
    vtbl_absent = vtbl_absent_names()

    # Aggregate body signals per class NAME (union over its per-TU definitions).
    virtual = defaultdict(bool)
    manual = defaultdict(bool)
    where = {}
    for name, path, lineno, body in iter_class_defs():
        where.setdefault(name, (path, lineno))
        if re.search(r"\bvirtual\b", body):
            virtual[name] = True
        if MANUAL_STAMP_RE.search(body):
            manual[name] = True

    # A VTBL_ABSENT claim contradicting a positive catalog binding is itself a
    # mis-catalog - surface it, never silently prefer either side.
    for name in sorted(vtbl_absent & (set(vtbl_ann) | set(rtti))):
        print(f"vtbl-absent CONTRADICTION: {name} is VTBL_ABSENT but also "
              f"positively bound (catalog/RTTI) - remove one", file=sys.stderr)

    # BASE-SIDE enforcement: a VTBL_ABSENT ??_7 that our own objs emit is a
    # surviving ctor/dtor stamp retail never had - a hierarchy mis-model. FATAL.
    emitted = absent_emitted_in_base(vtbl_absent)
    acked = _ob1_acks()
    kept = []
    for name, obj in emitted:
        unit = Path(obj).stem
        if (name, unit) in acked:
            print(f"vtbl-absent ACKNOWLEDGED: ??_7{name} in {unit} - "
                  f"/Ob1 inline-budget readout, not a mis-model "
                  f"(config/cleanliness/vtbl-absent-ob1-ack.tsv)")
            continue
        kept.append((name, obj))
    emitted = kept
    if emitted:
        print(f"vtbl-absent VIOLATION: {len(emitted)} base-obj emission(s) of "
              f"proven-absent vtables (a ctor/dtor stamp survives that retail "
              f"lacks - fix the emitting class's base):", file=sys.stderr)
        for name, obj in emitted:
            print(f"  ??_7{name}  emitted by {obj}", file=sys.stderr)
        return 1

    violators = []
    have_vtable = 0
    for name, (path, lineno) in sorted(where.items()):
        has_vtable = name in rtti or virtual[name] or manual[name]
        if not has_vtable:
            continue
        have_vtable += 1
        catalogued = (
            name in vtbl_ann
            or name in vtbl_absent
            or manual[name]
            or (name in rtti and (rtti[name] in present or rtti[name] in lib_rvas)))
        if not catalogued:
            reason = "rtti" if name in rtti else "virtual"
            violators.append((name, path, lineno, reason))

    if not have_csv:
        print("class-vtable completeness: WARN build/gen/symbol_names.csv absent - "
              "auto-??_7-catalogued classes will look uncatalogued (run gruntz build "
              "first for an accurate count).", file=sys.stderr)
    if violators:
        print(f"class-vtable completeness: {have_vtable} vtable-bearing class "
              f"name(s); {len(violators)} NOT catalogued:", file=sys.stderr)
        for name, path, lineno, reason in violators:
            print(f"  {rel(path)}:{lineno}: {name}  [{reason}]", file=sys.stderr)
        return 1
    if collisions:
        print(f"class-vtable completeness: all {have_vtable} names catalogued, but "
              f"{len(collisions)} vtable rva(s) are multiply-bound (see above)")
        return 1
    absent_n = len(vtbl_absent)
    print(f"class-vtable completeness: all {have_vtable} vtable-bearing class "
          f"names are catalogued"
          + (f" ({absent_n} via VTBL_ABSENT - proven-absent ??_7)" if absent_n else ""))
    return 0


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
                    help="only validate the game-vtable catalog")
    args = ap.parse_args()
    raise SystemExit(assert_unique_vtbls() if args.assert_unique else main())
