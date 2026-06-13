#!/usr/bin/env python3
"""
generate_objdiff_config.py - emit build/objdiff/objdiff.json pairing each
recompiled base TU object against its delinked target TU object.

Adapted from refs/vostok/scripts/generate_objdiff_config.py for the Gruntz
setup, which is simpler: there is no .sln/.vcproj graph. The base side is loose
zlib source compiled directly with `cl` (-> build/objdiff/base/<unit>.obj) and
the target side is delinked from a synthesised PDB (-> build/objdiff/target/
<unit>.c.obj for named units, or seg_NNNN.cpp.obj for the unnamed remainder).

Pairing rule (per in-scope unit, from config/symbol_names.csv's `unit` column):
  base   : build/objdiff/base/<unit>.obj      (cl /O2 /MT vendor/zlib-1.0.4/<unit>.c)
  target : build/objdiff/target/<unit>.c.obj  (delinked, named per the names map)

A unit with a base obj but no named target functions yet (target obj absent) is
still emitted, paired against a tiny empty dummy.obj, so objdiff lists it as
0%/target-only - that is the "ready to grow" state. Units are taken from the
in-scope list (CLI/--units) so the report reflects the whole zlib scope, not
only what is named.

Symbols are pre-named on both sides (cdecl `_<name>` from cl; the same `_<name>`
written into the target obj via the names map), so no objdiff `symbol_mappings`
overlay is needed - objdiff pairs `_adler32` <-> `_adler32` by name directly.
"""

import argparse
import json
import struct
from pathlib import Path


SCRIPT_DIR  = Path(__file__).resolve().parent
GRUNTZ_DIR  = SCRIPT_DIR.parent
OBJDIFF_DIR = GRUNTZ_DIR / "build" / "objdiff"


def write_dummy(path: Path) -> None:
    """A minimal valid empty i386 COFF (.text, no symbols) for units whose
    target side has no named functions yet, so objdiff still lists them."""
    symbol_table_offset = 20 + 40  # header + 1 section header
    header = struct.pack(
        "<HHIIIHH",
        0x14C,                 # Machine: IMAGE_FILE_MACHINE_I386
        1,                     # NumberOfSections
        0,                     # TimeDateStamp
        symbol_table_offset,   # PointerToSymbolTable
        0,                     # NumberOfSymbols
        0,                     # SizeOfOptionalHeader
        0,                     # Characteristics
    )
    section = struct.pack(
        "<8sIIIIIIHHI",
        b".text\0\0\0",
        0, 0, 0, 0, 0, 0, 0, 0,
        0x60000020,            # CODE | EXECUTE | READ
    )
    string_table = struct.pack("<I", 4)  # empty string table (size field only)
    path.write_bytes(header + section + string_table)


def gather_units(objdiff_dir: Path, units: list[str]) -> list[dict]:
    out = []
    for unit in units:
        base_rel   = f"./base/{unit}.obj"
        target_rel = f"./target/{unit}.c.obj"
        target_path = target_rel if (objdiff_dir / target_rel).exists() else "./dummy.obj"
        base_path   = base_rel   if (objdiff_dir / base_rel).exists()   else "./dummy.obj"
        out.append({
            "name": unit,
            "target_path": target_path,
            "base_path": base_path,
            "scratch": {"platform": "win32", "compiler": "msvc5.0"},
        })
    return out


def generate(objdiff_dir: Path, units: list[str]) -> Path:
    objdiff_dir.mkdir(parents=True, exist_ok=True)
    write_dummy(objdiff_dir / "dummy.obj")
    obj = {
        "$schema": "https://raw.githubusercontent.com/encounter/objdiff/main/config.schema.json",
        "build_base": False,
        "build_target": False,
        "watch_patterns": ["*.obj"],
        "units": gather_units(objdiff_dir, units),
    }
    out = objdiff_dir / "objdiff.json"
    with out.open("w", encoding="utf-8") as f:
        json.dump(obj, f, indent=2)
        f.write("\n")
    return out


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--objdiff-dir", default=OBJDIFF_DIR, type=Path)
    ap.add_argument("--units", nargs="*", default=[],
                    help="in-scope unit stems to pair (default: none).")
    args = ap.parse_args()
    path = generate(args.objdiff_dir, args.units)
    print(f"[objdiff-config] wrote {path} ({len(args.units)} units)")


if __name__ == "__main__":
    main()
