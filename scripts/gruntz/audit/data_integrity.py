#!/usr/bin/env python3
"""Ratchet data holes and linked-image referent defects toward zero."""
from __future__ import annotations

import argparse
import csv
import subprocess
from pathlib import Path

from gruntz.core.pe import REPO

BASELINE = REPO / "config/retail/data-integrity-ratchet.tsv"
CANDIDATE = REPO / "build/exe/GRUNTZ.candidate.EXE"
CANDIDATE_MAP = REPO / "build/exe/GRUNTZ.candidate.map"
BASE_OBJS = REPO / "build/objdiff/base"


def _ensure_candidate() -> None:
    inputs = list(BASE_OBJS.glob("*.obj"))
    newest = max((p.stat().st_mtime_ns for p in inputs), default=0)
    stale = (not CANDIDATE.is_file() or not CANDIDATE_MAP.is_file()
             or CANDIDATE.stat().st_mtime_ns < newest
             or CANDIDATE_MAP.stat().st_mtime_ns < newest)
    if stale:
        subprocess.run(["ninja", "build/exe/GRUNTZ.candidate.EXE"],
                       cwd=REPO, check=True)


def _unclaimed_runs() -> int:
    from gruntz.audit.data_access_map import (STRUCTS, attach_pct,
                                               derive_findings)
    from gruntz.core import get_context
    from gruntz.core.access_map import Types, build_claims, sweep

    ctx = get_context()
    types = Types(STRUCTS)
    accesses, cells, _stats = sweep(ctx)
    claims = attach_pct(build_claims(types, ctx.pe))
    findings, _finding_stats = derive_findings(
        ctx, types, accesses, cells, claims)
    return sum(row[0] == "unclaimed" for row in findings)


def measures() -> dict[str, int]:
    from gruntz.audit.image_diff import analyse

    _ensure_candidate()
    _retail, _candidate, reports = analyse()
    return {
        "unclaimed_runs": _unclaimed_runs(),
        "wrong_referent_regions": sum(r.cmp.ref_pairs_bad for r in reports),
        "ordering_only_regions": sum(r.cmp.ref_pairs_reordered for r in reports),
    }


def _read() -> tuple[dict[str, int], dict[str, str]]:
    limits, notes = {}, {}
    with BASELINE.open(newline="") as f:
        for row in csv.DictReader(f, delimiter="\t"):
            limits[row["metric"]] = int(row["maximum"])
            notes[row["metric"]] = row.get("note", "")
    return limits, notes


def _write(values: dict[str, int], notes: dict[str, str]) -> None:
    with BASELINE.open("w", newline="") as f:
        w = csv.writer(f, delimiter="\t", lineterminator="\n")
        w.writerow(("metric", "maximum", "note"))
        for key in ("unclaimed_runs", "wrong_referent_regions",
                    "ordering_only_regions"):
            w.writerow((key, values[key], notes.get(key, "")))


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--gate", action="store_true")
    ap.add_argument("--write", action="store_true",
                    help="bank the current lower counts as the new maxima")
    args = ap.parse_args(argv)
    limits, notes = _read()
    current = measures()
    for key, value in current.items():
        print(f"data-integrity: {key} {value} (ratchet {limits.get(key, -1)})")
    if args.write:
        _write(current, notes)
        print(f"data-integrity: wrote {BASELINE}")
        return 0
    increased = [k for k, v in current.items() if v > limits.get(k, -1)]
    improved = [k for k, v in current.items() if v < limits.get(k, -1)]
    if increased:
        print("data-integrity: REGRESSION: " + ", ".join(increased))
        return 1 if args.gate else 0
    if improved:
        print("data-integrity: improvement must be banked with --write: "
              + ", ".join(improved))
        return 1 if args.gate else 0
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
