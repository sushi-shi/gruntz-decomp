#!/usr/bin/env python3
"""gruntz.cleanliness.vtable_coverage - every vtable in GRUNTZ.EXE must be COVERED.

The authoritative vtable set comes from OUR analysis (``gruntz.core.vtable_scan``),
which finds every vtable by scanning the retail image for maximal stride-4 runs of
data words that reference into ``.text`` (i.e. runs of function pointers), cut at
COL / code-referenced starts. RTTI only *names* them; it does not *detect* them - so
this covers the NON-RTTI WAP/engine vtables too, not just the 224 RTTI classes.

A vtable is covered when its RVA is present in either the manually maintained
game catalog or the statically linked library catalog.

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

from gruntz.core import vtable_catalog

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parent)

# vtable_scan confidences that ARE real vtables (it excludes 'unref' = EH/switch tables).
REAL_CONF = {"rtti", "code-ref", "code-ref-weak"}


def _rva(s):
    try:
        return int(s, 16)
    except (ValueError, TypeError):
        return None


def real_vtables():
    """[(rva, size, conf, rtti_class, base_off)] - the analysis' real vtable set.
    base_off>0 marks a SECONDARY (multiple-inheritance) vtable - a non-primary
    polymorphic base sub-object's vftable, the exact kind that can be silently
    'missed'. It is a real vtable rva like any other, so requiring it COVERED here
    is what makes 'no secondary can be missed' an image-authoritative guarantee."""
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
                    rows.append((rva, int(r.get("size", 0)), r["confidence"],
                                 r.get("rtti_class") or "", int(r.get("base_off") or 0)))
        return rows
    finally:
        try:
            Path(out).unlink()
        except OSError:
            pass


def covered_rvas():
    """Game and library catalog RVAs."""
    game = {row["rva"] for row in vtable_catalog.game_rows()}
    library = {row["rva"] for row in vtable_catalog.library_rows()}
    return game, library


def main() -> int:
    vts = real_vtables()
    named, lib = covered_rvas()
    gaps = []
    for rva, size, conf, cls, boff in vts:
        if rva in named or rva in lib:
            continue
        gaps.append((rva, size, conf, cls, boff))
    if "--list" in sys.argv:
        for rva, size, conf, cls, boff in sorted(vts):
            where = "game.csv" if rva in named else ("library.csv" if rva in lib else "UNCOVERED")
            sec = f" +{boff}" if boff else ""
            print(f"  0x{rva:06x} sz={size:<3} {conf:<13} {where:<12} {cls}{sec}")
    if gaps:
        n_sec = sum(1 for g in gaps if g[4])
        print(f"vtable-coverage: {len(gaps)} of {len(vts)} analysed vtable(s) UNCOVERED "
              f"({n_sec} secondary/MI; add each row to data_vtables.tsv or "
              f"data_static_libs.tsv):", file=sys.stderr)
        for rva, size, conf, cls, boff in sorted(gaps):
            sec = f" +{boff} (SECONDARY)" if boff else ""
            print(f"  0x{rva:06x} sz={size:<3} {conf:<13} {cls or '(non-rtti)'}{sec}", file=sys.stderr)
        return 1
    n_lib = sum(1 for v in vts if v[0] in lib)
    n_sec = sum(1 for v in vts if v[4])
    print(f"vtable-coverage: all {len(vts)} analysed vtables covered "
          f"({len(vts) - n_lib} in game catalog, {n_lib} in library catalog; "
          f"{n_sec} are secondary/MI vtables - none missed)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
