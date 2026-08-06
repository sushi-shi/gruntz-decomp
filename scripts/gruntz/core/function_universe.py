"""Authoritative classification of every admitted retail ``.text`` function.

The tracked retail table supplies boundaries, not ownership. This module joins them
to source function claims, tracked library identities, compiler-private helper
evidence, and the retail opcodes used to recognize linker/import thunks.  Every
consumer of the full-engine denominator must use this module so the filters
cannot drift independently.
"""

from __future__ import annotations

import csv
import os
import struct
from pathlib import Path

from gruntz.core.library_labels import active_rows
from gruntz.core.pe import ILT_HI, ILT_LO
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


def _compiler_private(path: Path) -> dict[int, dict]:
    out = {}
    if not path.is_file():
        return out
    for line in path.read_text(encoding="utf-8").splitlines():
        if not line.startswith("0x"):
            continue
        parts = line.split("\t")
        if len(parts) < 4:
            continue
        try:
            rva, size = _rint(parts[0]), _rint(parts[1])
        except ValueError:
            continue
        out[rva] = {"size": size, "name": parts[2], "unit": parts[3],
                    "evidence": parts[4] if len(parts) > 4 else ""}
    return out


def _compiler_helpers(path: Path) -> dict[int, dict]:
    out = {}
    if not path.is_file():
        return out
    for lineno, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not line.startswith("0x"):
            continue
        parts = line.split("\t")
        if len(parts) != 5:
            raise ValueError(f"{path}:{lineno}: expected 5 tab-separated fields")
        rva, size, target = map(_rint, parts[:3])
        if rva in out:
            raise ValueError(f"{path}:{lineno}: duplicate RVA 0x{rva:08x}")
        out[rva] = {"size": size, "target_rva": target, "role": parts[3],
                    "evidence": parts[4]}
    return out


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
    library = _library(repo / "config/retail/library_labels.csv")
    private = _compiler_private(repo / "config/retail/compiler-generated-functions.tsv")
    helpers = _compiler_helpers(repo / "config/retail/compiler-helper-functions.tsv")
    read = _pe_reader(Path(os.environ.get("GRUNTZ_EXE")
                           or repo / "build/exe/GRUNTZ.EXE"))

    by_rva = {row["rva"]: row for row in rows}
    if strict:
        for rva, helper in helpers.items():
            row = by_rva.get(rva)
            if row is None:
                raise ValueError(f"compiler helper 0x{rva:08x} is not in the retail inventory")
            if row["size"] != helper["size"]:
                raise ValueError(
                    f"compiler helper 0x{rva:08x} size {row['size']}, "
                    f"expected {helper['size']}")
            if read is not None:
                body = read(rva, 5)
                if body is None or body[0] != 0xE9:
                    raise ValueError(f"compiler helper 0x{rva:08x} is not a rel32 jump")
                target = rva + 5 + struct.unpack_from("<i", body, 1)[0]
                if target != helper["target_rva"]:
                    raise ValueError(
                        f"compiler helper 0x{rva:08x} jumps to 0x{target:08x}, "
                        f"expected 0x{helper['target_rva']:08x}")

    ilt_end = ILT_HI
    iat = set()
    if read is not None:
        for row in rows:
            if row["size"] > 7 or row["rva"] in source:
                continue
            body = read(row["rva"], 2)
            if body and body[0] == 0xFF and body[1] in (0x25, 0x15):
                iat.add(row["rva"])

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
        elif ILT_LO <= rva < ilt_end or row["is_thunk"] or rva in iat:
            row["category"] = "thunk"
        elif rva in helpers:
            row.update({"category": "compiler", **helpers[rva]})
        elif rva in private:
            info = private[rva]
            if strict and info["size"] != size:
                raise ValueError(
                    f"compiler-private helper 0x{rva:08x} size {size}, "
                    f"expected {info['size']}")
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
