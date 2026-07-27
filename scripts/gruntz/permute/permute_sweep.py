#!/usr/bin/env python3
"""permute_sweep.py - run the permuter over a whole unit, TOP-DOWN in source order.

MSVC /O2 codegen can interact across a translation unit, so a function must be
permuted only AFTER the ones above it in the source are locked in - otherwise a later
win can be invalidated when an earlier function's codegen shifts. This discovers a
unit's <100% functions in SOURCE order (top of the .cpp downwards) and runs
`gruntz.permute.permute` on each in turn, accumulating wins in place.

Usage (inside `nix develop .#build`, from the repo/worktree root; run `gruntz build`
first so build/objdiff/{target,report.json} are current):
    python3 -m gruntz.permute.permute_sweep <unit> [iters]
  e.g.
    python3 -m gruntz.permute.permute_sweep gamelevel 60
"""
import subprocess, re, json, sys, os, tomllib, pathlib

def _root():
    """The repo/worktree root (nearest ancestor holding flake.nix), else $REPO/cwd."""
    cwd = pathlib.Path.cwd()
    return next((str(p) for p in [cwd, *cwd.parents] if (p / "flake.nix").exists()),
                os.environ.get("REPO") or str(cwd))


# NB: the argv parse and the chdir live in main(), NOT at module scope. A module that
# sys.exit()s and chdir()s on IMPORT cannot be unit-tested, and that is why the
# `_pcts` default-to-100.0 bug below went unnoticed - see gate_selftest
# TestOmittedZeroFuzzyPercent.


def _src_of(unit):
    with open("config/units.toml", "rb") as fh:
        cfg = tomllib.load(fh)
    for u in cfg.get("unit", []):
        if u.get("unit") == unit:
            return u["source"]
    sys.exit(f"unit '{unit}' not found in config/units.toml")


def _pcts(unit):
    """mangled name -> fuzzy_match_percent for `unit`, from build/objdiff/report.json.

    A missing key is 0.0, NOT 100.0: objdiff omits the field for a function scored at
    exactly 0.0% (serde skips the f32 default). Defaulting it to 100.0 made this sweep
    treat a 0%-matching function as already perfect and skip it - see
    gruntz.core.report."""
    d = json.load(open("build/objdiff/report.json"))
    for u in d.get("units", []):
        if u.get("name") == unit:
            return {f["name"]: float(f.get("fuzzy_match_percent") or 0.0)
                    for f in (u.get("functions") or []) if f.get("name")}
    sys.exit(f"unit '{unit}' not in report.json - run `gruntz build`.")


def _ordered(unit, src):
    """(sym, pct) for `unit`'s <100% funcs, in SOURCE order (RVA markers top-down)."""
    rva2sym = {}
    for line in open("build/gen/symbol_names.csv"):
        p = line.rstrip("\n").split(",")
        if len(p) >= 5 and p[2] == unit and p[4] == "func":
            rva2sym[int(p[0], 16)] = p[1]
    pct = _pcts(unit)
    out = []
    for ln in open(src):
        m = re.match(r'RVA\(0x([0-9a-f]{8})', ln)
        if not m:
            continue
        sym = rva2sym.get(int(m.group(1), 16))
        if sym is None or sym not in pct:
            continue  # not measured in this unit's report - nothing to climb toward
        if pct[sym] < 99.995:
            out.append((sym, pct[sym]))
    return out


def main(argv=None):
    argv = sys.argv[1:] if argv is None else list(argv)
    if not argv:
        sys.exit("usage: python3 -m gruntz.permute.permute_sweep <unit> [iters]")
    unit, iters = argv[0], (argv[1] if len(argv) > 1 else "60")
    os.chdir(_root())
    src = _src_of(unit)
    funcs = _ordered(unit, src)
    print(f"sweep {unit} ({src}): {len(funcs)} functions <100%, top-down", flush=True)
    wins = []
    for sym, p0 in funcs:
        short = re.sub(r'^\?([A-Za-z0-9_]+)@.*', r'\1', sym)
        r = subprocess.run(["python3", "-m", "gruntz.permute.permute", src, unit, sym, iters],
                           capture_output=True, text=True)
        m = re.search(r'FINAL ([0-9.]+)', r.stdout)
        p1 = float(m.group(1)) if m else p0
        gain = f"  +{p1 - p0:.2f}" if p1 > p0 + 1e-6 else ""
        print(f"  {p0:6.2f} -> {p1:6.2f}  {short}{gain}", flush=True)
        if p1 > p0 + 1e-6:
            wins.append((short, p0, p1))
    print(f"sweep done: {len(wins)}/{len(funcs)} improved", flush=True)
    for s, a, b in wins:
        print(f"  WIN {s}: {a:.2f} -> {b:.2f}", flush=True)


if __name__ == "__main__":
    main()
