"""Read the manually maintained retail vtable catalogs."""
from __future__ import annotations

import csv
import re
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
GAME = REPO / "config" / "retail" / "vtables_game.csv"
LIBRARY = REPO / "config" / "retail" / "vtables_library.csv"

# The deliberate holding unit for library DATA no cl output can re-emit (e.g.
# CDialog's messageMap, type_info's vtable): the delinker carves the enrolled
# definition there - which is what lets every reference bind by name - while
# config/units.toml does not declare it, so objdiff never opens it and no
# compared unit carries the unpairable payload. For GAME data that silence is
# a defect (the movieplayer lesson below); for library data it is the point.
LIBRARY_HOLDING_UNIT = "library_data"

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
    """Return structural catalog errors. Deliberate primary/secondary RVA aliases are valid.

    The `unit` column is not decoration: labels.py routes the row's retail extent to
    `<unit>.c.obj`, so a unit `config/units.toml` does not declare sends the payload
    into an object objdiff never opens. It is delinked, it is never compared, and no
    measure reports it - the failure mode is SILENT, which is why it is checked here.
    Measured 2026-08-09: `movieplayer` (dissolved 2026-08-06) still owned the
    `CArray<PLAYLISTINFOSTRUCT*>` vtable and its six relocated words went unscored
    while all three units that really emit it showed the symbol unpaired.
    """
    errors = []
    seen_pairs = set()
    name_rvas: dict[str, set[int]] = {}
    live = None
    for row in rows:
        pair = (row["name"], row["rva"])
        if pair in seen_pairs:
            errors.append(f"duplicate row {row['name']} at 0x{row['rva']:06x}")
        seen_pairs.add(pair)
        name_rvas.setdefault(row["name"], set()).add(row["rva"])
        if "kind" in row and row.get("kind") not in {"primary", "secondary", "template"}:
            errors.append(f"invalid kind {row.get('kind')!r} for {row['name']}")
        unit = (row.get("unit") or "").strip()
        if unit:
            if unit == LIBRARY_HOLDING_UNIT and row["path"] == LIBRARY:
                continue
            if live is None:
                from gruntz.core.manifest import unit_names
                live = unit_names()
            if unit not in live:
                errors.append(
                    f"{row['name']} at 0x{row['rva']:06x} names unit {unit!r}, which "
                    f"config/units.toml does not declare - its extent would be carved "
                    f"into an object objdiff never opens and go silently unscored")
    for name, rvas in name_rvas.items():
        if len(rvas) > 1:
            errors.append(f"{name} is assigned to multiple RVAs: " +
                          ", ".join(f"0x{rva:06x}" for rva in sorted(rvas)))
    return errors
