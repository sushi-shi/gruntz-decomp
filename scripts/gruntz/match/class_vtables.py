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

from gruntz.match.class_meta import REPO, iter_class_defs, rel, vtbl_annotated_names

_RTTI_RE = re.compile(r"^\?\?_7([A-Za-z_]\w*)@@6B@$")
_MANUAL_RE = re.compile(r"&\s*[A-Za-z_]\w*(?:[Vv]tbl|vftable)\b|\bm_v(?:tbl|ptr)")


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


def main() -> int:
    rtti = rtti_vtables()
    have_csv = (REPO / "build" / "gen" / "symbol_names.csv").exists()
    present = present_rvas()
    vtbl_ann = vtbl_annotated_names()

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
        has_vtable = name in rtti or virtual[name] or manual[name]
        if not has_vtable:
            continue
        have_vtable += 1
        catalogued = (
            name in vtbl_ann
            or manual[name]
            or (name in rtti and rtti[name] in present))
        if not catalogued:
            reason = "rtti" if name in rtti else "virtual"
            violators.append((name, path, lineno, reason))

    if not have_csv:
        print("class-vtable completeness: WARN build/gen/symbol_names.csv absent - "
              "auto-??_7-catalogued classes will look uncatalogued (run gruntz build "
              "first for an accurate count).", file=sys.stderr)
    if violators:
        print(f"class-vtable completeness: {have_vtable} vtable-bearing class "
              f"name(s); {len(violators)} NOT catalogued (need VTBL):", file=sys.stderr)
        for name, path, lineno, reason in violators:
            print(f"  {rel(path)}:{lineno}: {name}  [{reason}]", file=sys.stderr)
        return 1
    print(f"class-vtable completeness: all {have_vtable} vtable-bearing class "
          f"names are catalogued")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
