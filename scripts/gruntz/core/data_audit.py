#!/usr/bin/env python3
"""Attribute retail .rdata/.data/.bss bytes to source DATA() symbols + fingerprint them.

This is the pure-PE half of the DATA-match loop, ported from the sibling
homm2-decomp project (scripts/homm2/build/link_exe.py: read_pe / classify_pe_storage
/ read_pe_payload_evidence). It reads the RETAIL GRUNTZ.EXE only -- no NB09/PDB, no
delinker, no wine -- so it runs standalone and can never affect the matching graph.

For every named data symbol (kind=data in build/gen/symbol_names.csv, which is
generated from src/ DATA() annotations) it:
  * classifies PE storage (.rdata read-only / .data initialized / .data loader-zero
    tail / other) at the symbol RVA;
  * resolves an EXTENT -- the reviewed symbol_names.csv size when present, else the
    next-data-symbol gap (flagged `inferred`), else the globals.json declared-type
    sizeof when derivable;
  * reads that span, zeroes every HIGHLOW base-relocation field, and records
    sha256(payload), sha256(relocation-normalized), and the HIGHLOW offset set --
    a stable content+relocation fingerprint independent of image base.

The output `build/gen/data_attribution.tsv` is a reviewable ledger: it makes the
data-section attribution explicit (today it is ad-hoc) and gives the future
delink/relink data loop a fixed retail oracle to gate byte content + relocation
topology against. `--json` emits the full per-symbol evidence.

Usage:
  python -m gruntz.core.data_audit                 # -> build/gen/data_attribution.tsv + summary
  python -m gruntz.core.data_audit --json out.json # full per-symbol evidence
  python -m gruntz.core.data_audit --rva 0x1e8e98  # one symbol, printed
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import struct
from dataclasses import dataclass
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
RETAIL_EXE = REPO / "build/exe/GRUNTZ.EXE"
SYMBOLS = REPO / "build/gen/symbol_names.csv"
GLOBALS = REPO / "build/gen/globals.json"
OUTPUT = REPO / "build/gen/data_attribution.tsv"

COFF_SECTION_HEADER_SIZE = 40
IMAGE_DIRECTORY_ENTRY_BASERELOC = 5
IMAGE_REL_BASED_HIGHLOW = 3
PE32_MAGIC = 0x10B


# --- PE reader (ported verbatim in spirit from homm2 link_exe.read_pe) ---------
def read_pe(path: Path) -> dict:
    data = Path(path).read_bytes()
    if len(data) < 0x40 or data[:2] != b"MZ":
        raise ValueError(f"not a PE file: {path}")
    pe_offset = struct.unpack_from("<I", data, 0x3C)[0]
    if data[pe_offset:pe_offset + 4] != b"PE\0\0":
        raise ValueError(f"missing PE signature: {path}")
    coff = pe_offset + 4
    section_count = struct.unpack_from("<H", data, coff + 2)[0]
    optional_size = struct.unpack_from("<H", data, coff + 16)[0]
    optional = coff + 20
    if struct.unpack_from("<H", data, optional)[0] != PE32_MAGIC:
        raise ValueError(f"expected PE32 optional header: {path}")
    section_offset = optional + optional_size
    sections = {}
    for index in range(section_count):
        offset = section_offset + index * COFF_SECTION_HEADER_SIZE
        name = data[offset:offset + 8].split(b"\0", 1)[0].decode("ascii", "replace")
        virtual_size, rva, raw_size, raw_offset = struct.unpack_from("<IIII", data, offset + 8)
        sections[name] = {
            "rva": rva, "virtual_size": virtual_size,
            "raw_size": raw_size, "raw_offset": raw_offset,
            "characteristics": struct.unpack_from("<I", data, offset + 36)[0],
        }
    return {
        "data": data,
        "image_base": struct.unpack_from("<I", data, optional + 28)[0],
        # SizeOfRawData is rounded UP to FileAlignment, so a section's last
        # <FileAlignment bytes may be padding rather than emitted content. Needed to
        # bound that ambiguity - see classify_pe_storage().
        "file_alignment": struct.unpack_from("<I", data, optional + 36)[0],
        "sections": sections,
    }


def _emitted_content_floor(pe: dict, section: dict) -> int:
    """Largest section offset that is PROVABLY inside emitted initialized content.

    `SizeOfRawData` is `round_up(E, FileAlignment)` for the true end E of the
    linker's initialized content, so E > raw_size - FileAlignment. Every offset
    below that bound is emitted content no matter what E is; at or above it, an
    all-zero run is indistinguishable from alignment padding.
    """
    return max(0, section["raw_size"] - pe.get("file_alignment", 0))


def _zero_to_raw_edge(pe: dict, section: dict, offset: int) -> bool:
    """Is [offset, raw_size) all zero? Only then can it be alignment padding.

    A nonzero byte at or after `offset` proves the linker emitted content there,
    which resolves the ambiguity in favour of `data-initialized`.
    """
    start = section["raw_offset"] + offset
    end = section["raw_offset"] + section["raw_size"]
    return not any(pe["data"][start:end])


def classify_pe_storage(pe: dict, rva: int) -> dict:
    """.rdata (ro) / .data initialized / unprovable tail / loader-zero tail / other.

    `data-initialized` vs `data-loader-zero-tail` splits on `raw_size`, but MSVC
    merges .bss INTO .data and rounds `raw_size` up to FileAlignment - so the last
    <FileAlignment bytes of the file-backed span can be alignment padding that the
    first .bss symbols already live in. An all-zero run there is byte-identical to
    a zero-valued .data global, and the PE cannot tell them apart. Such an offset is
    reported `data-unprovable-tail` (fail-closed: callers must not enrol it) rather
    than asserted initialized. Zeros BELOW that bound are proven emitted content.
    """
    readonly = pe["sections"].get(".rdata")
    if readonly and readonly["rva"] <= rva < readonly["rva"] + readonly["virtual_size"]:
        return {"class": "rdata", "section": ".rdata", "section_offset": rva - readonly["rva"]}
    writable = pe["sections"].get(".data")
    if writable and writable["rva"] <= rva < writable["rva"] + writable["virtual_size"]:
        offset = rva - writable["rva"]
        if offset >= writable["raw_size"]:
            storage = "data-loader-zero-tail"
        elif offset >= _emitted_content_floor(pe, writable) \
                and _zero_to_raw_edge(pe, writable, offset):
            storage = "data-unprovable-tail"
        else:
            storage = "data-initialized"
        return {"class": storage, "section": ".data", "section_offset": offset}
    for name, section in pe["sections"].items():
        extent = max(section["virtual_size"], section["raw_size"])
        if section["rva"] <= rva < section["rva"] + extent:
            return {"class": "other-section", "section": name, "section_offset": rva - section["rva"]}
    return {"class": "outside-image", "section": None, "section_offset": None}


def _section_of(pe: dict, rva: int):
    for section in pe["sections"].values():
        extent = max(section["virtual_size"], section["raw_size"])
        if section["rva"] <= rva < section["rva"] + extent:
            return section
    return None


def _highlow_relocs(pe: dict) -> list:
    data = pe["data"]
    pe_offset = struct.unpack_from("<I", data, 0x3C)[0]
    optional = pe_offset + 4 + 20
    directory = optional + 96 + IMAGE_DIRECTORY_ENTRY_BASERELOC * 8
    reloc_rva, reloc_size = struct.unpack_from("<II", data, directory)
    if not (reloc_rva and reloc_size):
        return []
    section = _section_of(pe, reloc_rva)
    cursor = section["raw_offset"] + (reloc_rva - section["rva"])
    end = cursor + reloc_size
    rvas = []
    while cursor + 8 <= end:
        page_rva, block_size = struct.unpack_from("<II", data, cursor)
        if block_size < 8 or cursor + block_size > end:
            raise ValueError("invalid PE base-relocation block")
        for entry_offset in range(cursor + 8, cursor + block_size, 2):
            entry = struct.unpack_from("<H", data, entry_offset)[0]
            if entry >> 12 == IMAGE_REL_BASED_HIGHLOW:
                rvas.append(page_rva + (entry & 0x0FFF))
        cursor += block_size
    return rvas


def read_pe_payload_evidence(pe: dict, rva: int, size: int, highlow_rvas, audit_kind="bytes"):
    """Retail span bytes + relocation-normalized digest + HIGHLOW fingerprint."""
    data = pe["data"]
    image_base = pe["image_base"]

    def raw_offset(target_rva):
        section = _section_of(pe, target_rva)
        if section is None:
            raise ValueError(f"RVA 0x{target_rva:x} outside PE sections")
        delta = target_rva - section["rva"]
        return None if delta >= section["raw_size"] else section["raw_offset"] + delta

    payload = bytearray(size)
    remaining, payload_off, cur = size, 0, rva
    while remaining:
        section = _section_of(pe, cur)
        if section is None:
            raise ValueError(f"span at RVA 0x{rva:x} outside PE sections")
        delta = cur - section["rva"]
        chunk = min(remaining, max(section["virtual_size"], section["raw_size"]) - delta)
        raw_chunk = min(chunk, max(section["raw_size"] - delta, 0))
        if raw_chunk:
            start = section["raw_offset"] + delta
            payload[payload_off:payload_off + raw_chunk] = data[start:start + raw_chunk]
        remaining -= chunk
        payload_off += chunk
        cur += chunk

    rel = sorted(r - rva for r in highlow_rvas if rva <= r < rva + size)
    normalized = bytearray(payload)
    for off in rel:
        if off + 4 > size:
            raise ValueError("HIGHLOW relocation crosses audited span")
        normalized[off:off + 4] = b"\0\0\0\0"
    evidence = {
        "size": size,
        "nonzero_byte_count": sum(b != 0 for b in payload),
        "sha256": hashlib.sha256(payload).hexdigest(),
        "normalized_sha256": hashlib.sha256(normalized).hexdigest(),
        "highlow_base_relocation_count": len(rel),
        "highlow_relative_offsets": rel,
    }
    if audit_kind == "cstring-pointer-table":
        targets, aliases, pattern = [], {}, []
        for off in rel:
            target_va = struct.unpack_from("<I", payload, off)[0]
            target_rva = target_va - image_base
            toff = raw_offset(target_rva)
            if toff is None:
                raise ValueError(f"pointer target RVA 0x{target_rva:x} has no raw bytes")
            s = data[toff:data.index(0, toff)]
            targets.append({"rva": target_rva, "sha256": hashlib.sha256(s).hexdigest(),
                            "text": s.decode("cp1252", "replace")})
            aliases.setdefault(target_va, len(aliases))
            pattern.append(aliases[target_va])
        evidence["cstring_targets"] = targets
        evidence["pointer_target_alias_pattern"] = pattern
    return evidence


# --- symbol inventory ---------------------------------------------------------
@dataclass
class DataSymbol:
    rva: int
    name: str
    unit: str
    reviewed_size: int | None
    type_decl: str


def load_data_symbols(symbols_path=SYMBOLS, globals_path=GLOBALS):
    types = {}
    if Path(globals_path).exists():
        for g in json.loads(Path(globals_path).read_text()):
            types[int(g["rva"], 16)] = g.get("type") or ""
    rows = []
    with Path(symbols_path).open(newline="") as f:
        for r in csv.DictReader(l for l in f if not l.lstrip().startswith("#")):
            if (r.get("kind") or "").strip() != "data":
                continue
            rva = int(r["rva"], 16)
            size = r.get("size", "").strip()
            rows.append(DataSymbol(rva, r["name"].strip(), (r.get("unit") or "").strip(),
                                   int(size, 16) if size else None, types.get(rva, "")))
    rows.sort(key=lambda s: s.rva)
    return rows


def audit(retail_exe=RETAIL_EXE, symbols_path=SYMBOLS, globals_path=GLOBALS):
    pe = read_pe(retail_exe)
    highlow = _highlow_relocs(pe)
    symbols = load_data_symbols(symbols_path, globals_path)
    rows = []
    for index, sym in enumerate(symbols):
        storage = classify_pe_storage(pe, sym.rva)
        # Extent: reviewed size wins; else the next-data-symbol gap (inferred).
        if sym.reviewed_size:
            size, extent_src = sym.reviewed_size, "reviewed"
        elif index + 1 < len(symbols):
            size, extent_src = symbols[index + 1].rva - sym.rva, "next-symbol-gap"
        else:
            size, extent_src = 0, "unknown"
        row = {
            "rva": f"0x{sym.rva:x}", "name": sym.name, "unit": sym.unit,
            "type": sym.type_decl, "storage_class": storage["class"],
            "section": storage["section"], "extent": f"0x{size:x}", "extent_source": extent_src,
        }
        # Only fingerprint reasonable, in-image, non-bss initialized/rdata spans.
        if (storage["class"] in ("rdata", "data-initialized") and 0 < size <= 0x4000):
            try:
                ev = read_pe_payload_evidence(pe, sym.rva, size, highlow)
                row.update({
                    "nonzero_bytes": ev["nonzero_byte_count"],
                    "highlow_relocs": ev["highlow_base_relocation_count"],
                    "sha256": ev["sha256"], "normalized_sha256": ev["normalized_sha256"],
                    "evidence": ev,
                })
            except ValueError as e:
                row["fingerprint_error"] = str(e)
        rows.append(row)
    return pe, rows


def write_ledger(rows, path=OUTPUT):
    cols = ["rva", "name", "unit", "type", "storage_class", "section", "extent",
            "extent_source", "nonzero_bytes", "highlow_relocs", "normalized_sha256"]
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=cols, delimiter="\t", extrasaction="ignore")
        w.writeheader()
        for r in rows:
            w.writerow(r)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--exe", type=Path, default=RETAIL_EXE)
    ap.add_argument("--json", type=Path, help="write full per-symbol evidence JSON")
    ap.add_argument("--rva", help="audit a single symbol RVA and print it")
    ap.add_argument("-o", "--output", type=Path, default=OUTPUT)
    args = ap.parse_args(argv)

    pe, rows = audit(args.exe)
    if args.rva is not None:
        target = int(args.rva, 16)
        hit = [r for r in rows if int(r["rva"], 16) == target]
        print(json.dumps(hit or f"no data symbol at {args.rva}", indent=2))
        return 0
    write_ledger(rows, args.output)
    if args.json:
        args.json.write_text(json.dumps(rows, indent=1))

    from collections import Counter
    by_class = Counter(r["storage_class"] for r in rows)
    fp = [r for r in rows if "normalized_sha256" in r]
    src = Counter(r["extent_source"] for r in rows)
    print(f"[data-audit] {len(rows)} data symbols; storage: " +
          ", ".join(f"{k}={v}" for k, v in sorted(by_class.items())))
    print(f"[data-audit] extent source: " + ", ".join(f"{k}={v}" for k, v in sorted(src.items())))
    print(f"[data-audit] fingerprinted {len(fp)} rdata/data spans -> {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
