"""Sieve: retail SHRINK-WRAPS the callee-saved pushes, our base does not.

cl 5.0 emits a register's `push` at its first definition deep in the body -- not in
the prologue -- when the entry guard does not need that register, and the guard's
`return` then pops only what the entry pushed.  It makes that call on the SOURCE
flow graph, so a second `return` anywhere in the body pins every save back into the
prologue; our base is then +2/+4 instructions and every register rotates.

Tell, and what this tool counts: the number of leading `push ebx/esi/edi/ebp` before
the first control-flow instruction, base vs target.  A function is reported when the
base pushes MORE at entry than the target does AND the target pushes a callee-saved
register later in the body.

The fix is source-level: nested `if`/`else` with one tail `return`.  See
docs/patterns/shrink-wrapped-prologue-needs-one-tail-return.md.

    python -m gruntz.audit.shrink_wrap [--min-fuzzy 60] [--max-fuzzy 99.9]
"""

import argparse
import csv
import re
import subprocess
import sys
from pathlib import Path

from gruntz.core.branches import obj_paths

REPO = Path(__file__).resolve().parents[3]
QUEUE = REPO / "build" / "gen" / "residual_function_queue.tsv"

SAVED = ("%ebx", "%esi", "%edi", "%ebp")
FLOW = ("j", "call", "ret", "loop")


def _insns(obj: Path, sym: str):
    """(mnemonic, operands) for one symbol out of one obj, or None."""
    try:
        out = subprocess.run(
            ["llvm-objdump", "-d", f"--disassemble-symbols={sym}", str(obj)],
            capture_output=True,
            text=True,
            timeout=60,
        ).stdout
    except Exception:
        return None
    res, started = [], False
    for ln in out.splitlines():
        if ln.endswith(">:"):
            started = True
            continue
        if not started:
            continue
        m = re.match(r"^\s*[0-9a-f]+:\s+(?:[0-9a-f]{2} )+\s*(\S+)\s*(.*)$", ln)
        if m:
            res.append((m.group(1), m.group(2).strip()))
    return res


def _leading_pushes(ins) -> int:
    """callee-saved pushes before the first control-flow instruction.

    Non-push, non-flow instructions do NOT stop the scan: cl interleaves
    `mov esi,ecx` and parameter loads between the prologue pushes.
    """
    n = 0
    for mnem, ops in ins:
        if mnem == "pushl" and ops in SAVED:
            n += 1
        elif mnem.startswith(FLOW):
            break
    return n


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--min-fuzzy", type=float, default=60.0)
    ap.add_argument("--max-fuzzy", type=float, default=99.9)
    ap.add_argument("--queue", type=Path, default=QUEUE)
    args = ap.parse_args(argv)

    if not args.queue.exists():
        print(f"no residual queue at {args.queue} - run `gruntz build` first", file=sys.stderr)
        return 2

    hits = []
    for row in csv.DictReader(args.queue.open(), delimiter="\t"):
        fuzzy = float(row["fuzzy"])
        if not (args.min_fuzzy <= fuzzy < args.max_fuzzy):
            continue
        try:
            base, target = obj_paths(row["unit"])
        except Exception:
            continue
        bi, ti = _insns(base, row["name"]), _insns(target, row["name"])
        if not bi or not ti:
            continue
        nb, nt = _leading_pushes(bi), _leading_pushes(ti)
        # require the target to save a callee-saved register LATER, i.e. it really
        # shrink-wrapped rather than simply needing fewer registers.
        sunk = any(m == "pushl" and o in SAVED for m, o in ti[nt + 3 :])
        if nb > nt and sunk:
            hits.append((nb - nt, row["rva"], row["unit"], fuzzy, row["name"]))

    hits.sort(key=lambda h: (-h[0], -h[3]))
    print(f"{len(hits)} shrink-wrap candidate(s)")
    for delta, rva, unit, fuzzy, sym in hits:
        print(f"  +{delta}  {rva}  {unit:<24} {fuzzy:6.2f}%  {sym}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
