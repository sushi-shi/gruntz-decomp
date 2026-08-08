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
            symbol_names.csv + config/retail/functions.tsv + GRUNTZ.EXE
                --delink(synth_pdb -> vostok-delinker)--> target/<unit>.c.obj
            base vs target --objdiff--> report.json

        symbol_names.csv is REGENERATED every build from the src `@address`
        annotations, so renamed symbols re-mangle and removed ones drop
        automatically - no stale rows.

  labels        Just (re)generate build/gen/symbol_names.csv from src @address.
  structs       Just (re)generate build/gen/structs.json + enums.json (clang).
  ghidra-refresh  Populate an optional disposable Ghidra viewer database from
                  tracked retail boundaries and generated source data.
  init          One-time FULL local setup: dirs + configure + retail EXE copy +
                wine prefix + clangd compdb. Ghidra is not part of setup/build.
  clangd        (Re)generate the clangd compile DB (editor; after adding a unit).
  format [--check]
                clang-format src/ + include/ to the Rust-like house style
                (root .clang-format). Whitespace-only -> matching-neutral.
                --check is the CI gate (no writes; fail if unformatted).
  status        Print the last objdiff match summary (no rebuild).
  match-queue   Generate weighted residual and middle-to-worst campaign queues.
  todo          List obj symbols that lack an @address (matching worklist).
  clean         Nuke build/ + stray root artifacts (build.ninja/*.obj/.ninja_*)
                for a from-scratch init + build.
  sema <cmd>    Semantic navigation (one entrypoint; `gruntz sema -h` lists all):
                xref, refs/hover/rename (clangd LSP), rva/class/match dossiers,
                disasm, strings, vtable, map (.text layout), `-` (batch). Thin
                wrappers over gruntz/sema (in-process over gruntz/core) -
                SEMANTIC questions go here, grep is lexical-only.
  audit <tool>  One-shot campaign audits - dispatches to gruntz/audit/<tool>.py
                (no arg lists the tools).
  permute fn|sweep|variants
                The source-permutation climbers (gruntz/permute/).
"""

import argparse
import datetime
import json
import os
import re
import shutil
import subprocess
import sys
import time
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[2])
SCRIPTS            = REPO / "scripts"
PKG                = SCRIPTS / "gruntz"       # the pipeline package (grouped by area)
BUILD              = PKG / "build"            # labels, structs, synth_pdb, delink, ...
GHIDRA             = PKG / "ghidra"           # the PyGhidra driver (a normal runnable module)
GHIDRA_DRIVER      = GHIDRA / "ghidra_metadata_apply.py" # optional PyGhidra viewer importer
# GhidraScripts run INSIDE Ghidra (PyGhidra injects currentProgram/monitor/state);
# they are NOT importable and NOT `python -m`-runnable — the driver passes them by
# PATH. They live in ghidra/scripts/ (no __init__.py) so the boundary is explicit.
GHIDRA_SCRIPTS     = GHIDRA / "scripts"       # GhidraScripts: path-only, never imported/-m'd
GHIDRA_APPLY       = GHIDRA_SCRIPTS / "apply.py"   # enrichment GhidraScript (run under PyGhidra)
INIT               = PKG / "init"             # environment setup
LINK               = BUILD / "link.py"        # phase-2 VC5 link wrapper (candidate EXE + map)
MANIFEST           = REPO / "config" / "units.toml"
OBJDIFF_DIR        = REPO / "build" / "objdiff"
TARGET_DIR         = OBJDIFF_DIR / "target"
REPORT             = OBJDIFF_DIR / "report.json"
BUILD_TIMES        = REPO / "build" / "gen" / "build_times.tsv"  # per-invocation build wall-clock log (gitignored, per-worktree)
GEN_NAMES          = REPO / "build" / "gen" / "symbol_names.csv"
def _ghidra_project_dir() -> Path:
    """Where the Ghidra DB lives - normally build/ghidra-named, but NEVER under a dot-path.

    Ghidra's `ProjectLocator` hard-rejects any path element starting with `.`
    ("Path element starting with '.' is not permitted"), so a checkout under e.g.
    `.claude/worktrees/matcher-1` cannot host the DB at all - `init` used to die there
    AFTER doing the wine/clangd/analyze work, leaving `gruntz clean` unrecoverable in a
    pool worktree. The constraint is on the DB's OWN path, not the repo, so when the repo
    sits under a dot-path we relocate the DB to a dot-free dir keyed by the repo path
    (stable across runs, unique per worktree, and outside the tree so `clean` can't eat it).
    """
    d = REPO / "build" / "ghidra-named"
    if not any(part.startswith(".") for part in d.parts):
        return d
    import hashlib
    # NOT tempfile.gettempdir(): under `nix develop` that is the shell's own per-invocation
    # TMPDIR (/tmp/nix-shell.XXXX), so the DB would be rebuilt every shell and could vanish
    # mid-use. A fixed /tmp dir keyed by the repo path is stable across shells and unique
    # per worktree.
    tag = hashlib.sha1(str(REPO).encode()).hexdigest()[:12]
    return Path("/tmp") / f"gruntz-ghidra-{tag}" / "ghidra-named"


GHIDRA_PROJECT_DIR = _ghidra_project_dir()
GHIDRA_PROJECT     = "gruntz"                                          # project name (gruntz.{gpr,rep})
RETAIL_FUNCTIONS   = REPO / "config" / "retail" / "functions.tsv"
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
    from gruntz.core import manifest
    return manifest.units()


def _record_build_time(mode: str, total_s: float, ninja_s: float, gates_s: float) -> None:
    """Print + append a per-invocation build-timing row (BUILD_TIMES), so worker build
    cost can be analysed. `mode` is noop/fast/normal/full. build/ is gitignored + per-worktree,
    so each worktree accumulates its own log; the `worktree` column keeps rows
    distinguishable if pooled. Best-effort: a logging failure never fails the build."""
    parts = [f"ninja {ninja_s:.1f}s"] + ([f"gates {gates_s:.1f}s"] if gates_s else [])
    log(f"build timing: total {total_s:.1f}s ({', '.join(parts)}) [{mode}]")
    try:
        BUILD_TIMES.parent.mkdir(parents=True, exist_ok=True)
        header = not BUILD_TIMES.exists()
        with BUILD_TIMES.open("a") as f:
            if header:
                f.write("timestamp\tworktree\tmode\tninja_s\tgates_s\ttotal_s\n")
            ts = datetime.datetime.now().isoformat(timespec="seconds")
            f.write(f"{ts}\t{REPO.name}\t{mode}\t{ninja_s:.1f}\t{gates_s:.1f}\t{total_s:.1f}\n")
    except OSError:
        pass


# --- summary ---------------------------------------------------------------
def _i(v) -> int:
    return int(v) if v is not None else 0


def _pct(n: int, d: int) -> float:
    return 100.0 * n / d if d else 0.0


def summarize(report: dict, full: bool = True, table: bool = False,
              write: bool = False, vtable_health: bool = True,
              semantic_cleanliness: bool = False) -> None:
    """The report tail. `write=True` (the BUILD only) rolls the cleanliness
    baselines and enforces the hard ratchet gate; `gruntz status`/
    `report` are pure READS - same scoreboard vs the committed baselines,
    ratchet violations print as warnings, nothing is written."""
    m = report.get("measures", {})
    named = {u["unit"] for u in units() if (TARGET_DIR / f"{u['unit']}.c.obj").exists()}
    print()
    # Per-unit rollup (370 rows): ON DEMAND only (`gruntz status`/`report`), NEVER in the
    # build tail. A matcher works ONE unit and reads its own number; the full table is a
    # human progress scan, not agent-facing - printing it every build is pure noise.
    if table:
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
    # DOCTRINE reminder is NOT printed here every build - it fires only on a %-drop,
    # from the regressions reporter (gruntz.match.status check), where a matcher is
    # actually tempted to revert to chase %. That is the teachable moment.
    if not full:   # --fast: just the objdiff %, skip cleanliness/vtable probes
        return
    # Cleanliness scoreboard - part of the report so agents see their cast /
    # placeholder / view deltas (vs the committed baseline) immediately alongside
    # the match %, and steer on their own change. See docs/cleanliness-metrics.md.
    try:
        from gruntz.cleanliness.board import (count, report_lines, save_baseline,
                                              merge_baseline_downonly, load_baseline, _RATCHET)
        rows = count(include_semantic=semantic_cleanliness)
        for line in report_lines(rows):
            print(f"  {line}")
        # BUILD ONLY: roll the baseline forward, DOWN-ONLY for the ratcheted
        # view/cast metrics (a regression is held at the floor, never blessed
        # away); other tracked metrics roll forward. Blessing a LOWER floor stays
        # a deliberate act (`cleanliness --update`). `gruntz status` never writes.
        if write:
            save_baseline(merge_baseline_downonly(rows),
                          include_semantic=semantic_cleanliness)
        # RATCHET check. The cast / fake-view / fake-vtable metrics may only go
        # DOWN. A rise above the committed floor means a cast/view/virtual was
        # REINTRODUCED: the BUILD dies here (the gate); status just warns.
        _floor = load_baseline()
        _viol = [(lbl, _floor[lbl], n) for lbl, n in rows
                 if lbl in _RATCHET and lbl in _floor and n > _floor[lbl]]
        if _viol:
            for lbl, fl, n in _viol:
                print(f"  RATCHET VIOLATED: {lbl}  {fl} -> {n}  (+{n - fl})", file=sys.stderr)
            if write:
                die("cleanliness ratchet violated: a protected source metric rose above "
                    "its floor (see above). Fix the source regression: dissolve the view, "
                    "type the member, make the virtual real, or move a .cpp declaration "
                    "to its owner header. Never bless it up.")
    except Exception as exc:  # never let the SCOREBOARD break a build report (die() is not caught)
        print(f"  cleanliness: (unavailable: {exc})")
    # Vtable-health scoreboard (from the BINARY-PROVEN vtables, not text): the
    # hierarchy discrepancies that a topological override analysis finds - INHERIT
    # (derive the real base instead of re-listing its slots), REDECLARE (drop
    # redeclared inherited slots), OVERRIDE (unmarked overrides), MISSING (fewer
    # decls than slots) - plus the UNANCHORED src vtables not yet in the hierarchy
    # ('the proper ones not in the hierarchy'). Reducing these is what drives the
    # 'placeholder vtable slots' text metric to 0 AND removes placeholder view
    # classes. See `gruntz.core.vtable_hierarchy --audit / --coverage`.
    if vtable_health:
        try:
            import re as _re

            def _vh(mode: str) -> str:
                return subprocess.run(
                    [sys.executable, "-m", "gruntz.core.vtable_hierarchy", mode],
                    capture_output=True, text=True, cwd=str(REPO), env=_pkg_env()
                ).stdout

            aud, cov = _vh("--audit"), _vh("--coverage")

            def _n(txt: str, pat: str) -> str:
                m = _re.search(pat, txt)
                return m.group(1) if m else "?"

            inh = _n(aud, r"#\s*INHERIT\s*:\s*(\d+)")
            red = _n(aud, r"#\s*REDECLARE\s*:\s*(\d+)")
            ovr = _n(aud, r"#\s*OVERRIDE\s*:\s*(\d+)")
            mis = _n(aud, r"#\s*MISSING\s*:\s*(\d+)")
            anch = _n(cov, r"#\s*anchored\s*:\s*(\d+)")
            unanch = _n(cov, r"UNANCHORED[^:]*:\s*(\d+)")
            print(f"  vtable health (-> 0; binary-proven): INHERIT {inh}  "
                  f"REDECLARE {red}  OVERRIDE-unmarked {ovr}  MISSING {mis}  |  "
                  f"anchored {anch}, UNANCHORED {unanch}")
        except Exception as exc:  # never let the vtable probe break a build report
            print(f"  vtable health: (unavailable: {exc})")


# --- subcommands -----------------------------------------------------------
def _ensure_wineprefix_configured() -> None:
    """Refuse to compile against a prefix whose Wine registry was never written.

    `gruntz init` computes PATH/INCLUDE/LIB with winepath BEFORE writing any of
    them, so a failure in that window (a flaky cold wineserver has done it) leaves
    a prefix with NO INCLUDE. The dev shell deliberately does NOT abort on init
    failure - you get a shell to fix from - so nothing otherwise stops you building
    against it, and the first symptom is `IID_IDirectPlay4A: undeclared identifier`
    in NetMgr.cpp: VC5's DirectX 3-era DPLAY.H shadowing the DX6 one, three layers
    from the cause. init stamps the prefix once it has VERIFIED the registry, so
    this costs a stat and turns that into one clear sentence."""
    prefix = REPO / "build" / "wineprefix"
    if prefix.is_dir() and not (prefix / ".gruntz-configured").is_file():
        die("the Wine prefix exists but its registry was never verified - "
            "`gruntz init` did not finish (a cold wineserver can fail the first "
            "winepath). Re-run `gruntz init`; without it cl uses VC5's DirectX "
            "3-era headers and NetMgr.cpp fails on IID_IDirectPlay4A.")


def cmd_build(args) -> None:
    build_start = time.monotonic()   # wall-clock start (see _record_build_time)
    # Always re-configure. build.ninja's `configure` generator edge covers the
    # ordinary cases (units.toml, configure.py, any scanned source/header moved),
    # but it CANNOT cover a DELETED or renamed header: the stale manifest still
    # lists that header as an input, and ninja refuses to load a manifest whose
    # own inputs are missing -
    #     ninja: error: rebuilding 'build.ninja': 'include/X.h', needed by
    #                   'build.ninja', missing and no known rule to make it
    # - which wedges the build until someone runs configure.py by hand. Doing it
    # unconditionally costs ~0.3 s (the include scan is memoized; it was 13 s
    # before) and makes a rename a non-event.
    run([sys.executable, str(CONFIGURE)])
    _ensure_wineprefix_configured()                   # free (a stat); see the fn
    _ensure_retail_copy()                             # cheap, idempotent (stable retail copy)
    _ensure_compdb_fresh()                            # cheap, idempotent (unit list moved?)
    if not RETAIL_FUNCTIONS.exists():
        die(f"tracked retail inventory missing: {RETAIL_FUNCTIONS.relative_to(REPO)}")
    ninja = tool("ninja")
    # Keep ONE persistent wineserver alive for the whole dev-shell session (the
    # dev shell's shellHook reaps it on interactive exit). The first build boots it
    # (~1.2s); later builds find it up, so `wine cl` pays no cold-start. We no
    # longer kill it after each build - that re-paid wineboot on every rebuild.
    _start_wine_session()                 # ensure the session is up (cheap if already running)

    if getattr(args, "force_delink", False):
        (OBJDIFF_DIR / ".delink.stamp").unlink(missing_ok=True)
        log("force-delink: removed delink stamp -> delink will re-run this build.")

    # ninja builds the objs AND report.json in-graph (only what changed).
    #
    # This used to RETURN here when report.json had not moved ("a no-op build refreshes
    # nothing, so there is nothing to summarize/check"), which made `gruntz build` exit 0
    # having run ZERO gates. That reasoning holds for the objdiff summary and for nothing
    # else: the FATAL gates below read SOURCE, not report.json. verify_stubs, class_sizes,
    # vtable_bans, class_vtables, vtable_virtuality and the catalog asserts all
    # scan src/ + include/ + config/ - so the ONLY edits they exist to catch (an @stub tag,
    # a SIZE(), a vtable catalog binding, a header layout) are exactly the edits that need not
    # produce a byte of new codegen. The check was therefore skipped precisely when it was
    # the only thing that could fail, and "the build passed" could mean "nothing was
    # checked". Measured: an @stub metadata fix + a header edit -> ninja no-op -> exit 0,
    # gates never ran.
    #
    # So: a no-op is now reported, not returned on. The latency-sensitive matcher loop is
    # `--fast`, which deliberately runs no source gates and says so.
    before = REPORT.stat().st_mtime if REPORT.exists() else 0
    ninja_t0 = time.monotonic()
    run([ninja, *args.ninja_args])        # incremental: rebuilds only what changed
    ninja_s = time.monotonic() - ninja_t0
    noop = (REPORT.stat().st_mtime if REPORT.exists() else 0) == before
    if noop:
        log("up to date - nothing rebuilt (the selected tier determines which source "
            "checks run).")
    if not REPORT.exists():
        die("no objdiff report - the build did not produce build/objdiff/report.json.")

    # Gate tiers (measured wall-times in docs/build-system.md):
    #   fast   build + objdiff summary only - the matcher inner loop.
    #   normal quick structural-integrity checks - the DEFAULT, per commit.
    #   full   normal + exhaustive whole-tree discovery audits and durable worklists -
    #          periodic/daily, and allowed to be slow.
    tier = getattr(args, "tier", None) or "normal"
    if getattr(args, "fast", False):
        tier = "fast"
    elif getattr(args, "normal", False):
        tier = "normal"
    elif getattr(args, "full", False):
        tier = "full"
    _ORD = {"fast": 0, "normal": 1, "full": 2}
    if tier not in _ORD:
        die(f"unknown build tier {tier!r}; choose fast, normal, or full")
    req = _ORD[tier]
    gates_t0 = time.monotonic()
    gate_timings = []
    full_findings = []

    def _timed(label, start):
        gate_timings.append((label, time.monotonic() - start))

    def _print_gate_timings():
        if gate_timings:
            body = ", ".join(f"{name} {secs:.2f}s"
                             for name, secs in sorted(gate_timings,
                                                      key=lambda row: row[1], reverse=True))
            log(f"gate timings (slowest first): {body}")

    def _run_checked(mod, gargs=()):
        started = time.monotonic()
        try:
            run([sys.executable, "-m", mod, *gargs])
        finally:
            _timed(mod, started)

    def _gate(mod, gargs, die_msg, gtier):
        """Run a source gate when its tier is requested.

        Normal integrity checks fail immediately. Full discovery checks continue after
        findings so one periodic run searches the entire tree and produces every
        available worklist; their failures are reported together at the end.
        """
        if req < _ORD[gtier]:
            return
        started = time.monotonic()
        r = subprocess.run([sys.executable, "-m", mod, *gargs],
                           cwd=str(REPO), capture_output=True, text=True, env=_pkg_env())
        _timed(mod, started)
        if r.returncode != 0:
            for ln in (r.stdout + r.stderr).splitlines():
                print(ln, file=sys.stderr)
            if gtier == "full":
                full_findings.append((mod, die_msg))
                log(f"full audit finding: {mod} (continuing)")
                return
            _print_gate_timings()
            die(die_msg)
        out = (r.stdout + r.stderr).strip()
        if out:
            log(out.splitlines()[-1])

    _report = json.loads(REPORT.read_text())

    # FAST is a true matcher loop: compile/delink/objdiff and print the number, with
    # no source gates. Even the previously reduced gate set cost ~1.1s on a no-op
    # build. Normal owns the cheap integrity gates; full owns the periodic audits.
    if req == 0:
        summarize(_report, full=False)
        log("fast tier: build + objdiff summary only; no source gates. Run `gruntz build "
            "--normal` per commit and `gruntz build --full` for the periodic audit.")
        _record_build_time("fast", time.monotonic() - build_start, ninja_s,
                           time.monotonic() - gates_t0)
        return

    # Gate 0: the gates' own NEGATIVE CONTROLS (~0.01s, hermetic - no build artifacts).
    # Every gate below reports a number, and a gate nobody has watched FAIL reports it
    # whether or not it is true: vtable_slot_binding's baseline once parsed as empty (so it
    # passed everything), class_sizes read SIZE() out of a COMMENT (so it failed correct
    # code), and the MAX-% ratchet absorbed a 0/0 report's 100% and pinned itself there
    # forever. Each of those is now a test that fails against the code that shipped it.
    # This runs FIRST: if the checks are broken, their verdicts below are worthless.
    _run_checked("gruntz.match.gate_selftest")

    # NORMAL+ integrity gates. These are cheap and catch annotation/order/source
    # mistakes, but they do not belong in the latency-sensitive matcher loop.
    _gate("gruntz.audit.rva_size", ["--gate"],
          "rva-size: an RVA(addr, size) label is SHORTER than the function's real extent, "
          "so the delinker truncates the TARGET and objdiff scores a complete body against "
          "a partial one - a silently capped score, not a code problem "
          "(python -m gruntz.audit.rva_size)", "normal")
    _gate("gruntz.audit.compgen_order", ["--gate"],
          "compgen-order ratchet violated - move the RVA_COMPGEN invocation to its "
          "RVA-sorted slot (python -m gruntz.audit.compgen_order)", "normal")
    # The DATA half of the same problem: a datum cl emits as a COFF COMMON from a
    # header-inline's local static. No source macro can reach it, so its retail rva
    # lives in config/retail/compiler-generated-data.tsv - and this re-proves every
    # pin against the base objs and ratchets COVERAGE, because an unnamed COMMON
    # costs 0% (objdiff masks relocs) and so nothing else would ever report it.
    _gate("gruntz.audit.compgen_data", ["--gate"],
          "compgen-data ratchet violated - a compiler-generated COMMON is unpinned, "
          "or a pin is mis-spelled/stale/unbacked by the base objs "
          "(python -m gruntz.audit.compgen_data)", "normal")
    _gate("gruntz.audit.data_tu_order", ["--ratchet"],
          "data-tu-order ratchet violated - a DATA def lands inside another TU's "
          "same-storage band (python -m gruntz.audit.data_tu_order)", "normal")
    _gate("gruntz.audit.single_view", ["--ratchet"],
          "single-view ratchet violated - a global is declared with two types/"
          "linkages (python -m gruntz.audit.single_view)", "normal")
    _gate("gruntz.audit.self_recursion", ["--gate"],
          "self-recursion ratchet violated - a seam accessor returns a call to "
          "ITSELF (a cast-seam sweep rewrote the seam's own body; it compiles and "
          "the %% gate cannot see it) (python -m gruntz.audit.self_recursion)", "normal")

    # Normal measures only fast source-text cleanliness. The build/IR-derived semantic
    # baseline is deliberately reserved for the periodic full tier.
    started = time.monotonic()
    summarize(_report, write=True, vtable_health=False,
              semantic_cleanliness=req >= _ORD["full"])
    _timed("scoreboard", started)

    # NORMAL tier - structural uniqueness invariants (low per-edit violation, so out of
    # the fast inner loop): the @stub-backlog address format, the game-body/library-body
    # exclusivity, and one-mangled-name-per-RVA. All FATAL, no allowlist.
    _run_checked("gruntz.match.verify_stubs")
    _run_checked("gruntz.match.verify_library_overlap")
    _run_checked("gruntz.match.verify_unique_names")

    # NORMAL ratchets - rare-during-%-grind + trivial orchestrator fixups, so out of the
    # fast loop: a TU move (tu_order), a mal-formed label (label_style), a re-introduced
    # alias typedef (view_typedef).
    _gate("gruntz.audit.tu_order_check", ["--gate"],
          "tu-order ratchet violated - a function move broke the linker-order "
          "invariant (python -m gruntz.audit.tu_order_check --tu <name>)", "normal")
    _gate("gruntz.audit.label_style", ["--gate"],
          "label-style ratchet violated - spell the label canonically "
          "(python -m gruntz.audit.label_style)", "normal")
    _gate("gruntz.audit.view_typedef", ["--ratchet"],
          "view-typedef ratchet violated - delete the alias typedef and use the real "
          "class name (python -m gruntz.audit.view_typedef)", "normal")
    # Numeric domains: one belief per domain about retail's field width and no
    # bare header enums outside the GZ_ENUM_* layer.
    # See docs/enum-modeling-plan.md + docs/patterns/enum-domains.md.
    _gate("gruntz.audit.enum_domains", ["--gate"],
          "enum-domain gate violated - a split domain's storage width disagrees, a "
          "header declares a bare enum, or a range test names a domain member "
          "(python -m gruntz.audit.enum_domains)", "normal")
    # Once a switch KEY is enum-typed, every integer label under it has exactly one
    # correct enumerator, so a raw one is an oversight rather than a judgement. This
    # is libclang over the real compdb, which is why it catches what the campaign's
    # own regex rewriters kept missing (`case 0xa:` when the pattern wanted decimal,
    # `case 0: {` when it wanted end-of-line). FULL tier: it reparses every TU.
    _gate("gruntz.audit.enum_case_labels", ["--gate", "--detail"],
          "a switch with an enum-typed key still has numeric case labels - "
          "python -m gruntz.audit.enum_case_labels --apply", "full")
    # The rest of the same problem: a literal written where the surrounding TYPE
    # already says what it means - a 0 against a pointer, a byte count that is
    # sizeof the thing beside it, a bare value against an enum. Ratcheted
    # down-only against config/cleanliness/bare-constants-baseline.tsv. FULL tier: libclang.
    _gate("gruntz.audit.bare_constants", ["--gate", "--detail"],
          "bare-constants ratchet regressed - a literal was added where a type "
          "already names it (python -m gruntz.audit.bare_constants --detail)", "full")
    # The #include block is canonical: no duplicates, and one order tree-wide
    # (rva -> platform preludes -> own header -> project -> library). See
    # docs/patterns/include-order.md; the fixer is mechanical and line-conserving.
    _gate("gruntz.audit.include_order", ["--gate"],
          "include-order ratchet violated - the include block is duplicated or out of "
          "canonical order (python -m gruntz.audit.include_order --fix-dupes --fix)",
          "normal")
    # SECONDARY (MI) vtable coverage: every through-base name in the game catalog
    # must remain bound in symbol_names.csv. Sub-second, so it runs at normal tier.
    _gate("gruntz.cleanliness.vtable_secondary", [],
          "secondary-vtable coverage violated - a catalogued through-base vtable "
          "name is unbound (python -m gruntz.cleanliness.vtable_secondary --list)", "normal")

    # Non-fatal extras (normal+): per-function fingerprints, README score, regressions.
    if (REPO / "build" / "clangd" / "compile_commands.json").is_file():
        started = time.monotonic()
        subprocess.run([sys.executable, "-m", "gruntz.match.fingerprints"],
                       cwd=str(REPO), env=_pkg_env())
        _timed("feedback:fingerprints", started)
    started = time.monotonic()
    subprocess.run([sys.executable, "-m", "gruntz.match.status",
                    "--report", str(REPORT), "summary", "--write-readme"],
                   cwd=str(REPO), stdout=subprocess.DEVNULL, env=_pkg_env())
    _timed("feedback:readme", started)
    if (REPO / "config" / "match_baseline.tsv").is_file():
        log("regressions vs baseline ...")
        started = time.monotonic()
        subprocess.run([sys.executable, "-m", "gruntz.match.status",
                        "--report", str(REPORT), "check"], cwd=str(REPO), env=_pkg_env())
        _timed("feedback:regressions", started)

    # build/gen/structs.json holds clang's ACTUAL record layouts, and it is NOT a ninja
    # target - so it goes stale the instant a header changes, and every consumer then
    # answers from a snapshot of the old tree. Measured 2026-07-13: this made class_sizes
    # BOTH false-fail (3 classes flagged whose fixes had already landed) and, far worse,
    # capable of false-PASSING a class whose layout we have since broken - which is the
    # exact defect the gate exists to catch. It also fooled stale_walls into reporting all
    # 9 layout bugs as still-live for 90 minutes after they were fixed.
    # Regenerate it here, before anything reads it.
    #
    # It is NOT cheap (a clang layout+ast dump per TU, ~4.5 min - it dominates the gate
    # tail), so do it only when it is actually stale. structs.json is a pure function of
    # src/+include/, so "no source is newer than it" == "it already describes this tree";
    # the same _stale_sources() predicate class_sizes uses to decide whether it may
    # answer at all. This keeps the gates running on every full build (a no-op build must
    # still verify the source invariants) without paying 4.5 min to recompute a file that
    # cannot have changed.
    # Only the FULL-tier class/vtable gates read structs.json, so regen it only there
    # (the ~4.5-min-when-stale clang layout dump must not land in a normal commit build).
    if req >= _ORD["full"]:
        from gruntz.cleanliness.class_sizes import _stale_sources
        if _stale_sources() or not (GEN_NAMES.parent / "structs.json").is_file():
            started = time.monotonic()
            cmd_structs(argparse.Namespace(tu=[]))
            _timed("gruntz.structs", started)
        else:
            log("structs.json is current (no source newer) - skipping the layout regen.")

    # Class-metadata invariants. Every vtable-bearing class should carry a
    # retail/manual/RTTI catalog entry, and every class a SIZE/SIZE_UNKNOWN - so a
    # class added without one is caught here, not later.
    # SIZE reached 0 (all classes annotated) -> now a FATAL gate: a class added
    # without SIZE/SIZE_UNKNOWN fails the build. It ALSO now checks CORRECTNESS:
    # a class that DECLARES SIZE(C,N) but does not COMPUTE N, and is `new`ed, emits the
    # wrong `push <size>` immediate into operator new - a real byte defect that nothing
    # checked before (SIZE() was effectively a comment).
    _gate("gruntz.cleanliness.class_sizes", [],
          "class-sizes: a class lacks SIZE()/SIZE_UNKNOWN or its computed size is "
          "wrong (python -m gruntz.cleanliness.class_sizes)", "full")
    # The four manual-vtable idioms (*Vtbl structs / ->vtbl / g_*Vtbl / m_vtbl/m_vptr)
    # were driven to 0 - a FATAL gate so none can reappear (they must be real virtuals).
    _gate("gruntz.cleanliness.vtable_bans", [],
          "vtable-bans: a banned manual-vtable idiom reappeared - model it as real "
          "virtuals (python -m gruntz.cleanliness.vtable_bans)", "full")
    # The vtable-hierarchy AUDIT (every class's SOURCE vtable diffed against the binary-proven
    # one: INHERIT/RENAME/REDECLARE/OVERRIDE/MISSING) reached 0 - now a FATAL gate so the source
    # vtable modelling can never drift from the binary. `python -m gruntz.core.vtable_hierarchy --audit`.
    _gate("gruntz.core.vtable_hierarchy", ["--audit"],
          "vtable-audit: source vtable hierarchy does not match the binary - drive "
          "INHERIT/RENAME/REDECLARE/OVERRIDE/MISSING to 0 "
          "(python -m gruntz.core.vtable_hierarchy --audit)", "full")
    # Game-vtable catalog structure: unique rows/names and a valid kind for each entry.
    _gate("gruntz.cleanliness.class_vtables", ["--assert-unique"],
          "class-vtables: the game-vtable catalog is structurally invalid "
          "(python -m gruntz.cleanliness.class_vtables --assert-unique)", "full")
    # Catalog completeness: every vtable-bearing class positively bound or VTBL_ABSENT-proven.
    _gate("gruntz.cleanliness.class_vtables", [],
          "class-vtables: a vtable-bearing class is uncatalogued - add it to "
          "vtables_game.csv, dissolve the view, or prove VTBL_ABSENT "
          "(python -m gruntz.cleanliness.class_vtables)", "full")
    # Vtable COVERAGE: every analysed vtable must be in the game or library catalog.
    _gate("gruntz.cleanliness.vtable_coverage", [],
          "vtable-coverage: analysed vtable(s) uncovered - add them to "
          "vtables_game.csv or vtables_library.csv "
          "(python -m gruntz.cleanliness.vtable_coverage --list)", "full")
    # Vtable OWNERSHIP: re-derive every game-catalog binding from the image (the ??_7 slot ->
    # scalar-deleting dtor -> ??1 chain). Slowest of the vtable gates -> full.
    _gate("gruntz.cleanliness.vtable_owner", ["--audit"],
          "vtable-owner: a catalog binding contradicts the vtable that actually dispatches "
          "to the class's destructor (python -m gruntz.cleanliness.vtable_owner --audit)", "full")
    # Vtable VIRTUALITY: every primary game row binds a real class modelling the slots.
    _gate("gruntz.cleanliness.vtable_virtuality", [],
          "vtable-virtuality: a catalogued vtable is not modelled by real virtuals - the class "
          "must be defined and declare a virtual for each slot "
          "(python -m gruntz.cleanliness.vtable_virtuality --list)", "full")
    # Vtable SLOT BINDING: coverage says the vtable is bound; virtuality says the class
    # declares ENOUGH virtuals. Neither joins a SLOT to the body our source puts at its
    # rva - so a body bound under a NON-virtual name (a free fn / Gap_*, or a non-virtual
    # method sitting beside the declared virtual) satisfies both while the slot's reloc
    # dangles onto a symbol that is not the override. That is a real wrong-dispatch bug
    # (GO1's 4 Fader RenderFrames). This gate does the per-slot join: retail slot ->
    # chase_thunk -> the symbol src emits there -> must be a virtual of the class or a
    # base. PURE fail-closed since 2026-07-22: the 259-row frozen backlog drained to 0
    # and the baseline file was deleted - ANY violation is FATAL, fix the modeling.
    _gate("gruntz.cleanliness.vtable_slot_binding", [],
          "vtable-slot-binding: a vtable slot's body is bound under a non-virtual or "
          "wrong-class name - wire it to the class's declared virtual "
          "(python -m gruntz.cleanliness.vtable_slot_binding)", "full")
    # (the normal tu-order / compgen / data-rva / single-view / view-typedef /
    #  label-style integrity checks ran up top.)

    # FULL tier - the UNGAMEABLE fake-view metric (slowest gate; reached 0 2026-07-21).
    _gate("gruntz.cleanliness.view_debt", ["--fatal"],
          "view-debt: a fake view regressed - fold the phantom onto its real class "
          "(python -m gruntz.cleanliness.view_debt --fatal)", "full")
    # FULL tier - declarations without definitions (the alias ratchet): a symbol some
    # base obj references that nothing defines and retail's namespace never names is a
    # fabricated alias / phantom extern. Set-ratcheted vs config/declared-only-baseline.tsv.
    _gate("gruntz.cleanliness.declared_only", ["--fatal"],
          "declared-only: a NEW declaration without a definition (alias hack) - rename the "
          "decl to the real symbol or define the owner "
          "(python -m gruntz.cleanliness.declared_only --list)", "full")

    _print_gate_timings()
    _record_build_time(tier, time.monotonic() - build_start, ninja_s,
                       time.monotonic() - gates_t0)
    if full_findings:
        print("\nFull audit findings:", file=sys.stderr)
        for mod, message in full_findings:
            print(f"  {mod}: {message}", file=sys.stderr)
        die(f"full audit found unfinished work in {len(full_findings)} area(s); "
            "use its generated inventories and the commands above to plan the work")


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

    Source: matched src/ layouts (the clangd compdb); headers come in through their
    own #includes. (The src/Stub/types/ comprehension layer is gone.)
    """
    clang = _clang()
    cmd = [sys.executable, str(BUILD / "ghidra_metadata_generate.py"), "--clang", clang]
    for t in args.tu:
        cmd += ["--tu", t]
    run(cmd)


def cmd_gate_selftest(args) -> None:
    """Run the gates' negative controls (also gate 0 of every full build)."""
    run([sys.executable, "-m", "gruntz.match.gate_selftest", "-v"])


def cmd_ghidra_refresh(args) -> None:
    """Populate an optional Ghidra viewer from authoritative project data."""
    _ensure_retail_copy()
    if not RETAIL_EXE.is_file():
        die("retail EXE missing: run `gruntz init` inside `nix develop` first")
    cmd_structs(argparse.Namespace(tu=[]))
    cold = not GHIDRA_PROJECT_DIR.exists() or not any(GHIDRA_PROJECT_DIR.glob("*.rep"))
    GHIDRA_PROJECT_DIR.mkdir(parents=True, exist_ok=True)
    if cold:
        log("creating optional Ghidra viewer database (initial analysis takes minutes) ...")
    _ghidra_metadata_apply(analyze=cold)
    log("ghidra-refresh done: viewer populated from tracked/source inventory; "
        "the build does not read this database.")


def _ghidra_metadata_apply(analyze: bool) -> None:
    """Drive optional PyGhidra viewer enrichment.

    Runs scripts/gruntz/ghidra/ghidra_metadata_apply.py under THIS interpreter (sys.executable
    is the `nix develop` python that carries the pyghidra package): it boots
    PyGhidra in-process, imports/analyzes GRUNTZ.EXE into build/ghidra-named/gruntz,
    then runs apply.py. `analyze=True` is used only for a missing viewer DB.
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
           str(GHIDRA_APPLY)]
    if not analyze:
        cmd.append("--no-analyze")
    run(cmd)


def _ensure_compdb_fresh() -> None:
    """Regenerate build/clangd/compile_commands.json when config/units.toml is newer.

    The compdb is a GENERATED artifact with no ninja edge: only `gruntz init` / `gruntz
    clangd` write it, and nothing re-runs them. So it silently describes the unit list as
    it was at init time - and a unit ADDED or DELETED since then leaves it wrong, with no
    error from anything that reads it.

    That is not hypothetical. `ShowMultiDlg.cpp` was deleted in d34f5af3f; every worktree
    initialised before that kept a compdb entry for the dead file, so
    ghidra_metadata_generate could not compile it, correctly refused to emit a partial
    structs.json - and `gruntz build` died there, on a file the tree no longer has. With
    structs.json frozen, the class_sizes CORRECTNESS gate then refused to answer for as
    long as the compdb stayed stale.

    Regenerating costs ~0.5s (it emits JSON from units.toml; nothing is compiled), so
    just keep it honest on every build, like the retail copy below.
    """
    compdb = REPO / "build" / "clangd" / "compile_commands.json"
    manifest = REPO / "config" / "units.toml"
    if compdb.is_file() and manifest.is_file() and compdb.stat().st_mtime >= manifest.stat().st_mtime:
        return
    log("compile DB is older than config/units.toml - regenerating (cheap) ...")
    subprocess.run([sys.executable, str(INIT / "clangd.py")], cwd=str(REPO),
                   stdout=subprocess.DEVNULL, env=_pkg_env())


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


def cmd_init(args) -> None:
    """One-time FULL local setup for this checkout. Run inside `nix develop`.

    Builds the local, imperative state (under build/) that Nix does not - so a fresh
    checkout goes straight to `gruntz build` after one `init`:
      - the git-ignored build dirs;
      - build.ninja + compile_commands.json + objdiff.json (configure.py);
      - a stable retail copy at build/exe/GRUNTZ.EXE (the delink input + Ghidra import);
      - the Wine prefix + MSVC 5.0 toolchain registration (toolchain.py);
      - the clangd compile DB (clangd.py).

    The optional Ghidra viewer is intentionally absent from init. Run
    `gruntz ghidra-refresh` explicitly when a disposable decompiler DB is useful.
    """
    for d in ("build/gen", "build/objdiff", "build/clangd", "build/pdb",
              "build/delink/named", "build/exe"):
        (REPO / d).mkdir(parents=True, exist_ok=True)
    run([sys.executable, str(CONFIGURE)])            # build.ninja + compile_commands + objdiff.json
    _ensure_retail_copy()                            # stable retail copy (cheap, idempotent)
    if not os.environ.get("MSVC_DIR"):
        log("MSVC_DIR unset - run inside `nix develop` for the toolchain. "
            "Did dirs + configure + retail copy only.")
        return
    tc = [sys.executable, str(INIT / "toolchain.py")]
    if args.force:
        tc.append("--force")
    run(tc)                                          # wine prefix + registry (idempotent)
    run([sys.executable, str(INIT / "clangd.py")])   # clangd compile database
    log("init complete (Ghidra-free). Run `gruntz ghidra-refresh` only when wanted.")


def cmd_status(args) -> None:
    if not REPORT.exists():
        die(f"no report at {REPORT}; run `gruntz build` first")
    summarize(json.loads(REPORT.read_text()), table=True)


def cmd_match_queue(args) -> None:
    """Generate the exhaustive residual and weighted middle-to-worst queues."""
    if not REPORT.exists():
        die(f"no report at {REPORT}; run `gruntz build` first")
    if not GEN_NAMES.exists():
        die(f"no {GEN_NAMES}; run `gruntz build` or `labels` first")
    from gruntz.core import call_main
    sys.exit(call_main("gruntz.match.residual_queue", []))


def cmd_link(args) -> None:
    """Phase 2: link the base objs into a candidate GRUNTZ.EXE + map.

    Runs the genuine VC5 link.exe (5.10.7303) over build/objdiff/base/*.obj against
    the retail library set - static CRT + MFC via the objs' own `-defaultlib:`
    directives, plus DirectX 6, version/winmm and the synthesised mss32/smackw32
    import libs (gruntz.build.import_lib). There is NO /FORCE: the tree links for
    real (0 unresolved, 0 duplicate symbols), so a link FAILURE is a finding - an
    unresolved extern or an LNK2005 duplicate is a source defect to fix, not noise
    to tolerate. The .map exposes each function's link-assigned RVA
    and source object. Combined with the retail RVAs that gives the build-order
    model (intra-TU = source order, cross-TU = object order); see
    docs/link-order-investigation.md. Pass --order FILE to test a hypothesised link
    order, --analyze to print the layout report afterwards.
    """
    run([sys.executable, str(CONFIGURE)])
    ninja = tool("ninja")
    _start_wine_session()
    try:
        run([ninja, "base"])                       # ensure base objs are current
        cmd = [sys.executable, str(LINK)]
        if args.order:
            cmd += ["--order", args.order]
        if args.res:
            cmd += ["--res", args.res]
        if args.opt_ref:
            cmd += ["--opt-ref"]
        run(cmd)
    finally:
        _kill_wine_session()
    if args.analyze:
        run([sys.executable, "-m", "gruntz.audit.link_order",
             "--map", str(REPO / "build" / "exe" / "GRUNTZ.candidate.map"),
             "--names", str(GEN_NAMES)])


def cmd_audit(args) -> None:
    """`gruntz audit <tool> [args...]` - dispatch to gruntz.audit.<tool> IN-PROCESS
    (folder = command group: one module per audit). Dashes map to underscores;
    `tidy` -> tidy_audit. No tool / unknown tool lists what is available."""
    import importlib.util
    tool = (args.tool or "").replace("-", "_")
    tool = {"tidy": "tidy_audit"}.get(tool, tool)
    if not tool or importlib.util.find_spec(f"gruntz.audit.{tool}") is None:
        avail = sorted(f.stem for f in (PKG / "audit").glob("*.py")
                       if f.stem != "__init__")
        msg = f"unknown audit tool '{args.tool}'. " if args.tool else ""
        die(msg + "available: " + ", ".join(avail))
    from gruntz.core import call_main
    sys.exit(call_main(f"gruntz.audit.{tool}", args.rest))


def cmd_permute(args) -> None:
    """`gruntz permute fn|sweep|variants [args...]` - the climbers, in-process."""
    mod = {"fn": "permute", "sweep": "permute_sweep", "variants": "match_variants"}[args.which]
    from gruntz.core import call_main
    sys.exit(call_main(f"gruntz.permute.{mod}", args.rest))


def cmd_data_audit(args) -> None:
    """Attribute retail .rdata/.data/.bss bytes to source DATA() symbols + fingerprint.

    Thin alias for `python -m gruntz.core.data_audit`: reads ONLY the retail
    GRUNTZ.EXE (no delinker/PDB/wine), classifies each named data symbol's PE
    storage, resolves an extent (reviewed size, else next-symbol gap), and records a
    relocation-normalized content digest + HIGHLOW fingerprint into
    build/gen/data_attribution.tsv. This makes the data-section attribution explicit
    and gives the data-match loop a fixed retail oracle. See
    docs/data-attribution.md.
    """
    cmd = [sys.executable, "-m", "gruntz.core.data_audit"]
    if args.rva:
        cmd += ["--rva", args.rva]
    if args.json:
        cmd += ["--json", args.json]
    run(cmd)


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
    prefix, and any optional viewer DB stored under build/."""
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
# classifier reaches for. The implementations live in gruntz/sema/ - one module
# per subcommand (browse scripts/gruntz/sema/); cli.py only owns argparse and
# these lazy-import shims. Usage logging: gruntz.sema.run_logged (main dispatch).
def cmd_sema_xref(args) -> None:
    from gruntz.sema import xref
    xref.run(args)


def cmd_sema_batch(args) -> None:
    """`gruntz sema -`: read newline-delimited sema command lines from stdin and
    answer each against the process-wide Context (one EXE/symbol load for N
    queries). Echoes `== gruntz sema <line>` before each answer; logs each line;
    exits 0 (per-line rcs are in the log + echoed on non-zero)."""
    import shlex
    from gruntz.sema._common import log_invocation
    ap = argparse.ArgumentParser(prog="gruntz")
    _add_sema(ap.add_subparsers(dest="cmd", required=True))
    for line in sys.stdin:
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        try:
            toks = shlex.split(line)
        except ValueError as e:              # unbalanced quote: report, keep batching
            print(f"== gruntz sema {line}", flush=True)
            print(f"[gruntz] ERROR: unparseable line ({e})", file=sys.stderr)
            log_invocation(2, cmd=f"gruntz sema {line}")
            print("== rc=2", flush=True)
            continue
        if toks[:2] == ["gruntz", "sema"]:
            toks = toks[2:]
        elif toks[:1] == ["sema"]:
            toks = toks[1:]
        pretty = "gruntz sema " + " ".join(toks)
        print(f"== {pretty}", flush=True)
        if toks[:1] == ["-"]:                # no batch-in-batch (stdin is spoken for)
            print("[gruntz] ERROR: nested batch ('-') inside batch mode", file=sys.stderr)
            log_invocation(2, cmd=pretty)
            print("== rc=2", flush=True)
            continue
        rc = 0
        try:
            sub = ap.parse_args(["sema"] + toks)
            sub.func(sub)
        except SystemExit as e:
            rc = e.code if isinstance(e.code, int) else (0 if e.code is None else 1)
        log_invocation(rc, cmd=pretty)
        if rc:
            print(f"== rc={rc}", flush=True)
    sys.exit(0)


def cmd_sema_point(args) -> None:                 # refs / hover share this
    from gruntz.sema import clangd
    clangd.run_point(args)


def cmd_sema_rename(args) -> None:
    from gruntz.sema import clangd
    clangd.run_rename(args)


def cmd_sema_disasm(args) -> None:
    from gruntz.sema import disasm
    disasm.run(args)


def cmd_sema_strings(args) -> None:
    from gruntz.sema import strings
    strings.run(args)


def cmd_sema_map(args) -> None:
    from gruntz.sema import map as sema_map
    sema_map.run(args)


def cmd_sema_vtable(args) -> None:
    from gruntz.sema import vtable
    vtable.run(args)


def cmd_sema_class(args) -> None:
    from gruntz.sema import classof
    classof.run(args)


def cmd_sema_match(args) -> None:
    from gruntz.sema import match
    match.run(args)


def cmd_sema_rva(args) -> None:
    from gruntz.sema import rva
    rva.run(args)


def _add_sema(sub) -> None:
    """The `sema` semantic-navigation group: one self-teaching help screen, each
    subcommand a thin delegation (see the cmd_sema_* funcs)."""
    sema = sub.add_parser(
        "sema", formatter_class=argparse.RawDescriptionHelpFormatter,
        help="semantic navigation: xref / disasm / dossiers / clangd LSP",
        description="gruntz sema <cmd> - source & target navigation for matchers "
                    "and classifiers.\nOne module per subcommand in gruntz/sema/, "
                    "in-process over gruntz/core (one EXE/symbol load).\nSEMANTIC "
                    "questions go here; grep is lexical-only.\nrc: 0 answered, "
                    "1 answered-NO (differs / not found), 2 error.",
        epilog="examples (use when ...):\n"
        "  gruntz sema xref 0x00080850           who calls this fn (attribution)\n"
        "  gruntz sema xref --callees CFoo::Bar   its own call targets (names resolve)\n"
        "  gruntz sema refs  include/X.h 30       every ref (USR-exact; no grep collisions)\n"
        "  gruntz sema hover src/X.cpp 42         type/decl at point\n"
        "  gruntz sema rename include/X.h 40 m_new --dry-run   tree-wide rename preview\n"
        "  gruntz sema rva   0x00080850           address dossier (claim/src loc/lib/retail/%; chases ILT thunks)\n"
        "  gruntz sema map                        whole-.text layout: categories + gaps overview\n"
        "  gruntz sema map range 0x80000 0x81000  functions + gaps in an RVA window (owner: TU/MFC/CRT/...)\n"
        "  gruntz sema map file GruntzMgr.cpp     a file's functions + how many foreign fns interleave them\n"
        "  gruntz sema class CImage               vtable slots + hierarchy tags\n"
        "  gruntz sema vtable 0x001b8008          dump a binary vtable / find a fn's holding slot\n"
        "  gruntz sema match cplay                per-fn % of a unit (or an RVA)\n"
        "  gruntz sema disasm 0x00080850          retail disasm + relocs\n"
        "  gruntz sema strings 0x00080850         the string set of a fn\n"
        "  gruntz sema strings --find WORLDZ      reverse literal lookup\n"
        "  gruntz sema -                          BATCH: sema command lines on stdin,\n"
        "                                         answered against ONE loaded Context\n")
    ss = sema.add_subparsers(dest="sema", required=True)

    sb = ss.add_parser("-", help="batch: newline-delimited sema commands on stdin, "
                                 "one loaded Context (N queries, 1 parse)")
    sb.set_defaults(func=cmd_sema_batch)

    sx = ss.add_parser("xref", help="retail caller/callee graph (attribution)")
    sx.add_argument("target", nargs="+", help="RVA(s) (0x..) or symbol name(s)")
    sx.add_argument("--callees", action="store_true", help="forward: its call targets")
    sx.add_argument("--raw", action="store_true", help="every call site (no dedup)")
    sx.add_argument("--tree", action="store_true",
                    help="(DEFAULT) caller ancestry tree - expands callers-of-callers, "
                         "chasing ILT jmp-thunks + surfacing data/.text address-takings")
    sx.add_argument("--flat", action="store_true",
                    help="opt out of --tree: only direct rel32 callers (the old default)")
    sx.add_argument("--depth", type=int, default=4, metavar="N",
                    help="--tree expansion cap (default 4; 0 = unlimited, can be huge)")
    sx.set_defaults(func=cmd_sema_xref)

    # `symbol` / `def` retired (0 uses in 9,771 logged calls - the harness LSP
    # covers them); `refs`/`hover`/`rename` stay (rename is NOT in the harness LSP).
    for nm, hlp in (("refs", "references at point (USR-exact)"),
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

    srv = ss.add_parser("rva",
                        help="address dossier (src claim + file:line / lib / retail / "
                             "match; chases ILT jmp-thunks to the body)")
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

    svt = ss.add_parser("vtable",
                        help="binary vtable finder: dump a vtable's slots (ILT thunks "
                             "chased to bodies), or find which vtable/slot holds a fn")
    svt.add_argument("target", help="a vtable start RVA (-> dump), or a fn RVA (-> find holder)")
    svtg = svt.add_mutually_exclusive_group()
    svtg.add_argument("--dump", action="store_true", help="force: dump the vtable at TARGET")
    svtg.add_argument("--holds", action="store_true",
                      help="force: which vtable/slot resolves to fn TARGET")
    svt.set_defaults(func=cmd_sema_vtable)

    sm = ss.add_parser("match", help="per-function/unit match summary (report.json)")
    sm.add_argument("target", help="a unit name, or an 0x RVA a src fn claims")
    sm.set_defaults(func=cmd_sema_match)

    sd = ss.add_parser("disasm",
                       help="disasm: TARGET (retail, default) / --base (compiled) / "
                            "--rich (base + /Z7 source lines) / --diff (base vs "
                            "target) / --lite (asm only) / --branches (the branch "
                            "sequence --diff's masking hides)")
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
    sd.add_argument("--blocks", action="store_true",
                    help="IDA-style basic-block view (either side): in-edges per block, "
                         "branch arrows, loop back-edges, shared ret tails; composes "
                         "with --lite (skeleton only), --diff (block-aligned diff / "
                         "skeleton diff with --lite) and --dot")
    sd.add_argument("--dot", action="store_true",
                    help="with --blocks: emit the CFG as graphviz DOT (with --diff: "
                         "target graph, differing blocks filled red)")
    sd.add_argument("--switch", action="store_true",
                    help="dereference the jump table(s) behind an indirect `jmp` and print "
                         "case -> target, reading the bytes out of GRUNTZ.EXE. Cases that "
                         "share a target are ONE source arm. Enumerating a switch by "
                         "inference is how missing case runs got shipped.")
    sd.add_argument("--branches", action="store_true",
                    help="the ordered CONDITIONAL-BRANCH sequence, with each target named "
                         "by branch index (so a uniform displacement shift compares equal) "
                         "+ the ret counts. This is what --diff structurally cannot show: "
                         "it masks address operands, which also hides intra-function "
                         "branch displacements. With --diff, only the differing rows, "
                         "classified SIGNEDNESS / POLARITY / OTHER / TOPOLOGY; rc=1 if "
                         "they differ. Whole tree: python -m gruntz.audit.jcc_sieve")
    sd.set_defaults(func=cmd_sema_disasm)

    st = ss.add_parser("strings", help="per-fn string set / --find reverse lookup")
    st.add_argument("rva", nargs="?", help="RVA (0x..) whose string set to print")
    st.add_argument("--find", metavar="TEXT", help="reverse: fns referencing TEXT")
    st.set_defaults(func=cmd_sema_strings)

    smap = ss.add_parser("map",
                         help="retail .text space map: layout overview / RVA-range "
                              "listing / gaps / per-file breakdown (owner: TU / MFC / "
                              "CRT / EH / thunk / unknown)")
    smap.add_argument("rest", nargs=argparse.REMAINDER,
                      help="a gruntz.core.exe_map command: [overview] | "
                           "range <lo> <hi>[-|+len] | file <path> [--gaps] | at <rva> | "
                           "gaps | units | find <regex>   (append --json for machine output)")
    smap.set_defaults(func=cmd_sema_map)


def _clang() -> str:
    import os
    return os.environ.get("GRUNTZ_CLANG") or tool("clang")


def main() -> None:
    ap = argparse.ArgumentParser(prog="gruntz", description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)

    b = sub.add_parser("build", help="compile -> labels -> delink -> objdiff")
    # Gate tiers (measured wall-times in docs/build-system.md): fast is the matcher
    # inner loop, normal is the default per-commit integrity pass, and full is
    # the periodic/daily whole-tree audit (libclang, class layouts, vtables).
    tiers = b.add_mutually_exclusive_group()
    tiers.add_argument("--tier", choices=("fast", "normal", "full"),
                       help="which gate tier to run (default: normal).")
    tiers.add_argument("--fast", action="store_true",
                       help="matcher inner loop: build plus objdiff summary, with no source gates.")
    tiers.add_argument("--normal", action="store_true",
                       help="per-commit integrity gates (the default).")
    tiers.add_argument("--full", action="store_true",
                       help="periodic/daily whole-tree discovery audit and worklists.")
    b.add_argument("--force-delink", action="store_true",
                   help="re-delink the target objs even if symbol_names.csv is "
                        "unchanged (removes the delink stamp).")
    b.add_argument("ninja_args", nargs=argparse.REMAINDER,
                   help="extra ninja args after `--` (e.g. -j8).")
    b.set_defaults(func=cmd_build)

    sub.add_parser("labels", help="regenerate symbol_names.csv from src @address"
                   ).set_defaults(func=cmd_labels)

    s = sub.add_parser("structs", help="regenerate structs.json + enums.json")
    s.add_argument("--tu", action="append", default=[])
    s.set_defaults(func=cmd_structs)

    sub.add_parser("gate-selftest", help="negative controls: prove the build gates can FAIL"
                   ).set_defaults(func=cmd_gate_selftest)

    sub.add_parser("ghidra-refresh", help="populate an optional Ghidra viewer from project data"
                   ).set_defaults(func=cmd_ghidra_refresh)
    i = sub.add_parser("init", help="one-time local setup (dirs/configure/EXE/wine/clangd)")
    i.add_argument("--force", action="store_true", help="re-init the wine prefix")
    i.set_defaults(func=cmd_init)
    sub.add_parser("clangd", help="(re)generate the clangd compile DB (editor)"
                   ).set_defaults(func=cmd_clangd)
    fmt = sub.add_parser("format", help="clang-format src/ + include/ to the Rust-like style")
    fmt.add_argument("--check", action="store_true",
                     help="CI gate: don't write, exit non-zero if anything is unformatted")
    fmt.set_defaults(func=cmd_format)
    sub.add_parser("status", help="objdiff summary + full per-unit table (report.json; no rebuild)"
                   ).set_defaults(func=cmd_status)
    sub.add_parser("match-queue", help="generate residual-weighted matching queues"
                   ).set_defaults(func=cmd_match_queue)
    sub.add_parser("report", help="alias of status: full per-unit match table (report.json; no rebuild)"
                   ).set_defaults(func=cmd_status)
    lk = sub.add_parser("link", help="phase 2: link base objs -> candidate EXE + map")
    lk.add_argument("--order", help="file listing obj stems in link order to test")
    lk.add_argument("--res", help="optional .RES for a runnable candidate image")
    lk.add_argument("--opt-ref", action="store_true",
                    help="let the linker strip/fold unreferenced COMDATs (default keeps all)")
    lk.add_argument("--analyze", action="store_true",
                    help="print the layout/link-order report after linking")
    lk.set_defaults(func=cmd_link)
    au = sub.add_parser(
        "audit", help="one-shot campaign audits - `gruntz audit <tool> [args...]` "
                      "dispatches to gruntz/audit/<tool>.py (folder = command group)")
    au.add_argument("tool", nargs="?",
                    help="audit module, dashes ok: assert-relocs, data-home, "
                         "stale-walls, reinterpret-census, tu-order-check, "
                         "interleavers, exe-diff, tidy, ... (no arg lists all)")
    au.add_argument("rest", nargs=argparse.REMAINDER, help="args passed through")
    au.set_defaults(func=cmd_audit)
    pm = sub.add_parser(
        "permute", help="source-permutation climbers - `gruntz permute "
                        "fn|sweep|variants [args...]` (gruntz/permute/)")
    pm.add_argument("which", choices=("fn", "sweep", "variants"),
                    help="fn = one-function hill-climber, sweep = whole unit, "
                         "variants = exhaustive AST/TU-state search")
    pm.add_argument("rest", nargs=argparse.REMAINDER, help="args passed through")
    pm.set_defaults(func=cmd_permute)
    da = sub.add_parser("data-audit", help="attribute retail .rdata/.data/.bss bytes "
                        "to DATA() symbols + fingerprint (-> build/gen/data_attribution.tsv)")
    da.add_argument("--rva", help="audit + print a single data symbol RVA")
    da.add_argument("--json", help="also write full per-symbol evidence JSON to this path")
    da.set_defaults(func=cmd_data_audit)
    sub.add_parser("todo", help="obj symbols lacking an @address (worklist)"
                   ).set_defaults(func=cmd_todo)
    sub.add_parser("clean", help="nuke build/ + stray artifacts (HEAVY re-init after)"
                   ).set_defaults(func=cmd_clean)
    _add_sema(sub)   # sema: semantic-navigation group (xref/LSP/disasm/dossiers)

    args = ap.parse_args()
    if getattr(args, "ninja_args", None) and args.ninja_args[:1] == ["--"]:
        args.ninja_args = args.ninja_args[1:]
    if sys.argv[1:2] == ["sema"]:
        from gruntz.sema import run_logged
        run_logged(args)  # usage log -> build/gruntz_sema.log
    else:
        args.func(args)


if __name__ == "__main__":
    main()
