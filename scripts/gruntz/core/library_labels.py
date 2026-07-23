"""Shared contract for tracked FID library-label rows.

HIGH, MED, and AMBIG rows are active library claims. LOW rows are retained as
diagnostic leads, but are not authoritative enough to color the executable map,
leave the reconstruction queue, or shrink the full-engine denominator.
"""

from __future__ import annotations

import csv
from pathlib import Path


def is_active(row: dict[str, str]) -> bool:
    """Whether a CSV row is authoritative enough to carve out as library code."""
    return (row.get("confidence") or "").strip().upper() != "LOW"


def active_rows(path) -> list[dict[str, str]]:
    """Read the non-LOW rows from a library_labels.csv file."""
    p = Path(path)
    if not p.is_file():
        return []
    with p.open(encoding="latin-1", newline="") as stream:
        return [row for row in csv.DictReader(stream) if is_active(row)]


def active_rvas(path) -> set[int]:
    """Return the valid RVAs claimed by active library-label rows."""
    out: set[int] = set()
    for row in active_rows(path):
        value = (row.get("rva") or "").strip()
        try:
            out.add(int(value, 16) if value.lower().startswith("0x") else int(value))
        except ValueError:
            pass
    return out
