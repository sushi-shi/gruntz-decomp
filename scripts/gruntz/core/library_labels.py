"""Shared contract for the static-library function-label rows.

`config/retail/functions_static_libs.tsv` (rva, name, lib, confidence, source)
labels retail functions proven NOT reconstructable game C++. HIGH, MED, AMBIG,
and CRUNTIME rows are active library claims. LOW rows are retained as
diagnostic leads, but are not authoritative enough to color the executable map,
leave the reconstruction queue, or shrink the full-engine denominator.

An rva may carry several rows (alias spellings / independent provenance for the
same body); a consumer that keys by rva must state its pick explicitly.
"""

from __future__ import annotations

import csv
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
PATH = REPO / "config/retail/functions_static_libs.tsv"


def is_active(row: dict[str, str]) -> bool:
    """Whether a row is authoritative enough to carve out as library code."""
    return (row.get("confidence") or "").strip().upper() != "LOW"


def rows(path=None) -> list[dict[str, str]]:
    """All rows of the static-libs table (LOW leads included)."""
    p = Path(path or PATH)
    if not p.is_file():
        return []
    with p.open(encoding="latin-1", newline="") as stream:
        return list(csv.DictReader(
            (line for line in stream if not line.lstrip().startswith("#")),
            delimiter="\t"))


def active_rows(path=None) -> list[dict[str, str]]:
    """Read the non-LOW rows from the static-libs table."""
    return [row for row in rows(path) if is_active(row)]


def active_rvas(path=None) -> set[int]:
    """Return the valid RVAs claimed by active library-label rows."""
    out: set[int] = set()
    for row in active_rows(path):
        value = (row.get("rva") or "").strip()
        try:
            out.add(int(value, 16) if value.lower().startswith("0x") else int(value))
        except ValueError:
            pass
    return out
