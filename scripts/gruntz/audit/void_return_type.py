#!/usr/bin/env python3
"""void_return_type.py - functions we declare `void` that retail returns a value from.

Two independent signatures, both read out of the objdiff object pairs:

**CONSTANT** - an `xor eax,eax` / `mov eax,<imm>` immediately before a retail `ret`
(only pops and the stack adjust in between). cl5 never materialises a value for a
genuine `void`, so its presence proves the declared return type is wrong.

**GUARD** - retail keeps a bare `ret` that a conditional jump skips by one byte
(`test eax,eax / jne <+1> / ret`) while our base cross-jumps the same early-out onto
the shared epilogue, so the base has FEWER `ret`s. This is the subcase with no
constant anywhere: the success path is `return <callee result>`, so an `int` function
disassembles exactly like a `void` one and only the `ret` count gives it away.
See docs/patterns/void-return-collapses-the-guard-ret.md - four functions went to
EXACT on the signature flip alone.

    python -m gruntz.audit.void_return_type            # both, worklist
    python -m gruntz.audit.void_return_type --guard    # only the no-constant subcase
    python -m gruntz.audit.void_return_type --constant # only the eax-materialised subcase
    python -m gruntz.audit.void_return_type --max N    # exit 1 if the hit count exceeds N
"""
import argparse
import csv
import json
import re
import sys
from pathlib import Path

from gruntz.core.branches import decode, first_bad, obj_paths, rets

REPO = Path(__file__).resolve().parents[3]
REPORT = REPO / "build" / "objdiff" / "report.json"
GEN_NAMES = REPO / "build" / "gen" / "symbol_names.csv"

# `void` in the MSVC mangling: the return-type letter directly after the
# function-encoding's calling-convention letter.
VOID = re.compile(r"@@(?:[A-Z]+)?(?:QAE|UAE|AAE|IAE|MAE|SA|YA|QBE|UBE|ABE)X")


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


def constant_rets(insns, stop):
    """`ret`s whose value was materialised in eax - walking back over the epilogue."""
    out = []
    for i, (off, mn, _) in enumerate(insns):
        if not mn.startswith("ret") or (stop is not None and off >= stop):
            continue
        j = i - 1
        while j >= 0:
            m, o = insns[j][1], insns[j][2]
            if m.startswith("pop") or m == "leave" or \
               (m.startswith("add") and "%esp" in o):
                j -= 1
                continue
            break
        if j < 0:
            continue
        m, o = insns[j][1], insns[j][2]
        if (m == "xorl" and o == "%eax, %eax") or \
           (m.startswith("mov") and o.startswith("$") and o.endswith("%eax")):
            out.append((off, m, o))
    return out


def guard_rets(insns, stop):
    """`ret`s the preceding instruction is a conditional jump OVER - retail's
    inline per-guard epilogue."""
    out = []
    for i, (off, mn, _) in enumerate(insns):
        if not mn.startswith("ret") or (stop is not None and off >= stop):
            continue
        if i and insns[i - 1][1].startswith("j") and insns[i - 1][1] != "jmp":
            out.append(off)
    return out


def sweep():
    report = json.loads(REPORT.read_text())
    hits = []
    for u in report.get("units") or []:
        unit = u.get("name")
        fns = [f for f in (u.get("functions") or []) if VOID.search(f.get("name") or "")]
        if not fns:
            continue
        bobj, tobj = obj_paths(unit)
        if not (bobj.is_file() and tobj.is_file()):
            continue
        bs, ts = decode(bobj), decode(tobj)
        for f in fns:
            name = f.get("name")
            bi, ti = bs.get(name), ts.get(name)
            if not bi or not ti:
                continue
            bstop, tstop = first_bad(bi), first_bad(ti)
            bcs = code_stop(bi, bstop) or bstop
            tcs = code_stop(ti, tstop) or tstop
            rb, rt = rets(bi, bcs), rets(ti, tcs)
            const = constant_rets(ti, tcs)
            guard = guard_rets(ti, tcs) if rt > rb else []
            if not const and not guard:
                continue
            hits.append({
                "unit": unit, "name": name,
                "pct": float(f.get("fuzzy_match_percent") or 0.0),
                "rets": (rb, rt), "const": const, "guard": guard,
                "kind": "CONSTANT" if const else "GUARD",
            })
    return hits


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--guard", action="store_true", help="only the no-constant subcase")
    ap.add_argument("--constant", action="store_true",
                    help="only the eax-materialised subcase")
    ap.add_argument("--max", type=int, default=None,
                    help="exit 1 if the hit count exceeds N (ratchet)")
    a = ap.parse_args()

    if not REPORT.is_file():
        print("void-return-type: no build/objdiff/report.json - run `gruntz build` first")
        return 2

    hits = sweep()
    if a.guard:
        hits = [h for h in hits if h["kind"] == "GUARD"]
    if a.constant:
        hits = [h for h in hits if h["kind"] == "CONSTANT"]
    rvas = rva_index()

    print("void-return-type: %d function(s) declared `void` that retail returns a "
          "value from  |  CONSTANT %d  GUARD %d"
          % (len(hits), sum(1 for h in hits if h["kind"] == "CONSTANT"),
             sum(1 for h in hits if h["kind"] == "GUARD")))
    print("   flip the declaration to `i32` and spell each guard `return 0;` - see "
          "docs/patterns/void-return-collapses-the-guard-ret.md")
    for h in sorted(hits, key=lambda h: (h["kind"], -h["pct"])):
        print("   %-8s %6.2f%%  %-12s %s  %s"
              % (h["kind"], h["pct"], h["unit"],
                 rvas.get(h["name"], "?"), h["name"]))
        detail = ("eax set at " + ", ".join("0x%x (%s %s)" % (o, m, op)
                                            for o, m, op in h["const"][:3])
                  if h["const"] else
                  "guard ret at " + ", ".join("0x%x" % o for o in h["guard"][:4]))
        print("              rets %d->%d:  %s" % (h["rets"][0], h["rets"][1], detail))

    if a.max is not None and len(hits) > a.max:
        print("void-return-type: %d hits exceeds the %d ratchet" % (len(hits), a.max))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
