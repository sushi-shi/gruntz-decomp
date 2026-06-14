#!/usr/bin/env python3
"""gruntz.py - the single entry point for the Gruntz matching pipeline.

Run inside the Nix dev shell:

    nix develop .#build --command python3 gruntz.py build
    nix develop         --command python3 gruntz.py status

Subcommands
-----------
  build [-- <ninja args>]
        Compile -> derive labels -> delink -> objdiff, and print the match
        summary. The heavy lifting is ninja's incremental dependency graph
        (configure.py emits it from config/units.toml):

            src/<unit>.cpp --cl(wine)--> base/<unit>.obj
            ALL src @address + ALL base objs --gen_labels--> build/gen/symbol_names.csv
            symbol_names.csv + ghidra functions.csv/symbols.csv + delinkable EXE
                --delink(synth_pdb -> vostok-delinker)--> target/<unit>.c.obj
            base vs target --objdiff--> report.json

        symbol_names.csv is REGENERATED every build from the src `@address`
        annotations, so renamed symbols re-mangle and removed ones drop
        automatically - no stale rows.

  labels        Just (re)generate build/gen/symbol_names.csv from src @address.
  structs       Just (re)generate build/gen/structs.json + enums.json (clang).
  ghidra-refresh  Apply generated names/structs/enums to the Ghidra DB and
                  re-export functions.csv/symbols.csv (the Part-2 loop that feeds
                  the delink). See the docstring on cmd_ghidra_refresh.
  init          One-time FULL local setup: dirs + configure + delinkable EXE +
                wine prefix + clangd compdb + the Ghidra DB (import+analyze
                GRUNTZ.EXE -> functions.csv/symbols.csv). HEAVY first run.
  clangd        (Re)generate the clangd compile DB (editor; after adding a unit).
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
GHIDRA_DRIVER      = GHIDRA / "run_enrich.py" # PyGhidra driver: import/analyze + apply + export
GHIDRA_APPLY       = GHIDRA / "apply.py"      # enrichment GhidraScript (run under PyGhidra)
GHIDRA_EXPORT      = GHIDRA / "export.py"     # functions.csv/symbols.csv dump GhidraScript
INIT               = PKG / "init"             # environment setup
MANIFEST           = REPO / "config" / "units.toml"
OBJDIFF_DIR        = REPO / "build" / "objdiff"
TARGET_DIR         = OBJDIFF_DIR / "target"
REPORT             = OBJDIFF_DIR / "report.json"
GEN_NAMES          = REPO / "build" / "gen" / "symbol_names.csv"
GHIDRA_PROJECT_DIR = REPO / "build" / "ghidra-named"
GHIDRA_PROJECT     = "gruntz"                                          # project name (gruntz.{gpr,rep})
GHIDRA_FUNCTIONS   = REPO / "build" / "ghidra-enrich" / "exports" / "functions.csv"  # delink input
RETAIL_EXE         = REPO / "build" / "exe" / "GRUNTZ.EXE"             # stable copy of $GRUNTZ_EXE
DELINKABLE_EXE     = REPO / "build" / "exe" / "GRUNTZ.delinkable.EXE"  # reloc-broken (delink input)
CONFIGURE          = REPO / "configure.py"
RELOC              = BUILD / "reloc.py"


def log(msg: str) -> None:
    print(f"[gruntz] {msg}", flush=True)


def die(msg: str) -> None:
    print(f"[gruntz] ERROR: {msg}", file=sys.stderr)
    sys.exit(1)


def tool(name: str) -> str:
    """Resolve a tool on PATH - the `nix develop .#build` shell provides them all."""
    return shutil.which(name) or name  # bare name lets subprocess surface a clear error


def run(cmd: list, **kw) -> subprocess.CompletedProcess:
    log("$ " + " ".join(str(c) for c in cmd))
    return subprocess.run(cmd, check=True, cwd=str(REPO), **kw)


def units() -> list[dict]:
    with MANIFEST.open("rb") as f:
        return tomllib.load(f).get("unit", [])


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
    run([sys.executable, str(CONFIGURE)])             # regenerate build.ninja + the JSONs
    _ensure_delinkable_exe()                          # cheap, idempotent (reloc over $GRUNTZ_EXE)
    if not GHIDRA_FUNCTIONS.exists():
        die(f"no Ghidra exports ({GHIDRA_FUNCTIONS.relative_to(REPO)}) - run `gruntz init` first.")
    ninja = tool("ninja")
    run([ninja, *args.ninja_args])        # incremental: rebuilds only what changed

    objdiff = tool("objdiff-cli")
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
      2. PyGhidra driver (run_enrich.py): re-open the already-analyzed
         build/ghidra-named program (--no-analyze) and run apply.py (names from
         build/gen/symbol_names.csv, prototypes, struct this-types, enums) then
         export.py (re-dump functions.csv + symbols.csv from the enriched DB).
      3. those CSVs feed the next `build`.
    """
    cmd_structs(argparse.Namespace(tu=[]))
    if not GHIDRA_PROJECT_DIR.exists() or not any(GHIDRA_PROJECT_DIR.glob("*.rep")):
        die(f"no Ghidra project at {GHIDRA_PROJECT_DIR} - run `gruntz init` first.")
    _run_enrich(analyze=False)
    log("ghidra-refresh done: applied generated labels/structs/enums + re-exported "
        "functions.csv/symbols.csv.")


def _run_enrich(analyze: bool) -> None:
    """Drive the PyGhidra enrichment+export (replaces analyzeHeadless).

    Runs scripts/gruntz/ghidra/run_enrich.py under THIS interpreter (sys.executable
    is the `nix develop .#build` python that carries the pyghidra package): it boots
    PyGhidra in-process, imports/analyzes GRUNTZ.EXE into build/ghidra-named/gruntz,
    then runs apply.py + export.py as GhidraScripts. `analyze=True` for the first
    import; False to re-run apply/export on the already-analyzed DB.
    """
    cmd = [sys.executable, str(GHIDRA_DRIVER), str(RETAIL_EXE),
           str(GHIDRA_PROJECT_DIR), GHIDRA_PROJECT,
           str(GHIDRA_APPLY), str(GHIDRA_EXPORT)]
    if not analyze:
        cmd.append("--no-analyze")
    run(cmd)


def _ensure_delinkable_exe() -> None:
    """Reloc-break $GRUNTZ_EXE -> build/exe/GRUNTZ.delinkable.EXE (a delink input).

    Pure, deterministic transform of the pinned retail EXE via reloc.py. Also keeps
    a stable-named retail copy at build/exe/GRUNTZ.EXE so PyGhidra imports a program
    named GRUNTZ.EXE (the run_enrich.py import target). Idempotent.
    """
    retail = os.environ.get("GRUNTZ_EXE")
    if not retail:
        log("GRUNTZ_EXE unset - skipping delinkable-EXE prep (run inside nix develop).")
        return
    RETAIL_EXE.parent.mkdir(parents=True, exist_ok=True)
    if not RETAIL_EXE.exists():
        shutil.copyfile(retail, RETAIL_EXE)
    if not DELINKABLE_EXE.exists():
        run([sys.executable, str(RELOC), "--in-exe", str(RETAIL_EXE),
             "--out-exe", str(DELINKABLE_EXE)])


def _build_ghidra_db(reimport: bool = False) -> None:
    """Create build/ghidra-named (import + auto-analyze GRUNTZ.EXE) if absent, then
    enrich + export functions.csv/symbols.csv via ghidra-refresh. HEAVY; idempotent.

    apply.py CONSUMES the tracked config/{engine,library}_labels.csv - FID labels are
    NOT regenerated here (the committed config/library_labels.csv is canonical;
    regenerate it explicitly with scripts/analysis/fid_generate.py).
    """
    if GHIDRA_FUNCTIONS.exists() and not reimport:
        log(f"Ghidra exports present ({GHIDRA_FUNCTIONS.relative_to(REPO)}); "
            "skipping (--reimport to rebuild).")
        return
    if not RETAIL_EXE.exists():
        die("no build/exe/GRUNTZ.EXE - set $GRUNTZ_EXE (run inside `nix develop .#build`).")
    if reimport and GHIDRA_PROJECT_DIR.exists():
        shutil.rmtree(GHIDRA_PROJECT_DIR)
    GHIDRA_PROJECT_DIR.mkdir(parents=True, exist_ok=True)
    cmd_structs(argparse.Namespace(tu=[]))     # build/gen/structs.json+enums.json (apply.py needs them)
    log("Building Ghidra DB via PyGhidra: import + auto-analyze GRUNTZ.EXE "
        "(SEVERAL MINUTES on first run), then apply.py + export.py ...")
    _run_enrich(analyze=True)   # import/analyze (if needed) + apply.py + export.py, one process


def cmd_clangd(args) -> None:
    """(Re)generate the clangd compile DB (editor-only; run after adding a unit)."""
    run([sys.executable, str(INIT / "clangd.py")])


def cmd_init(args) -> None:
    """One-time FULL local setup for this checkout. Run inside `nix develop .#build`.

    Builds the local, imperative state (under build/) that Nix does not - so a fresh
    checkout goes straight to `gruntz build` after one `init`:
      - the git-ignored build dirs;
      - build.ninja + compile_commands.json + objdiff.json (configure.py);
      - reloc-break $GRUNTZ_EXE -> build/exe/GRUNTZ.delinkable.EXE (a delink input);
      - the Wine prefix + MSVC 5.0 toolchain registration (toolchain.py);
      - the clangd compile DB (clangd.py);
      - the Ghidra DB: PyGhidra (run_enrich.py) imports + auto-analyzes GRUNTZ.EXE
        -> build/ghidra-named, then runs apply.py + export.py (as GhidraScripts
        under PyGhidra) -> functions.csv/symbols.csv. apply.py CONSUMES the tracked
        config/library_labels.csv; FID labels are NOT regenerated here.

    HEAVY on first run (the Ghidra analysis takes minutes); idempotent afterwards.
    --force re-inits the Wine prefix; --reimport rebuilds the Ghidra DB.
    """
    for d in ("build/gen", "build/objdiff", "build/clangd", "build/pdb",
              "build/delink/named", "build/exe", "build/ghidra-named",
              "build/ghidra-enrich/exports"):
        (REPO / d).mkdir(parents=True, exist_ok=True)
    run([sys.executable, str(CONFIGURE)])            # build.ninja + compile_commands + objdiff.json
    _ensure_delinkable_exe()                         # reloc.py over $GRUNTZ_EXE (cheap, idempotent)
    if not os.environ.get("MSVC_DIR"):
        log("MSVC_DIR unset - run inside `nix develop .#build` for the toolchain + "
            "Ghidra steps. Did dirs + configure + delinkable EXE only.")
        return
    tc = [sys.executable, str(INIT / "toolchain.py")]
    if args.force:
        tc.append("--force")
    run(tc)                                          # wine prefix + registry (idempotent)
    run([sys.executable, str(INIT / "clangd.py")])   # clangd compile database
    _build_ghidra_db(reimport=args.reimport)         # Ghidra DB + functions.csv/symbols.csv
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
    i = sub.add_parser("init", help="one-time FULL local setup (dirs/configure/EXE/wine/clangd/Ghidra DB)")
    i.add_argument("--force", action="store_true", help="re-init the wine prefix")
    i.add_argument("--reimport", action="store_true", help="rebuild the Ghidra DB from scratch")
    i.set_defaults(func=cmd_init)
    sub.add_parser("clangd", help="(re)generate the clangd compile DB (editor)"
                   ).set_defaults(func=cmd_clangd)
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
