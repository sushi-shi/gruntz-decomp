"""Authoritative classification of every admitted retail ``.text`` function.

The tracked retail table supplies starts and structural kinds, not ownership.
This module joins them to source function claims, the static-libs labels, and
the RVA_DYNINIT compiler-private pins; kind=thunk/helper rows replace the old
opcode sniffing (helpers are still re-proven against the EXE bytes).  Every
consumer of the full-engine denominator must use this module so the filters
cannot drift independently.
"""

from __future__ import annotations

import csv
import os
import struct
from pathlib import Path

from gruntz.core.library_labels import active_rows
from gruntz.core.pe import ILT_HI
from gruntz.core.retail_functions import read as read_retail_functions


REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])


def _rint(value: str) -> int:
    value = str(value).strip()
    return int(value, 16) if value.lower().startswith("0x") else int(value)


def _functions(path: Path) -> list[dict]:
    return [{**row, "retail_name": row["name"], "is_thunk": row["kind"] == "thunk"}
            for row in read_retail_functions(path)]


def _source_functions(path: Path) -> dict[int, dict]:
    """Source function claims only; DATA rows must never inflate this universe."""
    out = {}
    if not path.is_file():
        return out
    with path.open(newline="") as stream:
        for row in csv.DictReader(stream):
            if (row.get("kind") or "func").strip() != "func":
                continue
            try:
                rva = _rint(row["rva"])
            except (KeyError, ValueError):
                continue
            out[rva] = row
    return out


def _library(path: Path) -> dict[int, dict]:
    out = {}
    for row in active_rows(path):
        try:
            out[_rint(row["rva"])] = row
        except (KeyError, ValueError):
            continue
    return out


def _compiler_private(repo: Path) -> dict[int, dict]:
    """The RVA_DYNINIT-pinned `$E` helpers (gruntz.core.dyninit). The pin's
    OWNER stands in for a name; the volatile build ordinal is never stored."""
    from gruntz.core.dyninit import rows as dyninit_rows
    return {r["rva"]: {"size": r["size"], "name": r["owner"], "unit": r["unit"],
                       "evidence": r["where"]}
            for r in dyninit_rows(Path(repo))}




def _pe_reader(exe: Path):
    try:
        data = exe.read_bytes()
        pe = struct.unpack_from("<I", data, 0x3C)[0]
        nsec = struct.unpack_from("<H", data, pe + 6)[0]
        optsz = struct.unpack_from("<H", data, pe + 20)[0]
        sections = []
        for i in range(nsec):
            base = pe + 24 + optsz + i * 40
            va = struct.unpack_from("<I", data, base + 12)[0]
            raw_size = struct.unpack_from("<I", data, base + 16)[0]
            raw_ptr = struct.unpack_from("<I", data, base + 20)[0]
            sections.append((va, raw_size, raw_ptr))
    except (OSError, struct.error, IndexError):
        return None

    def read(rva: int, size: int) -> bytes | None:
        for va, raw_size, raw_ptr in sections:
            if va <= rva and rva + size <= va + raw_size:
                off = raw_ptr + rva - va
                return data[off:off + size]
        return None

    return read


def classify(repo: Path = REPO, *, strict: bool = True) -> tuple[list[dict], dict]:
    """Return the classified function rows and universe metadata."""
    repo = Path(repo)
    funcs_path = repo / "config/retail/functions.tsv"
    if not funcs_path.is_file():
        raise FileNotFoundError(funcs_path)
    rows = _functions(funcs_path)
    source = _source_functions(repo / "build/gen/symbol_names.csv")
    library = _library(repo / "config/retail/functions_static_libs.tsv")
    private = _compiler_private(repo)
    read = _pe_reader(Path(os.environ.get("GRUNTZ_EXE")
                           or repo / "build/exe/GRUNTZ.EXE"))

    starts = {row["rva"] for row in rows}
    if strict and read is not None:
        # A kind=helper row states "5-byte forwarder"; re-prove it from the EXE:
        # the body must be `E9 rel32` and the jump must land on an admitted start.
        for row in rows:
            if row["kind"] != "helper":
                continue
            rva = row["rva"]
            body = read(rva, 5)
            if body is None or body[0] != 0xE9:
                raise ValueError(f"compiler helper 0x{rva:08x} is not a rel32 jump")
            target = rva + 5 + struct.unpack_from("<i", body, 1)[0]
            if target not in starts:
                raise ValueError(
                    f"compiler helper 0x{rva:08x} jumps to 0x{target:08x}, "
                    f"which is not an admitted function start")

    ilt_end = ILT_HI

    for row in rows:
        rva, size, name = row["rva"], row["size"], row["retail_name"]
        row.update({"category": "target", "claimed": False, "unit": "",
                    "source_name": "", "lib": "", "confidence": "",
                    "role": "", "evidence": ""})
        if rva in source:
            info = source[rva]
            row.update({"category": "target", "claimed": True,
                        "unit": (info.get("unit") or "").strip(),
                        "source_name": (info.get("name") or "").strip()})
        elif row["is_thunk"]:
            row["category"] = "thunk"
        elif row["kind"] == "helper":
            row.update({"category": "compiler", "role": "forwarder",
                        "evidence": "kind=helper; E9 target re-proven from EXE"})
        elif rva in private:
            info = private[rva]
            if strict and info["size"] > size:
                raise ValueError(
                    f"compiler-private helper 0x{rva:08x} extent {size}, "
                    f"smaller than its RVA_DYNINIT pin ({info['size']})")
            row.update({"category": "compiler", "role": info["name"],
                        "evidence": info["evidence"]})
        elif rva in library:
            info = library[rva]
            row.update({"category": "library", "lib": (info.get("lib") or "").strip(),
                        "confidence": (info.get("confidence") or "").strip(),
                        "source_name": (info.get("name") or "").strip()})
        elif row["kind"] == "eh":
            row["category"] = "eh"

    counts = {}
    code = {}
    for row in rows:
        category = row["category"]
        counts[category] = counts.get(category, 0) + 1
        code[category] = code.get(category, 0) + row["size"]
    unmatched = [row for row in rows if row["category"] == "target" and not row["claimed"]]
    meta = {"ilt_end": ilt_end, "counts": counts, "code": code,
            "unmatched": unmatched, "source": source, "library": library}
    return rows, meta
