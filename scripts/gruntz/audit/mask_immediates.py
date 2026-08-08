#!/usr/bin/env python3
"""Sieve: base and target mask the SAME word with DIFFERENT constants.

objdiff scores an instruction's operands, but `sema disasm --diff` masks address
operands -- and that masking hides plain immediates too, so a wrong magic number
shows up only as a downstream shift or an unexplained percentage. This reads the
constants directly, out of the same two objects objdiff compares, and prints the
ones that differ.

Scope is deliberately the BITWISE-MASK family (`and` / `or` / `xor` / `test`).
Those immediates are a hard property of the SOURCE -- a bit name, a field width,
a struct offset -- and unlike an `add $0x10, %esp` they do not move when the
register allocator does. A row here is a source fact to go and check, not
scheduling residue.

**Width is normalised before comparing.** cl reaches for the narrowest encoding
it can prove correct, so `andb $0xe0, %al` and `andl $0xffffffe0, %eax` are the
same operation and must not read as a difference: a sub-32-bit `and` is extended
with ONE bits (the untouched high bits survive), `or`/`xor`/`test` with ZERO. What
survives that normalisation is a genuine constant disagreement.

    ~ TRUNCATED ~
One shape gets its own name and its own count, because it is a correctness bug
and not a match defect: the base masks a 32-bit word with a value that is the
8- or 16-bit complement of the target's, `0xdf` against `0xffffffdf`. Retail
clears one bit of one byte; we clear that bit AND every bit above the byte. The
cause is a width or union-member slip -- `m_rowInts[y][x * 7 * 4 + 3] &= 0xdf`
where the cell's byte three is meant (CGrunt::ClaimSwitchTile, fixed 2026-08-08:
the `* 4` says the author meant BYTE indexing, but the array is `i32**`, so the
write landed in a different cell entirely). It is NOT the MSVC-5.0 enum-width
story: cl 5.0 promotes every enumerator to `int` under `~`, measured, so
`&= ~FLAG` cannot truncate. See docs/patterns/enum-complement-is-sixteen-bit.md.

**Two false positives to know about.** Neither can manufacture a TRUNCATED row, but
both show up in the full worklist, so read the disassembly before believing a row -
the tool ranks, it does not adjudicate.

  * A constant used twice can be hoisted into a REGISTER, and `and %ebp, %esi`
    carries no immediate at all, so the side that hoisted looks like it is missing
    the constant. CBootyState::BuildWarpStoneGlitterAnimation reads "base-only
    0xfffffffe x2" for exactly that: retail keeps -2 in ebp and spends it on both
    the `(i != idx) ? 1 : 3` ternary and the flag clear.
  * A high bit can be tested through a DISPLACED byte operand - `testb $0x1,
    0x1(%eax)` is `testl $0x100` shifted one byte up - and the displacement lives in
    the address, not the immediate. CMapMgr::ComputeCellFlags reads "base-only
    0x1 0x10 / target-only 0x100 0x1000" and its source constants are already right.

`--metric` prints the truncated count for the cleanliness board, where it is
ratcheted at 0.

    python -m gruntz.audit.mask_immediates              # the full worklist
    python -m gruntz.audit.mask_immediates --truncated  # only the bug class
    python -m gruntz.audit.mask_immediates --unit grunt
    python -m gruntz.audit.mask_immediates --metric
"""

from __future__ import annotations

import argparse
import collections
import glob
import re
import subprocess
import sys
from pathlib import Path

from gruntz.core.branches import is_local_label, obj_paths

REPO = Path(__file__).resolve().parents[3]
BASE_GLOB = str(REPO / "build" / "objdiff" / "base" / "*.obj")

SYM_HDR = re.compile(r"^[0-9a-f]+\s+<(.+)>:$")
INSN = re.compile(r"^\s*([0-9a-f]+):\s+(\S+)\s*(.*)$")
RELOC = re.compile(r"^\s*([0-9a-f]+):\s+(IMAGE_REL_I386_\S+)")
MASK = re.compile(r"^(and|or|xor|test)([bwl])$")
IMM = re.compile(r"\$(-?0x[0-9a-fA-F]+|-?\d+)")
WIDTH = {"b": 8, "w": 16, "l": 32}
M32 = 0xFFFFFFFF

# The two truncations cl can produce by masking a 32-bit word with a narrower
# constant: (low, high, the bits a correct 32-bit mask would also carry).
TRUNCATIONS = ((0x80, 0xFF, 0xFFFFFF00), (0x8000, 0xFFFF, 0xFFFF0000))


def _table_start(relocs: list[int]) -> int | None:
    """Offset of a switch's jump table: >=3 DIR32 relocations at stride 4.

    Shared with `insn_count`, and for the same reason: the delinker packs the
    table into the owning function's symbol, where its raw addresses disassemble
    as plausible code and manufacture immediates nobody wrote.
    """
    for i in range(len(relocs) - 2):
        if all(relocs[i + k + 1] - relocs[i + k] == 4 for k in range(2)):
            return relocs[i]
    return None


def _symbols(obj: Path) -> dict[str, list[tuple[int, str, str]]]:
    """{symbol: instructions}, jump tables trimmed off the tail."""
    out = subprocess.run(
        ["llvm-objdump", "-d", "-r", "--no-show-raw-insn", str(obj)],
        capture_output=True,
        text=True,
    ).stdout
    syms: dict[str, tuple[list, list]] = {}
    cur = relocs = None
    for line in out.split("\n"):
        m = SYM_HDR.match(line.strip())
        if m:
            name = m.group(1)
            # A `$L<n>` / `$<goto>$<n>` block label is part of the ENCLOSING
            # function's COMDAT, not a function of its own.
            if is_local_label(name) and cur is not None:
                continue
            cur, relocs = [], []
            syms[name] = (cur, relocs)
            continue
        if cur is None:
            continue
        m = RELOC.match(line)
        if m:
            if m.group(2).endswith("DIR32"):
                relocs.append(int(m.group(1), 16))
            continue
        m = INSN.match(line)
        if m:
            cur.append((int(m.group(1), 16), m.group(2), m.group(3).strip()))
    res = {}
    for name, (insns, rel) in syms.items():
        table = _table_start(rel)
        if table is not None:
            insns = [i for i in insns if i[0] < table]
        res[name] = insns
    return res


def mask_constants(insns) -> collections.Counter:
    """The multiset of 32-bit-normalised mask immediates in one function."""
    out: collections.Counter = collections.Counter()
    for _off, mnemonic, operands in insns:
        m = MASK.match(mnemonic)
        if not m:
            continue
        op, width = m.group(1), WIDTH[m.group(2)]
        for token in IMM.findall(operands):
            value = int(token, 0) & ((1 << width) - 1)
            if width < 32 and op == "and":
                value |= M32 ^ ((1 << width) - 1)
            out[value] += 1
    return out


def truncated(base_only: collections.Counter, target_only: collections.Counter):
    """The (narrow, wide) pairs where base masks 32 bits with a narrow complement."""
    rows = []
    for value in sorted(base_only):
        for low, high, extend in TRUNCATIONS:
            if low <= value <= high and (value | extend) in target_only:
                rows.append((value, value | extend))
    return rows


def scan(unit: str | None = None):
    """[(unit, symbol, base_only, target_only, truncated_pairs)] for differing fns."""
    units = sorted(Path(p).stem for p in glob.glob(BASE_GLOB))
    if unit:
        units = [u for u in units if u == unit]
    rows = []
    for name in units:
        base, target = obj_paths(name)
        if not base.is_file() or not target.is_file():
            continue
        bsyms, tsyms = _symbols(base), _symbols(target)
        for sym in sorted(set(bsyms) & set(tsyms)):
            mb, mt = mask_constants(bsyms[sym]), mask_constants(tsyms[sym])
            if mb == mt:
                continue
            base_only, target_only = mb - mt, mt - mb
            rows.append((name, sym, base_only, target_only,
                         truncated(base_only, target_only)))
    return rows


def _fmt(counter: collections.Counter) -> str:
    return " ".join(f"{v:#x}" + (f" x{n}" if n > 1 else "")
                    for v, n in sorted(counter.items())) or "-"


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--unit", help="restrict to one objdiff unit")
    ap.add_argument("--truncated", action="store_true",
                    help="only the narrow-complement bug class")
    ap.add_argument("--metric", action="store_true",
                    help="print the cleanliness-board count and nothing else")
    args = ap.parse_args(argv)

    rows = scan(args.unit)
    trunc = [r for r in rows if r[4]]

    if args.metric:
        print(f"truncated masks: {len(trunc)}")
        return 0

    shown = trunc if args.truncated else rows
    for name, sym, base_only, target_only, pairs in shown:
        tag = "   << TRUNCATED " + ", ".join(f"{a:#x} vs {b:#x}" for a, b in pairs) if pairs else ""
        print(f"{name}  {sym}{tag}")
        print(f"    base-only   {_fmt(base_only)}")
        print(f"    target-only {_fmt(target_only)}")
    print(f"\n# {len(rows)} function(s) with a mask-immediate difference; "
          f"{len(trunc)} TRUNCATED (drive to 0)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
