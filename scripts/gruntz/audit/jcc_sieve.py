#!/usr/bin/env python3
"""jcc_sieve.py - the whole-tree sweep for CONTROL-FLOW bugs `--diff` prints as identical.

`gruntz sema disasm --diff` masks address operands, which also hides intra-function
branch DISPLACEMENTS: a `je` that goes to a different basic block on the two sides prints
`je <tgt>` on both and compares EQUAL. This sweeps every sub-100% function's ordered
branch sequence, out of the objdiff object pairs where nothing is masked, and buckets
what it finds SIGNEDNESS / POLARITY / OTHER / TOPOLOGY.

**The comparison itself lives in `gruntz.core.branches`** - read that module's docstring
for the mechanism, the five ways a row is NOT this signal, and why `--diff`'s masking
must not be "fixed". This file is only the driver: pick the functions, render the
worklist. `gruntz sema disasm <rva> --branches` is the same comparison for one function.

    python -m gruntz.audit.jcc_sieve                # the worklist, SIGNEDNESS first
    python -m gruntz.audit.jcc_sieve --summary      # counts only
    python -m gruntz.audit.jcc_sieve --class SIGNEDNESS   # one bucket
    python -m gruntz.audit.jcc_sieve --unit gamelevel     # one unit
    python -m gruntz.audit.jcc_sieve --max N        # exit 1 if the hit count exceeds N
"""
import argparse
import collections
import csv
import json
import sys
from pathlib import Path

from gruntz.core.branches import compare, decode, obj_paths

REPO = Path(__file__).resolve().parents[3]
REPORT = REPO / "build" / "objdiff" / "report.json"
GEN_NAMES = REPO / "build" / "gen" / "symbol_names.csv"


def rva_index():
    """mangled name -> retail RVA, so a hit row is directly actionable."""
    idx = {}
    if GEN_NAMES.is_file():
        for r in csv.DictReader(GEN_NAMES.open()):
            idx.setdefault(r["name"], r["rva"])
    return idx


def render(res):
    """The one-line detail for a hit, in the shape the worklist prints."""
    if res["kind"] == "TOPOLOGY":
        return "  ".join("#%d ->blk%s not blk%s" % (i, y, x) for i, x, y in res["rows"])
    same = set(res.get("same_dest") or ())
    # `=dest` marks a flip whose two sides land on the SAME symbolic destination: the
    # condition is inverted, not the block layout. Those are the rows worth opening.
    arm = set(res.get("arm_selector") or ())
    # `=arm` = same block index but the two destinations begin with DIFFERENT code:
    # an if/else or ternary whose arms are swapped, so the flip cancels. Not a defect.
    out = ["#%d %s->%s%s" % (i, a, b,
                             "=dest" if i in same else "=arm" if i in arm else "")
           for i, a, b in res["rows"]]
    # `=mnem` rows: the mnemonic AGREES but the target moved - invisible before, and on
    # at least one function it was the row that explained the whole divergence.
    out += ["#%d =mnem ->blk%s not blk%s" % (i, y, x)
            for i, x, y in (res.get("moved_same_mnem") or ())]
    return "  ".join(out)


def sieve(unit_filter=None, max_flips=4):
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
            res = compare(bi, ti, max_flips)
            # A truncated stream is not a clean result, so the SWEEP excludes it rather
            # than reporting a prefix as if it were the whole function. The interactive
            # `--branches` view prints the prefix WITH the truncation notice instead.
            if any(t is not None for t in res["trunc"]):
                stats["skip"] += 1
                continue
            if res["status"] not in ("flips", "topology"):
                stats[res["status"]] += 1
                continue
            if res.get("je_jne_only"):
                stats["je/jne only"] += 1
            stats[res["kind"]] += 1
            if res.get("same_dest"):
                stats["same-dest (predicate)"] += 1
            if res.get("moved_same_mnem"):
                stats["=mnem target moved"] += 1
            if res.get("arm_selector"):
                stats["=arm (swapped arms, not a defect)"] += 1
            hits.append((res["kind"], unit, name,
                         float(f.get("fuzzy_match_percent") or 0.0), res["nbr"],
                         render(res), res["rets"][0], res["rets"][1]))
    return hits, stats


ORDER = {"SIGNEDNESS": 0, "TOPOLOGY": 1, "OTHER": 2, "POLARITY": 3}


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--summary", action="store_true", help="counts only")
    ap.add_argument("--class", dest="klass", help="list one bucket only")
    ap.add_argument("--unit", help="restrict to one unit")
    ap.add_argument("--max-flips", type=int, default=4,
                    help="most differing mnemonics still treated as this signal (4)")
    ap.add_argument("--max", type=int, default=None,
                    help="exit 1 if the hit count exceeds N (ratchet)")
    a = ap.parse_args()

    if not REPORT.is_file():
        print("jcc-sieve: no build/objdiff/report.json - run `gruntz build` first")
        return 2

    hits, stats = sieve(a.unit, a.max_flips)
    per = collections.Counter(h[0] for h in hits)
    print("jcc sieve: %d hits over %d sub-100%% functions  |  SIGNEDNESS %d  "
          "TOPOLOGY %d  OTHER %d  POLARITY %d (of which %d are pure je/jne)"
          % (len(hits), stats["sub100"], per["SIGNEDNESS"], per["TOPOLOGY"],
             per["OTHER"], per["POLARITY"], stats["je/jne only"]))
    print("   not this signal: %d differ in branch COUNT (structural)  |  %d >%d flips  "
          "|  %d branchless  |  %d identical branch sequence"
          % (stats["struct"], stats["many-flips"], a.max_flips,
             stats["no-branches"], stats["clean"]))
    if stats["skip"] or stats["unpaired"] or stats["no-objs"]:
        print("   uncomparable: %d truncated stream (jump-table data in .text - see the "
              "reliable prefix with `gruntz sema disasm <rva> --branches`)  |  %d symbol "
              "missing on one side  |  %d unit has no obj pair"
              % (stats["skip"], stats["unpaired"], stats["no-objs"]))

    if not a.summary:
        rvas = rva_index()
        print("\nWorklist - SIGNEDNESS first (a signed/unsigned twin is a real type bug):")
        listed = 0
        shown = [h for h in hits if not a.klass or h[0] == a.klass]
        for kind, unit, name, pct, nbr, detail, br, tr in sorted(
                shown, key=lambda h: (ORDER[h[0]], -h[3])):
            print("   %-10s %6.2f%%  %-10s %s  @%s" %
                  (kind, pct, unit, rvas.get(name, "?"), name))
            print("              %d branches, rets %d->%d%s:  %s"
                  % (nbr, br, tr, "  DUP-EXIT" if br > tr else "", detail))
            listed += 1
        # The listing IS the worklist; capping it would hide work while the header
        # still claimed the full count (that bug shipped once in cast_ledger).
        assert listed == len(shown), "listing (%d) != hits (%d)" % (listed, len(shown))

    if a.max is not None and len(hits) > a.max:
        print("jcc-sieve: %d hits exceeds the %d ratchet" % (len(hits), a.max))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
