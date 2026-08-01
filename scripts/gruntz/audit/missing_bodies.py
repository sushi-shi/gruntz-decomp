#!/usr/bin/env python3
"""missing_bodies.py - functions whose body is largely UNWRITTEN, not walled.

A lane re-derived six `@early-stop` notes and none was correct; three of them described
a codegen wall on a function whose body was 75-95% MISSING. `CTileTriggerLogic::Tick`
carries a "regalloc/scheduling wall" note and has 64 instructions against retail's 1166.
Nothing about register allocation is relevant to a function that has not been written.

The signal is the instruction-count ratio base:target. Two filters make it trustworthy:

  * SCORE. A high-scoring function with a low ratio is a DECODE artifact, not a missing
    body - `_inflate_blocks` reads 29 vs 1121 at 98.66%. So require a low score too.
  * TRUNCATION. llvm-objdump disassembles linearly into jump-table data after an indirect
    `jmp`. Some entries decode as valid instructions before the first `(bad)`, so the
    invalid instruction itself is NOT a safe prefix boundary. A truncated base is
    dropped. A truncated retail side is reported separately only when the full base is
    already smaller than the real-code prefix ending at the last indirect jump before
    the bad data; the true ratio can only be lower. (This caveat made the first run of
    this triage wrong - it listed zlib's inflate at 98% as "missing".)

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
INDIRECT_JMPS = frozenset(("jmp", "jmpl", "jmpw"))


def reliable_prefix_len(insns, bad_offset):
    """Instruction count through the switch jump that precedes bad table data."""
    candidates = [i + 1 for i, (off, mnemonic, operand) in enumerate(insns)
                  if off < bad_offset and mnemonic in INDIRECT_JMPS
                  and operand.startswith("*")]
    return candidates[-1] if candidates else 0


def scan(max_score=60.0, max_ratio=0.6, min_insns=60, include_prefix=False):
    rep = json.loads(REPORT.read_text())
    rows, prefix_rows, skipped = [], [], []
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
            if not bi or not ti:
                continue
            bstop, tstop = first_bad(bi), first_bad(ti)
            if bstop is not None:
                skipped.append((float(f.get("fuzzy_match_percent") or 0.0), bstop,
                                len(bi), u["name"], f["name"]))
                continue
            if tstop is not None:
                prefix_len = reliable_prefix_len(ti, tstop)
                if prefix_len >= min_insns:
                    r = len(bi) / prefix_len
                    if r < max_ratio:
                        prefix_rows.append((float(f.get("fuzzy_match_percent") or 0.0),
                                            r, len(bi), prefix_len, u["name"], f["name"]))
                continue
            if len(ti) < min_insns:
                continue
            r = len(bi) / len(ti)
            if r < max_ratio:
                rows.append((float(f.get("fuzzy_match_percent") or 0.0), r,
                             len(bi), len(ti), u["name"], f["name"]))
    rows.sort()
    prefix_rows.sort()
    skipped.sort()
    if include_prefix:
        return rows, prefix_rows, skipped
    return rows, skipped


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--max-score", type=float, default=60.0)
    ap.add_argument("--max-ratio", type=float, default=0.6)
    ap.add_argument("--top", type=int, default=40)
    a = ap.parse_args(argv)

    rows, prefix_rows, skipped = scan(a.max_score, a.max_ratio, include_prefix=True)
    print(f"missing bodies: {len(rows) + len(prefix_rows)} function(s) below "
          f"{a.max_score:g}% whose compiled body is under {a.max_ratio:g}x retail")
    print(f"  {len(rows)} use complete instruction counts; {len(prefix_rows)} compare the "
          "complete base with a reliable retail prefix")
    print(f"  ({len(skipped)} row(s) excluded because the base disassembly is truncated)\n")
    print(f"  {'score':>7} {'ratio':>6} {'base':>6} {'retail':>7}  unit / function")
    for pc, r, nb, nt, un, n in rows[:a.top]:
        print(f"  {pc:6.2f}% {r:6.2f} {nb:6} {nt:7}  {un}/{n}")
    if len(rows) > a.top:
        print(f"  ... and {len(rows) - a.top} more (--top N)")
    if prefix_rows:
        print("\n  Complete base versus reliable retail prefix (true ratio is lower):")
        for pc, r, nb, nt, un, n in prefix_rows[:a.top]:
            print(f"  {pc:6.2f}% {r:6.2f} {nb:6} {nt:6}+  {un}/{n}")
        if len(prefix_rows) > a.top:
            print(f"  ... and {len(prefix_rows) - a.top} more (--top N)")
    if skipped:
        print("\n  Excluded truncated-base rows (manual review required):")
        print(f"  {'score':>7} {'bad-at':>8} {'decoded':>7}  unit / function")
        for pc, bad_at, nb, un, n in skipped[:a.top]:
            print(f"  {pc:6.2f}% +0x{bad_at:05x} {nb:7}  {un}/{n}")
        if len(skipped) > a.top:
            print(f"  ... and {len(skipped) - a.top} more (--top N)")
    if rows or prefix_rows:
        print("\nThe classified rows want RECONSTRUCTION, not permutation. A wall note on "
              "one of them is describing codegen for code that does not exist yet.")
    else:
        print("\nNo function currently meets the mechanical missing-body criteria.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
