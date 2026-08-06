"""Read the admitted retail function-boundary inventory.

The table is deliberately small in meaning: it records only the start RVA and
byte extent of each function in the shipped executable.  Names and ownership
come from source annotations and the tracked library/compiler maps.  The table
was initially admitted from analysis output, but is hand-owned after admission;
neither the build nor this module consults a Ghidra database.
"""

from __future__ import annotations

import csv
from pathlib import Path

from gruntz.core.pe import IMAGEBASE, REPO

FUNCTIONS = REPO / "config/retail/functions.tsv"


def read(path: Path = FUNCTIONS) -> list[dict]:
    """Return sorted ``{rva, size, kind, name}`` rows from the tracked TSV."""
    rows = []
    with Path(path).open(encoding="utf-8", newline="") as stream:
        reader = csv.DictReader(
            (line for line in stream if not line.lstrip().startswith("#")),
            delimiter="\t",
        )
        for row in reader:
            rva = int(row["rva"], 0)
            size = int(row["size"], 0)
            if size <= 0:
                raise ValueError(f"{path}: function 0x{rva:08x} has invalid size {size}")
            rows.append({
                "rva": rva,
                "size": size,
                "kind": (row.get("kind") or "").strip(),
                "name": f"FUN_{IMAGEBASE + rva:08x}",
            })
    rows.sort(key=lambda item: item["rva"])
    for previous, current in zip(rows, rows[1:]):
        if previous["rva"] == current["rva"]:
            raise ValueError(f"{path}: duplicate function RVA 0x{current['rva']:08x}")
        # Shared compiler tails can make two admitted function bodies overlap;
        # start RVAs are unique, but intervals are not required to partition .text.
    return rows


def by_rva(path: Path = FUNCTIONS) -> dict[int, dict]:
    return {row["rva"]: row for row in read(path)}
