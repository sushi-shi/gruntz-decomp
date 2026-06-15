#!/usr/bin/env python3
"""delink.py - produce the delinked, named per-unit target objects.

This is the TARGET (delink) half of the matching pipeline, run as one ninja rule
(its declared outputs are the per-unit <unit>.c.obj target objects):

    build/gen/symbol_names.csv  (generated rva -> name,unit)
            |  overlay onto build/ghidra-enrich/exports/functions.csv
            v
    scripts/gruntz/build/synth_pdb.py  -> build/pdb/gruntz_named.{yaml,pdb}
            |
            v
    vostok-delinker          -> build/delink/named/<unit>.c.obj (+ seg_*.cpp.obj)
            |
            v
    collect the in-scope <unit>.c.obj into <target-dir>/

The base/compile half (cl /O2 /MT under wine) is a separate ninja graph driven
by scripts/gruntz/build/cc_wrap.py; the two are paired BY SYMBOL NAME in the
objdiff project (configure.py), no symbol_mappings. KEEP
scripts/gruntz/build/synth_pdb.py - this script orchestrates it, it does not
replace it.

Units to collect are taken from --unit (repeatable) so configure.py can pass
exactly the manifest's unit set. The remaining address-bucketed seg_NNNN.cpp.obj
for the un-named .text remainder stay in the raw delink dir and are not paired.
"""

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
GRUNTZ_DIR = next((p for p in SCRIPT_DIR.parents if (p / "flake.nix").exists()), SCRIPT_DIR)

# Functions named in symbol_names.csv are attributed to their per-unit source
# file; the un-named remainder fall into address buckets of 2**BUCKET_SHIFT
# bytes (matching the existing delink that the 42 functions were matched under).
BUCKET_SHIFT = 16


def log(msg: str) -> None:
    print(f"[delink] {msg}", flush=True)


def die(msg: str) -> None:
    print(f"[delink] ERROR: {msg}", file=sys.stderr)
    sys.exit(1)


def tool(name: str) -> str:
    """Resolve a tool on PATH - the `nix develop .#build` shell provides them all."""
    return shutil.which(name) or name  # bare name lets subprocess surface the error


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--exe", required=True, help="the retail GRUNTZ.EXE.")
    ap.add_argument("--functions", required=True, help="ghidra functions.csv.")
    ap.add_argument("--symbols", required=True, help="ghidra symbols.csv.")
    ap.add_argument("--names-map", required=True, help="build/gen/symbol_names.csv.")
    ap.add_argument("--pdb-dir", required=True, help="dir for the synth PDB/YAML.")
    ap.add_argument("--delink-dir", required=True, help="raw delinker output dir.")
    ap.add_argument("--target-dir", required=True,
                    help="dir to collect the per-unit <unit>.c.obj into.")
    ap.add_argument("--unit", action="append", default=[],
                    help="in-scope unit stem to collect (repeatable).")
    args = ap.parse_args()

    exe = Path(args.exe)
    functions = Path(args.functions)
    symbols = Path(args.symbols)
    names = Path(args.names_map)
    pdb_dir = Path(args.pdb_dir)
    delink_dir = Path(args.delink_dir)
    target_dir = Path(args.target_dir)
    units = args.unit

    for f in (exe, functions, symbols, names):
        if not f.exists():
            die(f"missing input: {f}")

    pdb_dir.mkdir(parents=True, exist_ok=True)
    named_yaml = pdb_dir / "gruntz_named.yaml"
    named_pdb = pdb_dir / "gruntz_named.pdb"

    log("Synthesising named PDB from functions.csv + symbol_names.csv ...")
    subprocess.run(
        [sys.executable, str(SCRIPT_DIR / "synth_pdb.py"),
         "--exe", str(exe),
         "--functions", str(functions),
         "--symbols", str(symbols),
         "--out-yaml", str(named_yaml),
         "--out-pdb", str(named_pdb),
         "--bucket-shift", str(BUCKET_SHIFT),
         "--names-map", str(names),
         # cl.exe base objs: oracle for MSVC ??_C@ string-pool names. The build
         # DAG runs base compile -> gen_labels -> symbol_names.csv -> delink, so
         # they exist here (apply_string_names degrades gracefully if not).
         "--base-dir", str(target_dir.parent / "base")],
        check=True,
    )

    delinker = tool("vostok-delinker")
    if delink_dir.exists():
        shutil.rmtree(delink_dir)
    delink_dir.mkdir(parents=True, exist_ok=True)
    log(f"Delinking {exe.name} -> {delink_dir} ...")
    subprocess.run(
        [delinker,
         "--pdb-path", str(named_pdb),
         "--exe-path", str(exe),
         "--output-path", str(delink_dir),
         "--engine-path", "c:\\proj\\"],
        check=True,
    )

    # Collect the per-unit named target objs (<unit>.c.obj) into target_dir.
    if target_dir.exists():
        shutil.rmtree(target_dir)
    target_dir.mkdir(parents=True, exist_ok=True)
    collected = []
    missing = []
    for unit in units:
        src = delink_dir / f"{unit}.c.obj"
        if src.exists():
            shutil.copy2(src, target_dir / f"{unit}.c.obj")
            collected.append(unit)
        else:
            missing.append(unit)
    log(f"Named target objs ({len(collected)}/{len(units)}): "
        f"{', '.join(collected) if collected else '(none)'}")
    if missing:
        log(f"  no named functions yet for: {', '.join(missing)}")


if __name__ == "__main__":
    main()
