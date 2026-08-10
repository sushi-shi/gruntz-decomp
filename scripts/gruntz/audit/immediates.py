#!/usr/bin/env python3
"""Per-function IMMEDIATE-multiset diff: base (our compile) vs target (delinked retail).

WHY
  A wrong literal CONSTANT - a mask, a divisor, a shift count, a state tag, a
  buffer size - is the defect class every other sieve is blind to:

    * objdiff's fuzzy % barely moves for one changed immediate, and
      `gruntz sema disasm --diff` MASKS large immediates outright, so a
      `/9`-vs-`/30` divisor shows only as a downstream shift.
    * `gruntz.audit.store_offsets` sees WHERE we store, never WHAT.
    * the reloc-addend sieve (docs/patterns/reloc-addend-is-masked-diff-the-addends.md)
      sees `g_tbl + K`, i.e. only operands that carry a relocation.

  This closes the remaining hole: the operands that carry NO relocation.

METHOD
  llvm-objdump prints AT&T for these objects, and that makes the extraction
  exact - an IMMEDIATE is precisely a `$`-prefixed literal:

      andl   $0x1f, %eax          <- immediate            COUNTED
      movl   0x14(%ebx), %eax     <- memory displacement   ignored
      je     0x58b <$L27601>      <- branch target         ignored

  A displacement tracks the frame layout and a branch target tracks code
  layout; both legitimately differ between the two sides, and neither is ever
  `$`-prefixed. So no heuristic is needed to tell them apart.

THREE NOISE SOURCES ARE SUBTRACTED, each of which otherwise floods the report
  1. RELOCATED immediates (`movl $g_foo+0x20, %ecx`). Our side carries the
     addend, the delinked side carries the linked address or a different
     addend; objdiff masks the operand, so we must too. Dropped by checking
     whether a relocation lands inside the instruction's bytes - not by a
     magnitude guess, which mis-drops a genuine large constant.
  2. FRAME ADJUSTS (`subl $0x4c, %esp`, `addl $0x34, %esp`). The frame size is
     a register-allocation outcome; a 4-byte difference is the single commonest
     benign row and says nothing about the source.
  3. Functions the two sides do not both define.

READING A ROW - and the three ways one is NOT a defect
  `RETAIL-ONLY 0x<v>` means retail's code contains a literal we never emit.  On
  a high-% function that is either a constant we got wrong or a statement we did
  not write.  Before believing a row, rule out:

  (a) INSTRUCTION SELECTION FOR THE SAME OPERATION.  `andl $0xff, %eax` and
      `movzbl %al, %eax` are both `(u8)x`; only the first carries an immediate.
      A one-sided `0xff` / `0xffff` / `0xffffffff` is therefore usually a
      narrowing BOTH sides perform.  Measured 2026-08-10: deleting the `(u8)`
      casts that produced three one-sided `0xff` rows in
      CShadeTableCache::GammaTable took it 94.07 -> 86.29, and the same edit
      took GreyTable 92.86 -> 89.14.  The casts were right; the immediate was
      never the signal.
  (b) A STRENGTH-REDUCED LOOP BOUND.  `for (i = 0; i < 256; i++) g[i] = ...`
      becomes either a counter (`cmp $0x100`) or a pointer compare against
      `&g[256]`, which is `g + 0x200` and shows as a one-sided `0x200`
      (CDDSurface::Blit168) or `0x400` (CDDSurface::DecodeBmp).
  (c) A CROSS-JUMP.  cl merges two identical arms retail keeps apart, so a
      constant retail stores twice appears once on our side
      (CDDrawSurfaceChildA::SetGeometry's two WORLDERR_CREATE_DEVICE blocks).

  What survives all three is the real class: a mask, a divisor, a shift count
  or a packed tag that only ONE side has - e.g. retail's four `0x60a0a0a` in
  CTileActionEvent::MorphByTool, which our source contains nowhere.

USAGE
  python -m gruntz.audit.immediates                 # whole tree
  python -m gruntz.audit.immediates --min-pct 95    # only near-matches
  python -m gruntz.audit.immediates --unit ddsurface --unit cimage
"""
from __future__ import annotations

import argparse
import collections
import re
import sys
from pathlib import Path

from gruntz.audit.global_refs import _coff, _functions
from gruntz.core import branches as B

REPO = Path(__file__).resolve().parents[3]

IMM = re.compile(r"\$(-?0x[0-9a-f]+|-?\d+)")
FRAME = re.compile(r"^(add|sub)l?$")


def _immediates(insns, reloc_offsets, base):
    """Multiset of non-relocated, non-frame-adjust immediates.

    `insns` is [(offset, mnemonic, operands)] with offsets relative to the
    symbol; `reloc_offsets` are the same-relative offsets of every relocation
    inside it. An instruction's extent is [off, next_off).
    """
    out: collections.Counter = collections.Counter()
    for i, (off, mn, ops) in enumerate(insns):
        end = insns[i + 1][0] if i + 1 < len(insns) else off + 16
        if any(off <= r < end for r in reloc_offsets):
            continue                                  # noise source 1
        if FRAME.match(mn) and "%esp" in ops:
            continue                                  # noise source 2
        for tok in IMM.findall(ops):
            out[int(tok, 16) if "x" in tok else int(tok)] += 1
    return out


def _percentages():
    rows = {}
    path = REPO / "config/match_baseline.tsv"
    if not path.exists():
        return rows
    for line in path.read_text().splitlines():
        f = line.split("\t")
        if len(f) > 6 and f[6].startswith("0x"):
            try:
                rows[f[1]] = (f[0], float(f[2]), f[6])
            except ValueError:
                pass
    return rows


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--unit", action="append", help="restrict to a unit (repeatable)")
    ap.add_argument("--min-pct", type=float, default=0.0)
    ap.add_argument("--project", default=str(REPO / "build/objdiff"))
    args = ap.parse_args(argv)

    proj = Path(args.project)
    basedir, tgtdir = proj / "base", proj / "target"
    if not basedir.is_dir():
        print(f"{basedir} missing - run `gruntz build` first", file=sys.stderr)
        return 2

    pcts = _percentages()
    units = args.unit or [p.stem for p in sorted(basedir.glob("*.obj"))]
    rows = 0
    for u in units:
        b, t = basedir / f"{u}.obj", tgtdir / f"{u}.c.obj"
        if not b.exists() or not t.exists():
            continue
        bfun, tfun = _functions(_coff(b)), _functions(_coff(t))
        bd, td = B.decode(b), B.decode(t)
        for sym, bi in sorted(bd.items()):
            ti = td.get(sym)
            if not ti or not bi or sym not in bfun or sym not in tfun:
                continue
            row = pcts.get(sym)
            if row and row[1] < args.min_pct:
                continue
            bs, be, brel = bfun[sym]
            ts, te, trel = tfun[sym]
            bo = [o - bs for o, _n, _ty, _a in brel if bs <= o < be]
            to = [o - ts for o, _n, _ty, _a in trel if ts <= o < te]
            bc = _immediates(bi, bo, bs)
            tc = _immediates(ti, to, ts)
            if bc == tc:
                continue
            ours, retail = bc - tc, tc - bc
            pct = f"{row[1]:7.2f}" if row else "      ?"
            rva = row[2] if row else "?"
            print(f"{pct}  {u:22s} {rva:>9s}  {sym}")
            if ours:
                print(f"          OURS-ONLY  : {sorted((hex(k), v) for k, v in ours.items())}")
            if retail:
                print(f"          RETAIL-ONLY: {sorted((hex(k), v) for k, v in retail.items())}")
            rows += 1
    print(f"\n{rows} function(s) with a one-sided literal immediate.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
