#!/usr/bin/env python3
"""gruntz.py - the single entry point for the Gruntz matching pipeline.

Run inside the Nix dev shell:

    nix develop .#build --command python3 gruntz.py build
    nix develop         --command python3 gruntz.py status

Subcommands
-----------
  build [--source P ...] [-- <ninja args>]
        Compile -> derive labels -> delink -> objdiff, and print the match
        summary. The heavy lifting is ninja's incremental dependency graph
        (configure.py emits it from config/units.toml):

            src/<unit>.cpp --cl(wine)--> base/<unit>.obj
            ALL src @address + ALL base objs --gen_labels--> build/gen/symbol_names.csv
            symbol_names.csv + ghidra functions.csv/symbols.csv + delinkable EXE
                --delink(synth_pdb -> vostok-delinker)--> target/<unit>.c.obj
            base vs target --objdiff--> report.json

        `--source` force-builds those TUs' base objs first (fail-fast on compile
        errors); ninja then re-runs only the affected edges. symbol_names.csv is
        REGENERATED every build from the src `@address` annotations, so renamed
        symbols re-mangle and removed ones drop automatically - no stale rows.

  labels        Just (re)generate build/gen/symbol_names.csv from src @address.
  structs       Just (re)generate build/gen/structs.json + enums.json (clang).
  ghidra-refresh  Apply generated names/structs/enums to the Ghidra DB and
                  re-export functions.csv/symbols.csv (the Part-2 loop that feeds
                  the delink). See the docstring on cmd_ghidra_refresh.
  status        Print the last objdiff match summary (no rebuild).
  todo          List obj symbols that lack an @address (matching worklist).
"""

import argparse
import json
import os
import shutil
import subprocess
import sys
import tomllib
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parent.parent)
SCRIPTS            = REPO / "scripts"
PKG                = SCRIPTS / "gruntz"       # the pipeline package (grouped by area)
BUILD              = PKG / "build"            # cc_wrap, labels, structs, synth_pdb, delink, ...
GHIDRA             = PKG / "ghidra"           # apply (the comprehension-DB enrichment)
INIT               = PKG / "init"             # environment setup
MANIFEST           = REPO / "config" / "units.toml"
OBJDIFF_DIR        = REPO / "build" / "objdiff"
TARGET_DIR         = OBJDIFF_DIR / "target"
REPORT             = OBJDIFF_DIR / "report.json"
GEN_NAMES          = REPO / "build" / "gen" / "symbol_names.csv"
GHIDRA_PROJECT_DIR = REPO / "build" / "ghidra-named"


def log(msg: str) -> None:
    print(f"[gruntz] {msg}", flush=True)


def die(msg: str) -> None:
    print(f"[gruntz] ERROR: {msg}", file=sys.stderr)
    sys.exit(1)


def tool(name: str, *symlinks: str) -> str:
    """Resolve a tool: PATH first, then ./result* nix-build symlinks."""
    found = shutil.which(name)
    if found:
        return found
    for link in symlinks:
        p = REPO / link / "bin" / name
        if p.exists():
            return str(p)
    return name  # let subprocess surface a clear error


def run(cmd: list, **kw) -> subprocess.CompletedProcess:
    log("$ " + " ".join(str(c) for c in cmd))
    return subprocess.run(cmd, check=True, cwd=str(REPO), **kw)


def units() -> list[dict]:
    with MANIFEST.open("rb") as f:
        return tomllib.load(f).get("unit", [])


def unit_of_source(src: str) -> str | None:
    s = src.lstrip("./")
    for u in units():
        if u["source"].lstrip("./") == s or Path(u["source"]).name == Path(s).name:
            return u["unit"]
    return None


# --- summary ---------------------------------------------------------------
def _i(v) -> int:
    return int(v) if v is not None else 0


def _pct(n: int, d: int) -> float:
    return 100.0 * n / d if d else 0.0


def summarize(report: dict) -> None:
    m = report.get("measures", {})
    named = {u["unit"] for u in units() if (TARGET_DIR / f"{u['unit']}.c.obj").exists()}
    print()
    print("  Unit              Funcs (exact)   Code matched   Status")
    print("  " + "-" * 62)
    for u in sorted(report.get("units", []), key=lambda x: x.get("name", "")):
        name = u.get("name", "?")
        um = u.get("measures", {})
        tf, mf = _i(um.get("total_functions")), _i(um.get("matched_functions"))
        tc, mc = _i(um.get("total_code")), _i(um.get("matched_code"))
        status = ("MATCHING" if (name in named and tf and mf == tf)
                  else "in progress" if name in named else "unnamed")
        funcs = f"{mf}/{tf}" if tf else "-"
        code = f"{_pct(mc, tc):.1f}%" if tc else "-"
        print(f"  {name:<16}  {funcs:>13}   {code:>12}   {status}")
    print("  " + "-" * 62)
    tf, mf = _i(m.get("total_functions")), _i(m.get("matched_functions"))
    print(f"  Overall: {mf}/{tf} functions exact ({_pct(mf, tf):.1f}%), "
          f"{m.get('fuzzy_match_percent', 0.0):.2f}% fuzzy across "
          f"{len(named)} named unit(s).")
    print(f"  Report: {REPORT}")


# --- subcommands -----------------------------------------------------------
def cmd_build(args) -> None:
    run([sys.executable, str(REPO / "configure.py")])
    ninja = tool("ninja")
    # --source forces a rebuild of those TUs. ninja has no --force flag, so we
    # clean their outputs (ninja's native "force": a missing output is rebuilt);
    # ninja then re-runs only the affected edges (obj -> gen_labels -> delink ->
    # objdiff). Unlike `touch`, this doesn't churn source-file mtimes. Normally
    # you don't need --source at all: editing a source already makes ninja
    # rebuild it on the next `build`.
    force_objs = []
    for s in args.source:
        u = unit_of_source(s)
        if u is None or not (REPO / s).exists():
            die(f"--source {s}: not a unit source in {MANIFEST}")
        force_objs.append(f"build/objdiff/base/{u}.obj")
    if force_objs:
        run([ninja, "-t", "clean", *force_objs])   # force: drop outputs
    run([ninja, *args.ninja_args])        # incremental: rebuilds only what changed
    objdiff = tool("objdiff-cli", "result-1", "result")
    run([objdiff, "report", "generate", "-p", str(OBJDIFF_DIR), "-o", str(REPORT)],
        stdout=subprocess.DEVNULL)
    summarize(json.loads(REPORT.read_text()))


def cmd_labels(args) -> None:
    """Regenerate build/gen/symbol_names.csv from src @address + base objs."""
    nm = tool("llvm-nm")
    clang = _clang()
    tu_obj = []
    for u in units():
        tu_obj += ["--tu", u["source"], "--obj", f"build/objdiff/base/{u['unit']}.obj"]
    run([sys.executable, str(BUILD / "labels.py"),
         "--clang", clang, "--nm", nm, *tu_obj, "--out", str(GEN_NAMES)])


def cmd_structs(args) -> None:
    """Regenerate build/gen/structs.json + enums.json via clang record layouts.

    Sources: matched src/ layouts (the clangd compdb) PLUS the converted
    comprehension headers under structure/ (each wrapped as a .cpp TU). src/ wins
    on overlapping names, so apply_ghidra.py's hardcoded fallback is unneeded for
    anything covered here.
    """
    clang = _clang()
    cmd = [sys.executable, str(BUILD / "structs.py"), "--clang", clang]
    for t in args.tu:
        cmd += ["--tu", t]
    if (REPO / "structure").is_dir():
        cmd += ["--header", "structure"]          # comprehension layouts
    run(cmd)


def cmd_ghidra_refresh(args) -> None:
    """Part-2 loop: push generated names/structs/enums into the Ghidra DB, then
    re-export the functions.csv/symbols.csv the delink consumes.

      1. gen_structs -> build/gen/structs.json + enums.json (clang layouts of
         src/ + the converted structure/ comprehension headers)
      2. analyzeHeadless: apply.py (names from build/gen/symbol_names.csv,
         prototypes, struct this-types, enums -> the build/ghidra-named DB) then
         export.py (re-dump functions.csv + symbols.csv from the enriched DB).
      3. those CSVs feed the next `build`.
    """
    cmd_structs(argparse.Namespace(tu=[]))
    analyze = tool("analyzeHeadless", "result", "result-1")
    if not GHIDRA_PROJECT_DIR.exists():
        die(f"no Ghidra project at {GHIDRA_PROJECT_DIR}")
    run([analyze, str(GHIDRA_PROJECT_DIR), "gruntz", "-process", "GRUNTZ.EXE",
         "-noanalysis", "-scriptPath", str(GHIDRA),
         "-postScript", "apply.py", "-postScript", "export.py"])
    log("ghidra-refresh done: applied generated labels/structs/enums + re-exported "
        "functions.csv/symbols.csv.")


def cmd_init(args) -> None:
    """Idempotent local-environment setup. Run inside `nix develop .#build`.

      - create the git-ignored build dirs;
      - initialise the Wine prefix + register the MSVC 5.0 toolchain
        (scripts/gruntz/init/toolchain.py; skips re-init when already set up);
      - (re)generate the clangd compile database (scripts/gruntz/init/clangd.py),
        which gen_structs + the editor read.

    The flake's `.#build` shellHook calls this on first entry, so the environment
    self-builds once and subsequent shells are instant. Re-running is safe.
    """
    for d in ("build/gen", "build/objdiff", "build/clangd", "build/pdb",
              "build/delink/named"):
        (REPO / d).mkdir(parents=True, exist_ok=True)
    if not os.environ.get("MSVC_DIR"):
        log("MSVC_DIR unset - run inside `nix develop .#build` for the wine/cl + "
            "clangd setup. Created build dirs only.")
        return
    tc = [sys.executable, str(INIT / "toolchain.py")]
    if args.force:
        tc.append("--force")
    run(tc)                                          # wine prefix + registry (idempotent)
    run([sys.executable, str(INIT / "clangd.py")])   # clangd compile database
    log("init complete (idempotent).")


def cmd_status(args) -> None:
    if not REPORT.exists():
        die(f"no report at {REPORT}; run `gruntz.py build` first")
    summarize(json.loads(REPORT.read_text()))


def cmd_todo(args) -> None:
    """Obj symbols with no @address yet (the matching worklist) - a discovery aid.

    A symbol is in the matched set iff its source function carries an @address;
    this lists base-obj code symbols whose RVA is absent from the generated
    symbol_names.csv, i.e. candidates to locate + annotate.
    """
    if not GEN_NAMES.exists():
        die(f"no {GEN_NAMES}; run `gruntz.py build` or `labels` first")
    have = set()
    for line in GEN_NAMES.read_text().splitlines():
        p = line.split(",")
        if len(p) == 3 and p[0].startswith("0x"):
            have.add(p[1])
    nm = tool("llvm-nm")
    total = 0
    for u in units():
        obj = REPO / f"build/objdiff/base/{u['unit']}.obj"
        if not obj.exists():
            continue
        res = subprocess.run([nm, "--defined-only", str(obj)],
                             capture_output=True, text=True)
        missing = [ln.split()[-1] for ln in res.stdout.splitlines()
                   if len(ln.split()) >= 2 and ln.split()[-2] in "TtWw"
                   and ln.split()[-1] not in have
                   and not ln.split()[-1].startswith(("?dtor$", "___ehhandler$"))]
        if missing:
            total += len(missing)
            print(f"  {u['unit']}: {len(missing)} unannotated")
            for s in missing[:12]:
                print(f"      {s}")
    log(f"{total} obj symbol(s) without an @address (worklist).")


def _clang() -> str:
    import os
    return os.environ.get("GRUNTZ_CLANG") or tool("clang")


def main() -> None:
    ap = argparse.ArgumentParser(prog="gruntz.py", description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)

    b = sub.add_parser("build", help="compile -> labels -> delink -> objdiff")
    b.add_argument("--source", action="append", default=[],
                   help="force-build this TU first (repeatable); omit = all.")
    b.add_argument("ninja_args", nargs=argparse.REMAINDER,
                   help="extra ninja args after `--` (e.g. -j8).")
    b.set_defaults(func=cmd_build)

    sub.add_parser("labels", help="regenerate symbol_names.csv from src @address"
                   ).set_defaults(func=cmd_labels)

    s = sub.add_parser("structs", help="regenerate structs.json + enums.json")
    s.add_argument("--tu", action="append", default=[])
    s.set_defaults(func=cmd_structs)

    sub.add_parser("ghidra-refresh", help="apply generated data to Ghidra + re-export"
                   ).set_defaults(func=cmd_ghidra_refresh)
    i = sub.add_parser("init", help="idempotent local-env setup (wine prefix + clangd compdb)")
    i.add_argument("--force", action="store_true", help="re-init the wine prefix")
    i.set_defaults(func=cmd_init)
    sub.add_parser("status", help="print the last objdiff summary"
                   ).set_defaults(func=cmd_status)
    sub.add_parser("todo", help="obj symbols lacking an @address (worklist)"
                   ).set_defaults(func=cmd_todo)

    args = ap.parse_args()
    if getattr(args, "ninja_args", None) and args.ninja_args[:1] == ["--"]:
        args.ninja_args = args.ninja_args[1:]
    args.func(args)


if __name__ == "__main__":
    main()
