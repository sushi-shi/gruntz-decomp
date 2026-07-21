#!/usr/bin/env python3
"""Class-vtable catalog completeness check: every class that HAS a virtual table
must be CATALOGUED, so its delinked vtable datum can be named (single source of
truth for the vtable set).

A class "has a vtable" when ANY of:
  * it has an RTTI ``??_7<Name>@@6B@`` entry in config/vtable_names.csv, OR
  * its body declares a real C++ ``virtual``, OR
  * its body carries a manual vtable stamp (``&...Vtbl`` / ``&..._vftable`` /
    an ``m_vtbl`` / ``m_vptr`` field - the WAP-engine hand-rolled-vtable idiom).

It is "catalogued" (NOT a violator) when ANY of:
  * it carries a ``VTBL(Name, 0x..)`` annotation (the preferred single source), OR
  * its ``??_7<Name>@@6B..`` datum is named through a ``// @data-symbol:`` /
    ``// @rva-symbol:`` label (the escape hatch for the MI-decorated
    ``??_7<Name>@@6B<Base>@@@`` name a plain ``VTBL()`` cannot spell), OR
  * it uses a manual ``&...Vtbl`` stamp - the vtable datum is already named through
    the older ``DATA(g_*Vtbl)`` global binding (a VTBL there would just collide on
    that rva; the sweep migrates these, it does not double-bind them), OR
  * its RTTI vtable rva is already a row in build/gen/symbol_names.csv (named via a
    real polymorphic class / the auto ??_7 path).

So the violator worklist is exactly the vtable-bearing classes with NO catalog
entry of any kind - the ones a ``VTBL(...)`` should be added to. Prints them and
exits nonzero if any. Runnable as ``python -m gruntz.match.class_vtables``.
(NB: cross-file manual stamps - a ctor in a .cpp that stamps a vtable declared in
another TU - are not seen here; this check reads only each class's own body. And
MFC/CRT library classes may surface; skip those in the sweep per the
game-not-library policy.)
"""
from __future__ import annotations

import csv
import re
import sys
from collections import defaultdict

from gruntz.match.class_meta import (
    REPO,
    iter_class_defs,
    rel,
    source_files,
    vtbl_absent_names,
    vtbl_annotated_names,
    vtbl_annotations,
)

_RTTI_RE = re.compile(r"^\?\?_7([A-Za-z_]\w*)@@6B@$")
_MANUAL_RE = re.compile(r"&\s*[A-Za-z_]\w*(?:[Vv]tbl|vftable)\b|\bm_v(?:tbl|ptr)")
# A ??_7<Class>@@6B... datum named via a `// @data-symbol:` / `// @rva-symbol:` label
# (the escape hatch for the MI-decorated ??_7<Class>@@6B<Base>@@@ names a plain
# VTBL()'s ??_7<Class>@@6B@ cannot express - e.g. zPTree @0x1e94ac). The datum IS
# named for the delinker, so the class IS catalogued.
_DATA_SYM_VTBL_RE = re.compile(
    r"@(?:data|rva)-symbol:\s*\?\?_7([A-Za-z_]\w*)@@6B")


def rtti_vtables():
    """{class_name: rva} for the simple (global-namespace) ??_7 vtables in
    config/vtable_names.csv."""
    out = {}
    path = REPO / "config" / "vtable_names.csv"
    if not path.exists():
        return out
    for r in csv.reader(path.open()):
        if not r or r[0].strip() in ("", "name") or r[0].lstrip().startswith("#"):
            continue
        m = _RTTI_RE.match(r[0].strip())
        if m:
            try:
                out[m.group(1)] = int(r[1], 16)
            except (ValueError, IndexError):
                pass
    return out


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


def data_symbol_vtable_classes():
    """{class_name} for every ??_7<Class>@@6B... datum named through a
    `// @data-symbol:` / `// @rva-symbol:` label tree-wide. Captures the plain and
    the MI-decorated (??_7<Class>@@6B<Base>@@@) forms alike - both name the class's
    vtable datum for the delinker, so the class is catalogued."""
    out = set()
    for path in source_files():
        for m in _DATA_SYM_VTBL_RE.finditer(path.read_text(errors="ignore")):
            out.add(m.group(1))
    return out


def library_vtable_rvas():
    """RVAs of MFC/CRT library vtables (config/library_vtables.csv). Statically linked, so
    present_rvas() (our emitted base objs) misses them; a class whose RTTI vtable is here IS
    catalogued - we model it as a minimal view, never reconstruct the library class."""
    out = set()
    path = REPO / "config" / "library_vtables.csv"
    if not path.exists():
        return out
    for r in csv.reader(path.open()):
        if len(r) >= 2:
            try:
                out.add(int(r[1], 16))
            except ValueError:
                pass
    return out


def vtbl_rva_collisions():
    """{rva: [(name, path, lineno), ...]} for every rva bound by MORE THAN ONE VTBL()
    annotation - the bijection assert. VTBL(name, rva) must be UNIQUE in both directions:
    a vtable datum has exactly one ??_7 name in retail (rva->name), and one canonical
    binding site. Two DIFFERENT names on one rva = a mis-catalog (aliasing one vtable under
    many names - collapse the fake views); the SAME name twice = a redundant duplicate
    binding (delete one). Either slips past the delink name-injectivity guard, which only
    enforces name->rva, not rva->one-annotation."""
    by_rva = defaultdict(list)
    for name, rva, path, lineno in vtbl_annotations():
        by_rva[rva].append((name, path, lineno))
    return {rva: sites for rva, sites in by_rva.items() if len(sites) > 1}


def main() -> int:
    # rva->name uniqueness: warn on any vtable rva bound by >1 VTBL() (the shared-base
    # / fallback mis-catalog). Reported always, so the build gate surfaces it.
    collisions = vtbl_rva_collisions()
    if collisions:
        dup_lines = sum(len(s) for s in collisions.values())
        print(
            f"vtable-rva collisions: {len(collisions)} rva(s) bound by >1 VTBL() "
            f"({dup_lines} annotations); a vtable datum has ONE ??_7 name:",
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
    data_sym_vtbl = data_symbol_vtable_classes()

    # Aggregate body signals per class NAME (union over its per-TU definitions).
    virtual = defaultdict(bool)
    manual = defaultdict(bool)
    where = {}
    for name, path, lineno, body in iter_class_defs():
        where.setdefault(name, (path, lineno))
        if re.search(r"\bvirtual\b", body):
            virtual[name] = True
        if _MANUAL_RE.search(body):
            manual[name] = True

    # A VTBL_ABSENT claim contradicting a positive binding (VTBL/RTTI) is itself a
    # mis-catalog - surface it, never silently prefer either side.
    for name in sorted(vtbl_absent & (set(vtbl_ann) | set(rtti))):
        print(f"vtbl-absent CONTRADICTION: {name} is VTBL_ABSENT but also "
              f"positively bound (VTBL/RTTI) - remove one", file=sys.stderr)

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
            or name in data_sym_vtbl
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
              f"name(s); {len(violators)} NOT catalogued (need VTBL()):", file=sys.stderr)
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
    """Hard bijection assert (its own build gate): every VTBL(name, rva) is UNIQUE - each
    rva bound by exactly one annotation. Exits nonzero on ANY multiply-bound rva, whether the
    duplicate names match (redundant binding - delete one) or differ (mis-catalog aliasing one
    vtable under several names - collapse the fake views). Independent of the catalog-
    completeness worklist, so a new duplicate fails the build even while violators remain."""
    collisions = vtbl_rva_collisions()
    if not collisions:
        print("vtbl-uniqueness: OK - every VTBL() rva is bound by exactly one annotation")
        return 0
    dup = sum(len(s) for s in collisions.values())
    print(f"vtbl-uniqueness: FATAL - {len(collisions)} rva(s) bound by >1 VTBL() "
          f"({dup} annotations); VTBL(name, rva) must be unique:", file=sys.stderr)
    for rva, sites in sorted(collisions.items(), key=lambda kv: -len(kv[1])):
        names = sorted({n for n, _p, _l in sites})
        kind = ("SAME name, redundant - delete the extra binding" if len(names) == 1
                else "DIFFERENT names, mis-catalog - collapse the fake views to one class")
        print(f"  0x{rva:08x} <- {len(sites)} bindings ({kind}):", file=sys.stderr)
        for n, p, l in sites:
            print(f"      {n}  {rel(p)}:{l}", file=sys.stderr)
    return 1


if __name__ == "__main__":
    import argparse
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--assert-unique", action="store_true",
                    help="only run the VTBL-rva bijection assert (fatal on any duplicate)")
    args = ap.parse_args()
    raise SystemExit(assert_unique_vtbls() if args.assert_unique else main())
