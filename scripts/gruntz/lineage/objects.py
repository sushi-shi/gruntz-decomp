"""Read-only paired library census and legacy C11 CodeView evidence reader.

This is a source oracle, not a source importer or a matching authority. Unknown
records retain their bytes. Debug locations are observations, never proposed
retail stack slots. Record layouts: microsoft/microsoft-pdb/include/cvinfo.h.
"""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

from gruntz.core.paths import CONFIG
from gruntz.delink.coffx import Obj


def records(data: bytes, start: int = 0):
    """Walk every length-prefixed record; fail on truncation, never resync."""
    offset = start
    while offset < len(data):
        if offset + 4 > len(data):
            raise ValueError(f"truncated record header at {offset:#x}")
        length, kind = struct.unpack_from("<HH", data, offset)
        end = offset + 2 + length
        if length < 2 or end > len(data):
            raise ValueError(f"invalid record length {length} at {offset:#x}")
        yield offset, kind, data[offset + 4:end]
        offset = end


def pstring(data: bytes, offset: int) -> str:
    if offset >= len(data) or offset + 1 + data[offset] > len(data):
        raise ValueError("truncated CodeView length-prefixed string")
    return data[offset + 1:offset + 1 + data[offset]].decode("latin-1")


def symbols(data: bytes, *, signature: bool) -> list[dict]:
    if signature and data[:4] != b"\x02\0\0\0":
        raise ValueError("only C11 (signature 2) symbols are supported")
    result, scopes = [], []
    for offset, kind, payload in records(data, 4 if signature else 0):
        row = {"offset": offset, "kind": hex(kind), "scope": list(scopes)}
        try:
            if kind in (0x100A, 0x100B):  # L/GPROC32_ST
                row.update(record="procedure", name=pstring(payload, 35),
                           code_size=struct.unpack_from("<I", payload, 12)[0],
                           type_index=hex(struct.unpack_from("<I", payload, 24)[0]))
                scopes.append(offset)
            elif kind == 0x207:  # BLOCK32_ST
                row.update(record="block", name=pstring(payload, 18),
                           code_size=struct.unpack_from("<I", payload, 8)[0])
                scopes.append(offset)
            elif kind == 0x6:  # END
                if not scopes:
                    raise ValueError("unbalanced END")
                row["record"] = "end"
                scopes.pop()
            elif kind == 0x1006:  # BPREL32_ST
                location, type_index = struct.unpack_from("<iI", payload)
                row.update(record="local", name=pstring(payload, 8),
                           frame_offset=location, type_index=hex(type_index))
            elif kind == 0x9:  # OBJNAME_ST
                row.update(record="object", name=pstring(payload, 4))
            elif kind == 0x1:  # COMPILE
                row.update(record="compiler", name=pstring(payload, 4))
            elif kind == 0x1003:  # UDT_ST
                row.update(record="typedef", name=pstring(payload, 4),
                           type_index=hex(struct.unpack_from("<I", payload)[0]))
            else:
                row.update(record="uninterpreted", payload_hex=payload.hex())
        except (struct.error, ValueError) as error:
            raise ValueError(f"symbol {kind:#x} at {offset:#x}: {error}") from error
        result.append(row)
    if scopes:
        raise ValueError("unclosed CodeView scopes")
    return result


def types(data: bytes) -> list[dict]:
    if data[:4] != b"\x02\0\0\0":
        raise ValueError("only C11 (signature 2) types are supported")
    result, index = [], 0x1000
    for offset, kind, payload in records(data, 4):
        row = {"offset": offset, "kind": hex(kind), "payload_hex": payload.hex()}
        if kind == 0x16:  # LF_TYPESERVER_ST: references are in an external PDB.
            signature, age = struct.unpack_from("<II", payload)
            row.update(record="type_server", name=pstring(payload, 8),
                       signature=hex(signature), age=age)
            index = None
        elif kind in (0x100E, 0x1200):  # PRECOMP / SKIP change index assignment.
            row["record"] = "unresolved_index_control"
            index = None
        else:
            row.update(record="type", index=hex(index) if index is not None else None)
            if index is not None:
                index += 1
        result.append(row)
    return result


def member_key(name: str) -> str:
    return name.replace("\\", "/").rsplit("/", 1)[-1].lower()


def members(path: Path) -> dict[str, str]:
    names = subprocess.check_output(["llvm-ar", "t", str(path)]).decode("latin-1").splitlines()
    result = {}
    for name in names:
        key = member_key(name)
        if key in result:
            raise ValueError(f"{path}: ambiguous member basename {key}")
        result[key] = name
    return result


def object_evidence(path: Path, member: str, *, detail: bool) -> dict:
    data = subprocess.check_output(["llvm-ar", "p", str(path), member])
    with tempfile.TemporaryDirectory(prefix="gruntz-lineage-object-") as directory:
        objpath = Path(directory) / "member.obj"
        objpath.write_bytes(data)
        obj = Obj(objpath)
        code_names = set()
        streams, type_streams = [], []
        first_symbols = True
        for section in obj.section_table:
            index = section["index"]
            if section["characteristics"] & 0x20:
                code_names.update(name for _, name in obj.defined_symbols(index))
            if section["name"] == ".debug$S":
                # Only the first contribution has a signature. Header-inline
                # COMDAT contributions are bare records, not missing debug data.
                rows = symbols(obj.section_payload(index), signature=first_symbols)
                first_symbols = False
                streams.append({"section": index, "records": rows})
            elif section["name"] == ".debug$T":
                type_streams.append({"section": index, "records": types(obj.section_payload(index))})
        result = {"member": member, "sha256": hashlib.sha256(data).hexdigest(),
                  "code_symbols": sorted(code_names),
                  "symbol_records": sum(len(s["records"]) for s in streams),
                  "local_records": sum(r["record"] == "local" for s in streams for r in s["records"]),
                  "type_records": sum(len(s["records"]) for s in type_streams),
                  "type_servers": [r for s in type_streams for r in s["records"]
                                   if r["record"] == "type_server"],
                  "compilers": sorted({r["name"] for s in streams for r in s["records"]
                                       if r["record"] == "compiler"})}
        if detail:
            result.update(symbol_streams=streams, type_streams=type_streams)
        return result


def baseline(path: Path) -> dict[str, dict]:
    result = {}
    for line in path.read_text().splitlines():
        if not line or line.startswith("#"):
            continue
        fields = line.split("\t")
        if len(fields) < 8:
            continue
        unit, name, bank, current, _, fingerprint, rva, historical = fields[:8]
        result[name] = {"unit": unit, "symbol": name, "rva": rva,
                        "bank": float(bank), "current": float(current),
                        "historical": float(historical), "source_hash": fingerprint}
    return result


def overlap(debug: set[str], release: set[str], bank: dict) -> dict:
    # Even exact mangling is only candidacy. No signature-erasing name match is
    # silently promoted to an ABI match or an adoption decision.
    common = debug & release
    exact = sorted(common & bank.keys())
    stems = {name.split("@@", 1)[0] for name in common if name.startswith("?")}
    loose = [row for name, row in bank.items() if row["historical"] < 100
             and name not in common and name.startswith("?")
             and name.split("@@", 1)[0] in stems]
    return {"exact_name_overlap": [bank[name] for name in exact],
            "below_historical_100": [bank[name] for name in exact if bank[name]["historical"] < 100],
            "signature_different_candidates": sorted(loose, key=lambda row: row["historical"])}


def pair(debug: Path, release: Path, bank: dict, member: str | None = None) -> dict:
    dm, rm = members(debug), members(release)
    keys = sorted(dm.keys() & rm.keys())
    if member:
        key = member_key(member)
        if key not in keys:
            raise ValueError(f"member {member!r} is not present in BOTH archives")
        keys = [key]
    evidence = []
    ds, rs = set(), set()
    for key in keys:
        sides = []
        for path, names, code in ((debug, dm, ds), (release, rm, rs)):
            result = object_evidence(path, names[key], detail=bool(member))
            code.update(result["code_symbols"])
            sides.append(result)
        evidence.append({"member": key, "debug": sides[0], "release": sides[1]})
    result = {"debug": str(debug), "release": str(release),
              "debug_sha256": hashlib.sha256(debug.read_bytes()).hexdigest(),
              "release_sha256": hashlib.sha256(release.read_bytes()).hexdigest(),
              "debug_only": sorted(dm.keys() - rm.keys()),
              "release_only": sorted(rm.keys() - dm.keys()), "members": evidence}
    if not member:
        unpaired = []
        for side, path, names, other in (("debug", debug, dm, rm), ("release", release, rm, dm)):
            for key in sorted(names.keys() - other.keys()):
                item = object_evidence(path, names[key], detail=False)
                item["side"] = side
                item["exact_name_overlap"] = [bank[n] for n in item["code_symbols"] if n in bank]
                unpaired.append(item)
        result["unpaired_members"] = unpaired
    result.update(overlap(ds, rs, bank))
    return result


def main(args) -> int:
    try:
        debug, release = Path(args.debug), Path(args.release)
        if debug.is_dir() and release.is_dir():
            if args.member:
                raise ValueError("--member requires two archive files")
            dm = {p.name.lower(): p for p in debug.iterdir() if p.suffix.lower() == ".lib"}
            rm = {p.name.lower(): p for p in release.iterdir() if p.suffix.lower() == ".lib"}
            paths = [(dm[k], rm[k]) for k in sorted(dm.keys() & rm.keys())]
            unmatched = {"debug_only": sorted(dm.keys() - rm.keys()),
                         "release_only": sorted(rm.keys() - dm.keys())}
        elif debug.is_file() and release.is_file():
            paths, unmatched = [(debug, release)], {}
        else:
            raise ValueError("provide two existing directories or two existing archives")
        if not paths:
            raise ValueError("no library pairs found")
        bank = baseline(Path(args.baseline))
        report = {"unpaired_libraries": unmatched,
                  "pairs": [pair(d, r, bank, args.member) for d, r in paths]}
        print(json.dumps(report, indent=2))
        return 0
    except (ValueError, OSError, struct.error, subprocess.CalledProcessError) as error:
        print(f"library evidence failed: {error}", file=sys.stderr)
        return 1


def add_parser(sub):
    parser = sub.add_parser("objects", help="audit paired libraries and legacy CodeView (read-only JSON)")
    parser.add_argument("--debug", required=True, help="Debug archive or directory of .lib files")
    parser.add_argument("--release", required=True, help="Release archive or directory of .lib files")
    parser.add_argument("--member", help="decode one paired member, including raw type records")
    parser.add_argument("--baseline", default=str(CONFIG / "match_baseline.tsv"))
