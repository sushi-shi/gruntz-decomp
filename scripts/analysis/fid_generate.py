#!/usr/bin/env python3
"""fid_generate.py - regenerate config/library_labels.csv (the FID library labels).

`config/library_labels.csv` (rva,name,lib,confidence,source) is the TRACKED output
of a custom **masked-byte COFF-signature matcher** (NOT Ghidra FID). `apply.py`
names CRT/MFC/zlib library functions from it and `gen_match_queue.py` excludes
them. The committed CSV is canonical and survives `git clean`; regenerate only
when the VC5 libs, GRUNTZ.EXE, or the Ghidra function boundaries change.

Pipeline (stages under scripts/analysis/fid/):
  1. unpack .obj members from $MSVC_DIR/lib/{LIBCMT,NAFXCW}.LIB   (llvm-ar)
  2. fid/coff_sig.py   -> build/fid/sigs.pkl       (masked per-symbol signatures)
  3. fid/classify.py   -> build/fid/library_labels.csv  (anchored: matches at known
                          function starts; prepends zlib from build/gen/symbol_names.csv)
  4. fid/unanchored.py -> build/fid/offstart_matches.csv (bodies at starts Ghidra missed)
  5. merge (3)+(4) with a `source` column -> config/library_labels.csv

Stages 2-4 (coff_sig/classify/unanchored) are the original matcher, verbatim. The
.obj unpack (1) and the source-tagged merge (5) are RECONSTRUCTED here - the
original glue was never committed - so on the first regeneration, diff the result
against the tracked config/library_labels.csv before trusting it.

Run inside `nix develop .#build`: needs $MSVC_DIR, $GRUNTZ_EXE, llvm-ar, and
build/ghidra-enrich/exports/functions.csv (produce it with `gruntz bootstrap`).
"""
import csv, os, shutil, subprocess, sys
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parent.parent)
FID = Path(__file__).resolve().parent / "fid"            # stage scripts
WORK = REPO / "build" / "fid"                            # scratch (gitignored)
FUNCS = REPO / "build" / "ghidra-enrich" / "exports" / "functions.csv"
OUT = REPO / "config" / "library_labels.csv"             # tracked, canonical


def sh(*cmd, cwd=None):
    print("[fid] $ " + " ".join(str(c) for c in cmd))
    subprocess.run([str(c) for c in cmd], check=True, cwd=str(cwd) if cwd else None)


def unpack_lib(lib_path: Path, objs_dir: Path) -> None:
    """llvm-ar-extract a .LIB's .obj members into objs_dir (cleared first)."""
    if not lib_path.exists():
        sys.exit(f"[fid] {lib_path} not found - check $MSVC_DIR.")
    objs_dir.mkdir(parents=True, exist_ok=True)
    for f in objs_dir.glob("*"):
        f.unlink()
    ar = shutil.which("llvm-ar") or "llvm-ar"
    sh(ar, "x", lib_path, cwd=objs_dir)   # extracts members into cwd


def main() -> None:
    msvc = os.environ.get("MSVC_DIR")
    if not msvc:
        sys.exit("[fid] $MSVC_DIR unset - run inside `nix develop .#build`.")
    exe = os.environ.get("GRUNTZ_EXE") or str(REPO / "build/exe/GRUNTZ.EXE")
    if not Path(exe).exists():
        sys.exit(f"[fid] EXE not found ({exe}) - set $GRUNTZ_EXE or run `gruntz init`.")
    if not FUNCS.exists():
        sys.exit(f"[fid] {FUNCS} missing - run `gruntz bootstrap` first.")
    WORK.mkdir(parents=True, exist_ok=True)

    # 1. unpack the static libs to .obj members
    libdir = Path(msvc) / "lib"
    unpack_lib(libdir / "LIBCMT.LIB", WORK / "libcmt_objs")
    unpack_lib(libdir / "NAFXCW.LIB", WORK / "nafxcw_objs")

    # 2. extract masked signatures
    sigs = WORK / "sigs.pkl"
    sh(sys.executable, FID / "coff_sig.py", WORK / "libcmt_objs", WORK / "nafxcw_objs", sigs, 1)

    # 3. anchored matches (classify.py also writes WORK/library_labels.csv next to its out-csv)
    sh(sys.executable, FID / "classify.py", sigs, exe, FUNCS, WORK / "matches.csv")

    # 4. off-start matches (bodies Ghidra did not carve)
    sh(sys.executable, FID / "unanchored.py", sigs, exe, FUNCS, WORK / "offstart_matches.csv")

    # 5. merge with a `source` column -> config/library_labels.csv
    rows, seen = [], set()
    with open(WORK / "library_labels.csv", newline="") as f:
        for r in csv.DictReader(f):
            rows.append((r["rva"], r["name"], r["lib"], r["confidence"], "anchored"))
            seen.add(r["rva"])
    with open(WORK / "offstart_matches.csv", newline="") as f:
        for r in csv.DictReader(f):
            if r["rva"] in seen:
                continue
            rows.append((r["rva"], r["name"], r["lib"], r["confidence"], "offstart-ghidra-missed"))
    with open(OUT, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["rva", "name", "lib", "confidence", "source"])
        w.writerows(rows)
    print(f"[fid] wrote {OUT.relative_to(REPO)} ({len(rows)} rows). "
          "Diff against the committed copy to confirm the reconstruction.")


if __name__ == "__main__":
    main()
