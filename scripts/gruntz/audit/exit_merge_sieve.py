#!/usr/bin/env python3
"""exit_merge_sieve.py - the whole-tree sweep for the EXIT-BLOCK MERGE lever.

cl 5.0 emits every `if (cond) return 0;` as its OWN inline exit block, but routes
several exits into ONE shared block when the source sends them to a common
destination. WHICH source construct is used decides how far the merging goes:

  * separate `if (...) return 0;`  -> zero merging, one copy per site
  * `goto fail;` + a trailing `fail: return 0;`
                                   -> the goto sites share ONE block; every OTHER
                                      `return 0` in the function keeps its own copy
  * `||` / `&&` / a positive-gate nest guarding a `return 0`
                                   -> ALL same-valued returns in the whole function
                                      collapse into ONE sunk block

So a base/target ret-count mismatch names the lever directly:

  base rets > target rets   DUP-EXIT   we duplicate what retail shares
                                       -> send the leading guards to `goto fail`
  base rets < target rets   OVER-MERGE we share what retail duplicates
                                       -> a `||`/`&&` guard is collapsing the
                                          function's other `return 0`s

Ret counts are read out of the objdiff object pair, where nothing is masked, via
`gruntz.core.branches` (same decode/table-cut the jcc sieve uses, so a switch's
trailing jump table is not miscounted as code).

    python -m gruntz.audit.exit_merge_sieve            # both buckets, worst % first
    python -m gruntz.audit.exit_merge_sieve --dup      # DUP-EXIT only (the fixable one)
    python -m gruntz.audit.exit_merge_sieve --unit ddrawsubmgr
    python -m gruntz.audit.exit_merge_sieve --summary

See docs/patterns/goto-fail-shares-one-exit-block.md.
"""
import argparse
import collections
import csv
import json
import sys
from pathlib import Path

from gruntz.core.branches import code_stop, decode, obj_paths, rets

REPO = Path(__file__).resolve().parents[3]
REPORT = REPO / "build" / "objdiff" / "report.json"
GEN_NAMES = REPO / "build" / "gen" / "symbol_names.csv"


def rva_index():
    idx = {}
    if GEN_NAMES.is_file():
        for r in csv.DictReader(GEN_NAMES.open()):
            idx.setdefault(r["name"], r["rva"])
    return idx


def sieve(unit_filter=None):
    report = json.loads(REPORT.read_text())
    hits, stats = [], collections.Counter()
    for u in report.get("units") or []:
        unit = u.get("name")
        if unit_filter and unit != unit_filter:
            continue
        sub = [f for f in (u.get("functions") or [])
               if float(f.get("fuzzy_match_percent") or 0.0) < 100.0]
        if not sub:
            continue
        stats["sub100"] += len(sub)
        bobj, tobj = obj_paths(unit)
        if not (bobj.is_file() and tobj.is_file()):
            stats["no-objs"] += len(sub)
            continue
        bs, ts = decode(bobj), decode(tobj)
        for f in sub:
            name = f.get("name")
            bi, ti = bs.get(name), ts.get(name)
            if not bi or not ti:
                stats["unpaired"] += 1
                continue
            br = rets(bi, code_stop(bi))
            tr = rets(ti, code_stop(ti))
            if br == tr:
                stats["equal"] += 1
                continue
            kind = "DUP-EXIT" if br > tr else "OVER-MERGE"
            stats[kind] += 1
            hits.append((kind, unit, name,
                         float(f.get("fuzzy_match_percent") or 0.0), br, tr))
    return hits, stats


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--summary", action="store_true", help="counts only")
    ap.add_argument("--dup", action="store_true", help="DUP-EXIT bucket only")
    ap.add_argument("--over", action="store_true", help="OVER-MERGE bucket only")
    ap.add_argument("--unit", help="restrict to one unit")
    ap.add_argument("--max", type=int, default=None,
                    help="exit 1 if the hit count exceeds N (ratchet)")
    a = ap.parse_args()

    if not REPORT.is_file():
        print("exit-merge-sieve: no build/objdiff/report.json - run `gruntz build` first")
        return 2

    hits, stats = sieve(a.unit)
    print("exit-merge sieve: %d hits over %d sub-100%% functions  |  DUP-EXIT %d "
          "(goto-fail lever)  OVER-MERGE %d (a ||/&& guard is collapsing the others)"
          % (len(hits), stats["sub100"], stats["DUP-EXIT"], stats["OVER-MERGE"]))
    print("   not this signal: %d have equal ret counts" % stats["equal"])
    if stats["unpaired"] or stats["no-objs"]:
        print("   uncomparable: %d symbol missing on one side  |  %d unit has no obj pair"
              % (stats["unpaired"], stats["no-objs"]))

    if not a.summary:
        rvas = rva_index()
        want = None
        if a.dup:
            want = "DUP-EXIT"
        elif a.over:
            want = "OVER-MERGE"
        shown = [h for h in hits if want is None or h[0] == want]
        print("\nWorklist - DUP-EXIT first (that is the one the goto-fail lever fixes):")
        listed = 0
        for kind, unit, name, pct, br, tr in sorted(
                shown, key=lambda h: (h[0] != "DUP-EXIT", -h[3])):
            print("   %-10s %6.2f%%  rets %2d->%-2d  %-22s @%s  %s"
                  % (kind, pct, br, tr, unit, rvas.get(name, "?"), name))
            listed += 1
        assert listed == len(shown), "listing (%d) != hits (%d)" % (listed, len(shown))

    if a.max is not None and len(hits) > a.max:
        print("exit-merge-sieve: %d hits exceeds the %d ratchet" % (len(hits), a.max))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
