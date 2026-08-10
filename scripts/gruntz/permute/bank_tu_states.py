#!/usr/bin/env python3
"""gruntz.permute.bank_tu_states - sweep TU parse states and BANK each function's MAX.

cl 5.0 picks a function's register scheme partly from how many file-scope declarations
it has parsed before reaching it, so a function whose whole residue is a canonical
operand order or a register rotation scores differently in different TU states while its
source is untouched (docs/patterns/declaration-count-window-steers-regalloc.md,
docs/patterns/commutative-operand-order-is-canonical.md). Reaching 100 that way PROVES
the source correct: the residue was TU composition, not the body.

This drives that measurement to its conclusion. For each state it inserts a deterministic
throwaway declaration block into every requested .cpp, rebuilds, and runs
`gruntz.match.status update`, which ratchets `best_pct` per function (the per-function
`src_hash` never changes, so every row is a TU-composition bank, never a source edit).
The probes are removed and the index restored on exit, including on failure - they are
diagnostics and must never be committed.

The declaration KIND matters as much as the count, and a uniform run reaches only the
functions whose parse-state phase it steps through, so the block MIXES typedef / enum /
struct / class-with-an-inline-member / extern / file-scope static datum / prototype /
static function-with-a-body and splits them across both insertion points - see
docs/patterns/tu-state-probe-family-decides-reachability.md for the measurement that
established this.

Run inside `nix develop .#build`, from the repo/worktree root, with the target sources
COMMITTED (`status update` refuses to bank beside an unstaged build input, and this
stages them for the duration of each trial)::

    python3 -m gruntz.permute.bank_tu_states \
        --source src/DDrawMgr/DDrawShadeBlit.cpp \
        --source src/Gruntz/LightFxRender.cpp \
        --states 1-40

Then `git diff config/match_baseline.tsv` is the banked result, and the working tree is
back to where it started.
"""

from __future__ import annotations

import argparse
import json
import os
import pathlib
import random
import subprocess
import sys

MARK = "// GZPROBE"
DEFAULT_SEED = 0x9E3779B9


def project_root() -> pathlib.Path:
    """Repo/worktree root: the nearest ancestor holding flake.nix (worktree-safe),
    falling back to $GRUNTZ_DIR then CWD. NEVER anchor on the package location -
    PYTHONPATH can point at main."""
    cwd = pathlib.Path.cwd()
    for candidate in [cwd, *cwd.parents]:
        if (candidate / "flake.nix").exists():
            return candidate.resolve()
    return pathlib.Path(os.environ.get("GRUNTZ_DIR", cwd)).resolve()


ROOT = project_root()


def _declarations(rng: random.Random, count: int, tag: str) -> list[str]:
    """`count` file-scope declarations of MIXED kind - the kind is the load-bearing
    dimension, not just the count."""
    out = []
    for i in range(count):
        name = f"gzp_{tag}_{i}"
        kind = rng.randrange(8)
        if kind == 0:
            out.append(f"typedef int {name}_t;")
        elif kind == 1:
            out.append(f"enum {name}_e {{ {name}_A = 0, {name}_B = 1 }};")
        elif kind == 2:
            out.append(f"struct {name}_s {{ int a; int b; }};")
        elif kind == 3:
            out.append(f"class {name}_c {{ public: int a; int f(int x) {{ return x + a; }} }};")
        elif kind == 4:
            out.append(f"extern int {name}_x;")
        elif kind == 5:
            out.append(f"static int {name}_d = {i};")
        elif kind == 6:
            out.append(f"int {name}_p(int, int);")
        else:
            out.append(f"static int {name}_fn(int x) {{ return x * {i + 1}; }}")
    return [f"{line}  {MARK}" for line in out]


def strip_probes(text: str) -> str:
    return "\n".join(line for line in text.split("\n") if MARK not in line)


def apply_state(path: pathlib.Path, n: int, seed: int) -> None:
    """Write `path` with the state-`n` probe block, or with no probes when n == 0."""
    text = strip_probes(path.read_text())
    if n:
        rng = random.Random(seed ^ (n * 2654435761))
        lines = text.split("\n")
        above = rng.randrange(0, n + 1)
        below = n - above
        last_include = max(i for i, l in enumerate(lines) if l.startswith("#include"))
        if below:
            lines[last_include + 1:last_include + 1] = [MARK] + _declarations(rng, below, "b")
        first_include = next(i for i, l in enumerate(lines) if l.startswith("#include"))
        if above:
            lines[first_include + 1:first_include + 1] = [MARK] + _declarations(rng, above, "t")
        text = "\n".join(lines)
    path.write_text(text)


def _git(*args: str) -> subprocess.CompletedProcess:
    return subprocess.run(["git", *args], cwd=str(ROOT), capture_output=True, text=True)


def staged_paths(sources: list[str]) -> set[str]:
    out = _git("diff", "--cached", "--name-only", "--", *sources)
    return {line for line in out.stdout.splitlines() if line}


def sub_pcts(units: set[str]) -> dict[tuple[str, str], float]:
    report = json.loads((ROOT / "build/objdiff/report.json").read_text())
    scores: dict[tuple[str, str], float] = {}
    for unit in report.get("units", []):
        if units and unit.get("name") not in units:
            continue
        for fn in unit.get("functions") or []:
            if fn.get("name"):
                scores[(unit["name"], fn["name"])] = float(fn.get("fuzzy_match_percent") or 0.0)
    return scores


def units_of(sources: list[str]) -> set[str]:
    import tomllib
    with open(ROOT / "config/units.toml", "rb") as fh:
        cfg = tomllib.load(fh)
    wanted = set(sources)
    return {u["unit"] for u in cfg.get("unit", []) if u.get("source") in wanted}


def parse_states(spec: str) -> list[int]:
    states: list[int] = []
    for part in spec.split(","):
        part = part.strip()
        if not part:
            continue
        if "-" in part:
            lo, hi = part.split("-", 1)
            states.extend(range(int(lo), int(hi) + 1))
        else:
            states.append(int(part))
    return states


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--source", action="append", required=True,
                    help="repo-relative .cpp to perturb (repeatable)")
    ap.add_argument("--states", default="1-24",
                    help="state list/ranges, e.g. '1-24' or '3,7,11-14' (default 1-24)")
    ap.add_argument("--seed", type=lambda v: int(v, 0), default=DEFAULT_SEED)
    ap.add_argument("--no-bank", action="store_true",
                    help="measure only; do not run `status update`")
    args = ap.parse_args(argv)

    paths = [ROOT / s for s in args.source]
    for p in paths:
        if not p.is_file():
            sys.exit(f"no such source: {p}")
    units = units_of(args.source)
    if not units:
        sys.exit("none of --source is a unit in config/units.toml")
    was_staged = staged_paths(args.source)
    history: dict[tuple[str, str], dict[int, float]] = {}

    try:
        for n in parse_states(args.states):
            for p in paths:
                apply_state(p, n, args.seed)
            build = subprocess.run(["ninja", "-C", str(ROOT)], capture_output=True, text=True)
            if build.returncode != 0:
                print(f"### n={n} BUILD FAILED\n{build.stdout[-2000:]}{build.stderr[-2000:]}")
                continue
            subprocess.run([sys.executable, "-m", "gruntz.match.fingerprints"],
                           cwd=str(ROOT), capture_output=True, text=True)
            banked = ""
            if not args.no_bank:
                _git("add", "--", *args.source)
                upd = subprocess.run([sys.executable, "-m", "gruntz.match.status", "update"],
                                     cwd=str(ROOT), capture_output=True, text=True)
                banked = next((l.strip() for l in (upd.stdout + upd.stderr).splitlines()
                               if "raised best" in l), "")
            for key, pct in sub_pcts(units).items():
                history.setdefault(key, {})[n] = pct
            print(f"### n={n}  {banked}", flush=True)
    finally:
        for p in paths:
            apply_state(p, 0, args.seed)
        _git("reset", "--quiet", "--", *args.source)
        for path in sorted(was_staged):
            _git("add", "--", path)

    print("\nunit             function                                   min    MAX  at-state")
    for key in sorted(history):
        seen = history[key]
        best = max(seen.values())
        worst = min(seen.values())
        at = [n for n in sorted(seen) if seen[n] == best]
        tag = "MOVES" if best - worst > 0.05 else "FLAT "
        print(f"{tag} {key[0]:15s} {key[1][:40]:40s} {worst:6.2f} {best:6.2f}  {at[:3]}")
    print("\nFLAT under a VARIED declaration-kind family is the real worklist: those need "
          "source work, not TU state (docs/patterns/tu-state-probe-family-decides-reachability.md).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
