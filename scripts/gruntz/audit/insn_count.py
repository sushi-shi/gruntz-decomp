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

**Why llvm-objdump and not `sema disasm --diff`.** The latter truncates at the first `$L`
jump-table label, and the delinker packs a jump table INTO the owning function's symbol on
the target side while the base keeps it in separate `$L` symbols - so every switch shows a
huge bogus delta.

**The count is only evidence once the non-code bytes are gone**, and getting that wrong
manufactures bugs that are not there. Two artifacts, both handled in `trim()`:

  * COMDAT alignment `nop`s, which cl emits and the delinker does not. Untrimmed, they
    put spikes of 56/73/114 functions at exactly delta +4/+8/+12.
  * the switch JUMP TABLE, decoded linearly and DIFFERENTLY on the two sides (base entries
    are all-zero reloc slots, target entries hold real addresses). `streams()` finds it
    exactly, from the relocations rather than by a rule about `ret`s.

Between them those two accounted for 39 of the 304 "mismatches" a naive count reports,
including the two largest: CSpriteRef::Build (-33 -> 275 vs 275) and
CTileActionEvent::Process (-40 -> 261 vs 261). Both are regalloc walls.

    python -m gruntz.audit.insn_count                 # the 85-99.6% band, |delta| first
    python -m gruntz.audit.insn_count --summary       # counts only
    python -m gruntz.audit.insn_count --unit play     # one unit
    python -m gruntz.audit.insn_count --min 0 --max 100   # a different band
    python -m gruntz.audit.insn_count --eh            # only the EH-frame mismatches
    python -m gruntz.audit.insn_count --tsv out.tsv   # the whole table, machine-readable
"""
import argparse
import csv
import re
import subprocess
import sys
from pathlib import Path

from gruntz.core.branches import obj_paths

REPO = Path(__file__).resolve().parents[3]


def _warn_wrong_tree():
    """Shout when this package was imported from a DIFFERENT worktree than the
    one being worked on.

    REPO comes from THIS file's location, so a PYTHONPATH inherited from another
    worktree's `nix develop` makes `python -m gruntz.audit.*` silently read that
    tree's objects while `gruntz sema ...` (which resolves the repo from its own
    wrapper) reads yours - a lane analysed another worktree for a whole batch
    before noticing. Fix: `export PYTHONPATH=$PWD/scripts`."""
    import os
    d = os.environ.get("GRUNTZ_DIR")
    if d and Path(d).resolve() != REPO:
        print(f"WARNING: reading {REPO} but GRUNTZ_DIR={d} - this package was "
              f"imported from another worktree. Run "
              f"`export PYTHONPATH=$GRUNTZ_DIR/scripts` and retry.",
              file=sys.stderr)


_warn_wrong_tree()
QUEUE = REPO / "build" / "gen" / "residual_function_queue.tsv"

PAD = ("nop", "int3")
SYM_HDR = re.compile(r"^([0-9a-f]{8}) <(.+)>:$")
INSN = re.compile(r"^\s*([0-9a-f]+):\s+(\S+)\s*(.*)$")
RELOC = re.compile(r"^\s+([0-9a-f]{8}):\s+IMAGE_REL_I386_(\w+)\s")
BAD = ("(bad)", "<unknown>")


def streams(obj):
    """{symbol: (instructions, table_offset|None)} for one object.

    `llvm-objdump -d -r` in a single pass: instruction lines carry a section offset and a
    relocation is printed inline underneath the instruction (or the datum) it applies to.

    MSVC emits `$L<n>` symbols for switch arms and other local blocks; llvm-objdump gives
    each one a function-style header although it is still part of the enclosing COMDAT, so
    they are folded back into the current owner (same rule as `gruntz.core.branches`).

    `table_offset` is where the switch JUMP TABLE starts, and it has to be exact. The
    table lives in `.text` at the end of the COMDAT, llvm-objdump decodes those bytes
    linearly as instructions, and the two sides decode DIFFERENTLY - the base obj's
    entries are still all-zero reloc slots (two `addb %al,(%eax)` each) while the target's
    hold real addresses. That artifact alone was the whole -33 on CSpriteRef::Build, which
    is in fact 275 vs 275: a pure regalloc wall.

    Heuristics do not survive here. "Cut after the last `ret`" fails on
    CTileActionEvent::Process, whose table bytes happen to decode a `retl $0x0`. What IS
    exact on both sides is the RELOCATION shape: a jump table is a run of DIR32 relocs at
    a 4-byte stride, and three in a row cannot occur in code (every instruction that
    carries a DIR32 is at least 5 bytes long)."""
    out = subprocess.run(["llvm-objdump", "-d", "-r", "--no-show-raw-insn", str(obj)],
                         capture_output=True, text=True).stdout
    syms, cur, relocs = {}, None, None
    for ln in out.split("\n"):
        m = SYM_HDR.match(ln.strip())
        if m:
            name = m.group(2)
            if name.startswith("$L") and cur is not None:
                continue
            cur, relocs = [], []
            syms[name] = (cur, relocs)
            continue
        if cur is None:
            continue
        m = RELOC.match(ln)
        if m:
            if m.group(2).startswith("DIR32"):
                relocs.append(int(m.group(1), 16))
            continue
        m = INSN.match(ln)
        if m:
            cur.append((int(m.group(1), 16), m.group(2), m.group(3).strip()))
    return {k: (v[0], table_start(v[1])) for k, v in syms.items()}


def table_start(relocs):
    """Offset of the first jump-table entry: a run of >=3 DIR32 relocs at stride 4."""
    for i in range(len(relocs) - 2):
        if all(relocs[i + k + 1] - relocs[i + k] == 4 for k in range(2)):
            return relocs[i]
    return None


def trim(insns, table):
    """Drop the jump table and the trailing COMDAT alignment padding.

    cl pads each `.text$` COMDAT out to a 16-byte boundary with single-byte `nop`s that
    llvm-objdump counts inside the symbol while the delinker's target objs have none.
    Untrimmed, that alone put spikes of 56/73/114 functions at exactly delta +4/+8/+12."""
    n = len(insns)
    if table is not None:
        while n and insns[n - 1][0] >= table:
            n -= 1
    while n and insns[n - 1][1].startswith(PAD):
        n -= 1
    return insns[:n]


def truncated(insns, table):
    """Did llvm-objdump stop decoding real instructions inside the CODE region?

    A `(bad)` byte inside the trimmed body means the count is not to be trusted - read the
    function by hand instead."""
    return any(mn.startswith(BAD) for _, mn, _ in trim(insns, table))


def has_eh(insns):
    """Does this function carry the /GX exception frame?

    `mov fs:0, esp` in the prologue is the whole tell, and it is a HARD source fact, not a
    codegen choice: cl5 emits it exactly when the function owns a local whose destructor
    must run during an unwind. So a side-mismatch is one of two concrete bugs -

      EXTRA (ours has it, retail does not)   - we invented a destructible local, or the
        retail TU was compiled without /GX (check the whole unit: retail movinglogic has
        ZERO EH functions, so that one is a flags profile, not a body).
      MISSING (retail has it, ours does not) - retail owns a destructible local we never
        reconstructed. Usually a CString, and often a release-dead `TRACE` argument
        (release-trace-leaves-flag-guarded-dtor.md) - a dtor with no constructor.

    Both are invisible to the instruction count on a big function, because the frame is
    ~10 instructions against a body of hundreds."""
    return any(mn.startswith("mov") and "%fs:" in op and "%esp" in op
               for _, mn, op in insns)


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
        cache[u] = (streams(b), streams(t))
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
        b, t = c[0].get(r["name"]), c[1].get(r["name"])
        if b is None or t is None:
            r.update(status="NO-SYM")
            out.append(r)
            continue
        trunc = truncated(*b) or truncated(*t)
        bi, ti = trim(*b), trim(*t)
        be, te = has_eh(bi), has_eh(ti)
        r.update(status="TRUNC" if trunc else ("MISMATCH" if len(bi) != len(ti) else "EQUAL"),
                 base_insn=len(bi), tgt_insn=len(ti), delta=len(bi) - len(ti),
                 eh="EXTRA" if be and not te else "MISSING" if te and not be else "")
        out.append(r)
    return out


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--min", type=float, default=85.0, help="low fuzzy bound (default 85)")
    ap.add_argument("--max", type=float, default=99.6, help="high fuzzy bound (default 99.6)")
    ap.add_argument("--unit", help="restrict to one unit")
    ap.add_argument("--summary", action="store_true", help="bucket counts only")
    ap.add_argument("--eh", action="store_true",
                    help="list only the EH-frame mismatches (see has_eh)")
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
            w.writerow("status rva unit fuzzy size base_insn tgt_insn delta eh name".split())
            for r in sorted(rs, key=lambda r: -abs(r.get("delta") or 0)):
                w.writerow([r["status"], r["rva"], r["unit"], r["fuzzy"], r["size"],
                            r.get("base_insn", ""), r.get("tgt_insn", ""),
                            r.get("delta", ""), r.get("eh", ""), r["name"]])
        print("wrote %s" % a.tsv)

    eh = [r for r in rs if r.get("eh")]
    if eh:
        print("  EH-frame  %4d  (--eh to list)" % len(eh))
    if a.eh:
        for r in sorted(eh, key=lambda r: (r["eh"], float(r["fuzzy"]))):
            print("%-8s %6.2f%%  %-9s %-22s %s"
                  % (r["eh"], float(r["fuzzy"]), r["rva"], r["unit"], r["name"]))
        return 0
    if a.summary:
        return 0
    hits = sorted(buckets.get("MISMATCH", []), key=lambda r: -abs(r["delta"]))
    for r in hits:
        print("%+5d  %-9s %-22s %6.2f%%  %s"
              % (r["delta"], r["rva"], r["unit"], float(r["fuzzy"]), r["name"]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
