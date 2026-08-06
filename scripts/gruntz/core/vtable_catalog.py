"""Read the manually maintained retail vtable catalogs."""
from __future__ import annotations

import csv
import re
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
GAME = REPO / "config" / "retail" / "vtables_game.csv"
LIBRARY = REPO / "config" / "retail" / "vtables_library.csv"

_PRIMARY_RE = re.compile(r"^\?\?_7([A-Za-z_]\w*)@@6B@$")
_SECONDARY_RE = re.compile(r"^\?\?_7([A-Za-z_]\w*)@@6B([A-Za-z_]\w*)@@@$")


def read(path: Path) -> list[dict]:
    """Return normalized rows, retaining the CSV line for diagnostics."""
    lines = path.read_text().splitlines()
    data = [line for line in lines if line.strip() and not line.lstrip().startswith("#")]
    out = []
    for row in csv.DictReader(data):
        row = dict(row)
        row["rva"] = int(row["rva"], 16)
        row["size"] = int(row["size"], 16)
        row["path"] = path
        # Account for leading comments and the header.
        needle = row["name"] + ","
        row["line"] = next(i for i, line in enumerate(lines, 1) if line.startswith(needle))
        out.append(row)
    return out


def game_rows() -> list[dict]:
    return read(GAME)


def library_rows() -> list[dict]:
    return read(LIBRARY)


def primary_class(name: str) -> str | None:
    match = _PRIMARY_RE.match(name)
    return match.group(1) if match else None


def secondary_classes(name: str) -> tuple[str, str] | None:
    match = _SECONDARY_RE.match(name)
    return (match.group(1), match.group(2)) if match else None


def validate(rows: list[dict]) -> list[str]:
    """Return structural catalog errors. Deliberate primary/secondary RVA aliases are valid."""
    errors = []
    seen_pairs = set()
    name_rvas: dict[str, set[int]] = {}
    for row in rows:
        pair = (row["name"], row["rva"])
        if pair in seen_pairs:
            errors.append(f"duplicate row {row['name']} at 0x{row['rva']:06x}")
        seen_pairs.add(pair)
        name_rvas.setdefault(row["name"], set()).add(row["rva"])
        if "kind" in row and row.get("kind") not in {"primary", "secondary", "template"}:
            errors.append(f"invalid kind {row.get('kind')!r} for {row['name']}")
    for name, rvas in name_rvas.items():
        if len(rvas) > 1:
            errors.append(f"{name} is assigned to multiple RVAs: " +
                          ", ".join(f"0x{rva:06x}" for rva in sorted(rvas)))
    return errors
