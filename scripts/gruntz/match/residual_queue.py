#!/usr/bin/env python3
"""residual_queue.py - the exhaustive live non-exact function queue.

Every objdiff function under 100% fuzzy, joined to its RVA via symbol_names.csv,
sorted best-first (highest fuzzy, then RVA). This is the persistent worklist the
max-% campaign drains: a worker pops the top rows and permutes each to 100%.

Ported from homm2 (scripts/homm2/match/residual_queue.py). Reads the two build
artifacts gruntz already emits - build/objdiff/report.json (objdiff fuzzy %) and
build/gen/symbol_names.csv (the RVA inventory) - and writes
build/gen/residual_function_queue.tsv.

Run inside `nix develop .#build`, from the repo/worktree root, AFTER a `gruntz build`:
    python3 -m gruntz.match.residual_queue
"""

import csv
import json
import os
import pathlib

from gruntz.core.pe import IMAGEBASE as IMAGE_BASE

# Anchor on the CWD's repo root (flake.nix), NOT the package location: run from a
# worktree, we must read THAT tree's build/ artifacts, never main (PYTHONPATH can
# point at main). Mirrors gruntz.permute.permute.
_CWD = pathlib.Path.cwd()
REPO = pathlib.Path(
    next((str(p) for p in [_CWD, *_CWD.parents] if (p / "flake.nix").exists()),
         os.environ.get("REPO") or str(_CWD)))
REPORT = REPO / "build/objdiff/report.json"
SYMBOLS = REPO / "build/gen/symbol_names.csv"
LIBRARY_LABELS = REPO / "config/library_labels.csv"
OUTPUT = REPO / "build/gen/residual_function_queue.tsv"


def _rint(s):
    s = str(s).strip()
    return int(s, 16) if s.lower().startswith("0x") else int(s)


def library_rvas(path):
    """FID-identified CRT/MFC library RVAs (config/library_labels.csv). These are NOT
    reconstruction targets - library code is IDENTIFIED (via FID matching) and carved
    out, never matched - so they must not appear in the work queue."""
    rvas = set()
    p = pathlib.Path(path)
    if not p.is_file():
        return rvas
    with p.open(encoding="latin-1", newline="") as stream:
        for row in csv.DictReader(stream):
            try:
                rvas.add(_rint(row["rva"]))
            except (KeyError, ValueError):
                pass
    return rvas


def symbol_inventory(path):
    rows = {}
    with pathlib.Path(path).open(encoding="latin-1", newline="") as stream:
        for row in csv.DictReader(stream):
            if row.get("kind") != "func":
                continue
            rows[(row["unit"], row["name"])] = row
    return rows


def residual_rows(report, symbols, lib_rvas=frozenset()):
    rows = []
    missing = []
    n_library = 0
    for unit in report["units"]:
        unit_name = unit["name"]
        for function in unit.get("functions", []):
            fuzzy = float(function.get("fuzzy_match_percent", 0.0) or 0.0)
            if fuzzy >= 100.0:
                continue
            key = (unit_name, function["name"])
            symbol = symbols.get(key)
            if symbol is None:
                missing.append(key)
                continue
            rva = int(symbol["rva"], 0)
            if rva in lib_rvas:
                # FID-identified CRT/MFC library - carved out, not a work item.
                n_library += 1
                continue
            rows.append({
                "unit": unit_name,
                "rva": rva,
                "fuzzy": fuzzy,
                "size": int(function.get("size", 0) or 0),
                "name": function["name"],
                "demangled": function.get("metadata", {}).get(
                    "demangled_name", function["name"]),
            })
    if missing:
        sample = ", ".join("%s:%s" % row for row in missing[:5])
        raise ValueError("%d report functions lack an RVA inventory row: %s" %
                         (len(missing), sample))
    rows.sort(key=lambda row: (-row["fuzzy"], row["rva"], row["unit"], row["name"]))
    for rank, row in enumerate(rows, 1):
        row["rank"] = rank
    return rows, n_library


def write_queue(path, rows):
    path = pathlib.Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    fields = ("rank", "fuzzy", "unit", "rva", "va", "size", "name", "demangled")
    with path.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields, delimiter="\t",
                                lineterminator="\n")
        writer.writeheader()
        for row in rows:
            output = dict(row)
            output["fuzzy"] = "%.6f" % row["fuzzy"]
            output["rva"] = "0x%08x" % row["rva"]
            output["va"] = "0x%08x" % (row["rva"] + IMAGE_BASE)
            writer.writerow({field: output.get(field, "") for field in fields})


def main():
    report = json.loads(REPORT.read_text(encoding="utf-8"))
    rows, n_library = residual_rows(
        report, symbol_inventory(SYMBOLS), library_rvas(LIBRARY_LABELS))
    write_queue(OUTPUT, rows)
    print("residual queue: %d live non-exact GAME functions -> %s "
          "(%d FID-labeled library carved out)" %
          (len(rows), OUTPUT.relative_to(REPO), n_library))


if __name__ == "__main__":
    main()
