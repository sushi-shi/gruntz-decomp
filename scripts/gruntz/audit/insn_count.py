#!/usr/bin/env python3
"""insn_count.py - partition the residual worklist by INSTRUCTION COUNT.

The count of instructions a function compiles to is a hard invariant of the SOURCE
shape. It is not a codegen preference, so it splits the sub-100% worklist in one pass:

  * **counts differ**  - the source is WRONG. Something is missing or extra: a reload
    retail does and we cached, a local retail does not have, a comparison operand not
    materialized before its call, a whole statement. There is a real, findable fix.
  * **counts agree**   - every remaining difference is register naming, operand role or
    a schedule transposition: the documented regalloc/scheduling wall. Do not grind it.

Measured on the `??0C*@@QAE@PAUCGameObject@@@Z` ctor family (61 partial): 26 mismatched
and every one opened was a genuine source bug. See
docs/patterns/instruction-count-mismatch-finds-the-real-bug.md.

**Why llvm-objdump and not `sema disasm --diff`.** The latter truncates at the first
`$L` jump-table label, and the delinker packs a jump table INTO the owning function's
symbol on the target side while the base keeps it in separate `$L` symbols - so every
switch shows a huge bogus delta. `gruntz.core.branches.parse_objdump` already folds `$L`
runs back into their owner on both sides, which is why this module reuses it.

**Trailing alignment padding must be trimmed or the whole table is noise.** cl pads each
`.text$` COMDAT out to a 16-byte boundary with single-byte `nop`s and llvm-objdump counts
them inside the symbol; the delinker's target objs have none. Untrimmed, that put spikes
of 56/73/114 functions at exactly delta +4/+8/+12 - pure padding, no bug. The embedded
jump TABLE is the same trap one level down: llvm-objdump decodes it linearly, and the
two sides decode it DIFFERENTLY (base entries are all-zero reloc slots, target entries
hold real addresses), which is the whole -33 on CSpriteRef::Build. `trim()` removes both.

**A TRUNC row is not a verdict.** llvm-objdump disassembles linearly, so jump-table bytes
sitting in `.text` decode as garbage; `first_bad()` finds where that starts and the row
is flagged TRUNC. Its delta is unreliable - read the function by hand instead of trusting
the number.

    python -m gruntz.audit.insn_count                 # the 85-99.6% band, |delta| first
    python -m gruntz.audit.insn_count --summary       # counts only
    python -m gruntz.audit.insn_count --unit play     # one unit
    python -m gruntz.audit.insn_count --min 0 --max 100   # a different band
    python -m gruntz.audit.insn_count --tsv out.tsv   # the whole table, machine-readable
"""
import argparse
import csv
import re
import sys
from pathlib import Path

from gruntz.core.branches import UNCOND, decode, first_bad, obj_paths

REPO = Path(__file__).resolve().parents[3]
QUEUE = REPO / "build" / "gen" / "residual_function_queue.tsv"


PAD = ("nop", "int3")
# `jmpl *0x3a0(,%eax,4)` / `jmpl *(,%eax,4)` - the switch dispatch. The scale-4 index is
# what distinguishes it from an ordinary indirect call through a pointer.
INDIRECT4 = re.compile(r"\*.*,%\w+,4\)")


def trim(insns):
    """Drop the trailing COMDAT alignment padding and any embedded jump TABLE.

    The nops are pure COMDAT alignment (see the module docstring). The jump table is the
    subtler half: it lives in `.text` right after the last `ret`, llvm-objdump decodes
    those bytes linearly as instructions, and the two sides decode DIFFERENTLY - the base
    obj's entries are still all-zero reloc slots (`addb %al,(%eax)` x2 per entry) while
    the target's hold real addresses. CSpriteRef::Build showed -33 from that alone.

    So when the stream dispatches through a scale-4 indirect jump, everything past the
    last `ret` is data. The test is deliberately narrow: without a switch, a block placed
    after the epilogue is real code and must be counted."""
    n = len(insns)
    if any(mn in UNCOND and INDIRECT4.search(op) for _, mn, op in insns):
        for i in range(n - 1, -1, -1):
            if insns[i][1].startswith("ret"):
                n = i + 1
                break
    while n and insns[n - 1][1].startswith(PAD):
        n -= 1
    return insns[:n]


def rows(queue, lo, hi, unit=None):
    """The worklist rows in the [lo, hi] fuzzy band, as dicts."""
    out = []
    with queue.open() as fh:
        for r in csv.DictReader(fh, delimiter="\t"):
            f = float(r["fuzzy"])
            if f < lo or f > hi:
                continue
            if unit and r["unit"] != unit:
                continue
            out.append(r)
    return out


def counts(units):
    """{unit: (base_syms, target_syms)} - one llvm-objdump per object, not per symbol."""
    cache = {}
    for u in sorted(units):
        b, t = obj_paths(u)
        if not b.is_file() or not t.is_file():
            cache[u] = None
            continue
        cache[u] = (decode(b), decode(t))
    return cache


def measure(rs):
    """Annotate each row with base/target instruction counts and the delta."""
    cache = counts({r["unit"] for r in rs})
    out = []
    for r in rs:
        c = cache.get(r["unit"])
        if c is None:
            r.update(status="NO-OBJ")
            out.append(r)
            continue
        bi, ti = c[0].get(r["name"]), c[1].get(r["name"])
        if bi is None or ti is None:
            r.update(status="NO-SYM")
            out.append(r)
            continue
        trunc = first_bad(bi) is not None or first_bad(ti) is not None
        bi, ti = trim(bi), trim(ti)
        r.update(status="TRUNC" if trunc else ("MISMATCH" if len(bi) != len(ti) else "EQUAL"),
                 base_insn=len(bi), tgt_insn=len(ti), delta=len(bi) - len(ti))
        out.append(r)
    return out


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--min", type=float, default=85.0, help="low fuzzy bound (default 85)")
    ap.add_argument("--max", type=float, default=99.6, help="high fuzzy bound (default 99.6)")
    ap.add_argument("--unit", help="restrict to one unit")
    ap.add_argument("--summary", action="store_true", help="bucket counts only")
    ap.add_argument("--tsv", help="write the full table here")
    ap.add_argument("--queue", default=str(QUEUE), help="residual_function_queue.tsv")
    a = ap.parse_args(argv)

    q = Path(a.queue)
    if not q.is_file():
        sys.exit("no %s - run `gruntz build` then `gruntz match-queue`" % q)
    rs = measure(rows(q, a.min, a.max, a.unit))

    buckets = {}
    for r in rs:
        buckets.setdefault(r["status"], []).append(r)
    print("band %.1f-%.1f%%: %d functions" % (a.min, a.max, len(rs)))
    for k in ("MISMATCH", "EQUAL", "TRUNC", "NO-SYM", "NO-OBJ"):
        if buckets.get(k):
            print("  %-9s %4d" % (k, len(buckets[k])))

    if a.tsv:
        with open(a.tsv, "w", newline="") as fh:
            w = csv.writer(fh, delimiter="\t")
            w.writerow("status rva unit fuzzy size base_insn tgt_insn delta name".split())
            for r in sorted(rs, key=lambda r: -abs(r.get("delta") or 0)):
                w.writerow([r["status"], r["rva"], r["unit"], r["fuzzy"], r["size"],
                            r.get("base_insn", ""), r.get("tgt_insn", ""),
                            r.get("delta", ""), r["name"]])
        print("wrote %s" % a.tsv)

    if a.summary:
        return 0
    hits = sorted(buckets.get("MISMATCH", []), key=lambda r: -abs(r["delta"]))
    for r in hits:
        print("%+5d  %-9s %-22s %6.2f%%  %s"
              % (r["delta"], r["rva"], r["unit"], float(r["fuzzy"]), r["name"]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
