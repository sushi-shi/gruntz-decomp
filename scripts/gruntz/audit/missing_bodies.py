#!/usr/bin/env python3
"""missing_bodies.py - functions whose body is largely UNWRITTEN, not walled.

A lane re-derived six `@early-stop` notes and none was correct; three of them described
a codegen wall on a function whose body was 75-95% MISSING. `CTileTriggerLogic::Tick`
carries a "regalloc/scheduling wall" note and has 64 instructions against retail's 1166.
Nothing about register allocation is relevant to a function that has not been written.

The signal is the instruction-count ratio base:target. Two filters make it trustworthy:

  * SCORE. A high-scoring function with a low ratio is a DECODE artifact, not a missing
    body - `_inflate_blocks` reads 29 vs 1121 at 98.66%. So require a low score too.
  * TRUNCATION. llvm-objdump disassembles linearly and stops at a jump table embedded in
    `.text` after an indirect `jmp`, so on switch-heavy functions the count is a prefix
    and lies. `first_bad()` finds that point; rows where EITHER side truncated are
    dropped rather than reported with a bogus ratio. (This is the caveat that made the
    first run of this triage wrong - it listed zlib's inflate at 98% as "missing".)

What survives both filters is a reconstruction worklist: write the body, do not permute.

    python -m gruntz.audit.missing_bodies              # the worklist
    python -m gruntz.audit.missing_bodies --max-score 40 --max-ratio 0.4
"""
import argparse
import json
import sys
from pathlib import Path

from gruntz.core.branches import decode, first_bad, obj_paths

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
REPORT = REPO / "build" / "objdiff" / "report.json"


def scan(max_score=60.0, max_ratio=0.6, min_insns=60):
    rep = json.loads(REPORT.read_text())
    rows, skipped = [], 0
    for u in rep.get("units") or []:
        sub = [f for f in (u.get("functions") or [])
               if float(f.get("fuzzy_match_percent") or 0.0) < max_score]
        if not sub:
            continue
        b, t = obj_paths(u["name"])
        if not (b.is_file() and t.is_file()):
            continue
        try:
            bs, ts = decode(b), decode(t)
        except Exception:
            continue
        for f in sub:
            bi, ti = bs.get(f["name"]), ts.get(f["name"])
            if not bi or not ti or len(ti) < min_insns:
                continue
            if first_bad(bi) is not None or first_bad(ti) is not None:
                skipped += 1          # jump-table truncation: the count is a prefix
                continue
            r = len(bi) / len(ti)
            if r < max_ratio:
                rows.append((float(f.get("fuzzy_match_percent") or 0.0), r,
                             len(bi), len(ti), u["name"], f["name"]))
    rows.sort()
    return rows, skipped


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--max-score", type=float, default=60.0)
    ap.add_argument("--max-ratio", type=float, default=0.6)
    ap.add_argument("--top", type=int, default=40)
    a = ap.parse_args(argv)

    rows, skipped = scan(a.max_score, a.max_ratio)
    print(f"missing bodies: {len(rows)} function(s) below {a.max_score:g}% whose compiled "
          f"body is under {a.max_ratio:g}x retail's instruction count")
    print(f"  ({skipped} row(s) skipped - a jump table in .text truncated one side's "
          f"disassembly, so its count is only a prefix)\n")
    print(f"  {'score':>7} {'ratio':>6} {'base':>6} {'retail':>7}  unit / function")
    for pc, r, nb, nt, un, n in rows[:a.top]:
        print(f"  {pc:6.2f}% {r:6.2f} {nb:6} {nt:7}  {un}/{n}")
    if len(rows) > a.top:
        print(f"  ... and {len(rows) - a.top} more (--top N)")
    print("\nThese want RECONSTRUCTION, not permutation. A wall note on one of these rows "
          "is describing codegen for code that does not exist yet.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
