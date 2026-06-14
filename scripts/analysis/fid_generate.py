#!/usr/bin/env python3
"""fid_generate.py - regenerate config/library_labels.csv (the FID library labels).

`config/library_labels.csv` (rva,name,lib,confidence,source) is TRACKED in the
repo: it is the FID output that `apply.py` consumes to name CRT/MFC/zlib library
functions in GRUNTZ.EXE (so they are excluded from the hand-matching surface).
The committed CSV is canonical and survives `git clean`; you only need to
regenerate it if the VC5 libs, the zlib build, or GRUNTZ.EXE change.

This script does the deterministic, scriptable prefix of the FID workflow and
then guides the FunctionID-plugin step, which Ghidra exposes through its FID API
rather than a plain headless flag. Full recipe + rationale: docs/libraries-and-funcid.md §4.1.

Inputs (inside `nix develop .#build`):
  $MSVC_DIR/lib/LIBCMT.LIB   - static MT CRT (also covers MSVC5 iostreams)
  $MSVC_DIR/lib/NAFXCW.LIB   - static MFC 4.2 release
  a zlib-1.0.4 .lib rebuilt under VC5 /O2 /MT (see docs/zlib-matching.md)

Steps:
  1. import each .lib as a Ghidra program + auto-analyze (this script, headless);
  2. create/populate a FidDb from those programs (FunctionID plugin) and run the
     FID analyzer on the GRUNTZ.EXE program in build/ghidra-named;
  3. export the matched (rva,name,lib,confidence) rows -> config/library_labels.csv.

Steps 2-3 use the Ghidra FunctionID API (ghidra.feature.fid.*); they are NOT yet
wired headless here - see §4.1 for the exact plugin actions. Until they are, run
them from the Ghidra GUI and drop the export at config/library_labels.csv.
"""
import os
import shutil
import subprocess
import sys
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parent.parent)
OUT = REPO / "config" / "library_labels.csv"        # tracked, canonical
FID_WORK = REPO / "build" / "fid"                    # scratch: imported .lib programs


def find_libs() -> list[Path]:
    msvc = os.environ.get("MSVC_DIR")
    if not msvc:
        sys.exit("[fid] $MSVC_DIR unset - run inside `nix develop .#build`.")
    libdir = Path(msvc) / "lib"
    libs = []
    for name in ("LIBCMT.LIB", "NAFXCW.LIB"):
        p = libdir / name
        if p.exists():
            libs.append(p)
        else:
            print(f"[fid] WARNING: {p} not found - FID hit-rate will drop.", file=sys.stderr)
    # zlib has no prebuilt VC5 .lib; rebuild zlib-1.0.4 under VC5 /O2 /MT first.
    return libs


def analyze_headless():
    found = shutil.which("analyzeHeadless")
    if found:
        return found
    for link in ("result", "result-1"):
        p = REPO / link / "bin" / "analyzeHeadless"
        if p.exists():
            return str(p)
    return "analyzeHeadless"


def main() -> None:
    libs = find_libs()
    if not libs:
        sys.exit("[fid] no VC5 libs found; cannot regenerate.")
    FID_WORK.mkdir(parents=True, exist_ok=True)
    analyze = analyze_headless()
    # Step 1: import each .lib + auto-analyze into its own scratch program.
    for lib in libs:
        print(f"[fid] importing + analyzing {lib.name} ...")
        subprocess.run([analyze, str(FID_WORK), "libs", "-import", str(lib)], check=True)
    # Steps 2-3: FidDb create/populate + analyze GRUNTZ.EXE + export matches.
    print("[fid] imported the library programs into", FID_WORK)
    print("[fid] NEXT (Ghidra FunctionID plugin - see docs/libraries-and-funcid.md §4.1):")
    print("        create+populate a FidDb from those programs, attach it, run the")
    print("        FID analyzer on build/ghidra-named, and export the matches to:")
    print(f"        {OUT}")
    print("[fid] The committed config/library_labels.csv remains canonical until then.")


if __name__ == "__main__":
    main()
