#!/usr/bin/env python3
"""dup_compare.py - retail keeps a re-test cl5's peephole deleted for us.

cl5 drops the second of two comparisons on the same value only when the two are spelled
the SAME way. `if (n < 0x37) { for (i = n; i < 0x37; i++) … }` therefore loses the loop's
entry test, while retail - whose source spelled one of them `0x37 > i` - keeps both.
See docs/patterns/redundant-test-elimination-is-syntactic.md
(CPlay::BuildHelpReveal 0xd72c0 97.92 -> 100.00 EXACT on the operand swap).

The fingerprint: the TARGET has an adjacent conditional-branch pair with the same
mnemonic, the same destination and an identical preceding compare, which the BASE lacks -
and the base has fewer branches overall.

    python -m gruntz.audit.dup_compare              # the worklist
    python -m gruntz.audit.dup_compare --near 20    # only pairs within 20 bytes
    python -m gruntz.audit.dup_compare --max N      # exit 1 if the hit count exceeds N

**Read the byte gap between the two offsets.** Within ~20 bytes the tests are adjacent and
the operand swap is the lever. Far apart with calls in between it is the different
mechanism in redundant-sibling-guard-retest.md, whose fix is to DE-NEST the two guards.
"""
import argparse
import csv
import json
import re
import sys
from pathlib import Path

from gruntz.core.branches import branches, decode, first_bad, obj_paths

REPO = Path(__file__).resolve().parents[3]
REPORT = REPO / "build" / "objdiff" / "report.json"
GEN_NAMES = REPO / "build" / "gen" / "symbol_names.csv"


def rva_index():
    idx = {}
    if GEN_NAMES.is_file():
        for r in csv.DictReader(GEN_NAMES.open()):
            idx.setdefault(r["name"], r["rva"])
    return idx


def code_stop(insns, stop):
    """One past the last `ret` - where a switch's trailing jump table begins."""
    last = None
    for off, mn, _ in insns:
        if stop is not None and off >= stop:
            break
        if mn.startswith("ret"):
            last = off
    return None if last is None else last + 1


def _setter(insns, i):
    """The nearest preceding cmp/test for the branch at index `i`, or None if a call
    or another branch intervenes (which makes the two tests incomparable)."""
    j = i - 1
    while j >= 0:
        mn = insns[j][1]
        if mn.startswith("cmp") or mn.startswith("test"):
            return (mn, insns[j][2])
        if mn.startswith("j") or mn.startswith("call"):
            return None
        j -= 1
    return None


GPR = re.compile(r"%(?:e?(?:ax|bx|cx|dx|si|di|bp|sp)|[abcd][lh])")


def _clobbers(insns, i, j, regs):
    """True if any instruction in (i, j] writes one of `regs`.

    Without this the tool reports `if (p && p->next)` - two `testl %edx,%edx` with a
    `mov edx,[eax+0xc]` between them - as a re-test of one value. AT&T puts the
    destination last, so the write is the final operand; a `call` clobbers everything
    caller-saved, which is enough to disqualify the pair either way.
    """
    for k in range(i + 1, j + 1):
        mn, op = insns[k][1], insns[k][2]
        if mn.startswith("call"):
            return True
        if mn.startswith("cmp") or mn.startswith("test") or mn.startswith("push") \
                or mn.startswith("j"):
            continue
        dst = op.rsplit(",", 1)[-1].strip()
        if any(r in dst for r in regs):
            return True
    return False


def dup_pairs(insns, stop):
    """Adjacent branch pairs that re-test the SAME value to the same destination.

    "Same value", not merely the same compare text: the registers the compare reads
    must be untouched between the two branches.
    """
    brs = branches(insns, stop)
    at = {off: i for i, (off, _, _) in enumerate(insns)}
    out = []
    for k in range(len(brs) - 1):
        (o1, m1, t1), (o2, m2, t2) = brs[k], brs[k + 1]
        if m1 != m2 or t1 is None or t1 != t2:
            continue
        i1, i2 = at[o1], at[o2]
        s1, s2 = _setter(insns, i1), _setter(insns, i2)
        if not s1 or s1 != s2:
            continue
        regs = set(GPR.findall(s1[1]))
        if regs and _clobbers(insns, i1, i2, regs):
            continue
        out.append((o1, o2, s1))
    return out


def sweep():
    report = json.loads(REPORT.read_text())
    hits = []
    for u in report.get("units") or []:
        unit = u.get("name")
        sub = [f for f in (u.get("functions") or [])
               if float(f.get("fuzzy_match_percent") or 0.0) < 100.0]
        if not sub:
            continue
        bobj, tobj = obj_paths(unit)
        if not (bobj.is_file() and tobj.is_file()):
            continue
        bs, ts = decode(bobj), decode(tobj)
        for f in sub:
            name = f.get("name")
            bi, ti = bs.get(name), ts.get(name)
            if not bi or not ti:
                continue
            bstop, tstop = first_bad(bi), first_bad(ti)
            bcs = code_stop(bi, bstop) or bstop
            tcs = code_stop(ti, tstop) or tstop
            nb, nt = len(branches(bi, bcs)), len(branches(ti, tcs))
            if nb >= nt:
                continue
            tp, bp = dup_pairs(ti, tcs), dup_pairs(bi, bcs)
            if len(tp) <= len(bp):
                continue
            hits.append({"unit": unit, "name": name,
                         "pct": float(f.get("fuzzy_match_percent") or 0.0),
                         "br": (nb, nt), "pairs": tp, "base_pairs": len(bp)})
    return hits


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--near", type=int, default=None,
                    help="only pairs whose two branches are within N bytes")
    ap.add_argument("--max", type=int, default=None,
                    help="exit 1 if the hit count exceeds N (ratchet)")
    a = ap.parse_args()

    if not REPORT.is_file():
        print("dup-compare: no build/objdiff/report.json - run `gruntz build` first")
        return 2

    hits = sweep()
    if a.near is not None:
        for h in hits:
            h["pairs"] = [p for p in h["pairs"] if p[1] - p[0] <= a.near]
        hits = [h for h in hits if h["pairs"]]
    rvas = rva_index()

    print("dup-compare: %d function(s) where retail re-tests a value cl5 elided for us"
          % len(hits))
    print("   adjacent (<~20 B apart) => reverse ONE comparison's operands; far apart "
          "with calls between => de-nest the guards (redundant-sibling-guard-retest.md)")
    for h in sorted(hits, key=lambda h: -h["pct"]):
        print("   %6.2f%%  %-14s %s  %s"
              % (h["pct"], h["unit"], rvas.get(h["name"], "?"), h["name"]))
        print("            branches %d->%d, dup pairs %d->%d:  %s"
              % (h["br"][0], h["br"][1], h["base_pairs"], len(h["pairs"]),
                 "  ".join("0x%x/0x%x (+%d) %s %s" % (o1, o2, o2 - o1, s[0], s[1])
                           for o1, o2, s in h["pairs"][:3])))

    if a.max is not None and len(hits) > a.max:
        print("dup-compare: %d hits exceeds the %d ratchet" % (len(hits), a.max))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
