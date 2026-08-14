"""Read the admitted retail function-boundary inventory.

The table is deliberately small in meaning: one row per admitted `.text` start,
`rva` + structural `kind` - no sizes, no names, no ownership. A row's EXTENT is
DERIVED: it runs to the next row's rva (the last row runs to `.text`'s virtual
end), so it covers the function's whole retail contribution - trailing jump
tables and alignment included. The exact CODE size of a byte-matched body lives
on its CLAIM instead: the `RVA(rva, size)` macro for game code, the `size`
column of config/retail/functions_zlib.tsv for the vendored TUs. Everything
else is a label, and a label needs only its start.

kind values:
    (empty)  a body - game target, static-lib label, or not yet claimed
    thunk    linker glue (ILT `E9 rel32` band entries, `FF 25` IAT jumps)
    eh       a /GX EH funclet or registration stub in the packed band
    helper   a compiler/source-induced 5-byte forwarder (`jmp rel32`), the
             target re-proven from the EXE bytes by function_universe
    pad      non-code filler (the 0xCC tail after the EH band); partition
             bookkeeping only - read() derives extents from it, then drops it

Names and ownership come from source annotations and the tracked provider
tables. The table was initially admitted from analysis output, but is
hand-owned after admission; neither the build nor this module consults a
Ghidra database.
"""

from __future__ import annotations

import csv
from pathlib import Path

from gruntz.core.pe import IMAGEBASE, REPO, TEXT_END, TEXT_LO

FUNCTIONS = REPO / "config/retail/functions.tsv"

KINDS = ("", "thunk", "eh", "helper", "pad")


def all_rows(path: Path = FUNCTIONS) -> list[dict]:
    """Every row of the partition - `pad` rows included - as
    ``{rva, size, kind, name}`` sorted by rva, sizes derived to the next start."""
    rows = []
    with Path(path).open(encoding="utf-8", newline="") as stream:
        reader = csv.DictReader(
            (line for line in stream if not line.lstrip().startswith("#")),
            delimiter="\t",
        )
        for row in reader:
            rva = int(row["rva"], 0)
            kind = (row.get("kind") or "").strip()
            if kind not in KINDS:
                raise ValueError(f"{path}: unknown kind {kind!r} at 0x{rva:08x}")
            rows.append({
                "rva": rva,
                "kind": kind,
                "name": f"FUN_{IMAGEBASE + rva:08x}",
            })
    rows.sort(key=lambda item: item["rva"])
    for previous, current in zip(rows, rows[1:]):
        if previous["rva"] == current["rva"]:
            raise ValueError(f"{path}: duplicate function RVA 0x{current['rva']:08x}")
    for row in rows:
        if not (TEXT_LO <= row["rva"] < TEXT_END):
            raise ValueError(f"{path}: RVA 0x{row['rva']:08x} outside .text")
    for row, following in zip(rows, rows[1:]):
        row["size"] = following["rva"] - row["rva"]
    if rows:
        rows[-1]["size"] = TEXT_END - rows[-1]["rva"]
    return rows


def read(path: Path = FUNCTIONS) -> list[dict]:
    """The function rows (``pad`` partition rows dropped), extents derived."""
    return [row for row in all_rows(path) if row["kind"] != "pad"]


def by_rva(path: Path = FUNCTIONS) -> dict[int, dict]:
    return {row["rva"]: row for row in read(path)}
