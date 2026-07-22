#!/usr/bin/env python3
"""gruntz.cleanliness.vtable_secondary - the SECONDARY (multiple-inheritance)
vtable coverage gate: the bounded completion of the VTBL2 work.

A multiply-inheriting class emits, besides its primary ``??_7<C>@@6B@`` vtable,
one SECONDARY vtable ``??_7<C>@@6B<Base>@@@`` per extra polymorphic base
sub-object (a base at a nonzero offset). VTBL2(derived, base, addr) binds each.

Why a DEDICATED gate (vtable_coverage is not enough). vtable_coverage asserts
every real vtable RVA is COVERED (named in symbol_names.csv OR the library
catalog). That protects most secondaries - deleting the VTBL2 leaves the rva
UNCOVERED, so coverage fires. But cl names a class's OWN +0x00 (primary) vtable
THROUGH its ultimate polymorphic base when that base is at offset 0 (e.g.
??_7CButeNode@@6BCContainerErr@@@ at 0x1f051c, the same datum a plain
VTBL(CButeNode) also binds). There the rva stays covered by the plain VTBL, so
dropping the through-base VTBL2 silently regresses the NAME with no gate firing.
This gate closes that hole by locking the through-base secondary vtable NAMES.

Two FATAL checks (both drive to the frozen census config/secondary-vtables.tsv):

  * STRUCTURE - the set of source VTBL2(C, base, rva) -> (rva,
    ??_7<C>@@6B<base>@@@) pairs must EQUAL the census. A dropped VTBL2 (deletion),
    a renamed one, or an unrecorded new one all diverge -> FATAL. Regenerate the
    census deliberately after modeling a new MI class:
        python -m gruntz.cleanliness.vtable_secondary --write-baseline
  * BINDING - every census NAME must actually be bound in build/gen/symbol_names.csv.
    Proves the VTBL2 survived the delink per-rva dedup and emitted its symbol (so a
    through-base alias that lost the dedup, or a TU that never compiled, is caught).

The image-completeness side ("no game secondary silently absent from source") is
already enforced by gruntz.cleanliness.vtable_coverage: every image-proven
secondary (a base_off>0 vtable in gruntz.core.vtable_scan) is a real vtable rva it
requires covered. This gate is the NAME-level complement, and needs no binary scan
(census + source + symbol_names only), so it is sub-second and runs every build.

Runnable as ``python -m gruntz.cleanliness.vtable_secondary`` (``--list`` to dump
the census + source VTBL2 set; ``--write-baseline`` to regenerate the census).
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

from gruntz.core.class_meta import REPO, rel, source_files, _blank_comments

CENSUS = REPO / "config" / "secondary-vtables.tsv"
SYMS = REPO / "build" / "gen" / "symbol_names.csv"

# VTBL2(derived, base, 0xrva) - the same spelling labels.py text-scans. The
# emitted secondary vtable symbol is ??_7<derived>@@6B<base>@@@.
_VTBL2_RE = re.compile(
    r"\bVTBL2\s*\(\s*([A-Za-z_]\w*)\s*,\s*([A-Za-z_]\w*)\s*,\s*(0x[0-9a-fA-F]+)\s*\)")


def secondary_name(derived: str, base: str) -> str:
    return f"??_7{derived}@@6B{base}@@@"


def source_vtbl2():
    """{rva: name} for every VTBL2(derived, base, rva) in src/+include/ (comments
    blanked, rva.h's own #define excluded by source_files()). rva is an int."""
    out = {}
    for path in source_files():
        text = _blank_comments(path.read_text(errors="ignore"))
        for m in _VTBL2_RE.finditer(text):
            rva = int(m.group(3), 16)
            out[rva] = secondary_name(m.group(1), m.group(2))
    return out


def load_census():
    """{rva: name} from config/secondary-vtables.tsv (rva<TAB>name; # comments)."""
    out = {}
    if not CENSUS.exists():
        return out
    for ln in CENSUS.read_text().splitlines():
        ln = ln.strip()
        if not ln or ln.startswith("#"):
            continue
        parts = ln.split("\t")
        if len(parts) >= 2:
            try:
                out[int(parts[0], 16)] = parts[1].strip()
            except ValueError:
                pass
    return out


def symbol_names():
    """(rva->name, set(names)) from build/gen/symbol_names.csv, or (None, None)
    when it is absent (pre-build)."""
    if not SYMS.exists():
        return None, None
    by_rva, names = {}, set()
    for ln in SYMS.read_text().splitlines():
        p = ln.split(",")
        if len(p) >= 2 and p[0].startswith("0x"):
            try:
                by_rva[int(p[0], 16)] = p[1]
            except ValueError:
                continue
            names.add(p[1])
    return by_rva, names


def write_baseline():
    src = source_vtbl2()
    lines = [
        "# secondary (multiple-inheritance) vtable census - the FROZEN complete set of",
        "# through-base ??_7<Class>@@6B<Base>@@@ vtable names our source binds via VTBL2.",
        "#",
        "# WHY a frozen census (not just re-derived from source): a source scan cannot",
        "# detect a DELETED VTBL2 line. vtable_coverage catches a dropped secondary only",
        "# when its rva goes uncovered - but a through-base PRIMARY alias (a VTBL2 that",
        "# shares its rva with a plain VTBL, e.g. CButeNode/CBSecStream @+0x00) keeps the",
        "# rva covered by the plain VTBL, so the correct through-base NAME silently",
        "# vanishes from symbol_names.csv with no gate firing. This census locks each name:",
        "# gruntz.cleanliness.vtable_secondary FATALs if any listed name is no longer bound.",
        "#",
        "# Regenerate deliberately (after modeling a NEW MI class's secondary) with:",
        "#   python -m gruntz.cleanliness.vtable_secondary --write-baseline",
        "# Columns: rva<TAB>name",
    ]
    for rva in sorted(src):
        lines.append(f"0x{rva:06x}\t{src[rva]}")
    CENSUS.write_text("\n".join(lines) + "\n")
    print(f"wrote {rel(CENSUS)} ({len(src)} secondary vtable(s))")


def main() -> int:
    if "--write-baseline" in sys.argv:
        write_baseline()
        return 0

    census = load_census()
    src = source_vtbl2()
    by_rva, names = symbol_names()

    if "--list" in sys.argv:
        print(f"# census ({len(census)}):")
        for rva in sorted(census):
            print(f"  0x{rva:06x}  {census[rva]}")
        print(f"# source VTBL2 ({len(src)}):")
        for rva in sorted(src):
            print(f"  0x{rva:06x}  {src[rva]}")
        return 0

    rc = 0
    # STRUCTURE: source VTBL2 set must EQUAL the frozen census (by (rva, name)).
    added = [(r, src[r]) for r in src if census.get(r) != src[r]]
    removed = [(r, census[r]) for r in census if src.get(r) != census[r]]
    if added or removed:
        rc = 1
        print("secondary-vtable census DRIFT - source VTBL2 set != "
              "config/secondary-vtables.tsv:", file=sys.stderr)
        for rva, nm in sorted(removed):
            reason = ("rva now bound to " + src[rva]) if rva in src else "VTBL2 DELETED/renamed"
            print(f"  MISSING from source: 0x{rva:06x} {nm}  ({reason})", file=sys.stderr)
        for rva, nm in sorted(added):
            print(f"  NEW in source (not in census): 0x{rva:06x} {nm}", file=sys.stderr)
        print("  -> a dropped/renamed secondary regresses a through-base vtable name; "
              "if the change is intended, run "
              "`python -m gruntz.cleanliness.vtable_secondary --write-baseline`.",
              file=sys.stderr)

    # BINDING: every census name must actually be emitted into symbol_names.csv.
    if names is None:
        print("secondary-vtable coverage: WARN build/gen/symbol_names.csv absent - "
              "binding check skipped (run gruntz build first).", file=sys.stderr)
    else:
        unbound = [(r, census[r]) for r in sorted(census)
                   if census[r] not in names]
        if unbound:
            rc = 1
            print("secondary-vtable coverage: census names NOT bound in "
                  "symbol_names.csv (VTBL2 dropped, or lost the delink per-rva dedup):",
                  file=sys.stderr)
            for rva, nm in unbound:
                at = by_rva.get(rva)
                tail = f" (rva 0x{rva:06x} now names {at})" if at else f" (rva 0x{rva:06x} unnamed)"
                print(f"  {nm}{tail}", file=sys.stderr)

    if rc == 0:
        print(f"secondary-vtable coverage: all {len(census)} MI secondary vtable "
              f"name(s) bound; source VTBL2 set matches the census")
    return rc


if __name__ == "__main__":
    raise SystemExit(main())
