#!/usr/bin/env python3
"""jcc_sieve.py - find the CONTROL-FLOW bugs that `--diff` prints as "identical asm".

`gruntz sema disasm --diff` masks every address operand so that reloc-bound targets do
not show up as spurious diffs. That masking also hides **intra-function conditional-
branch displacements**: a `je` that goes to a different basic block on the two sides is
printed `je <tgt>` on both and compares EQUAL. So a genuine divergence in control flow
renders as "identical asm" while the function sits below 100%.

`CDDrawSurfaceMgr::SetDimensions` 0x155f60 was parked at 99.88% with "identical asm";
the whole bug was `74 2e` vs `74 16` - retail's early-out skipped the level rebuild, we
ran it unconditionally on every no-op resize. A real behavioural bug, invisible to the
diff we look at all day. See docs/patterns/masked-diff-hides-branch-target.md.

So this compares the two sides' ordered **branch sequences** directly, out of the object
files, where nothing is masked. For every sub-100% function it extracts the ordered list
of `(offset, mnemonic, target)` over base and target and reports:

  SIGNEDNESS  a signed<->unsigned pair swapped at the same position (`jl`<->`jb`,
              `jg`<->`ja`, `jle`<->`jbe`, `jge`<->`jae`). Nearly always a REAL source
              bug: a loop guard or a comparison whose operand wants to be u32. Four
              unsigned counters were recovered this way in one afternoon.
  POLARITY    the condition is inverted at the same position (`je`<->`jne`,
              `jl`<->`jge`, ...). Usually a negated condition or an inverted block
              layout; sometimes a genuine `<` vs `<=` slip.
  OTHER       a mnemonic pair that is neither - read it by hand.
  TOPOLOGY    every mnemonic agrees, but a branch lands on a DIFFERENT block. This is
              the SetDimensions shape, and it is the one `--diff` hides hardest,
              because both sides read identically instruction for instruction.

## What is NOT a hit, and why

**Differing branch COUNTS are not this signal.** An extra or missing conditional jump
is a structural difference - a whole basic block we did not reconstruct, an `if` the
optimizer folded, an inlining decision. Those need a normal reconstruction pass, not a
one-line condition fix, and they swamp the listing (they are the common case among
sub-100% functions). They are counted in the summary as `struct` and never listed.

**Constant displacement shifts are noise.** If our side is a few bytes longer upstream,
every subsequent branch target shifts by the same amount while the control flow is
identical. TOPOLOGY therefore compares targets *symbolically* - each target is named by
the index of the first branch at or after it - so a uniform shift compares equal. The
residue this cannot see: two blocks that both sit after the last branch (both normalize
to "past the end"), so TOPOLOGY under-reports at the epilogue.

**More than 4 flips is not this signal either.** At that point the two functions are
differently shaped and the positional pairing is meaningless; a 30-branch function with
11 flips is telling you the reconstruction is wrong, not that a comparison is signed.

**A base side that stops early cannot be compared.** llvm-objdump disassembles linearly
and a jump table embedded in `.text` after an indirect `jmp` decodes as garbage (or
`(bad)`), truncating or poisoning the stream. Those functions are counted as `skip` and
excluded - this is a known limitation, not a clean result.

**A hit is a lead, not a verdict.** cl picks the branch polarity that suits its block
layout, so a POLARITY row can be pure codegen preference (see
docs/patterns/positive-gate-enables-shrink-wrap.md - count the `ret`s first). SIGNEDNESS
is the half that is nearly always real, so it prints first.

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
import re
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
REPORT = REPO / "build" / "objdiff" / "report.json"
BASE_DIR = REPO / "build" / "objdiff" / "base"
TGT_DIR = REPO / "build" / "objdiff" / "target"
GEN_NAMES = REPO / "build" / "gen" / "symbol_names.csv"

# A WHITELIST, not `j` minus `jmp`: llvm-objdump spells the indirect jump `jmpl`, which
# a `(?!mp\b)` exclusion lets through (`\b` fails before the `l`) - that put unconditional
# jumps into the branch sequence and produced a `jne->jmpl` "flip". This is the complete
# set observed over every base+target obj in the tree; `jcxz`/`loop*` never appear in
# MSVC5 /O2 output and an unexpected spelling raises rather than being dropped silently.
JCC = frozenset("je jne jl jle jg jge ja jae jb jbe js jns jo jno jp jnp".split())
JUMPY = re.compile(r"^j[a-z]+$")
UNCOND = frozenset(("jmp", "jmpl", "jmpw"))
SYM_HDR = re.compile(r"^[0-9a-f]{8} <(.+)>:$")
INSN = re.compile(r"^\s*([0-9a-f]+):\s+(\S+)\s*(.*)$")
TGT_OP = re.compile(r"^(0x[0-9a-f]+)\b")

# The signed/unsigned twins. x86 has two condition families over the same flags; picking
# the wrong one is a source-level type bug, not a codegen choice.
SIGNED_TWIN = {
    "jl": "jb", "jb": "jl",
    "jle": "jbe", "jbe": "jle",
    "jg": "ja", "ja": "jg",
    "jge": "jae", "jae": "jge",
}

# The inverse of each condition: same test, opposite sense.
INVERSE = {
    "je": "jne", "jne": "je",
    "jl": "jge", "jge": "jl", "jle": "jg", "jg": "jle",
    "jb": "jae", "jae": "jb", "jbe": "ja", "ja": "jbe",
    "js": "jns", "jns": "js",
    "jo": "jno", "jno": "jo",
    "jp": "jnp", "jnp": "jp",
}


def disasm(obj: Path):
    """symbol -> [(offset, mnemonic, operand)] for one object file.

    One llvm-objdump per OBJECT, not per symbol: a unit has up to a few hundred
    functions and the per-symbol form costs a process each."""
    out = subprocess.run(
        ["llvm-objdump", "-d", "--no-show-raw-insn", str(obj)],
        capture_output=True, text=True).stdout
    syms, cur = {}, None
    for ln in out.split("\n"):
        m = SYM_HDR.match(ln.strip())
        if m:
            cur = syms.setdefault(m.group(1), [])
            continue
        if cur is None:
            continue
        m = INSN.match(ln)
        if m:
            cur.append((int(m.group(1), 16), m.group(2), m.group(3).strip()))
    return syms


def branches(insns):
    """The ordered conditional-branch list: [(offset, mnemonic, target|None)].

    A target that is not a plain address (never seen for a jcc, but a corrupt stream
    can produce one) is kept as None so the sequence length stays honest."""
    out = []
    for off, mn, op in insns:
        if mn in JCC:
            m = TGT_OP.match(op)
            out.append((off, mn, int(m.group(1), 16) if m else None))
        elif JUMPY.match(mn) and mn not in UNCOND:
            raise SystemExit("jcc_sieve: unclassified jump mnemonic %r - add it to JCC "
                             "or UNCOND, do not let it fall through" % mn)
    return out


def rets(insns):
    """How many `ret` the side has. `positive-gate-enables-shrink-wrap.md` turns this
    count into the rewrite: base > target means an exit block is duplicated that retail
    tail-merges (write the positive gate / shared exit); equal means the gate spelling is
    already right and the polarity has another cause."""
    return sum(1 for _, mn, _ in insns if mn.startswith("ret"))


def poisoned(insns):
    """True when llvm-objdump could not decode the stream (jump-table data in .text
    after an indirect jmp is the usual cause). Such a side cannot be compared."""
    return any(mn == "(bad)" or mn.startswith("<") for _, mn, _ in insns)


def sym_target(brs, tgt):
    """Name a branch target symbolically: the index of the first branch at or after it.

    This is what makes a uniform displacement shift compare EQUAL - the thing the
    docstring calls noise. `len(brs)` means "past the last branch"."""
    if tgt is None:
        return None
    for i, (off, _, _) in enumerate(brs):
        if off >= tgt:
            return i
    return len(brs)


def classify(a, b):
    if SIGNED_TWIN.get(a) == b:
        return "SIGNEDNESS"
    if INVERSE.get(a) == b:
        return "POLARITY"
    return "OTHER"


def rva_index():
    """mangled name -> retail RVA, so a hit row is directly actionable."""
    idx = {}
    if GEN_NAMES.is_file():
        for r in csv.DictReader(GEN_NAMES.open()):
            idx.setdefault(r["name"], r["rva"])
    return idx


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
        bobj, tobj = BASE_DIR / (unit + ".obj"), TGT_DIR / (unit + ".c.obj")
        if not (bobj.is_file() and tobj.is_file()):
            stats["no-objs"] += len(sub)
            continue
        bs, ts = disasm(bobj), disasm(tobj)
        for f in sub:
            name = f.get("name")
            bi, ti = bs.get(name), ts.get(name)
            if not bi or not ti:
                stats["unpaired"] += 1
                continue
            if poisoned(bi) or poisoned(ti):
                stats["skip"] += 1
                continue
            bb, tb = branches(bi), branches(ti)
            if len(bb) != len(tb):
                stats["struct"] += 1
                continue
            if not bb:
                stats["no-branches"] += 1
                continue
            flips = [(i, x[1], y[1]) for i, (x, y) in enumerate(zip(bb, tb))
                     if x[1] != y[1]]
            if len(flips) > max_flips:
                stats["many-flips"] += 1
                continue
            if flips:
                kinds = collections.Counter(classify(a, b) for _, a, b in flips)
                kind = ("SIGNEDNESS" if kinds["SIGNEDNESS"] else
                        "OTHER" if kinds["OTHER"] else "POLARITY")
                if all({a, b} == {"je", "jne"} for _, a, b in flips):
                    stats["je/jne only"] += 1
                detail = "  ".join("#%d %s->%s" % (i, a, b) for i, a, b in flips)
            else:
                # Same mnemonics everywhere: the only thing left that a masked diff
                # can hide is a branch landing on a different block.
                bt = [sym_target(bb, t) for _, _, t in bb]
                tt = [sym_target(tb, t) for _, _, t in tb]
                moved = [(i, x, y) for i, (x, y) in enumerate(zip(bt, tt)) if x != y]
                if not moved or len(moved) > max_flips:
                    stats["clean" if not moved else "many-flips"] += 1
                    continue
                kind = "TOPOLOGY"
                detail = "  ".join("#%d ->blk%s not blk%s" % (i, y, x)
                                   for i, x, y in moved)
            stats[kind] += 1
            hits.append((kind, unit, name,
                         float(f.get("fuzzy_match_percent") or 0.0), len(bb), detail,
                         rets(bi), rets(ti)))
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
        print("   uncomparable: %d undecodable stream (jump-table data in .text)  |  "
              "%d symbol missing on one side  |  %d unit has no obj pair"
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
