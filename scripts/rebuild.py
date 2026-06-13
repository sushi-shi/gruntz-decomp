#!/usr/bin/env python3
"""rebuild.py - thin convenience wrapper around the ninja matching build.

The matching pipeline is now a native incremental ninja build generated from
config/units.toml by configure.py (see docs/build-system.md). This script is a
one-command front door that:

    1. runs `configure.py`         (manifest -> build.ninja + compile_commands
                                     + build/objdiff/objdiff.json)
    2. runs `ninja`                (compile changed TUs -> base objs; delink the
                                     target -> per-unit <unit>.c.obj)
    3. runs `objdiff-cli report`   (-> build/objdiff/report.json)
    4. prints the per-unit + roll-up match summary.

It no longer recompiles everything every time - ninja rebuilds only what
changed (edit a unit's source / add a row to config/symbol_names.csv / add an
[[unit]] to config/units.toml, then re-run). The heavy lifting lives in:
    configure.py            - manifest -> build graph + objdiff project
    scripts/cc_wrap.py      - the wine-cl rule
    scripts/delink_target.py- the synth_pdb -> vostok-delinker -> collect step

USAGE (inside the build dev shell):
    nix develop .#build --command python3 scripts/rebuild.py
    nix develop .#build --command python3 scripts/rebuild.py -- -j8   # ninja args
"""

import argparse
import json
import shutil
import subprocess
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
REPO = SCRIPT_DIR.parent

OBJDIFF_DIR = REPO / "build" / "objdiff"
TARGET_DIR = OBJDIFF_DIR / "target"
REPORT = OBJDIFF_DIR / "report.json"
NAMES_CSV = REPO / "config" / "symbol_names.csv"
MANIFEST = REPO / "config" / "units.toml"


def log(msg: str) -> None:
    print(f"[rebuild] {msg}", flush=True)


def tool(name: str, *symlinks: str) -> str:
    found = shutil.which(name)
    if found:
        return found
    for link in symlinks:
        p = REPO / link / "bin" / name
        if p.exists():
            return str(p)
    return name


def manifest_units() -> list[str]:
    import tomllib
    with MANIFEST.open("rb") as f:
        return [u["unit"] for u in tomllib.load(f).get("unit", [])]


# --- summary (kept from the old loop) ---------------------------------------
def _int(v) -> int:
    return int(v) if v is not None else 0


def _pct(n: int, d: int) -> float:
    return 100.0 * n / d if d else 0.0


def summarize(report: dict, units: list[str]) -> None:
    m = report.get("measures", {})
    runits = report.get("units", [])
    named_targets = {u for u in units if (TARGET_DIR / f"{u}.c.obj").exists()}

    print()
    print("  Unit          Funcs (exact)   Code matched   Status")
    print("  " + "-" * 60)
    for u in sorted(runits, key=lambda x: x.get("name", "")):
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
    print(f"  Manifest: {MANIFEST} ; names: {NAMES_CSV}")


def main() -> None:
    ap = argparse.ArgumentParser(description="Rebuild + objdiff the Gruntz matching loop.")
    ap.add_argument("ninja_args", nargs=argparse.REMAINDER,
                    help="extra args passed to ninja (after `--`, e.g. -j8).")
    args = ap.parse_args()
    ninja_args = args.ninja_args
    if ninja_args and ninja_args[0] == "--":
        ninja_args = ninja_args[1:]

    log("configure (manifest -> build.ninja + objdiff project) ...")
    subprocess.run([sys.executable, str(REPO / "configure.py")], check=True, cwd=str(REPO))

    log("ninja (compile changed TUs + delink target) ...")
    ninja = tool("ninja")
    subprocess.run([ninja, *ninja_args], check=True, cwd=str(REPO))

    log("objdiff report ...")
    objdiff = tool("objdiff-cli", "result-1", "result")
    subprocess.run(
        [objdiff, "report", "generate", "-p", str(OBJDIFF_DIR), "-o", str(REPORT)],
        check=True, stdout=subprocess.DEVNULL,
    )

    summarize(json.loads(REPORT.read_text()), manifest_units())


if __name__ == "__main__":
    main()
