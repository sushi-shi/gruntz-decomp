#!/usr/bin/env python3
"""gruntz.cli - the single entry point for the Gruntz matching pipeline.

Run inside the Nix dev shell (the `gruntz` wrapper, or `python -m gruntz`):

    nix develop .#build --command gruntz build
    nix develop         --command gruntz status

Subcommands
-----------
  build [-- <ninja args>]
        Compile -> derive labels -> delink -> objdiff, and print the match
        summary. The heavy lifting is ninja's incremental dependency graph
        (configure.py emits it from config/units.toml):

            src/<unit>.cpp --cl(wine)--> base/<unit>.obj
            ALL src @address + ALL base objs --gen_labels--> build/gen/symbol_names.csv
            symbol_names.csv + ghidra functions.csv/symbols.csv + GRUNTZ.EXE
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
  init          One-time FULL local setup: dirs + configure + retail EXE copy +
                wine prefix + clangd compdb + the Ghidra DB (import+analyze
                GRUNTZ.EXE -> functions.csv/symbols.csv). HEAVY first run.
  clangd        (Re)generate the clangd compile DB (editor; after adding a unit).
  status        Print the last objdiff match summary (no rebuild).
  todo          List obj symbols that lack an @address (matching worklist).
  clean         Nuke build/ + stray root artifacts (build.ninja/*.obj/.ninja_*)
                for a from-scratch init + build. HEAVY re-init (wine + Ghidra DB).
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
            Path(__file__).resolve().parents[2])
SCRIPTS            = REPO / "scripts"
PKG                = SCRIPTS / "gruntz"       # the pipeline package (grouped by area)
BUILD              = PKG / "build"            # cc_wrap, labels, structs, synth_pdb, delink, ...
GHIDRA             = PKG / "ghidra"           # the PyGhidra driver (a normal runnable module)
GHIDRA_DRIVER      = GHIDRA / "ghidra_metadata_apply.py" # PyGhidra driver: import/analyze + apply + export
# GhidraScripts run INSIDE Ghidra (PyGhidra injects currentProgram/monitor/state);
# they are NOT importable and NOT `python -m`-runnable — the driver passes them by
# PATH. They live in ghidra/scripts/ (no __init__.py) so the boundary is explicit.
GHIDRA_SCRIPTS     = GHIDRA / "scripts"       # GhidraScripts: path-only, never imported/-m'd
GHIDRA_APPLY       = GHIDRA_SCRIPTS / "apply.py"   # enrichment GhidraScript (run under PyGhidra)
GHIDRA_EXPORT      = GHIDRA_SCRIPTS / "export.py"  # functions.csv/symbols.csv dump GhidraScript
INIT               = PKG / "init"             # environment setup
MANIFEST           = REPO / "config" / "units.toml"
OBJDIFF_DIR        = REPO / "build" / "objdiff"
TARGET_DIR         = OBJDIFF_DIR / "target"
REPORT             = OBJDIFF_DIR / "report.json"
GEN_NAMES          = REPO / "build" / "gen" / "symbol_names.csv"
GHIDRA_PROJECT_DIR = REPO / "build" / "ghidra-named"
GHIDRA_PROJECT     = "gruntz"                                          # project name (gruntz.{gpr,rep})
GHIDRA_FUNCTIONS   = REPO / "build" / "ghidra-enrich" / "exports" / "functions.csv"  # delink input
RETAIL_EXE         = REPO / "build" / "exe" / "GRUNTZ.EXE"             # stable copy of $GRUNTZ_EXE (delink input + Ghidra import)
CONFIGURE          = REPO / "configure.py"


def log(msg: str) -> None:
    print(f"[gruntz] {msg}", flush=True)


def die(msg: str) -> None:
    print(f"[gruntz] ERROR: {msg}", file=sys.stderr)
    sys.exit(1)


def tool(name: str) -> str:
    """Resolve a tool on PATH - the `nix develop .#build` shell provides them all."""
    return shutil.which(name) or name  # bare name lets subprocess surface a clear error


def _pkg_env() -> dict:
    """os.environ with scripts/ guaranteed on PYTHONPATH, so child `python -m
    gruntz.<x>` invocations import the package even if the shell did not export it
    (the nix shells + the `gruntz` wrapper do, but keep the CLI self-contained)."""
    env = dict(os.environ)
    existing = env.get("PYTHONPATH", "")
    if str(SCRIPTS) not in existing.split(os.pathsep):
        env["PYTHONPATH"] = os.pathsep.join(p for p in (str(SCRIPTS), existing) if p)
    return env


def run(cmd: list, **kw) -> subprocess.CompletedProcess:
    log("$ " + " ".join(str(c) for c in cmd))
    kw.setdefault("env", _pkg_env())
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
    _ensure_retail_copy()                             # cheap, idempotent (stable retail copy)
    if not GHIDRA_FUNCTIONS.exists():
        die(f"no Ghidra exports ({GHIDRA_FUNCTIONS.relative_to(REPO)}) - run `gruntz init` first.")
    # Gate: the src/Stub @stub backlog is skipped by labels.py (engine_label_stubs),
    # so this is the only check on its address uniqueness + format. Fail fast.
    run([sys.executable, "-m", "gruntz.match.verify_stubs"])
    ninja = tool("ninja")
    _start_wine_session()                 # boot Wine clean BEFORE ninja's -j fan-out
    try:
        run([ninja, *args.ninja_args])    # incremental: rebuilds only what changed
    finally:
        _kill_wine_session()              # reap this prefix's wineserver + session

    objdiff = tool("objdiff-cli")
    run([objdiff, "report", "generate", "-p", str(OBJDIFF_DIR), "-o", str(REPORT)],
        stdout=subprocess.DEVNULL)

    summarize(json.loads(REPORT.read_text()))

    # Non-fatal extras: refresh per-function source fingerprints (so regression
    # checks can tell an edited function from a collateral drop), rewrite the
    # README score block (3 metrics vs the full engine), and print regressions
    # vs the committed best-% baseline. See gruntz.match.status +
    # docs/match-status.md. None of these gate the build.
    if (REPO / "build" / "clangd" / "compile_commands.json").is_file():
        subprocess.run([sys.executable, "-m", "gruntz.match.fingerprints"],
                       cwd=str(REPO), env=_pkg_env())
    subprocess.run([sys.executable, "-m", "gruntz.match.status",
                    "--report", str(REPORT), "summary", "--write-readme"],
                   cwd=str(REPO), stdout=subprocess.DEVNULL, env=_pkg_env())
    if (REPO / "config" / "match_baseline.tsv").is_file():
        log("regressions vs baseline ...")
        subprocess.run([sys.executable, "-m", "gruntz.match.status",
                        "--report", str(REPORT), "check"], cwd=str(REPO), env=_pkg_env())


def cmd_labels(args) -> None:
    """Regenerate build/gen/symbol_names.csv from src @address + base objs."""
    nm = tool("llvm-nm")
    clang = _clang()
    tu_obj = []
    for u in units():
        tu_obj += ["--tu", u["source"], "--obj", f"build/objdiff/base/{u['unit']}.obj"]
    compdb = REPO / "build" / "clangd" / "compile_commands.json"
    cmd = [sys.executable, str(BUILD / "labels.py"),
           "--clang", clang, "--nm", nm, *tu_obj, "--out", str(GEN_NAMES)]
    if compdb.is_file():
        cmd += ["--compdb", str(compdb)]
    run(cmd)


def cmd_structs(args) -> None:
    """Regenerate build/gen/structs.json + enums.json via clang record layouts.

    Sources: matched src/ layouts (the clangd compdb) PLUS the converted
    comprehension headers under structure/ (each wrapped as a .cpp TU). src/ wins
    on overlapping names, so apply_ghidra.py's hardcoded fallback is unneeded for
    anything covered here.
    """
    clang = _clang()
    cmd = [sys.executable, str(BUILD / "ghidra_metadata_generate.py"), "--clang", clang]
    for t in args.tu:
        cmd += ["--tu", t]
    if (REPO / "structure").is_dir():
        cmd += ["--header", "structure"]          # comprehension layouts
    run(cmd)


def cmd_ghidra_refresh(args) -> None:
    """Part-2 loop: push generated names/structs/enums into the Ghidra DB, then
    re-export the functions.csv/symbols.csv the delink consumes.

      1. ghidra_metadata_generate -> build/gen/structs.json + enums.json (clang layouts of
         src/ + the converted structure/ comprehension headers)
      2. PyGhidra driver (ghidra_metadata_apply.py): re-open the already-analyzed
         build/ghidra-named program (--no-analyze) and run apply.py (names from
         build/gen/symbol_names.csv, prototypes, struct this-types, enums) then
         export.py (re-dump functions.csv + symbols.csv from the enriched DB).
      3. those CSVs feed the next `build`.
    """
    cmd_structs(argparse.Namespace(tu=[]))
    if not GHIDRA_PROJECT_DIR.exists() or not any(GHIDRA_PROJECT_DIR.glob("*.rep")):
        die(f"no Ghidra project at {GHIDRA_PROJECT_DIR} - run `gruntz init` first.")
    _ghidra_metadata_apply(analyze=False)
    log("ghidra-refresh done: applied generated labels/structs/enums + re-exported "
        "functions.csv/symbols.csv.")


def _ghidra_metadata_apply(analyze: bool) -> None:
    """Drive the PyGhidra enrichment+export (replaces analyzeHeadless).

    Runs scripts/gruntz/ghidra/ghidra_metadata_apply.py under THIS interpreter (sys.executable
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


def _ensure_retail_copy() -> None:
    """Keep a stable-named retail copy at build/exe/GRUNTZ.EXE.

    PyGhidra imports a program named GRUNTZ.EXE (the ghidra_metadata_apply.py import target)
    and the delinker reads this same copy as its input. The delinker handles
    cyclic relocation pointers itself, so no reloc-break preprocessing is needed.
    Idempotent.
    """
    retail = os.environ.get("GRUNTZ_EXE")
    if not retail:
        log("GRUNTZ_EXE unset - skipping retail-EXE copy (run inside nix develop).")
        return
    RETAIL_EXE.parent.mkdir(parents=True, exist_ok=True)
    if not RETAIL_EXE.exists():
        shutil.copyfile(retail, RETAIL_EXE)


def _start_wine_session() -> None:
    """Boot this prefix's Wine session ONCE, stdio detached, before ninja fans out.

    Every `wine cl` (via cc_wrap) shares one per-prefix wineserver. If a parallel
    cc_wrap is the first to boot the session, the daemonised, persistent (-p)
    session (wineserver/services.exe/...) inherits THAT cc_wrap's stdout/stderr -
    which is ninja's capture pipe - and holds the write-end open forever, so ninja
    never sees EOF and the build hangs at zero CPU. Booting it up front with DEVNULL
    stdio means the parallel cc_wraps only ever *connect* to a running session.
    """
    if not os.environ.get("WINEPREFIX") or shutil.which("wineserver") is None:
        return
    n = subprocess.DEVNULL
    subprocess.run(["wineserver", "-p"], stdin=n, stdout=n, stderr=n, check=False)
    subprocess.run(["wineboot"], stdin=n, stdout=n, stderr=n, check=False)


def _kill_wine_session() -> None:
    """SIGKILL this prefix's wineserver + session after the build (`wineserver -k`).

    Scoped to $WINEPREFIX, so it only reaps THIS worktree's server + leftover
    cl.exe/winedevice/..., never another prefix's. Also clears the persistent (-p)
    sessions that would otherwise linger for days.
    """
    if not os.environ.get("WINEPREFIX") or shutil.which("wineserver") is None:
        return
    n = subprocess.DEVNULL
    subprocess.run(["wineserver", "-k"], stdin=n, stdout=n, stderr=n, check=False)


def _build_ghidra_db(reimport: bool = False) -> None:
    """Create build/ghidra-named (import + auto-analyze GRUNTZ.EXE) if absent, then
    enrich + export functions.csv/symbols.csv via ghidra-refresh. HEAVY; idempotent.

    apply.py CONSUMES the tracked config/{engine,library}_labels.csv - FID labels are
    NOT regenerated here (the committed config/library_labels.csv is canonical;
    regenerate it explicitly with `python -m gruntz.analysis.fid_generate`).
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
    _ghidra_metadata_apply(analyze=True)   # import/analyze (if needed) + apply.py + export.py, one process


def cmd_clangd(args) -> None:
    """(Re)generate the clangd compile DB (editor-only; run after adding a unit)."""
    run([sys.executable, str(INIT / "clangd.py")])


def _ghidra_warm() -> bool:
    """True if the Ghidra export already has a function at every labeled RVA, i.e.
    the warmup has run. Keeps cmd_init's warmup IDEMPOTENT so the build-shell hook's
    startup `gruntz init` stays light on an already-warmed checkout (only a cold DB
    triggers the heavy build->refresh->build)."""
    if not (GEN_NAMES.exists() and GHIDRA_FUNCTIONS.exists()):
        return False
    import csv

    def rint(s):
        s = str(s).strip()
        return int(s, 16) if s.lower().startswith("0x") else int(s)

    want = set()
    for r in csv.reader(GEN_NAMES.open()):
        if len(r) >= 5 and r[0].startswith("0x") and r[4] == "func":
            try:
                want.add(rint(r[0]))
            except ValueError:
                pass
    if not want:
        return False
    have = set()
    for r in csv.DictReader(GHIDRA_FUNCTIONS.open()):
        try:
            have.add(rint(r["entry_rva"]))
        except Exception:
            pass
    return want <= have


def cmd_init(args) -> None:
    """One-time FULL local setup for this checkout. Run inside `nix develop .#build`.

    Builds the local, imperative state (under build/) that Nix does not - so a fresh
    checkout goes straight to `gruntz build` after one `init`:
      - the git-ignored build dirs;
      - build.ninja + compile_commands.json + objdiff.json (configure.py);
      - a stable retail copy at build/exe/GRUNTZ.EXE (the delink input + Ghidra import);
      - the Wine prefix + MSVC 5.0 toolchain registration (toolchain.py);
      - the clangd compile DB (clangd.py);
      - the Ghidra DB: PyGhidra (ghidra_metadata_apply.py) imports + auto-analyzes GRUNTZ.EXE
        -> build/ghidra-named, then runs apply.py + export.py (as GhidraScripts
        under PyGhidra) -> functions.csv/symbols.csv. apply.py CONSUMES the tracked
        config/library_labels.csv; FID labels are NOT regenerated here.

    Then WARMS the Ghidra DB (unless --no-warmup): build -> ghidra-refresh ->
    build. The cold DB carves only FID/auto-analysis functions; the warmup runs
    one build to produce symbol_names.csv, refreshes Ghidra so a function exists
    at each labeled RVA, and re-exports - so `gruntz build` and the "vs full
    engine" metric are reproducible (not dependent on accumulated DB state).

    HEAVY on first run (Ghidra analysis + two builds take a while); idempotent
    afterwards. --force re-inits the Wine prefix; --reimport rebuilds the Ghidra
    DB; --no-warmup leaves the DB cold.
    """
    for d in ("build/gen", "build/objdiff", "build/clangd", "build/pdb",
              "build/delink/named", "build/exe", "build/ghidra-named",
              "build/ghidra-enrich/exports"):
        (REPO / d).mkdir(parents=True, exist_ok=True)
    run([sys.executable, str(CONFIGURE)])            # build.ninja + compile_commands + objdiff.json
    _ensure_retail_copy()                            # stable retail copy (cheap, idempotent)
    if not os.environ.get("MSVC_DIR"):
        log("MSVC_DIR unset - run inside `nix develop .#build` for the toolchain + "
            "Ghidra steps. Did dirs + configure + retail copy only.")
        return
    tc = [sys.executable, str(INIT / "toolchain.py")]
    if args.force:
        tc.append("--force")
    run(tc)                                          # wine prefix + registry (idempotent)
    run([sys.executable, str(INIT / "clangd.py")])   # clangd compile database
    _build_ghidra_db(reimport=args.reimport)         # Ghidra DB + functions.csv/symbols.csv (cold)
    if args.no_warmup:
        log("init complete (cold; --no-warmup): the Ghidra DB has only FID + "
            "auto-analysis functions. Run `build` then `ghidra-refresh` to warm it.")
        return
    if _ghidra_warm():
        log("init complete (Ghidra DB already warm).")
        return
    # Warm the Ghidra DB. A cold init carves only FID/auto-analysis functions -
    # the labeled RVAs in symbol_names.csv don't exist yet (build/labels hasn't
    # run). So: build (compile + labels -> symbol_names.csv), ghidra-refresh
    # (apply.py CREATES a function at each labeled RVA + re-exports the warm
    # functions.csv), build again (the delink + the "vs full engine" metric now
    # read the warm exports). This is the reproducible warm state.
    log("init warmup: build -> ghidra-refresh -> build ...")
    cmd_build(argparse.Namespace(ninja_args=[]))
    cmd_ghidra_refresh(argparse.Namespace())
    cmd_build(argparse.Namespace(ninja_args=[]))
    log("init complete (warmed).")


def cmd_status(args) -> None:
    if not REPORT.exists():
        die(f"no report at {REPORT}; run `gruntz build` first")
    summarize(json.loads(REPORT.read_text()))


def cmd_todo(args) -> None:
    """Obj symbols with no @address yet (the matching worklist) - a discovery aid.

    A symbol is in the matched set iff its source function carries an @address;
    this lists base-obj code symbols whose RVA is absent from the generated
    symbol_names.csv, i.e. candidates to locate + annotate.
    """
    if not GEN_NAMES.exists():
        die(f"no {GEN_NAMES}; run `gruntz build` or `labels` first")
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


def cmd_clean(args) -> None:
    """Nuke build/ + stray root build artifacts so `gruntz init && gruntz build`
    rebuilds from scratch. Touches nothing under src/, config/, or the AI tooling
    dirs (.claude/.codex/.agents). NOTE: this also removes build/ref, the wine
    prefix, and the Ghidra DB, so the next `gruntz init` is a HEAVY first run."""
    import shutil
    # Reap this prefix's wineserver BEFORE deleting build/wineprefix: a server
    # left running against a deleted prefix errors saving its registry ("could
    # not save registry branch ... No such file or directory") and lingers as a
    # stale server that flakes the next fresh build's first compiles.
    _kill_wine_session()
    targets = [REPO / "build", REPO / "build.ninja", REPO / ".ninja_lock",
               REPO / ".ninja_log", REPO / ".ninja_deps", *sorted(REPO.glob("*.obj"))]
    removed = 0
    for t in targets:
        if t.is_dir():
            shutil.rmtree(t); removed += 1; log(f"removed {t.relative_to(REPO)}/")
        elif t.exists():
            t.unlink(); removed += 1; log(f"removed {t.relative_to(REPO)}")
    log(f"clean: removed {removed} path(s). Next: `gruntz init` then `gruntz build`.")


def _clang() -> str:
    import os
    return os.environ.get("GRUNTZ_CLANG") or tool("clang")


def main() -> None:
    ap = argparse.ArgumentParser(prog="gruntz", description=__doc__,
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
    i.add_argument("--no-warmup", action="store_true",
                   help="skip the build->ghidra-refresh->build warmup (leave the Ghidra DB cold)")
    i.set_defaults(func=cmd_init)
    sub.add_parser("clangd", help="(re)generate the clangd compile DB (editor)"
                   ).set_defaults(func=cmd_clangd)
    sub.add_parser("status", help="print the last objdiff summary"
                   ).set_defaults(func=cmd_status)
    sub.add_parser("todo", help="obj symbols lacking an @address (worklist)"
                   ).set_defaults(func=cmd_todo)
    sub.add_parser("clean", help="nuke build/ + stray artifacts (HEAVY re-init after)"
                   ).set_defaults(func=cmd_clean)

    args = ap.parse_args()
    if getattr(args, "ninja_args", None) and args.ninja_args[:1] == ["--"]:
        args.ninja_args = args.ninja_args[1:]
    args.func(args)


if __name__ == "__main__":
    main()
