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
    vtbl_annotated_names,
    vtbl_annotations,
)

_RTTI_RE = re.compile(r"^\?\?_7([A-Za-z_]\w*)@@6B@$")
_MANUAL_RE = re.compile(r"&\s*[A-Za-z_]\w*(?:[Vv]tbl|vftable)\b|\bm_v(?:tbl|ptr)")
_BASE_RE = re.compile(
    r"^(?:class|struct)\s+\w+\s*:\s*(?:public\s+|protected\s+|private\s+|virtual\s+)*"
    r"([\w:]+)")


def instantiated_or_based_names():
    """Class NAMES for which a vtable (??_7) is actually EMITTED: the class is INSTANTIATED
    (``new X`` / a by-value declaration), defines an out-of-line member (``X::``), or is used
    as a BASE. A polymorphic class used ONLY through a pointer - a cast-interface / declared-
    only view (AttractActor, CExitV44) - is NEVER instantiated, so cl emits NO vtable for it:
    it is not "missing a VTBL", it has none. (Coverage=100% is independent of this list, so
    excluding such a class can never hide a genuinely-unbound vtable.)"""
    blob = "\n".join(p.read_text(errors="ignore") for p in source_files())
    names = set(re.findall(r'\bnew\s+([A-Z]\w+)', blob))
    names |= set(re.findall(r'\bnew\s*\([^)\n]*\)\s*([A-Z]\w+)', blob))   # placement new
    names |= set(re.findall(r'\b([A-Z]\w+)::~', blob))                     # out-of-line DTOR (only
    # instantiated/based classes emit a dtor; a plain X::method is a non-virtual helper on a
    # cast-interface view - e.g. RezDir::Check on a pointer-only interface - and does NOT imply
    # an emitted ??_7, so a generic X:: is not counted here).
    names |= set(re.findall(r'\b([A-Z]\w+)\s+(?!\*)[a-z]\w*\s*[;,)=\[]', blob))  # by-value decl
    for m in re.finditer(r'\b(?:class|struct)\s+\w+\s*:\s*([^{;]+)\{', blob):
        names |= set(re.findall(r'\b([A-Z]\w+)\b', m.group(1)))            # any base
    return names


def primary_base_names():
    """Names that appear as the PRIMARY (first) base of some class. A primary base's vtable
    is structurally the derived's primary-vtable PREFIX - it emits no distinct ``??_7`` of its
    own unless the base is itself instantiated standalone (which, with vtable-coverage at 100%
    and no rva-collisions, would already have bound it, so it would not be a violator). So a
    virtual-carrying class that is un-VTBL'd yet is only ever a primary base has NO vtable of
    its own to catalog - it is an abstract/intermediate base (CLoadable, AnimWorker), not a
    class 'missing a VTBL'. The concrete leaf that instantiates the chain carries the binding.
    (The head may wrap; join the decl line with the next two so ``X :\\n  public Base`` parses.)"""
    bases = set()
    for path in source_files():
        lines = path.read_text(errors="ignore").splitlines()
        for i, ln in enumerate(lines):
            s = ln.lstrip()
            if s.startswith(("class ", "struct ")) and ":" in " ".join(lines[i:i + 3]).split("{", 1)[0]:
                decl = " ".join(lines[i:i + 3]).split("{", 1)[0]
                m = _BASE_RE.match(decl.strip())
                if m:
                    bases.add(m.group(1).split("::")[-1])
    return bases


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


_RELOC_RE = re.compile(r"\bRELOC_VTBL\s*\(\s*([\w:]+)\s*,\s*(0x[0-9a-fA-F]+)\s*\)")


def reloc_vtbl_annotations():
    """{class_name: rva} for every RELOC_VTBL(type, addr) - a matching-required placeholder
    whose ??_7 reloc-masks the retail vtable `addr` (bound by a DIFFERENT real class). Text-
    scanned tree-wide like VTBL()."""
    out = {}
    for path in source_files():
        for m in _RELOC_RE.finditer(path.read_text(errors="ignore")):
            try:
                out[m.group(1).split("::")[-1]] = int(m.group(2), 16)
            except ValueError:
                pass
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
    prim_bases = primary_base_names()
    emitted = instantiated_or_based_names()
    reloc = reloc_vtbl_annotations()
    # A RELOC_VTBL(cls, addr) placeholder MUST reloc-mask a vtable REALLY bound by another class -
    # via VTBL(), an RTTI ??_7 in vtable_names.csv, or the MFC/CRT library catalog. NOT merely
    # `present` (symbol_names has non-vtable data too): the alias must point at an actual vtable
    # binding, so it can never hide a genuinely-missing binding or alias a non-vtable datum.
    catalogued_rvas = ({rva for _n, rva, _p, _l in vtbl_annotations()}
                       | set(rtti.values()) | lib_rvas)
    bad_reloc = {n: rva for n, rva in reloc.items() if rva not in catalogued_rvas}
    if bad_reloc:
        print("RELOC_VTBL: reloc-masked rva(s) NOT bound by any real VTBL() - each placeholder "
              "must alias a vtable a real class owns:", file=sys.stderr)
        for n, rva in sorted(bad_reloc.items()):
            print(f"  {n} -> 0x{rva:08x} (no VTBL() binds this rva)", file=sys.stderr)

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

    violators = []
    have_vtable = 0
    for name, (path, lineno) in sorted(where.items()):
        # A ??_7 is emitted only for a class that is RTTI-typed, manually stamped, or
        # (polymorphic AND actually instantiated / defines out-of-line members / is a base).
        # A pointer-only declared-only view is never instantiated -> it has no vtable at all.
        has_vtable = (name in rtti or manual[name]
                      or (virtual[name] and name in emitted))
        if not has_vtable:
            continue
        # A class that is only ever a PRIMARY base (never VTBL'd/RTTI-bound itself) has no
        # distinct vtable of its own - the concrete leaf that instantiates it carries the
        # binding (coverage=100% guarantees the shared vtable IS bound). Not a violator.
        if name not in rtti and name in prim_bases:
            continue
        have_vtable += 1
        catalogued = (
            name in vtbl_ann
            or manual[name]
            or name in reloc
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
              f"name(s); {len(violators)} NOT catalogued (need VTBL or RELOC_VTBL):", file=sys.stderr)
        for name, path, lineno, reason in violators:
            print(f"  {rel(path)}:{lineno}: {name}  [{reason}]", file=sys.stderr)
        return 1
    if bad_reloc:
        return 1
    if collisions:
        print(f"class-vtable completeness: all {have_vtable} names catalogued, but "
              f"{len(collisions)} vtable rva(s) are multiply-bound (see above)")
        return 1
    print(f"class-vtable completeness: all {have_vtable} vtable-bearing class "
          f"names are catalogued")
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
