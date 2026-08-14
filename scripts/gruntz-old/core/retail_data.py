"""Read the admitted retail data census.

`config/retail/data.tsv` is the data analog of `functions.tsv`: one row per
admitted datum START in `.rdata`/`.data`/`.bss`, `rva` + structural `kind` -
no sizes, no names, no ownership. A row's EXTENT is DERIVED: it runs to the
next row within its section, else to the section edge (`gruntz.core.pe`
constants; the image is fixed). Names and ownership come from the providers -
`DATA()`/`DATA_COMPGEN` claims, data_vtables.tsv, data_static_libs.tsv,
data_zlib.tsv, data_compgen.tsv - and from the build-time `??_C@`/`$T` oracles
(strings and FP pools carry no committed names at all).

kind values:
    (empty)  a datum - game target, static-lib label, or not yet claimed
    string   a pooled literal (`??_C@`), name re-proven from the base objs
    fppool   a `$T` floating-point pool constant
    vtable   a `??_7` table (game or library catalog claims it)
    rtti     `??_R*` RTTI records (/GR complete-object locator chains)
    ehtable  a /GX FuncInfo record + its unwind/try maps
    guard    a `??_B` dynamic-init guard byte
    common   a header-inline local static (COFF COMMON, data_compgen class)
    copy     a per-TU copy of a header static (data_compgen class)
    pad      zero fill / alignment - never a reconstruction target

The census is hand-owned after its initial assembly; the build never
regenerates it (`gruntz.audit.data_denominator --check` re-proves kinds and
tiling against the image and the current enrolment every build).
"""

from __future__ import annotations

import csv
from pathlib import Path

from gruntz.core.pe import BSS_END, BSS_LO, DATA_LO, RDATA_END, RDATA_LO, REPO

DATA_CENSUS = REPO / "config/retail/data.tsv"

KINDS = ("", "string", "fppool", "vtable", "rtti", "ehtable", "guard",
         "common", "copy", "pad")

#: region key -> [lo, hi) edges the extent derivation caps against.
REGIONS = {
    "rdata": (RDATA_LO, RDATA_END),
    "data": (DATA_LO, BSS_LO),
    "bss": (BSS_LO, BSS_END),
}


def _region(rva: int) -> str | None:
    for key, (lo, hi) in REGIONS.items():
        if lo <= rva < hi:
            return key
    return None


def all_rows(path: Path = DATA_CENSUS) -> list[dict]:
    """Every census row - `pad` rows included - as
    ``{rva, size, kind, region}`` sorted by rva, sizes derived to the next
    row within the same region (else the region edge)."""
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
            region = _region(rva)
            if region is None:
                raise ValueError(f"{path}: RVA 0x{rva:08x} outside every data region")
            rows.append({"rva": rva, "kind": kind, "region": region})
    rows.sort(key=lambda item: item["rva"])
    for previous, current in zip(rows, rows[1:]):
        if previous["rva"] == current["rva"]:
            raise ValueError(f"{path}: duplicate datum RVA 0x{current['rva']:08x}")
    for row, following in zip(rows, rows[1:] + [None]):
        hi = REGIONS[row["region"]][1]
        if following is not None and following["region"] == row["region"]:
            hi = following["rva"]
        row["size"] = hi - row["rva"]
    return rows


def read(path: Path = DATA_CENSUS) -> list[dict]:
    """The datum rows (``pad`` bookkeeping rows dropped), extents derived."""
    return [row for row in all_rows(path) if row["kind"] != "pad"]


def by_rva(path: Path = DATA_CENSUS) -> dict[int, dict]:
    return {row["rva"]: row for row in all_rows(path)}
