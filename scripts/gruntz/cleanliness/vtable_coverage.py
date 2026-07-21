#!/usr/bin/env python3
"""gruntz.cleanliness.vtable_coverage - every vtable in GRUNTZ.EXE must be COVERED.

The authoritative vtable set comes from OUR analysis (``gruntz.core.vtable_scan``),
which finds every vtable by scanning the retail image for maximal stride-4 runs of
data words that reference into ``.text`` (i.e. runs of function pointers), cut at
COL / code-referenced starts. RTTI only *names* them; it does not *detect* them - so
this covers the NON-RTTI WAP/engine vtables too, not just the 224 RTTI classes.

A vtable (rva) is COVERED when EITHER:
  * its rva is named in build/gen/symbol_names.csv - the reconstruction binds it
    (a real polymorphic ``??_7`` / a ``VTBL(...)`` / a ``DATA()`` datum), OR
  * its rva is listed in config/library_vtables.csv - an MFC/CRT/iostream library
    vtable we deliberately DON'T reconstruct (statically linked), catalogued there
    instead of in source.

Anything left is a GAME/engine vtable with no source binding: a real gap. This tool
prints the gaps and exits nonzero. Wired into ``gruntz build`` as a FATAL gate, so
a vtable can never go uncovered. Runnable as ``python -m gruntz.cleanliness.vtable_coverage``
(``--list`` for every covered/uncovered row).
"""
from __future__ import annotations

import csv
import subprocess
import sys
import tempfile
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parent)
LIB_CSV = REPO / "config" / "library_vtables.csv"
SYMS = REPO / "build" / "gen" / "symbol_names.csv"

# vtable_scan confidences that ARE real vtables (it excludes 'unref' = EH/switch tables).
REAL_CONF = {"rtti", "code-ref", "code-ref-weak"}


def _rva(s):
    try:
        return int(s, 16)
    except (ValueError, TypeError):
        return None


def real_vtables():
    """[(rva, size, conf, rtti_class)] - the analysis' real vtable set."""
    with tempfile.NamedTemporaryFile(suffix=".csv", delete=False) as tf:
        out = tf.name
    try:
        subprocess.run([sys.executable, "-m", "gruntz.core.vtable_scan", "--csv", out],
                       cwd=str(REPO), capture_output=True, text=True, check=True)
        rows = []
        for r in csv.DictReader(open(out)):
            if r.get("confidence") in REAL_CONF:
                rva = _rva(r.get("start_rva"))
                if rva is not None:
                    rows.append((rva, int(r.get("size", 0)), r["confidence"], r.get("rtti_class") or ""))
        return rows
    finally:
        try:
            Path(out).unlink()
        except OSError:
            pass


def covered_rvas():
    """RVAs the reconstruction binds in source (symbol_names) + the library catalog.

    Reads source VTBL(...) annotations DIRECTLY (not just the generated symbol_names)
    so an added binding counts even before the once-per-build symbol_names merge
    re-runs - the gate is never fooled by a stale symbol_names.csv."""
    named = set()
    if SYMS.exists():
        for ln in SYMS.read_text().splitlines():
            rva = _rva(ln.split(",", 1)[0])
            if rva is not None:
                named.add(rva)
    # source VTBL() rvas, tree-wide (robust to symbol_names staleness)
    try:
        from gruntz.core.class_meta import vtbl_annotations
        for _name, rva, _path, _ln in vtbl_annotations():
            named.add(rva)
    except Exception:
        pass
    lib = set()
    if LIB_CSV.exists():
        for r in csv.DictReader(LIB_CSV.open()):
            rva = _rva(r.get("rva"))
            if rva is not None:
                lib.add(rva)
    return named, lib


def main() -> int:
    vts = real_vtables()
    named, lib = covered_rvas()
    gaps = []
    for rva, size, conf, cls in vts:
        if rva in named or rva in lib:
            continue
        gaps.append((rva, size, conf, cls))
    if "--list" in sys.argv:
        for rva, size, conf, cls in sorted(vts):
            where = "symbol_names" if rva in named else ("library.csv" if rva in lib else "UNCOVERED")
            print(f"  0x{rva:06x} sz={size:<3} {conf:<13} {where:<12} {cls}")
    if gaps:
        print(f"vtable-coverage: {len(gaps)} of {len(vts)} analysed vtable(s) UNCOVERED "
              f"(bind in source via VTBL()/DATA(), or add MFC/CRT to config/library_vtables.csv):",
              file=sys.stderr)
        for rva, size, conf, cls in sorted(gaps):
            print(f"  0x{rva:06x} sz={size:<3} {conf:<13} {cls or '(non-rtti)'}", file=sys.stderr)
        return 1
    print(f"vtable-coverage: all {len(vts)} analysed vtables covered "
          f"({len(vts) - sum(1 for v in vts if v[0] in lib)} in source, "
          f"{sum(1 for v in vts if v[0] in lib)} in library catalog)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
