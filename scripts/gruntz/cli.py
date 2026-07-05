#!/usr/bin/env python3
"""gruntz.cli - the single entry point for the Gruntz matching pipeline.

Run inside the Nix dev shell (the `gruntz` wrapper, or `python -m gruntz`).
There is ONE shell now - `nix develop` (`.#build` is a kept alias):

    nix develop --command gruntz build
    nix develop --command gruntz status

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
  format [--check]
                clang-format src/ + include/ to the Rust-like house style
                (root .clang-format). Whitespace-only -> matching-neutral.
                --check is the CI gate (no writes; fail if unformatted).
  status        Print the last objdiff match summary (no rebuild).
  todo          List obj symbols that lack an @address (matching worklist).
  clean         Nuke build/ + stray root artifacts (build.ninja/*.obj/.ninja_*)
                for a from-scratch init + build. HEAVY re-init (wine + Ghidra DB).
  sema <cmd>    Semantic navigation (one entrypoint; `gruntz sema -h` lists all):
                xref/symbol/def/refs/hover/rename (clangd LSP), rva/class/match
                dossiers, disasm, strings. Thin wrappers over gruntz.analysis /
                gruntz.match - SEMANTIC questions go here, grep is lexical-only.
"""

import argparse
import json
import os
import re
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
GHIDRA_EXPORT_USER = GHIDRA_SCRIPTS / "export_user.py" # capture human edits -> config/user_annotations.json
INIT               = PKG / "init"             # environment setup
LINK               = BUILD / "link.py"        # phase-2 VC5 link wrapper (candidate EXE + map)
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
    """Resolve a tool on PATH - the `nix develop` shell provides them all."""
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
    # Cleanliness scoreboard - part of the report so agents see their cast /
    # placeholder / view deltas (vs the committed baseline) immediately alongside
    # the match %, and steer on their own change. See docs/cleanliness-metrics.md.
    try:
        from gruntz.match.cleanliness import count, report_lines, save_baseline
        rows = count()
        for line in report_lines(rows):
            print(f"  {line}")
        # Roll the baseline forward as part of the build: the delta printed above is
        # 'change since the last committed build', and the refreshed baseline is a
        # tracked build artifact (committed with the work, like the README score
        # block / match baseline). Keeps it from silently freezing.
        save_baseline(rows)
    except Exception as exc:  # never let the scoreboard break a build report
        print(f"  cleanliness: (unavailable: {exc})")
    # Vtable-health scoreboard (from the BINARY-PROVEN vtables, not text): the
    # hierarchy discrepancies that a topological override analysis finds - INHERIT
    # (derive the real base instead of re-listing its slots), REDECLARE (drop
    # redeclared inherited slots), OVERRIDE (unmarked overrides), MISSING (fewer
    # decls than slots) - plus the UNANCHORED src vtables not yet in the hierarchy
    # ('the proper ones not in the hierarchy'). Reducing these is what drives the
    # 'placeholder vtable slots' text metric to 0 AND removes placeholder view
    # classes. See `gruntz.analysis.vtable_hierarchy --audit / --coverage`.
    try:
        import re as _re
        def _vh(mode: str) -> str:
            return subprocess.run([sys.executable, "-m", "gruntz.analysis.vtable_hierarchy", mode],
                                  capture_output=True, text=True, cwd=str(REPO)).stdout
        aud, cov = _vh("--audit"), _vh("--coverage")
        def _n(txt: str, pat: str) -> str:
            m = _re.search(pat, txt)
            return m.group(1) if m else "?"
        inh, red = _n(aud, r"#\s*INHERIT\s*:\s*(\d+)"), _n(aud, r"#\s*REDECLARE\s*:\s*(\d+)")
        ovr, mis = _n(aud, r"#\s*OVERRIDE\s*:\s*(\d+)"), _n(aud, r"#\s*MISSING\s*:\s*(\d+)")
        anch, unanch = _n(cov, r"#\s*anchored\s*:\s*(\d+)"), _n(cov, r"UNANCHORED[^:]*:\s*(\d+)")
        print(f"  vtable health (-> 0; binary-proven): INHERIT {inh}  REDECLARE {red}  "
              f"OVERRIDE-unmarked {ovr}  MISSING {mis}  |  anchored {anch}, UNANCHORED {unanch}")
    except Exception as exc:  # never let the vtable probe break a build report
        print(f"  vtable health: (unavailable: {exc})")


# --- subcommands -----------------------------------------------------------
def cmd_build(args) -> None:
    # build.ninja regenerates itself via its `configure` generator edge whenever
    # config/units.toml or configure.py change, so don't re-run configure every
    # build - only bootstrap it when it doesn't exist yet (fresh tree / pre-init).
    if not (REPO / "build.ninja").exists():
        run([sys.executable, str(CONFIGURE)])
    _ensure_retail_copy()                             # cheap, idempotent (stable retail copy)
    if not GHIDRA_FUNCTIONS.exists():
        die(f"no Ghidra exports ({GHIDRA_FUNCTIONS.relative_to(REPO)}) - run `gruntz init` first.")
    ninja = tool("ninja")
    # Keep ONE persistent wineserver alive for the whole dev-shell session (the
    # dev shell's shellHook reaps it on interactive exit). The first build boots it
    # (~1.2s); later builds find it up, so `wine cl` pays no cold-start. We no
    # longer kill it after each build - that re-paid wineboot on every rebuild.
    _start_wine_session()                 # ensure the session is up (cheap if already running)

    # ninja builds the objs AND report.json in-graph (only what changed). Gate the
    # feedback tail on whether report.json actually moved: a no-op build refreshes
    # nothing, so there is nothing to summarize/check and `gruntz build` returns
    # near-instantly. Everything below is non-fatal reporting, not part of the build.
    before = REPORT.stat().st_mtime if REPORT.exists() else 0
    run([ninja, *args.ninja_args])        # incremental: rebuilds only what changed
    if (REPORT.stat().st_mtime if REPORT.exists() else 0) == before:
        log("up to date - nothing rebuilt.")
        return

    # Gate: the src/Stub @stub backlog is skipped by labels.py (engine_label_stubs),
    # so this is the only check on its address uniqueness + format.
    run([sys.executable, "-m", "gruntz.match.verify_stubs"])
    # Gate: no retail RVA may be double-claimed by a src RVA() reconstruction AND a
    # config/library_labels.csv carve-out row (game body vs library body must be
    # mutually exclusive). FATAL, no allowlist. See gruntz.match.verify_library_overlap.
    run([sys.executable, "-m", "gruntz.match.verify_library_overlap"])
    # One mangled fn name = one RVA (MSVC5 keeps one COMDAT copy per name, so a
    # duplicate across units contradicts the binary). FATAL. See
    # gruntz.match.verify_unique_names.
    run([sys.executable, "-m", "gruntz.match.verify_unique_names"])
    summarize(json.loads(REPORT.read_text()))

    # Non-fatal extras: per-function source fingerprints (so regression checks can
    # tell an edited function from a collateral drop), the README score block, and
    # regressions vs the committed best-% baseline. See gruntz.match.status.
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

    # Class-metadata invariants. Every vtable-bearing class should carry a
    # VTBL/manual/RTTI catalog entry, and every class a SIZE/SIZE_UNKNOWN - so a
    # class added without one is caught here, not later.
    # SIZE reached 0 (all classes annotated) -> now a FATAL gate: a class added
    # without SIZE/SIZE_UNKNOWN fails the build (class_sizes exits nonzero).
    run([sys.executable, "-m", "gruntz.match.class_sizes"])
    # VTBL still has a backlog (view-scaffolding + terminal manual stamps); REPORT
    # until it too reaches 0, then flip to a fatal run(...). See gruntz.match.class_vtables.
    r = subprocess.run([sys.executable, "-m", "gruntz.match.class_vtables"],
                       cwd=str(REPO), capture_output=True, text=True, env=_pkg_env())
    n = sum(1 for ln in (r.stdout + r.stderr).splitlines()
            if re.match(r"\s*\S+:\d+:", ln))
    if n:
        log(f"VTBL: {n} class(es) missing VTBL() "
            f"(python -m gruntz.match.class_vtables for the list)")


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
    comprehension headers under src/Stub/types/ (each wrapped as a .cpp TU). src/
    wins on overlapping names, so apply_ghidra.py's hardcoded fallback is unneeded
    for anything covered here.
    """
    clang = _clang()
    cmd = [sys.executable, str(BUILD / "ghidra_metadata_generate.py"), "--clang", clang]
    for t in args.tu:
        cmd += ["--tu", t]
    if (REPO / "src/Stub/types").is_dir():
        cmd += ["--header", "src/Stub/types"]     # comprehension layouts
    run(cmd)


def cmd_ghidra_refresh(args) -> None:
    """Part-2 loop: push generated names/structs/enums into the Ghidra DB, then
    re-export the functions.csv/symbols.csv the delink consumes.

      1. ghidra_metadata_generate -> build/gen/structs.json + enums.json (clang layouts of
         src/ + the converted src/Stub/types/ comprehension headers)
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
    is the `nix develop` python that carries the pyghidra package): it boots
    PyGhidra in-process, imports/analyzes GRUNTZ.EXE into build/ghidra-named/gruntz,
    then runs apply.py + export.py as GhidraScripts. `analyze=True` for the first
    import; False to re-run apply/export on the already-analyzed DB.
    """
    # build/gen/locals.json: CodeView locals from a /Z7 debug build of each
    # byte-exact function (apply.py injects them as named Ghidra stack vars).
    # Harvest before EVERY apply (cold init + refresh) so locals are always fresh;
    # needs functions.json (the rva<-name join) from a prior `gruntz build`.
    if (REPO / "build" / "gen" / "functions.json").exists():
        run([sys.executable, str(BUILD / "harvest_locals.py")])
    else:
        log("no functions.json yet - skipping locals harvest (run `gruntz build` first)")

    cmd = [sys.executable, str(GHIDRA_DRIVER), str(RETAIL_EXE),
           str(GHIDRA_PROJECT_DIR), GHIDRA_PROJECT,
           str(GHIDRA_APPLY), str(GHIDRA_EXPORT)]
    if not analyze:
        cmd.append("--no-analyze")
    run(cmd)


def cmd_capture(args) -> None:
    """Capture human edits from the Ghidra DB into git (round-trip back-direction).

    Reads build/ghidra-named (the already-analyzed/enriched program), extracts the
    human-made annotations - renamed functions, comments, named/typed stack locals
    (told apart from generated enrichment by SourceType.USER_DEFINED + the non-
    [LABEL] comment marker) - and writes the TRACKED config/user_annotations.json.
    apply.py re-applies those on every refresh, so they survive a clean rebuild.

    Run this AFTER editing in Ghidra (and saving the project), then commit the
    updated config/user_annotations.json.
    """
    if not GHIDRA_PROJECT_DIR.exists() or not any(GHIDRA_PROJECT_DIR.glob("*.rep")):
        die(f"no Ghidra project at {GHIDRA_PROJECT_DIR} - run `gruntz init` first.")
    run([sys.executable, str(GHIDRA_DRIVER), str(RETAIL_EXE),
         str(GHIDRA_PROJECT_DIR), GHIDRA_PROJECT,
         str(GHIDRA_EXPORT_USER), "--no-analyze"])
    log("capture done: human edits -> config/user_annotations.json "
        "(commit it to persist across clean rebuilds).")


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
        die("no build/exe/GRUNTZ.EXE - set $GRUNTZ_EXE (run inside `nix develop`).")
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


# Sources to format: src/ + include/ (not vendor/, not generated build/).
_FMT_ROOTS    = (REPO / "src", REPO / "include")
_FMT_SUFFIXES = (".cpp", ".h", ".cc", ".cxx", ".hpp", ".hh", ".c")


def _fmt_files() -> list:
    return sorted(p for root in _FMT_ROOTS if root.is_dir()
                  for p in root.rglob("*") if p.suffix in _FMT_SUFFIXES)


def cmd_format(args) -> None:
    """Format src/ + include/ to the house style. `--check` is a no-write CI gate."""
    cf = tool("clang-format")
    files = _fmt_files()
    if not files:
        die("no source files found under src/ or include/")
    if args.check:
        log(f"checking {len(files)} file(s) (clang-format --dry-run --Werror) ...")
        rc = subprocess.run([cf, "--style=file", "--dry-run", "--Werror", *map(str, files)],
                            cwd=str(REPO), env=_pkg_env()).returncode
        if rc != 0:
            die("some files are not formatted - run `gruntz format`")
        log(f"OK - all {len(files)} file(s) already formatted.")
    else:
        log(f"formatting {len(files)} file(s) in place (clang-format -i) ...")
        run([cf, "--style=file", "-i", *map(str, files)])
        log(f"done - {len(files)} file(s) formatted.")


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
    """One-time FULL local setup for this checkout. Run inside `nix develop`.

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
        log("MSVC_DIR unset - run inside `nix develop` for the toolchain + "
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


def cmd_link(args) -> None:
    """Phase 2: link the base objs into a candidate (non-runnable) GRUNTZ.EXE + map.

    Runs the genuine VC5 link.exe (5.10.7303) over build/objdiff/base/*.obj with
    /FORCE. The reconstruction is PARTIAL, so most externals are unresolved and the
    EXE does not run - the point is the .map, which exposes each function's
    link-assigned RVA and source object. Combined with the retail RVAs that gives
    the build-order model (intra-TU = source order, cross-TU = object order); see
    docs/link-order-investigation.md. Pass --order FILE to test a hypothesised
    link order, --analyze to print the layout report afterwards.
    """
    run([sys.executable, str(CONFIGURE)])
    ninja = tool("ninja")
    _start_wine_session()
    try:
        run([ninja, "base"])                       # ensure base objs are current
        cmd = [sys.executable, str(LINK)]
        if args.order:
            cmd += ["--order", args.order]
        if args.opt_ref:
            cmd += ["--opt-ref"]
        run(cmd)
    finally:
        _kill_wine_session()
    if args.analyze:
        run([sys.executable, "-m", "gruntz.analysis.link_order",
             "--map", str(REPO / "build" / "exe" / "GRUNTZ.candidate.map"),
             "--names", str(GEN_NAMES)])


def cmd_exe_diff(args) -> None:
    """Whole-EXE comparison: candidate GRUNTZ.EXE vs retail (layout + bytes).

    One level up from per-object objdiff: reads the candidate EXE + .map produced
    by `gruntz link` and diffs the whole image against retail - PE headers/section
    table, .text RVA layout fidelity (the RVA-reorder lever), and name-aligned
    linked-byte identity. Prints the proposed tracked EXE-match numbers. Run
    `gruntz link` first to (re)generate the candidate. See gruntz.analysis.exe_diff.
    """
    cmd = [sys.executable, "-m", "gruntz.analysis.exe_diff"]
    if args.json:
        cmd += ["--json"]
    run(cmd)


def cmd_lint(args) -> None:
    """On-demand clang-tidy DE-HACK finder (READ-ONLY worklist; never auto-fix).

    Thin alias for `python -m gruntz.analysis.tidy_audit`: runs the curated
    finder config (config/tidy-audit.yaml, deliberately OFF the editor's
    always-on path) over src/ and prints the categorized de-hack backlog -
    C-style/reinterpret casts, dead/unused decls, bugprone smells - with
    per-check totals + top files by cast count. Pass paths to scope it, --csv
    to dump file,line,check,message. Never runs with `-fix`.
    """
    run([sys.executable, "-m", "gruntz.analysis.tidy_audit", *args.rest])


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


# --- sema: semantic navigation group ---------------------------------------
# ONE discoverable entrypoint for the source/target-navigation tools a matcher or
# classifier reaches for - retail xref graph, clangd LSP (symbol/def/refs/hover/
# rename), disasm, strings, and the report/label dossiers. Each subcommand is a
# THIN delegation to an existing gruntz.analysis / gruntz.match module (all still
# runnable as `python -m gruntz.<...>`); nothing here re-implements analysis.
def _sema_tool(module: str, argv: list) -> int:
    """Stream a read-only navigation tool's output (package on PYTHONPATH; no
    `[gruntz] $` log noise on these interactive queries)."""
    return subprocess.run([sys.executable, "-m", module, *map(str, argv)],
                          cwd=str(REPO), env=_pkg_env()).returncode


def _sema_log(rc: int, secs: float) -> None:
    """One line per `gruntz sema` invocation (ALL subcommands, logged from the
    main dispatch); must NEVER break the tool. Metadata first, command after the
    `: ` (shell-quoted) so it copies straight out:
        [2026-07-04][19:55:01][0]: gruntz sema xref 0x00080850 --raw
    Usage-analysis feed: what agents actually run -> tool improvements."""
    del secs  # not logged (format: [date][time][rc]: <command>)
    try:
        import datetime
        import shlex
        now = datetime.datetime.now()
        cmd = shlex.join(["gruntz", *sys.argv[1:]])  # exactly what was invoked
        path = REPO / "build" / "gruntz_sema.log"
        path.parent.mkdir(parents=True, exist_ok=True)
        with open(path, "a") as f:
            f.write("[{}][{}][{}]: {}\n".format(
                now.date(), now.strftime("%H:%M:%S"), rc, cmd))
    except Exception:
        pass  # logging is best-effort by design


def _run_sema_logged(args) -> None:
    """Dispatch a sema subcommand with usage logging (rc captured through
    sys.exit; covers the delegating AND the inline-dossier subcommands)."""
    import time
    t0 = time.time()
    rc = 0
    try:
        args.func(args)
    except SystemExit as e:
        rc = e.code if isinstance(e.code, int) else (0 if e.code is None else 1)
        _sema_log(rc, time.time() - t0)
        raise
    _sema_log(rc, time.time() - t0)


def _point_argv(args) -> list:
    """`<file> <line> [<col>]` -> the clangd_query positional list."""
    return [args.file, args.line] + ([args.col] if args.col is not None else [])


def cmd_sema_xref(args) -> None:
    flags = (["--callees"] if args.callees else []) + (["--raw"] if args.raw else [])
    if args.tree:
        flags.append("--tree")
    if args.depth:
        flags += ["--depth", str(args.depth)]
    sys.exit(_sema_tool("gruntz.analysis.xref", flags + args.target))


def cmd_sema_symbol(args) -> None:
    sys.exit(_sema_tool("gruntz.analysis.clangd_query", ["symbol", args.query]))


def cmd_sema_point(args) -> None:                 # def / refs / hover share this
    sys.exit(_sema_tool("gruntz.analysis.clangd_query",
                        [args.sema, *_point_argv(args)]))


def cmd_sema_rename(args) -> None:
    argv = ["rename", *_point_argv(args), args.new_name]
    if args.dry_run:
        argv.append("--dry-run")
    sys.exit(_sema_tool("gruntz.analysis.clangd_query", argv))


def _disasm_capture(cmd: list) -> str:
    """Run a disasm producer, return stdout (stderr passes through)."""
    res = subprocess.run(cmd, cwd=str(REPO), env=_pkg_env(),
                         capture_output=True, text=True)
    sys.stderr.write(res.stderr)
    return res.stdout


def _disasm_target_text(rva: str) -> str:
    return _disasm_capture([sys.executable, "-m", "gruntz.analysis.dump_target", rva])


def _disasm_base_text(rva: str) -> str:
    """The CURRENT compiled asm: the fn's symbol disassembled out of its unit's
    base obj (what objdiff compares against retail)."""
    import csv as _csv
    try:
        n = int(rva, 16)
    except ValueError:
        die(f"'{rva}' is not a hex RVA (--base needs an RVA)")
    claim = _csv_find(GEN_NAMES, n)
    if not claim:
        die("no src claim at this RVA - base disasm needs a reconstructed fn "
            "(check `gruntz sema rva`)")
    obj = REPO / "build" / "objdiff" / "base" / (claim["unit"] + ".obj")
    if not obj.is_file():
        die(f"{obj.relative_to(REPO)} missing - run `gruntz build` first")
    out = _disasm_capture(["llvm-objdump", "-dr", "--x86-asm-syntax=intel",
                           f"--disassemble-symbols={claim['name']}", str(obj)])
    return f"{claim['name']}  [{claim['unit']}]\n" + out


_DISASM_ROW = None  # compiled lazily


def _disasm_lite(text: str) -> str:
    """Only the asm: drop addresses, byte columns, reloc blocks; keep title lines."""
    import re as _re
    global _DISASM_ROW
    if _DISASM_ROW is None:
        _DISASM_ROW = _re.compile(r"^\s*[0-9a-f]+:\s+((?:[0-9a-f]{2}\s)+)\s*(\S.*)$")
    keep = []
    for ln in text.splitlines():
        m = _DISASM_ROW.match(ln)
        if m:
            keep.append("    " + m.group(2).strip())
        elif " @ RVA " in ln or ln.rstrip().endswith(">:") or "  [" in ln[:1]:
            keep.append(ln)
        elif ln.startswith(("CState", "?")) and ln.rstrip().endswith("]"):
            keep.append(ln)
    return "\n".join(keep) + "\n"


def _disasm_norm(text: str) -> list:
    """Lite + case/whitespace-unify + mask absolute-address immediates for --diff.
    base (llvm-objdump, 'dword ptr') and target (dump_target, 'DWORD PTR') disagree
    on case and spacing - lowercase + collapse runs so only real diffs survive."""
    import re as _re
    # reloc-aware pre-pass (base side only): llvm-objdump -dr emits IMAGE_REL_ lines
    # after the owning insn - mask that insn's placeholder imm (often 0x0) as <addr>
    raw = text.splitlines()
    for i, ln in enumerate(raw):
        if "IMAGE_REL_I386_" not in ln:
            continue
        for j in range(i - 1, -1, -1):
            if "IMAGE_REL_I386_" in raw[j] or not _re.search(r"0x[0-9a-f]+", raw[j]):
                continue
            m = _re.search(r":\s+(?:[0-9a-f]{2} )+\s*([a-z]\w*)", raw[j])
            if m and _re.fullmatch(r"call|jmp|j[a-z]{1,2}|loop\w*", m.group(1)):
                break  # rel32 target - the <tgt> rule owns these
            # DIR32 on a memory disp32 (pure-absolute bracket) beats an imm32 guess
            raw[j], n = _re.subn(r"\[0x[0-9a-f]+\]", "[<addr>]", raw[j], count=1)
            if not n:
                raw[j] = _re.sub(r"0x[0-9a-f]+(?=[^x]*$)", "<addr>", raw[j], count=1)
            break
    text = "\n".join(raw)
    lines = []
    for ln in _disasm_lite(text).splitlines():
        if not ln.startswith("    "):
            continue  # instructions only in the diff body
        ln = _re.sub(r"[ \t]+", " ", ln.strip().lower())
        if _re.fullmatch(r"(?:[0-9a-f]{2} )*[0-9a-f]{2}", ln):
            continue  # byte-dump continuation of a long insn (dump_target wrap)
        ln = _re.sub(r"0x[0-9a-f]{6,8}\b", "<addr>", ln)
        ln = _re.sub(r" ?([,+*]) ?", r"\1", ln)   # 'ebp, ecx'/'esp + 0xc' -> tight
        ln = ln.replace("ds:", "")                 # default-segment prefix (dump_target)
        ln = _re.sub(r"\bptr (<addr>|0x[0-9a-f]+)(?![\w\]])", r"ptr [\1]", ln)  # bare -> bracketed
        ln = _re.sub(r"\[(0x[0-9a-f]+|<addr>)\]", "[<addr>]", ln)  # absolute mem ref
        # bare <addr> as a mov-class operand is a memory ref (dump_target drops brackets)
        ln = _re.sub(r"(?<=[ ,])<addr>(?=,|$)", "[<addr>]",
                     ln) if not ln.startswith(("push", "j", "call", "loop")) else ln
        ln = _re.sub(r"(dword|word|byte) ptr \[<addr>\]", "[<addr>]", ln)
        # direct jump/call targets: base prints rel+symbol, target prints absolute
        ln = _re.sub(r"^((?:j[a-z]{1,3}|call|loop\w*) )(0x[0-9a-f]+|<addr>)( <[^>]*>)?$",
                     r"\1<tgt>", ln)
        lines.append(ln)
    while lines and lines[-1] == "nop":
        lines.pop()  # COMDAT alignment padding (base only; absent in delinked target)
    return lines


def _flags_for(udef: dict) -> list:
    """Resolve a unit's flags-profile name to its cl flag list (units.toml [flags])."""
    with MANIFEST.open("rb") as f:
        profiles = tomllib.load(f).get("flags", {})
    return list(profiles.get(udef.get("flags", ""), []))


def _debug_obj_for(unit: str, source: str, flags: list):
    """build/debug/<unit>.obj compiled `<flags> /Z7` (codegen-neutral CodeView),
    cached on source mtime - same artifact harvest_locals.py builds. Path or None."""
    obj = REPO / "build" / "debug" / f"{unit}.obj"
    src = REPO / source
    if not src.is_file():
        return None
    if obj.is_file() and obj.stat().st_mtime >= src.stat().st_mtime:
        return obj  # fresh
    obj.parent.mkdir(parents=True, exist_ok=True)
    cmd = [sys.executable, str(BUILD / "cc_wrap.py"), "--out", str(obj),
           "--src", str(src), "--", *flags, "/Z7"]
    res = subprocess.run(cmd, cwd=str(REPO), env=_pkg_env(),
                         capture_output=True, text=True)
    if res.returncode != 0 or not obj.is_file():
        sys.stderr.write(f"[--rich] /Z7 compile of {unit} failed (wine/cl missing?); "
                         f"showing bare asm.\n  {res.stderr.strip()[-200:]}\n")
        return None
    return obj


def _disasm_rich(rva: str, lite: bool) -> str:
    """BASE disasm interleaved with the /Z7 CodeView source lines it came from:
    each mapped code offset prints its source statement (flush-left) above the
    instruction(s) it lowered to. Shows which statements survive /O2 and which
    got folded (a run of instructions under one line = merged; a source line
    that never appears = optimized away)."""
    try:
        n = int(rva, 16)
    except ValueError:
        die(f"'{rva}' is not a hex RVA (--rich needs an RVA)")
    claim = _csv_find(GEN_NAMES, n)
    if not claim:
        die("no src claim at this RVA - --rich needs a reconstructed fn "
            "(check `gruntz sema rva`)")
    unit, name = claim["unit"], claim["name"]
    udef = next((u for u in units() if u.get("unit") == unit), None)
    source = (udef or {}).get("source", "")
    # line map from the (fresh) /Z7 debug obj; degrade to bare disasm if absent.
    linemap, bf = {}, None
    if udef and source.startswith("src/"):
        dbg = _debug_obj_for(unit, source, _flags_for(udef))
        if dbg is not None:
            sys.path.insert(0, str(BUILD))
            import codeview  # noqa: E402  (build helper, package-local)
            info = codeview.parse_lines(str(dbg)).get(name)
            if info:
                linemap, bf = info["lines"], info["bf"]
    src_path = REPO / source
    src_lines = (src_path.read_text(errors="replace").splitlines()
                 if src_path.is_file() else None)

    def src_text(lineno: int) -> str:
        if src_lines and 1 <= lineno <= len(src_lines):
            return src_lines[lineno - 1].rstrip() or f"{source}:{lineno}"
        return f"{source}:{lineno}"

    try:
        size = int(claim.get("size", "0") or "0", 16)
    except ValueError:
        size = 0
    import re as _re
    row = _re.compile(r"^(\s*)([0-9a-f]+):\s+((?:[0-9a-f]{2}\s)+)\s*(\S.*)$")
    out = [f"{name}  [{unit}]"]
    if bf is None:
        out.append("(no /Z7 line info for this fn - bare asm)")
    current = None
    for ln in _disasm_base_text(rva).splitlines():
        m = row.match(ln)
        if not m:
            if "IMAGE_REL" in ln and not lite:
                out.append(ln)  # reloc annotation - attaches to the instr above
            continue  # else drop llvm-objdump boilerplate; keep the rich view clean
        off = int(m.group(2), 16)
        if size and off >= size:
            break  # trailing COMDAT padding (nops) past the function
        want = linemap.get(off, bf if current is None else current)
        if want is not None and want != current:
            out.append(src_text(want))
            current = want
        out.append("    " + m.group(4).strip() if lite else ln)
    return "\n".join(out) + "\n"


def cmd_sema_disasm(args) -> None:
    if getattr(args, "rich", False):
        if args.target:
            die("--rich is BASE-only (retail GRUNTZ.EXE carries no line info); "
                "drop --target")
        if args.diff:
            die("--rich does not combine with --diff (rich is a single-side view)")
        if not args.base:
            print("[--rich implies --base: source lines come from the /Z7 debug "
                  "build of your compiled obj]")
        print(_disasm_rich(args.rva, args.lite), end="")
        sys.exit(0)
    if args.diff:
        import difflib
        base = _disasm_norm(_disasm_base_text(args.rva))
        tgt = _disasm_norm(_disasm_target_text(args.rva))
        if base == tgt:
            print(f"identical asm ({len(tgt)} instruction(s); addresses/relocs masked)")
            sys.exit(0)
        print(f"[diff: BASE (compiled) vs TARGET (retail) @ {args.rva}; "
              "addresses masked as <addr>]")
        print("[caveat: base prints reloc-site immediates as their placeholder "
              "(e.g. 'push 0x0') where target shows the resolved '<addr>' - such "
              "lone pairs are usually NOT real diffs; objdiff is reloc-aware truth]")
        for ln in difflib.unified_diff(base, tgt, "base", "target", lineterm=""):
            print(ln)
        sys.exit(1)
    if args.base:
        print("[disasm source: BASE - your compiled obj (build/objdiff/base)]")
        text = _disasm_base_text(args.rva)
    else:
        print("[disasm source: TARGET - retail GRUNTZ.EXE (delinked bytes + relocs)]")
        text = _disasm_target_text(args.rva)
    print(_disasm_lite(text) if args.lite else text, end="")
    sys.exit(0)


def cmd_sema_strings(args) -> None:
    if args.find:
        sys.exit(_sema_tool("gruntz.analysis.string_xref", ["--find", args.find]))
    if not args.rva:
        die("sema strings: give an <rva> or --find <text>")
    sys.exit(_sema_tool("gruntz.analysis.string_xref", ["--rva", args.rva]))


def _sema_class_of_fn(query: str) -> int:
    """FUNCTION -> vtable topology: which class(es) hold this fn in which slot,
    tagged new/override/inherited with the origin class; then the owner's table."""
    import csv as _csv
    import tempfile
    rva = None
    try:
        rva = int(query, 16)
    except ValueError:
        pass
    with tempfile.NamedTemporaryFile(suffix=".csv", delete=False) as tf:
        tmp = tf.name
    rc = subprocess.run([sys.executable, "-m", "gruntz.analysis.vtable_hierarchy",
                         "--csv", tmp], cwd=str(REPO), env=_pkg_env(),
                        capture_output=True, text=True).returncode
    if rc:
        die("vtable_hierarchy --csv failed (run `gruntz init` first?)")
    hits = []
    for r in _csv.DictReader(open(tmp)):
        try:
            row_rva = int(r["fn_rva"], 16)
        except ValueError:
            continue
        if (rva is not None and row_rva == rva) or \
           (rva is None and query in (r.get("fn_name") or "")):
            hits.append(r)
    if not hits:
        print(f"no vtable slot holds '{query}' (not a virtual, or not in the RTTI/VTBL graph)")
        return 1
    print(f"vtable slots holding {query}:")
    owners = []
    for r in hits:
        print(f"  {r['class']}  slot[{r['slot_index']}]  {r['disposition']:<9} "
              f"origin={r['origin_class']}  {r['fn_name']}")
        if r["disposition"] in ("new", "override") and r["class"] not in owners:
            owners.append(r["class"])
    for owner in owners[:2]:  # the defining class(es), not every inheritor
        print()
        sys.stdout.flush()  # our lines must precede the subprocess's
        subprocess.run([sys.executable, "-m", "gruntz.analysis.vtable_hierarchy",
                        "--class", owner], cwd=str(REPO), env=_pkg_env())
    return 0


def cmd_sema_class(args) -> None:
    argv = ["--class", args.name]
    if args.tree:
        argv.append("--tree")
    # hex RVA or mangled/Fn-name -> function->slot topology lookup instead
    looks_fn = args.name.lower().startswith("0x") or "@" in args.name
    if looks_fn:
        sys.exit(_sema_class_of_fn(args.name))
    sys.exit(_sema_tool("gruntz.analysis.vtable_hierarchy", argv))


def cmd_sema_match(args) -> None:
    t = args.target
    if t in {u["unit"] for u in units()}:
        sys.exit(_sema_tool("gruntz.match.status", ["status", "--unit", t]))
    grep = t
    if t.lower().startswith("0x") and GEN_NAMES.exists():
        import csv
        want = int(t, 16)
        grep = None
        for r in csv.reader(GEN_NAMES.open()):
            if r and r[0].lower().startswith("0x"):
                try:
                    if int(r[0], 16) == want:
                        grep = r[1]
                        break
                except ValueError:
                    pass
        if grep is None:
            die(f"no src function claims RVA {t} (nothing to score) - try a unit name")
    sys.exit(_sema_tool("gruntz.match.status", ["status", "--grep", grep]))


def _csv_find(path: Path, rva: int, key: str = "rva"):
    """First CSV row whose `key` column parses to `rva` (hex), or None."""
    import csv
    if not path.is_file():
        return None
    for r in csv.DictReader(path.open()):
        try:
            if int(r[key], 16) == rva:
                return r
        except (ValueError, KeyError):
            pass
    return None


def cmd_sema_rva(args) -> None:
    """One-shot address dossier: joins symbol_names.csv, library_labels.csv,
    Ghidra functions.csv and objdiff report.json - pure lookups, no analysis."""
    try:
        rva = int(args.addr, 16)
    except ValueError:
        die(f"'{args.addr}' is not a hex RVA (e.g. 0x00080850)")
    print(f"RVA 0x{rva:08x}")

    claim = _csv_find(GEN_NAMES, rva)
    if claim:
        print(f"  src claim : {claim['name']}  [{claim['unit']}] "
              f"({claim.get('kind', '?')})")
    else:
        print("  src claim : (none - not reconstructed under src/)")

    librow = _csv_find(REPO / "config" / "library_labels.csv", rva)
    if librow:
        print(f"  library   : {librow['name']}  {librow['lib']} / "
              f"{librow['confidence']} / {librow['source']}  "
              f"(carve-out: excluded from the match %)")

    grow = _csv_find(GHIDRA_FUNCTIONS, rva, key="entry_rva")
    if grow:
        print(f"  ghidra    : {grow['name']}  size {grow['byte_size']} B")
    else:
        print("  ghidra    : (no function start at this RVA in the export)")

    if claim and REPORT.is_file():
        rep = json.loads(REPORT.read_text())
        pct = None
        for u in rep.get("units", []):
            if claim.get("unit") and u.get("name") != claim["unit"]:
                continue
            for fn in u.get("functions") or []:
                if fn.get("name") == claim["name"]:
                    pct = fn.get("fuzzy_match_percent")
                    break
            if pct is not None:
                break
        if pct is not None:
            print(f"  match     : {pct:.2f}% fuzzy"
                  + ("  (EXACT)" if pct >= 100.0 else ""))


def _add_sema(sub) -> None:
    """The `sema` semantic-navigation group: one self-teaching help screen, each
    subcommand a thin delegation (see the cmd_sema_* funcs)."""
    sema = sub.add_parser(
        "sema", formatter_class=argparse.RawDescriptionHelpFormatter,
        help="semantic navigation: xref / clangd LSP / disasm / dossiers",
        description="gruntz sema <cmd> - source & target navigation for matchers "
                    "and classifiers.\nThin wrappers over the analysis tools (each "
                    "also runnable as `python -m gruntz.<...>`).\nSEMANTIC questions "
                    "go here; grep is lexical-only.",
        epilog="examples (use when ...):\n"
        "  gruntz sema xref 0x00080850           who calls this fn (attribution)\n"
        "  gruntz sema xref --callees CFoo::Bar   its own call targets\n"
        "  gruntz sema symbol CGruntzApp          fuzzy workspace-symbol search\n"
        "  gruntz sema def   src/X.cpp 42         jump to the definition\n"
        "  gruntz sema refs  include/X.h 30       every ref (USR-exact; no grep collisions)\n"
        "  gruntz sema hover src/X.cpp 42         type/decl at point\n"
        "  gruntz sema rename include/X.h 40 m_new --dry-run   tree-wide rename preview\n"
        "  gruntz sema rva   0x00080850           address dossier (claim/lib/ghidra/%)\n"
        "  gruntz sema class CImage               vtable slots + hierarchy tags\n"
        "  gruntz sema match cplay                per-fn % of a unit (or an RVA)\n"
        "  gruntz sema disasm 0x00080850          retail disasm + relocs\n"
        "  gruntz sema strings 0x00080850         the string set of a fn\n"
        "  gruntz sema strings --find WORLDZ      reverse literal lookup\n")
    ss = sema.add_subparsers(dest="sema", required=True)

    sx = ss.add_parser("xref", help="retail caller/callee graph (attribution)")
    sx.add_argument("target", nargs="+", help="RVA(s) (0x..) or symbol name(s)")
    sx.add_argument("--callees", action="store_true", help="forward: its call targets")
    sx.add_argument("--raw", action="store_true", help="every call site (no dedup)")
    sx.add_argument("--tree", action="store_true",
                    help="FULL caller ancestry tree - expands callers-of-callers to the "
                         "roots, chasing ILT jmp-thunks automatically (attribution in one shot)")
    sx.add_argument("--depth", type=int, default=0, metavar="N",
                    help="cap --tree expansion at N levels (default 0 = unlimited)")
    sx.set_defaults(func=cmd_sema_xref)

    sy = ss.add_parser("symbol", help="fuzzy workspace-symbol search (clangd)")
    sy.add_argument("query")
    sy.set_defaults(func=cmd_sema_symbol)

    for nm, hlp in (("def", "definition at point"),
                    ("refs", "references at point (USR-exact)"),
                    ("hover", "type/decl at point")):
        p = ss.add_parser(nm, help=hlp + " (clangd)")
        p.add_argument("file")
        p.add_argument("line", type=int)
        p.add_argument("col", type=int, nargs="?")
        p.set_defaults(func=cmd_sema_point)

    sr = ss.add_parser("rename", help="tree-wide symbol rename (clangd; USR-exact)")
    sr.add_argument("file")
    sr.add_argument("line", type=int)
    sr.add_argument("col", type=int, nargs="?")  # optional; new_name is the required tail
    sr.add_argument("new_name")
    sr.add_argument("--dry-run", action="store_true",
                    help="preview the edit set (file:line: old -> new); write nothing")
    sr.set_defaults(func=cmd_sema_rename)

    srv = ss.add_parser("rva", help="address dossier (src claim / lib / ghidra / match)")
    srv.add_argument("addr", help="hex RVA, e.g. 0x00080850")
    srv.set_defaults(func=cmd_sema_rva)

    sc = ss.add_parser("class",
                       help="vtable slots + hierarchy tags for a class; give a fn "
                            "RVA/mangled name instead to find its owning slot(s); "
                            "--tree adds the inheritance topology")
    sc.add_argument("name", help="class name, or fn RVA (0x..) / mangled name (has @)")
    sc.add_argument("--tree", action="store_true",
                    help="also print the binary-proven inheritance forest (topological)")
    sc.set_defaults(func=cmd_sema_class)

    sm = ss.add_parser("match", help="per-function/unit match summary (report.json)")
    sm.add_argument("target", help="a unit name, or an 0x RVA a src fn claims")
    sm.set_defaults(func=cmd_sema_match)

    sd = ss.add_parser("disasm",
                       help="disasm: TARGET (retail, default) / --base (compiled) / "
                            "--rich (base + /Z7 source lines) / --diff (base vs "
                            "target) / --lite (asm only)")
    sd.add_argument("rva", help="RVA (0x..) or symbol name")
    sdside = sd.add_mutually_exclusive_group()
    sdside.add_argument("--target", action="store_true",
                        help="retail GRUNTZ.EXE side (the default; explicit for clarity)")
    sdside.add_argument("--base", action="store_true",
                        help="disassemble YOUR compiled fn from its base obj instead of retail")
    sd.add_argument("--rich", action="store_true",
                    help="BASE disasm interleaved with the /Z7 CodeView source lines "
                         "each instruction came from (implies --base; composes with "
                         "--lite; rejects --target/--diff)")
    sd.add_argument("--lite", action="store_true",
                    help="asm only - no addresses, no byte columns, no reloc blocks")
    sd.add_argument("--diff", action="store_true",
                    help="unified diff of base-vs-target asm (addresses masked; rc=1 if differs)")
    sd.set_defaults(func=cmd_sema_disasm)

    st = ss.add_parser("strings", help="per-fn string set / --find reverse lookup")
    st.add_argument("rva", nargs="?", help="RVA (0x..) whose string set to print")
    st.add_argument("--find", metavar="TEXT", help="reverse: fns referencing TEXT")
    st.set_defaults(func=cmd_sema_strings)


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
    sub.add_parser("capture", help="capture human Ghidra edits -> config/user_annotations.json"
                   ).set_defaults(func=cmd_capture)
    i = sub.add_parser("init", help="one-time FULL local setup (dirs/configure/EXE/wine/clangd/Ghidra DB)")
    i.add_argument("--force", action="store_true", help="re-init the wine prefix")
    i.add_argument("--reimport", action="store_true", help="rebuild the Ghidra DB from scratch")
    i.add_argument("--no-warmup", action="store_true",
                   help="skip the build->ghidra-refresh->build warmup (leave the Ghidra DB cold)")
    i.set_defaults(func=cmd_init)
    sub.add_parser("clangd", help="(re)generate the clangd compile DB (editor)"
                   ).set_defaults(func=cmd_clangd)
    fmt = sub.add_parser("format", help="clang-format src/ + include/ to the Rust-like style")
    fmt.add_argument("--check", action="store_true",
                     help="CI gate: don't write, exit non-zero if anything is unformatted")
    fmt.set_defaults(func=cmd_format)
    sub.add_parser("status", help="print the last objdiff summary"
                   ).set_defaults(func=cmd_status)
    lk = sub.add_parser("link", help="phase 2: link base objs -> candidate EXE + map "
                        "(non-runnable; for layout/link-order study)")
    lk.add_argument("--order", help="file listing obj stems in link order to test")
    lk.add_argument("--opt-ref", action="store_true",
                    help="let the linker strip/fold unreferenced COMDATs (default keeps all)")
    lk.add_argument("--analyze", action="store_true",
                    help="print the layout/link-order report after linking")
    lk.set_defaults(func=cmd_link)
    xd = sub.add_parser("exe-diff", help="whole-EXE diff: candidate vs retail "
                        "(layout + linked bytes; needs `gruntz link` first)")
    xd.add_argument("--json", action="store_true", help="emit the JSON summary only")
    xd.set_defaults(func=cmd_exe_diff)
    sub.add_parser("todo", help="obj symbols lacking an @address (worklist)"
                   ).set_defaults(func=cmd_todo)
    ln = sub.add_parser("lint", help="on-demand clang-tidy de-hack finder "
                        "(read-only worklist; casts/dead/unused; never auto-fix)")
    ln.add_argument("rest", nargs=argparse.REMAINDER,
                    help="args passed through to gruntz.analysis.tidy_audit "
                         "(paths, --csv FILE, --top N, -j N)")
    ln.set_defaults(func=cmd_lint)
    sub.add_parser("clean", help="nuke build/ + stray artifacts (HEAVY re-init after)"
                   ).set_defaults(func=cmd_clean)
    _add_sema(sub)   # sema: semantic-navigation group (xref/LSP/disasm/dossiers)

    args = ap.parse_args()
    if getattr(args, "ninja_args", None) and args.ninja_args[:1] == ["--"]:
        args.ninja_args = args.ninja_args[1:]
    if sys.argv[1:2] == ["sema"]:
        _run_sema_logged(args)  # usage log -> build/gruntz_sema.log
    else:
        args.func(args)


if __name__ == "__main__":
    main()
