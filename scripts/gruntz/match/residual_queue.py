#!/usr/bin/env python3
"""residual_queue.py - live weighted non-exact function queues.

Every objdiff function under 100% fuzzy is joined to its RVA via
symbol_names.csv and assigned an estimated residual-byte cost::

    size * (100 - fuzzy) / 100

The exhaustive queue remains best-first for diagnosis. The matching campaign
queue starts at the weighted median of that residual cost and proceeds toward
the least-matched functions. This makes "start in the middle" mean half of the
remaining fuzzy deficit, not half of the function count.

Ported from homm2 (scripts/homm2/match/residual_queue.py). Reads the two build
artifacts gruntz already emits - build/objdiff/report.json (objdiff fuzzy %) and
build/gen/symbol_names.csv (the RVA inventory) - and writes
build/gen/residual_function_queue.tsv.

Run inside `nix develop .#build`, from the repo/worktree root, AFTER a `gruntz build`:
    gruntz match-queue
"""

import csv
import json
import os
import pathlib

from gruntz.core.library_labels import active_rvas
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
LIBRARY_LABELS = REPO / "config/retail/library_labels.csv"
OUTPUT = REPO / "build/gen/residual_function_queue.tsv"
CAMPAIGN_OUTPUT = REPO / "build/gen/matching_campaign_queue.tsv"


def _rint(s):
    s = str(s).strip()
    return int(s, 16) if s.lower().startswith("0x") else int(s)


def library_rvas(path):
    """Active FID library RVAs, excluding LOW-confidence diagnostic leads."""
    return active_rvas(path)


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
    total_residual = sum(
        row["size"] * (100.0 - row["fuzzy"]) / 100.0 for row in rows)
    cumulative = 0.0
    for rank, row in enumerate(rows, 1):
        row["rank"] = rank
        row["unmatched_pct"] = 100.0 - row["fuzzy"]
        row["residual_bytes"] = row["size"] * row["unmatched_pct"] / 100.0
        row["residual_share_pct"] = (
            row["residual_bytes"] * 100.0 / total_residual if total_residual else 0.0)
        cumulative += row["residual_bytes"]
        row["cumulative_residual_pct"] = (
            cumulative * 100.0 / total_residual if total_residual else 0.0)
    return rows, n_library


def campaign_rows(rows):
    """Return the weighted-middle-to-worst campaign slice.

    ``rows`` must be in best-to-worst fuzzy order with residual weights populated
    by :func:`residual_rows`. The pivot is the first row at or beyond 50% of the
    cumulative remaining byte deficit.
    """
    if not rows:
        return []
    pivot = next(
        (i for i, row in enumerate(rows)
         if row["cumulative_residual_pct"] >= 50.0),
        len(rows) - 1,
    )
    campaign = rows[pivot:]
    for rank, row in enumerate(campaign, 1):
        row["campaign_rank"] = rank
    return campaign


def write_queue(path, rows):
    path = pathlib.Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    fields = (
        "campaign_rank", "rank", "fuzzy", "unmatched_pct", "residual_bytes",
        "residual_share_pct", "cumulative_residual_pct", "unit", "rva", "va",
        "size", "name", "demangled",
    )
    with path.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields, delimiter="\t",
                                lineterminator="\n")
        writer.writeheader()
        for row in rows:
            output = dict(row)
            output["fuzzy"] = "%.6f" % row["fuzzy"]
            output["unmatched_pct"] = "%.6f" % row["unmatched_pct"]
            output["residual_bytes"] = "%.3f" % row["residual_bytes"]
            output["residual_share_pct"] = "%.6f" % row["residual_share_pct"]
            output["cumulative_residual_pct"] = "%.6f" % row["cumulative_residual_pct"]
            output["rva"] = "0x%08x" % row["rva"]
            output["va"] = "0x%08x" % (row["rva"] + IMAGE_BASE)
            writer.writerow({field: output.get(field, "") for field in fields})


def main():
    report = json.loads(REPORT.read_text(encoding="utf-8"))
    rows, n_library = residual_rows(
        report, symbol_inventory(SYMBOLS), library_rvas(LIBRARY_LABELS))
    campaign = campaign_rows(rows)
    write_queue(OUTPUT, rows)
    write_queue(CAMPAIGN_OUTPUT, campaign)
    print("residual queue: %d live non-exact GAME functions -> %s "
          "(%d FID-labeled library carved out)" %
          (len(rows), OUTPUT.relative_to(REPO), n_library))
    if campaign:
        pivot = campaign[0]
        print("matching campaign: %d functions, weighted middle -> least matched -> %s" %
              (len(campaign), CAMPAIGN_OUTPUT.relative_to(REPO)))
        print("  start: rank %d, %.2f%% fuzzy, %.1f residual bytes, %s @ 0x%08x" %
              (pivot["rank"], pivot["fuzzy"], pivot["residual_bytes"],
               pivot["demangled"], pivot["rva"]))


if __name__ == "__main__":
    main()
