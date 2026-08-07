#!/usr/bin/env python3
"""insn_seq.py - name the MISSING STATEMENT in a big-deficit function.

`insn_count` says *how many* instructions we are short. This says *which ones*, two ways,
both robust to the register renaming and block scheduling that make a raw `--diff`
unreadable on a 300-instruction body:

  **--hist**  a per-MNEMONIC histogram, base vs target, printing only the rows that
    differ. A wrong source idiom appears as a matched *pair* of rows - one family we
    over-emit against one we under-emit
    (docs/patterns/mnemonic-histogram-diff-finds-the-wrong-idiom.md).

  **--seq**   an ordered difflib over the `(mnemonic, reloc-symbol)` pairs. Only the
    instructions that CARRY a relocation are kept, so the sequence is a list of the
    function's calls, global reads and string references *in program order* - which
    survives scheduling (the optimizer reorders within a block, not across calls) and
    names the missing/extra/misplaced STATEMENT rather than a register.

Both sides come from the same objects objdiff compares, via `gruntz.core.branches`:
`build/objdiff/base/<unit>.obj` and the delinked target. The jump-table / COMDAT-padding
trimming is shared with `insn_count` so a switch does not manufacture a fake delta.

    python -m gruntz.audit.insn_seq 0x0002c690            # both views
    python -m gruntz.audit.insn_seq 0x0002c690 --seq      # reloc sequence only
    python -m gruntz.audit.insn_seq 0x0002c690 --hist     # histogram only
    python -m gruntz.audit.insn_seq ?Run@CGruntzMgr@@... --unit rezsync
"""
import argparse
import csv
import difflib
import re
import subprocess
import sys
from collections import Counter
from pathlib import Path

from gruntz.audit.insn_count import PAD, table_start, trim
from gruntz.core.branches import obj_paths

REPO = Path(__file__).resolve().parents[3]
QUEUE = REPO / "build" / "gen" / "residual_function_queue.tsv"

SYM_HDR = re.compile(r"^([0-9a-f]{8}) <(.+)>:$")
INSN = re.compile(r"^\s*([0-9a-f]+):\s+(\S+)\s*(.*)$")
RELOC = re.compile(r"^\s+([0-9a-f]{8}):\s+IMAGE_REL_I386_(\w+)\s+(.*)$")

# The delinked target disassembles an in-.text jump table as these garbage opcodes.
# They are DATA; never read them as a deficit.
TABLE_NOISE = {"addb", "aas", "hlt", "sldtw", "lretl", "lcalll", "ljmpl", "outsl", "insb"}


def stream(obj):
    """{symbol: (insns, table_offset)} where each insn is (off, mnemonic, ops, reloc|None).

    Same `$L`-folding rule as insn_count.streams: MSVC's local switch/block labels get a
    function-style header from llvm-objdump but belong to the enclosing COMDAT.

    llvm-objdump prints a relocation on the line AFTER the instruction it applies to, so
    the reloc is attached backwards onto the last instruction seen."""
    out = subprocess.run(["llvm-objdump", "-d", "-r", "--no-show-raw-insn", str(obj)],
                         capture_output=True, text=True).stdout
    syms, cur, dirs = {}, None, None
    for ln in out.split("\n"):
        m = SYM_HDR.match(ln.strip())
        if m:
            name = m.group(2)
            if name.startswith("$L") and cur is not None:
                continue
            cur, dirs = [], []
            syms[name] = (cur, dirs)
            continue
        if cur is None:
            continue
        m = RELOC.match(ln)
        if m:
            if m.group(2).startswith("DIR32"):
                dirs.append(int(m.group(1), 16))
            if cur:
                cur[-1] = cur[-1][:3] + (m.group(3).strip(),)
            continue
        m = INSN.match(ln)
        if m:
            cur.append((int(m.group(1), 16), m.group(2), m.group(3).strip(), None))
    return {k: (v[0], table_start(v[1])) for k, v in syms.items()}


def body(obj, name):
    insns, table = stream(obj).get(name, (None, None))
    if insns is None:
        return None
    return trim(insns, table)


def norm(sym):
    """Strip the delinker/base naming difference so the two sides compare.

    A base obj names a callee by its mangled symbol; the delinked target may name the same
    address `sub_<rva>` or carry a `+0x<n>` addend. Keep the addend (it distinguishes two
    fields of one struct) but drop `.text`-style section decoration."""
    s = sym.strip()
    s = re.sub(r"^(\.text|\.rdata|\.data|\.bss)\$?", "", s)
    return s


def histogram(base, tgt):
    b = Counter(mn for _, mn, _, _ in base)
    t = Counter(mn for _, mn, _, _ in tgt)
    rows = []
    for mn in sorted(set(b) | set(t)):
        if b[mn] == t[mn]:
            continue
        rows.append((mn, b[mn], t[mn], t[mn] - b[mn]))
    return rows


def relseq(insns):
    return [(mn, norm(rel)) for _, mn, _, rel in insns if rel]


def show_seq(base, tgt):
    b, t = relseq(base), relseq(tgt)
    print("== reloc sequence: base %d, target %d ==" % (len(b), len(t)))
    sm = difflib.SequenceMatcher(a=b, b=t, autojunk=False)
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag == "equal":
            n = i2 - i1
            if n > 3:
                print("  ... %d matching ..." % n)
            else:
                for k in range(i1, i2):
                    print("      %-8s %s" % b[k])
            continue
        for k in range(i1, i2):
            print("  -   %-8s %s" % b[k])
        for k in range(j1, j2):
            print("  +   %-8s %s" % t[k])


def resolve(what, unit, queue):
    """(unit, mangled-name) from an rva or a mangled symbol."""
    if unit and not what.startswith("0x"):
        return unit, what
    with Path(queue).open() as fh:
        for r in csv.DictReader(fh, delimiter="\t"):
            if what.startswith("0x"):
                if int(r["rva"], 16) == int(what, 16):
                    return r["unit"], r["name"]
            elif r["name"] == what:
                return r["unit"], r["name"]
    return None, None


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("what", help="rva (0x...) or mangled symbol")
    ap.add_argument("--unit", help="unit name (needed only for a symbol not in the queue)")
    ap.add_argument("--seq", action="store_true", help="reloc sequence only")
    ap.add_argument("--hist", action="store_true", help="mnemonic histogram only")
    ap.add_argument("--queue", default=str(QUEUE))
    a = ap.parse_args(argv)

    unit, name = resolve(a.what, a.unit, a.queue)
    if not unit:
        sys.exit("not in %s - pass --unit and the mangled name" % a.queue)
    b, t = obj_paths(unit)
    base, tgt = body(b, name), body(t, name)
    if base is None or tgt is None:
        sys.exit("%s missing from %s obj" % (name, "base" if base is None else "target"))

    print("%s  [%s]" % (name, unit))
    print("insns: base %d, target %d  (delta %+d)" % (len(base), len(tgt),
                                                      len(base) - len(tgt)))
    if not a.seq:
        rows = histogram(base, tgt)
        print("== mnemonic histogram (base -> target), differing rows ==")
        for mn, bc, tc, d in sorted(rows, key=lambda r: -abs(r[3])):
            note = "   <- jump-table bytes, ignore" if mn in TABLE_NOISE else ""
            print("  %-10s %4d -> %-4d  %+d%s" % (mn, bc, tc, d, note))
    if not a.hist:
        show_seq(base, tgt)
    return 0


if __name__ == "__main__":
    sys.exit(main())
