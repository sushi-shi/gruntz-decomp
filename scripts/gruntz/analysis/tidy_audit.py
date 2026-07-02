#!/usr/bin/env python3
"""tidy_audit.py - on-demand clang-tidy DE-HACK finder + categorized worklist.

A READ-ONLY worklist generator for the matching campaign. It runs clang-tidy
(21.x, from the Nix shell) over src/ with a curated finder config
(config/tidy-audit.yaml) and reports the de-hack backlog the campaign cares
about: C-style pointer-punning casts (`(char*)this`, `(CFoo*)m_8`,
`*(i32*)(B+0xNN)`, `(T*)0xADDR`), explicit reinterpret_casts, dead/unused
declarations, and a few bugprone smells. Because clang parses each TU, every
finding is AST-real - it never counts a cast that only appears in a comment or
a string literal (the false positives an `rg ')this'` sweep would hit).

WHY A SEPARATE CONFIG (not a root .clang-tidy): clangd auto-discovers a file
named `.clang-tidy` and would run these checks LIVE in the editor. The config
lives at config/tidy-audit.yaml (a non-magic name, off the editor path) and is
passed explicitly via `--config-file`. And it is NEVER run with `-fix`: the
real fix is always a later, manual, objdiff-verified de-hack.

Scope: every src/*.cpp that has a compile command in build/clangd
(one TU per file - no aggregation), plus any file/dir given as an argument.
Findings in project headers (src/*.h, include/*.h) are reported too, deduped
across the many TUs that include them.

Usage:
    python -m gruntz.analysis.tidy_audit                 # whole src/ tree
    python -m gruntz.analysis.tidy_audit src/Gruntz      # a subtree
    python -m gruntz.analysis.tidy_audit --csv hacks.csv # file,line,check,message
    python -m gruntz.analysis.tidy_audit --top 40 -j 24

Run inside `nix develop` (any shell - it only needs clang-tidy + the compdb).
Also exposed as `gruntz lint`.
"""

import argparse
import csv
import json
import os
import re
import shutil
import subprocess
import sys
import time
from collections import Counter, defaultdict
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
CONFIG = REPO / "config" / "tidy-audit.yaml"
COMPDB_DIR = REPO / "build" / "clangd"
COMPDB = COMPDB_DIR / "compile_commands.json"

# A clang-tidy diagnostic line:
#   src/Gruntz/Foo.cpp:1114:15: warning: do not use ... [cppcoreguidelines-...]
# The trailing [...] holds one or more comma-joined check names; `note:` lines
# (source context / fix-it carets) carry no bracket and are skipped.
DIAG_RE = re.compile(
    r"^(?P<file>.+?):(?P<line>\d+):(?P<col>\d+):\s+"
    r"(?P<level>warning|error):\s+(?P<msg>.*?)\s+\[(?P<checks>[A-Za-z0-9.\-,]+)\]\s*$")

# The cast checks, in de-hack priority order. google-readability-casting fires
# on EVERY C-style cast (the total C-cast surface); pro-type-cstyle-cast is the
# subset that puns UNRELATED types (the `(char*)this` / `(T*)ptr` hacks - the
# ones a de-hack actually has to retype); pro-type-reinterpret-cast counts
# reinterpret_cast<> already written out.
CAST_ALL = "google-readability-casting"
CAST_UNRELATED = "cppcoreguidelines-pro-type-cstyle-cast"
CAST_REINTERPRET = "cppcoreguidelines-pro-type-reinterpret-cast"
CAST_CHECKS = (CAST_ALL, CAST_UNRELATED, CAST_REINTERPRET)


def die(msg: str) -> None:
    print(f"[tidy-audit] ERROR: {msg}", file=sys.stderr)
    sys.exit(1)


def norm(path: str) -> str | None:
    """Normalize a diagnostic path to REPO-relative (posix). Returns None for a
    path outside the tree or not under src/|include/ (a robust backstop to the
    HeaderFilterRegex - only project files are ever a de-hack target)."""
    p = Path(path)
    if not p.is_absolute():
        p = (REPO / p)
    try:
        rel = os.path.relpath(os.path.realpath(p), REPO)
    except ValueError:
        return None
    rel = rel.replace(os.sep, "/")
    if rel.startswith("src/") or rel.startswith("include/"):
        return rel
    return None


def load_db_files() -> set[str]:
    """REPO-relative paths that have a compile command (the only files clang-tidy
    can process; a header or an un-listed .cpp has no command)."""
    if not COMPDB.is_file():
        die(f"no compile DB at {COMPDB.relative_to(REPO)} - run `gruntz clangd` (or `init`).")
    db = json.loads(COMPDB.read_text())
    out = set()
    for e in db:
        f = e.get("file", "")
        rel = os.path.relpath(f, REPO) if os.path.isabs(f) else f
        out.add(rel.replace(os.sep, "/"))
    return out


def resolve_targets(paths: list[str], db_files: set[str]) -> tuple[list[str], list[str]]:
    """Expand CLI paths (files or dirs) to the src/ .cpp TUs that are in the DB.
    Default (no paths): every src/*.cpp with a compile command. Returns
    (targets, skipped) where skipped are requested .cpp files with no command."""
    src_db = sorted(f for f in db_files if f.startswith("src/") and f.endswith(".cpp"))
    if not paths:
        return src_db, []
    wanted: set[str] = set()
    requested_cpp: set[str] = set()
    for raw in paths:
        p = Path(raw)
        abs_p = p if p.is_absolute() else (Path.cwd() / p)
        if abs_p.is_dir():
            for f in abs_p.rglob("*.cpp"):
                requested_cpp.add(os.path.relpath(f, REPO).replace(os.sep, "/"))
        else:
            rel = os.path.relpath(abs_p, REPO).replace(os.sep, "/")
            requested_cpp.add(rel)
    for rel in requested_cpp:
        if rel in db_files:
            wanted.add(rel)
    skipped = sorted(requested_cpp - db_files)
    return sorted(wanted), skipped


def run_one(clang_tidy: str, config: Path, compdb_dir: Path, target: str):
    """Run clang-tidy on ONE TU; return (diagnostics, returncode, stderr_tail).

    diagnostics: list of (relpath, line, col, check, level, msg). One file per
    invocation so a parse failure isolates to that TU and the pool stays even.
    """
    cmd = [clang_tidy, f"--config-file={config}", "-p", str(compdb_dir),
           "--quiet", target]
    proc = subprocess.run(cmd, cwd=str(REPO), capture_output=True, text=True)
    diags = []
    for ln in proc.stdout.splitlines():
        m = DIAG_RE.match(ln)
        if not m:
            continue
        rel = norm(m.group("file"))
        if rel is None:
            continue
        for check in m.group("checks").split(","):
            diags.append((rel, int(m.group("line")), int(m.group("col")),
                          check, m.group("level"), m.group("msg")))
    # --quiet still lets real compile errors through on stderr; keep a short tail.
    err_tail = ""
    if proc.returncode != 0:
        errlines = [l for l in proc.stderr.splitlines() if "error:" in l]
        err_tail = "; ".join(errlines[-2:])[:200]
    return diags, proc.returncode, err_tail


def audit(targets: list[str], clang_tidy: str, config: Path, compdb_dir: Path,
          jobs: int):
    """Run the finder over all targets in parallel; return (uniq, errored).

    uniq: set of (relpath, line, col, check, level, msg) - deduped across TUs so
    a header finding seen through N includers counts ONCE.
    errored: list of (target, err_tail) for TUs clang-tidy could not fully parse.
    """
    uniq: set = set()
    errored: list = []
    done = 0
    n = len(targets)
    with ThreadPoolExecutor(max_workers=jobs) as ex:
        futs = {ex.submit(run_one, clang_tidy, config, compdb_dir, t): t
                for t in targets}
        for fut in as_completed(futs):
            t = futs[fut]
            diags, rc, err = fut.result()
            uniq.update(diags)
            if rc != 0 and err:
                errored.append((t, err))
            done += 1
            if done % 50 == 0 or done == n:
                print(f"  [{done}/{n}] scanned ...", file=sys.stderr, flush=True)
    return uniq, errored


def report(uniq: set, targets: list[str], skipped: list[str], errored: list,
           top: int, elapsed: float) -> None:
    """Print the categorized de-hack worklist."""
    per_check = Counter(d[3] for d in uniq)
    per_file_cast = defaultdict(Counter)  # file -> Counter{check: n} (cast checks)
    for f, _ln, _col, check, _lvl, _msg in uniq:
        if check in CAST_CHECKS:
            per_file_cast[f][check] += 1

    total_all = per_check.get(CAST_ALL, 0)
    total_unrel = per_check.get(CAST_UNRELATED, 0)
    total_reint = per_check.get(CAST_REINTERPRET, 0)

    print()
    print("=" * 72)
    print(f"  clang-tidy DE-HACK worklist   ({len(targets)} TUs, {elapsed:.1f}s, "
          f"{len(uniq)} unique findings)")
    print("=" * 72)

    print("\n  PER-CHECK TOTALS (unique findings, deduped across TUs)")
    print("  " + "-" * 62)
    for check, n in per_check.most_common():
        tag = "  <- cast" if check in CAST_CHECKS else ""
        print(f"    {n:6d}  {check}{tag}")
    if not per_check:
        print("    (no findings)")

    print("\n  CAST BACKLOG (tree-wide)")
    print("  " + "-" * 62)
    print(f"    {total_all:6d}  C-style casts total          ({CAST_ALL})")
    print(f"    {total_unrel:6d}  ... unrelated-type / pun     ({CAST_UNRELATED})")
    print(f"    {total_reint:6d}  reinterpret_cast<> in source ({CAST_REINTERPRET})")
    print(f"    ------  de-hack backlog size = {total_all} C-casts "
          f"({total_unrel} are pointer-punning hacks)")

    ranked = sorted(per_file_cast.items(),
                    key=lambda kv: (kv[1][CAST_ALL], kv[1][CAST_UNRELATED]),
                    reverse=True)
    print(f"\n  TOP {top} FILES BY C-STYLE CAST COUNT")
    print("  " + "-" * 62)
    print(f"    {'casts':>6} {'unrel':>6} {'reint':>6}  file")
    for f, c in ranked[:top]:
        print(f"    {c[CAST_ALL]:6d} {c[CAST_UNRELATED]:6d} {c[CAST_REINTERPRET]:6d}  {f}")
    if not ranked:
        print("    (no casts found)")

    if skipped:
        print(f"\n  NOTE: {len(skipped)} requested .cpp had no compile command "
              f"(skipped): {', '.join(skipped[:6])}"
              + (" ..." if len(skipped) > 6 else ""))
    if errored:
        print(f"\n  NOTE: {len(errored)} TU(s) had parse errors (findings still "
              f"counted from what parsed):")
        for t, err in errored[:8]:
            print(f"    {t}: {err}")
    print()


def write_csv(uniq: set, path: Path) -> None:
    """Write file,line,check,message - one row per unique finding, sorted."""
    rows = sorted(uniq, key=lambda d: (d[0], d[1], d[2], d[3]))
    with path.open("w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["file", "line", "check", "message"])
        for rel, line, _col, check, _lvl, msg in rows:
            w.writerow([rel, line, check, msg])
    print(f"[tidy-audit] wrote {len(rows)} rows -> {path}")


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("paths", nargs="*",
                    help="files/dirs to scan (default: all src/*.cpp in the compile DB)")
    ap.add_argument("--csv", type=Path, help="write file,line,check,message to this CSV")
    ap.add_argument("--top", type=int, default=20, help="top-N files by cast count (default 20)")
    ap.add_argument("-j", "--jobs", type=int, default=(os.cpu_count() or 8),
                    help="parallel clang-tidy jobs (default: nproc)")
    ap.add_argument("--config", type=Path, default=CONFIG,
                    help="clang-tidy config file (default: config/tidy-audit.yaml)")
    ap.add_argument("-p", "--compdb-dir", type=Path, default=COMPDB_DIR,
                    help="dir holding compile_commands.json (default: build/clangd)")
    args = ap.parse_args()

    clang_tidy = shutil.which("clang-tidy")
    if not clang_tidy:
        die("clang-tidy not on PATH - run inside `nix develop`.")
    if not args.config.is_file():
        die(f"no config at {args.config} (expected config/tidy-audit.yaml).")

    db_files = load_db_files()
    targets, skipped = resolve_targets(args.paths, db_files)
    if not targets:
        die("no matching src/*.cpp TUs to scan.")

    print(f"[tidy-audit] {clang_tidy} over {len(targets)} TU(s), "
          f"-j{args.jobs}, config={args.config.relative_to(REPO) if args.config.is_relative_to(REPO) else args.config}",
          file=sys.stderr)
    t0 = time.time()
    uniq, errored = audit(targets, clang_tidy, args.config, args.compdb_dir, args.jobs)
    elapsed = time.time() - t0

    report(uniq, targets, skipped, errored, args.top, elapsed)
    if args.csv:
        write_csv(uniq, args.csv)


if __name__ == "__main__":
    main()
