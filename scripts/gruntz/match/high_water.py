#!/usr/bin/env python3
"""The MAX-% high-water ratchet (config/match-max.tsv), with a DEGENERACY GUARD.

WHY THIS MODULE EXISTS
----------------------
The campaign gates on MAX fuzzy% ("a current-% dip never matters - MAX preserves your
best result forever"), so this one number is the thing that says whether the tree got
worse. It was computed inline in `gruntz build` as:

    prev = float(maxf.read_text().split()[0]) if maxf.is_file() else cur
    peak = max(prev, cur)                      # <- accepts ANY reading, irreversibly
    maxf.write_text(f"{peak:.4f}\\n")

Two properties make that unsafe, and together they killed the metric:

  * it accepts a reading from a DEGENERATE report. objdiff computes fuzzy% as
    matched/total; a build whose objs are missing/empty reports total_code == 0, and
    0/0 is published as **100%**. Nothing checked that the report described the same
    tree as the last one.
  * `max()` is IRREVERSIBLE. One bad reading is absorbed as the peak forever, and
    since the file is COMMITTED it propagates to every worktree and every agent.

Both fired. Measured from the git history of config/match-max.tsv: the peak climbed
honestly across ~70 commits (64.0300 -> 72.7691), then jumped

    72.7691 -> 100.0000   in bb4d94cef ("reloc: canonical g_gameReg ... fold")

and stuck. From that commit until now every build printed "MAX %: 100.00% (high-water)
- unchanged" against a real 73.17% - i.e. the campaign's headline gate became a
constant that can never be raised, can never signal a NEW HIGH, and can never detect a
regression. It was a green light, not a check.

THE GUARD
---------
A percentage is only comparable to another percentage measured over the SAME
population, so the peak is stored WITH the scale it was measured at:

    73.1693<TAB>3975          peak_pct <TAB> total_functions

`total_functions` is objdiff's own denominator. A new reading may raise the peak only
when it was taken over a comparable population (>= MIN_SCALE_RATIO of the recorded
scale, and non-empty). A degenerate/partial report is REFUSED - the peak is kept, the
file is NOT rewritten, and the reason is reported. Refusing to raise is safe: if the
refusal is a false alarm the next good build raises the peak anyway. Silently
absorbing a bad reading is NOT safe - that is what happened.

Back-compat: readers doing `.read_text().split()[0]` still get the peak. A legacy
one-token file has an UNKNOWN scale, so the first run adopts the current scale without
raising (it cannot validate what it cannot compare).

A deliberate re-baseline (the only way the peak ever goes DOWN, e.g. after the poison
above) is `--reset`, mirroring `cleanliness --update`.

    python -m gruntz.match.high_water              # show the recorded peak + scale
    python -m gruntz.match.high_water --reset      # re-baseline from build/objdiff/report.json
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
MAXFILE = REPO / "config" / "match-max.tsv"
REPORT = REPO / "build" / "objdiff" / "report.json"

# A reading is comparable when its denominator is at least this fraction of the one the
# peak was measured at. Adding units RAISES total_functions, so only a collapse trips
# this - and a collapse is exactly the partial/failed build we must not learn from.
MIN_SCALE_RATIO = 0.9
EPS = 0.005


def read(path: Path = MAXFILE):
    """-> (peak, scale) from `path`. peak/scale are None when absent/unparseable.

    scale is None for a legacy one-token file: the peak is known, the population it was
    measured over is not, so it cannot be validated against (only adopted).
    """
    if not path.is_file():
        return None, None
    parts = path.read_text().split()
    if not parts:
        return None, None
    try:
        peak = float(parts[0])
    except ValueError:
        return None, None
    scale = None
    if len(parts) > 1:
        try:
            scale = int(float(parts[1]))
        except ValueError:
            scale = None
    return peak, scale


def write(peak: float, scale: int, path: Path = MAXFILE) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(f"{peak:.4f}\t{scale}\n")


def update(cur: float, cur_scale: int, path: Path = MAXFILE):
    """Ratchet `path` with a fresh reading. -> (peak, note, wrote).

    `peak` is the value to REPORT, `wrote` says whether the file changed. A reading from
    a non-comparable population never raises the peak (see the module docstring).
    """
    prev, prev_scale = read(path)

    # A reading with no population behind it is not a measurement. (0/0 -> objdiff
    # publishes 100% - the exact shape that poisoned this file.)
    if not cur_scale or cur_scale <= 0:
        return (prev if prev is not None else 0.0,
                f"REFUSED: report has no functions (total_functions={cur_scale}) - a "
                f"0/0 report publishes 100%; not a measurement of this tree", False)

    if prev is None:
        write(cur, cur_scale, path)
        return cur, f"first reading (scale {cur_scale})", True

    if prev_scale is None:  # legacy 1-token file: adopt the scale, don't trust a raise
        write(prev, cur_scale, path)
        return prev, (f"recorded peak {prev:.2f}% had no scale; adopted the current "
                      f"scale ({cur_scale}) - next build can compare"), True

    if cur_scale < prev_scale * MIN_SCALE_RATIO:
        return prev, (f"REFUSED: this report covers {cur_scale} functions vs the peak's "
                      f"{prev_scale} (< {MIN_SCALE_RATIO:.0%}) - a partial/failed build "
                      f"is not comparable; peak kept, file unchanged"), False

    if cur > prev + EPS:
        write(cur, cur_scale, path)
        return cur, f"NEW HIGH (+{cur - prev:.2f})", True

    if cur_scale != prev_scale:      # same peak, population moved -> record the scale
        write(prev, cur_scale, path)
        return prev, ("unchanged" if cur > prev - EPS else
                      f"unchanged; current {cur:.2f}% is below the peak"), True

    return prev, ("unchanged" if cur > prev - EPS else
                  f"unchanged; current {cur:.2f}% is a transient structural dip "
                  "(inlined members re-match as their vtable-TU coverage lands)"), False


def _cur_from_report(path: Path = REPORT):
    m = json.loads(path.read_text()).get("measures", {})
    return float(m.get("fuzzy_match_percent", 0.0) or 0.0), int(m.get("total_functions", 0) or 0)


def main() -> int:
    peak, scale = read()
    if "--reset" in sys.argv:
        if not REPORT.is_file():
            print(f"high-water: {REPORT} missing - run a build first", file=sys.stderr)
            return 1
        cur, cur_scale = _cur_from_report()
        if cur_scale <= 0:
            print("high-water: REFUSING to reset from a report with 0 functions "
                  "(a 0/0 report publishes 100%)", file=sys.stderr)
            return 1
        write(cur, cur_scale)
        print(f"high-water: re-baselined {peak} -> {cur:.4f} (scale {cur_scale})")
        return 0
    print(f"high-water: peak {peak}% over {scale} functions ({MAXFILE.relative_to(REPO)})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
