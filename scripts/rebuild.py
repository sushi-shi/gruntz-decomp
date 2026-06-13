#!/usr/bin/env python3
"""
rebuild.py - the single tracked entry point for the Gruntz matching loop.

One command re-runs the whole preservation/decomp matching pipeline and reports
per-function / per-unit match % against the delinked target (GRUNTZ.EXE):

    config/symbol_names.csv  (curated RVA -> source symbol + unit, the loop's heart)
            |  overlay onto build/ghidra-enrich/exports/functions.csv
            v
    scripts/synth_pdb.py     (rename matched FUN_<rva> -> _adler32, group per unit)
            |  -> build/pdb/gruntz_named.pdb
            v
    vostok-delinker          (slice GRUNTZ.EXE into per-unit COFF objs)
            |  -> build/objdiff/target/<unit>.c.obj   (named symbols, real relocs)
            v
    cl /O2 /MT (under Wine)  (compile the in-scope zlib TUs -> base objs)
            |  -> build/objdiff/base/<unit>.obj
            v
    objdiff-cli report       (pair base <unit>.obj vs target <unit>.c.obj)
            |  -> build/objdiff/report.json
            v
    per-unit + roll-up match summary (printed; this script)

Calibrated by docs/zlib-matching.md: cl /c /O2 /MT (cdecl) byte-matches adler32.
The loop grows by adding rows to config/symbol_names.csv and re-running.

USAGE (run inside the build dev shell, which provides the toolchain + tools):
    nix develop .#build --command python3 scripts/rebuild.py
    nix develop .#build --command python3 scripts/rebuild.py --force   # re-delink

Outside that shell it falls back to MSVC_DIR=/tmp/gtc/msvc + system wine and the
./result* nix-build symlinks for vostok-delinker / objdiff-cli, if present.

All outputs live under build/ (git-ignored). Tracked inputs: this script,
scripts/generate_objdiff_config.py, scripts/synth_pdb.py, config/symbol_names.csv.
"""

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

import generate_objdiff_config

SCRIPT_DIR = Path(__file__).resolve().parent
GRUNTZ_DIR = SCRIPT_DIR.parent

# --- paths ------------------------------------------------------------------
NAMES_CSV   = GRUNTZ_DIR / "config" / "symbol_names.csv"
FUNCTIONS   = GRUNTZ_DIR / "build" / "ghidra-enrich" / "exports" / "functions.csv"
SYMBOLS     = GRUNTZ_DIR / "build" / "ghidra-enrich" / "exports" / "symbols.csv"
EXE         = GRUNTZ_DIR / "build" / "exe" / "GRUNTZ.delinkable.EXE"
ZLIB_DIR    = GRUNTZ_DIR / "vendor" / "zlib-1.0.4"

PDB_DIR     = GRUNTZ_DIR / "build" / "pdb"
NAMED_PDB   = PDB_DIR / "gruntz_named.pdb"
NAMED_YAML  = PDB_DIR / "gruntz_named.yaml"

OBJDIFF_DIR = GRUNTZ_DIR / "build" / "objdiff"
TARGET_DIR  = OBJDIFF_DIR / "target"      # delinked, named target objs (per-unit)
BASE_DIR    = OBJDIFF_DIR / "base"        # freshly compiled base objs
DELINK_OUT  = GRUNTZ_DIR / "build" / "delink" / "named"  # raw delinker output
REPORT      = OBJDIFF_DIR / "report.json"
STAMP       = OBJDIFF_DIR / ".delink.stamp"  # hash of inputs the target reflects

# In-scope zlib 1.0.4 translation units (docs/zlib-matching.md). The target obj
# for each is built only once its functions are named in symbol_names.csv; an
# un-named unit still appears in the report (paired against the empty dummy).
ZLIB_UNITS = [
    "deflate", "inflate", "infblock", "infcodes", "inffast",
    "inftrees", "infutil", "trees", "adler32", "zutil",
]

# Calibrated VC5 flags (docs/zlib-matching.md): /O2 /MT, cdecl, byte-exact adler32.
CL_FLAGS = ["/nologo", "/c", "/O2", "/MT"]

# Functions are attributed to per-unit source files for named RVAs; the rest fall
# into address buckets of 2**16 bytes (~30 segs), matching the existing delink.
BUCKET_SHIFT = 16


def log(msg: str) -> None:
    print(f"[rebuild] {msg}", flush=True)


def die(msg: str) -> None:
    print(f"[rebuild] ERROR: {msg}", file=sys.stderr)
    sys.exit(1)


# --- toolchain discovery ----------------------------------------------------
def _msvc_dir() -> Path:
    """MSVC root: $MSVC_DIR (build dev shell) or /tmp/gtc/msvc fallback."""
    return Path(os.environ["MSVC_DIR"]) if os.environ.get("MSVC_DIR") else Path("/tmp/gtc/msvc")


def _find_ci(d: Path, name: str):
    if not d.is_dir():
        return None
    for p in d.iterdir():
        if p.name.lower() == name.lower():
            return p
    return None


def _tool(name: str, *symlinks: str) -> str:
    """Resolve a tool: PATH first, then ./result* nix-build symlinks."""
    found = shutil.which(name)
    if found:
        return found
    for link in symlinks:
        p = GRUNTZ_DIR / link / "bin" / name
        if p.exists():
            return str(p)
    return name  # let subprocess surface the error


def winepath_w(p: Path) -> str:
    return subprocess.check_output(["winepath", "-w", str(p)], text=True).strip()


# --- inputs hash (skip re-delink when nothing changed) ----------------------
def _inputs_hash() -> str:
    h = hashlib.sha256()
    for p in (NAMES_CSV, FUNCTIONS, SYMBOLS, EXE):
        try:
            st = p.stat()
            h.update(p.name.encode())
            h.update(str(st.st_size).encode())
            h.update(str(int(st.st_mtime)).encode())
        except OSError:
            h.update(b"missing")
    h.update(str(BUCKET_SHIFT).encode())
    return h.hexdigest()


# --- pipeline steps ---------------------------------------------------------
def synth_and_delink(force: bool) -> None:
    """Apply the names overlay, regenerate the synth PDB, and delink the target
    into per-unit named COFF objs. Skips if inputs are unchanged (unless force)."""
    for f in (NAMES_CSV, FUNCTIONS, SYMBOLS, EXE):
        if not f.exists():
            die(f"missing input: {f}")

    want = _inputs_hash()
    if not force and STAMP.exists() and STAMP.read_text().strip() == want \
            and TARGET_DIR.is_dir() and any(TARGET_DIR.glob("*.c.obj")):
        log("Target up to date (names/functions/EXE unchanged); skipping re-delink.")
        return

    PDB_DIR.mkdir(parents=True, exist_ok=True)
    log("Synthesising named PDB from functions.csv + symbol_names.csv ...")
    subprocess.run(
        [sys.executable, str(SCRIPT_DIR / "synth_pdb.py"),
         "--exe", str(EXE),
         "--functions", str(FUNCTIONS),
         "--symbols", str(SYMBOLS),
         "--out-yaml", str(NAMED_YAML),
         "--out-pdb", str(NAMED_PDB),
         "--bucket-shift", str(BUCKET_SHIFT),
         "--names-map", str(NAMES_CSV)],
        check=True,
    )

    delinker = _tool("vostok-delinker", "result")
    if DELINK_OUT.exists():
        shutil.rmtree(DELINK_OUT)
    DELINK_OUT.mkdir(parents=True, exist_ok=True)
    log(f"Delinking {EXE.name} -> {DELINK_OUT} ...")
    subprocess.run(
        [delinker,
         "--pdb-path", str(NAMED_PDB),
         "--exe-path", str(EXE),
         "--output-path", str(DELINK_OUT),
         "--engine-path", "c:\\proj\\"],
        check=True,
    )

    # Collect the per-unit named target objs (<unit>.c.obj) for the in-scope
    # units into build/objdiff/target/. (The address-bucketed seg_NNNN.cpp.obj
    # for the un-named remainder stay in DELINK_OUT and are not paired.)
    if TARGET_DIR.exists():
        shutil.rmtree(TARGET_DIR)
    TARGET_DIR.mkdir(parents=True, exist_ok=True)
    collected = []
    for unit in ZLIB_UNITS:
        src = DELINK_OUT / f"{unit}.c.obj"
        if src.exists():
            shutil.copy2(src, TARGET_DIR / f"{unit}.c.obj")
            collected.append(unit)
    log(f"Named target objs: {', '.join(collected) if collected else '(none yet)'}")
    STAMP.write_text(want + "\n")


def compile_base() -> list[str]:
    """Compile the in-scope zlib TUs with cl /O2 /MT under Wine -> base objs."""
    msvc = _msvc_dir()
    cl = _find_ci(msvc / "bin", "cl.exe")
    if not cl:
        die(f"CL.EXE not found under {msvc}/bin - run inside `nix develop .#build` "
            "or build the toolchain (nix build .#gruntz-toolchain --out-link /tmp/gtc).")
    if shutil.which("wine") is None:
        die("wine not found - run inside `nix develop .#build` (provides wine-staging).")

    BASE_DIR.mkdir(parents=True, exist_ok=True)
    built = []
    for unit in ZLIB_UNITS:
        src = ZLIB_DIR / f"{unit}.c"
        if not src.exists():
            log(f"  skip {unit}: {src} missing")
            continue
        obj = BASE_DIR / f"{unit}.obj"
        if obj.exists():
            obj.unlink()
        r = subprocess.run(
            ["wine", str(cl), *CL_FLAGS, winepath_w(src)],
            cwd=str(BASE_DIR), text=True, capture_output=True,
        )
        # cl writes <unit>.obj into cwd; wine spews unrelated noise on stderr.
        if obj.exists():
            built.append(unit)
        else:
            log(f"  FAILED to compile {unit}.c:")
            tail = "\n".join((r.stdout + r.stderr).strip().splitlines()[-8:])
            print(tail, file=sys.stderr)
    log(f"Compiled base objs ({len(built)}/{len(ZLIB_UNITS)}): {', '.join(built)}")
    return built


def run_report() -> dict:
    generate_objdiff_config.generate(OBJDIFF_DIR, ZLIB_UNITS)
    objdiff = _tool("objdiff-cli", "result-1", "result")
    log("Generating objdiff report ...")
    subprocess.run(
        [objdiff, "report", "generate", "-p", str(OBJDIFF_DIR), "-o", str(REPORT)],
        check=True, stdout=subprocess.DEVNULL,
    )
    return json.loads(REPORT.read_text())


# --- summary ----------------------------------------------------------------
def _int(v) -> int:
    return int(v) if v is not None else 0


def _pct(n: int, d: int) -> float:
    return 100.0 * n / d if d else 0.0


def summarize(report: dict) -> None:
    """Per-unit table + roll-up (vostok match_score.py style, adapted)."""
    m = report.get("measures", {})
    units = report.get("units", [])
    named_targets = {u for u in ZLIB_UNITS
                     if (TARGET_DIR / f"{u}.c.obj").exists()}

    print()
    print("  Unit          Funcs (exact)   Code matched   Status")
    print("  " + "-" * 60)
    for u in sorted(units, key=lambda x: x.get("name", "")):
        name = u.get("name", "?")
        um = u.get("measures", {})
        tf, mf = _int(um.get("total_functions")), _int(um.get("matched_functions"))
        tc, mc = _int(um.get("total_code")), _int(um.get("matched_code"))
        if name in named_targets:
            status = "MATCHING" if tf and mf == tf else "in progress"
        else:
            status = "unnamed (target-only)"
        funcs = f"{mf}/{tf}" if tf else "-"
        code = f"{_pct(mc, tc):.1f}%" if tc else "-"
        print(f"  {name:<12}  {funcs:>13}   {code:>12}   {status}")
    print("  " + "-" * 60)

    tf, mf = _int(m.get("total_functions")), _int(m.get("matched_functions"))
    fuzzy = m.get("fuzzy_match_percent", 0.0)
    print(f"  Overall: {mf}/{tf} functions exact ({_pct(mf, tf):.1f}%), "
          f"{fuzzy:.2f}% fuzzy across {len(named_targets)} named unit(s).")
    print(f"  Report: {REPORT}")
    print(f"  Names map: {NAMES_CSV} "
          f"(add rows + re-run to grow; in-scope units: {len(ZLIB_UNITS)}).")


def main() -> None:
    ap = argparse.ArgumentParser(description="Rebuild + objdiff the Gruntz matching loop.")
    ap.add_argument("--force", action="store_true",
                    help="re-synth the PDB + re-delink even if inputs are unchanged.")
    ap.add_argument("--no-compile", action="store_true",
                    help="skip the base cl compile (reuse existing base objs).")
    args = ap.parse_args()

    synth_and_delink(args.force)
    if not args.no_compile:
        compile_base()
    report = run_report()
    summarize(report)


if __name__ == "__main__":
    main()
