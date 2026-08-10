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

SIX NOISE SOURCES ARE SUBTRACTED, each of which otherwise floods the report
  1. RELOCATED immediates (`movl $g_foo+0x20, %ecx`). Our side carries the
     addend, the delinked side carries the linked address or a different
     addend; objdiff masks the operand, so we must too. Dropped by checking
     whether a relocation lands inside the instruction's bytes - not by a
     magnitude guess, which mis-drops a genuine large constant.
  2. FRAME ADJUSTS (`subl $0x4c, %esp`, `addl $0x34, %esp`). The frame size is
     a register-allocation outcome; a 4-byte difference is the single commonest
     benign row and says nothing about the source.
  3. Functions the two sides do not both define.
  4. THE TRAILING JUMP TABLE - the loudest false positive this tool had, and the
     reason `B.code_stop` is not optional.  MSVC5 parks a switch's dword jump
     table AND its byte index table at the END of the function's COMDAT, inside
     the symbol, so llvm-objdump decodes both as instructions.  The two sides
     then RESYNC DIFFERENTLY on the SAME bytes: retail's index table read
     `addl $0x60a0a0a, %eax` (an immediate - counted) where ours read
     `orb (%eax,%eax,1), %al` with displacement `0xa0a0a05` (a displacement -
     ignored).  That alone manufactured the two most alarming rows the first
     run produced - four retail-only `0x60a0a0a` in CTileActionEvent::MorphByTool
     and a `0x5020501` in CTriggerMgr::ApplySwitch, read as packed per-kind
     decision tables we had failed to reconstruct.  They are neither packed nor
     missing: extracted as bytes, all three index tables are IDENTICAL on the
     two sides -

       MorphByTool  base +0x224 / retail 0x113650
         00 01 0e 02 03 04 0e 0e 0e 05 06 07 0e 0e 0e 08 09 0a 0e 0e 0e 0b 0c 0d
       MorphByTool  base +0x268 / retail 0x113694
         00 01 0a 02 03 03 0a 0a 0a 04 05 05 0a 0a 0a 06 07 07 0a 0a 0a 08 09 09
       ApplySwitch  base +0x5cc / retail 0x6d8cc
         00 05 01 05 02 05 05 05 05 05 05 03 05 04

     `add eax, $0x60a0a0a` is opcode 05 eating `0a 0a 0a 06` from the middle of
     the first of those.  Any table byte can land in an immediate slot on one
     side and a displacement slot on the other, so EVERY switch function was
     suspect until the table was excluded.
  5. A `$` INSIDE A SYMBOL NAME.  llvm-objdump annotates a branch with its
     target's name, and MSVC5's local labels carry a decimal after a `$`
     (`jne 0x482 <$loop_restart$32369+0x37>`).  That is a name, not an operand;
     unstripped it read as the literal 0x7e71 and gave CAniAdvanceCursor::Advance
     eight one-sided immediates that are not in the instruction stream at all.

  6. `ret imm16`.  The operand is the callee's stack-cleanup byte count, fixed
     by the signature - it CANNOT be a wrong constant.  A one-sided row only
     ever says the two sides emitted a different NUMBER of epilogue copies
     (cl cross-jumped where retail did not, or the reverse), which the fuzzy %
     already reports.  `CTileTriggerContainer::AddLogic` read three retail-only
     `ret 0x84` and `CCheckpointTriggerSwitchLogic::BuildSmall` six `ret 0x24`
     purely from tail merging.

  Measured 2026-08-10, whole tree: 283 rows before these were subtracted, 247
  after - 36 of the first run's rows, including its single loudest, were the
  tool's own decode artifacts or values that cannot carry a defect.

READING A ROW - and the seven ways one is NOT a defect
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
  (d) `add reg, K` ON ONE SIDE, `lea reg, K(base)` ON THE OTHER - the single
      commonest surviving row, and never a defect.  `lea`'s K is a DISPLACEMENT,
      so only the `add` form is counted.  CAniElement::Build (retail `mov ebx,
      ebp; add ebp, 0x20` vs ours `lea ebp, [ebx+0x20]`), CTriggerMgr::CombatCue
      (`add ecx, 0x1c` vs `lea eax, [ecx+0x1c]`) and CTileActionEvent::Process
      (`add edi, -0x132` vs `lea eax, [edi-0x132]`) are all this and nothing
      else.  Read the carrier before believing any row whose value also appears
      as a displacement on the other side.
  (e) A CONSTANT REMATERIALIZED vs HOISTED.  Retail spells `1` as an immediate
      at three sites where cl hoists one `mov esi, 1` and reuses it (or the
      reverse); the multiset then differs by two even though the VALUE is the
      same everywhere.  CTileActionEvent::MorphByTool and
      CTileTriggerContainer::SetCell are both exactly this.  The commonest
      instance is ZERO: `xor ecx,ecx` once and store the register, against
      retail's `movl $0x0, ...` at each site - so a one-sided `0x0` almost
      never means a missing initializer (CRezImage::FlipVertical,
      CDDSurface::Blit1624, CMulti::Connect).
  (f) THE EH STATE SLOT.  A repeated `movl $K, <fixed>(%esp)` at one displacement
      is cl's __ehstate write; K numbers the try/cleanup scopes, so it moves with
      EH structure, not with any source constant (CDDrawSurfaceMgr::RestoreChildren,
      CTileTriggerContainer::LoadElement).
  (g) A BYTE OP ON A HIGH-BYTE REGISTER.  `orb $0x1, %ah` IS `or reg, 0x100`, and
      `andb $0xe0, %al` IS `and reg, 0xffffffe0` - cl uses the short encoding only
      when the value happens to live in a byte-addressable register, so the SAME
      source produces the wide immediate the moment regalloc picks %esi over %eax.
      CDDrawShadeBlit::Rebuild (`or ah, 1` vs retail `or esi, 0x100`) is entirely
      this, and it survives --strong because the shifted magnitude is real.

  What survives all of those is the real class: a mask, a divisor, a shift count
  or a packed tag that only ONE side has.

USAGE
  python -m gruntz.audit.immediates                 # whole tree
  python -m gruntz.audit.immediates --min-pct 95    # only near-matches
  python -m gruntz.audit.immediates --unit ddsurface --unit cimage
  python -m gruntz.audit.immediates --strong --context     # THE WORKLIST

  --context prints the instruction carrying each value, which is what makes a
  row adjudicable at all.  --strong keeps only the rows carrying a value the
  other side does not contain ANYWHERE, in any operand - 247 rows fall to 40.
  It is a ranking, not a verdict: (a), (b), (f) and (g) all still pass it,
  because in each of those the two sides genuinely hold different numbers while
  computing the same thing.  Read the carrier.
"""
from __future__ import annotations

import argparse
import collections
import re
import sys
from pathlib import Path

from gruntz.audit.global_refs import _coff, _functions
from gruntz.core import branches as B
from gruntz.core import report as R

REPO = Path(__file__).resolve().parents[3]

IMM = re.compile(r"\$(-?0x[0-9a-f]+|-?\d+)")
FRAME = re.compile(r"^(add|sub)l?$")
# llvm-objdump appends the symbolic name of a branch target in angle brackets, and
# MSVC5's own local labels embed a decimal after a `$` (`<$loop_restart$32369+0x37>`).
# That is a NAME, not an operand - left in, it read as the immediate 0x7e71 and made
# CAniAdvanceCursor::Advance report eight one-sided literals that do not exist.
SYMBOLIC = re.compile(r"<[^>]*>")


def _reloc_table_start(reloc_offsets):
    """Offset where a trailing jump table begins, proven by the RELOCATIONS.

    `B.code_stop` ends the walk at the last `ret`, and a 0xc3 byte inside the
    table silently defeats that: CDDSurface::Refresh's delinked index table
    decodes a spurious `retl` at +364 and drags the stop 60 B past the real end
    at +305, re-admitting the very bytes noise source 4 exists to drop.

    The relocations settle it exactly, and on both sides at once, because every
    dword of a jump table is an address: the table is a run of relocations at a
    stride of 4.  No x86 instruction can produce that - an instruction carrying
    a relocation is at least 5 bytes (`call rel32`, `push imm32`, `mov eax,
    moffs32`, any modrm+disp32 form), so two relocations exactly 4 bytes apart
    prove the bytes between them are DATA.  Refresh: code relocations at
    49/54/92/99/172/179, then the two tables at 308+4k and 356+4k.
    """
    r = sorted(reloc_offsets)
    for i in range(len(r) - 1):
        if r[i + 1] - r[i] == 4:
            return r[i]
    return None


def _stop(insns, reloc_offsets):
    """Where this side's real code ends: the earlier of the two witnesses."""
    stops = [s for s in (B.code_stop(insns), _reloc_table_start(reloc_offsets))
             if s is not None]
    return min(stops) if stops else None


def _scan(insns, reloc_offsets, stop):
    """Yield (offset, 'mn ops', [values]) for each surviving CODE instruction.

    `insns` is [(offset, mnemonic, operands)] with offsets relative to the
    symbol; `reloc_offsets` are the same-relative offsets of every relocation
    inside it. An instruction's extent is [off, next_off).

    The LAST instruction has no successor, and guessing `off + 16` for its
    extent is how CDDrawShadeBlit::Select lost a `ret 0x8`: on the base side the
    stream ends at the final `ret`, so the guessed extent swallowed the jump
    table's first relocation at +0x74 and noise source 1 dropped the `ret` as
    "relocated".  Clamp to `stop`, which is where the code provably ends.
    """
    for i, (off, mn, ops) in enumerate(insns):
        if stop is not None and off >= stop:
            break
        end = insns[i + 1][0] if i + 1 < len(insns) else off + 16
        if stop is not None:
            end = min(end, stop)
        if any(off <= r < end for r in reloc_offsets):
            continue                                  # noise source 1
        if FRAME.match(mn) and "%esp" in ops:
            continue                                  # noise source 2
        if mn.startswith("ret"):
            continue                                  # noise source 6
        yield off, f"{mn} {ops}".rstrip(), [
            int(t, 16) if "x" in t else int(t)
            for t in IMM.findall(SYMBOLIC.sub("", ops))]   # noise source 5


def _immediates(insns, reloc_offsets, stop):
    """Multiset of non-relocated, non-frame-adjust immediates in the CODE."""
    out: collections.Counter = collections.Counter()
    for _off, _ins, vals in _scan(insns, reloc_offsets, stop):
        out.update(vals)
    return out


def _carriers(insns, reloc_offsets, stop, wanted):
    """[(offset, 'mnemonic operands')] for every instruction carrying a wanted value.

    A bare value is not adjudicable: `0x8` is a `cmp` bound, a `push` argument, a
    struct size and a shift count, and which one it is decides whether the row is
    a defect at all.  The instruction is the smallest thing that answers that, so
    `--context` prints it instead of making the reader disassemble by hand.
    """
    return [(off, ins) for off, ins, vals in _scan(insns, reloc_offsets, stop)
            if wanted.intersection(vals)]


def _absent(values, insns, stop):
    """Of `values`, those whose magnitude appears NOWHERE in the other side's text.

    This is the tool's actual discriminator, and it is worth more than the row
    itself.  Nearly every surviving row is a value BOTH sides compute, spelled
    two ways - `add reg, K` here against `lea reg, K(base)` there, an index
    against a strength-reduced pointer, one `mov reg, 1` reused where retail
    rematerialized three.  In all of those the number is still present on the
    other side, just not as a `$` operand.

    A value that is absent outright is the different, rarer thing: a constant
    one side HAS and the other simply does not.  That is what a wrong mask,
    bound, stride or divisor looks like, and it is how the one true defect of
    the 2026-08-10 sweep surfaced - CDDrawShadeBlit::Build reading the PID
    payload from `pid + 0x24` where retail reads `pid + 0x20`, with 0x24 found
    nowhere in retail's 371 bytes.

    Matched as a bare hex magnitude (`0x24`, and `0x18` for -0x18), so a value
    hiding in a displacement, a scale or a branch target still counts as
    present - deliberately conservative: this must not over-promote.
    """
    seen = " ".join(ins for _off, ins, _v in _scan(insns, [], stop))
    out = []
    for v in values:
        # llvm-objdump prints the same 32-bit value either way round: `$0xfffffece`
        # for the immediate and `-0x132(%edi)` for the displacement. Both spellings
        # must count as PRESENT or every negative constant reads as absent.
        mags = {abs(v), abs(v - (1 << 32)) if v >= (1 << 31) else abs(v)}
        if not any(f"0x{m:x}" in seen or str(m) in seen.split() for m in mags):
            out.append(v)
    return out


def _percentages():
    """{mangled: (unit, live fuzzy %, rva)}.

    The % comes from THIS build's report.json, not from the `cur_pct` column of
    config/match_baseline.tsv: `gruntz build --fast` never runs `status update`,
    so the ledger column is whatever the last full build left behind, and reading
    it made CDDrawShadeBlit::Select print `100.00` next to a genuine one-sided
    `ret 0x8`.  A stale rank is worse than no rank - the whole point of the report
    is that a one-sided literal in a HIGH-scoring body is the strongest signal.
    """
    rows = {}
    path = REPO / "config/match_baseline.tsv"
    if path.exists():
        for line in path.read_text().splitlines():
            f = line.split("\t")
            if len(f) > 6 and f[6].startswith("0x"):
                rows[f[1]] = (f[0], None, f[6])
    rep = R.Report()
    for u in rep.units:
        for fn in u.get("functions") or []:
            name = fn.get("name")
            if name:
                prev = rows.get(name)
                rows[name] = (u.get("name"), R.fn_fuzzy(fn), prev[2] if prev else "?")
    return {k: v for k, v in rows.items() if v[1] is not None}


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--unit", action="append", help="restrict to a unit (repeatable)")
    ap.add_argument("--min-pct", type=float, default=0.0)
    ap.add_argument("--context", action="store_true",
                    help="print the instruction carrying each one-sided value")
    ap.add_argument("--strong", action="store_true",
                    help="only rows whose value is absent from the other side entirely")
    ap.add_argument("--project", default=str(REPO / "build/objdiff"))
    args = ap.parse_args(argv)

    proj = Path(args.project)
    basedir, tgtdir = proj / "base", proj / "target"
    if not basedir.is_dir():
        print(f"{basedir} missing - run `gruntz build` first", file=sys.stderr)
        return 2

    pcts = _percentages()
    units = args.unit or [p.stem for p in sorted(basedir.glob("*.obj"))]
    rows = strong = 0
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
            bstop, tstop = _stop(bi, bo), _stop(ti, to)   # noise source 4
            bc = _immediates(bi, bo, bstop)
            tc = _immediates(ti, to, tstop)
            if bc == tc:
                continue
            ours, retail = bc - tc, tc - bc
            gone_t = _absent(ours, ti, tstop)         # ours, absent from retail
            gone_b = _absent(retail, bi, bstop)       # retail's, absent from ours
            if args.strong and not (gone_t or gone_b):
                continue
            pct = f"{row[1]:7.2f}" if row else "      ?"
            rva = row[2] if row else "?"
            mark = "  ABSENT-ON-THE-OTHER-SIDE" if (gone_t or gone_b) else ""
            print(f"{pct}  {u:22s} {rva:>9s}  {sym}{mark}")
            if ours:
                tag = "".join(f" !{hex(v)}" for v in sorted(gone_t))
                print(f"          OURS-ONLY  : {sorted((hex(k), v) for k, v in ours.items())}{tag}")
                for off, ins in _carriers(bi, bo, bstop, set(ours)) if args.context else []:
                    print(f"            ours   +{off:#06x}  {ins}")
            if retail:
                tag = "".join(f" !{hex(v)}" for v in sorted(gone_b))
                print(f"          RETAIL-ONLY: {sorted((hex(k), v) for k, v in retail.items())}{tag}")
                for off, ins in _carriers(ti, to, tstop, set(retail)) if args.context else []:
                    print(f"            retail +{off:#06x}  {ins}")
            rows += 1
            strong += bool(gone_t or gone_b)
    print(f"\n{rows} function(s) with a one-sided literal immediate; "
          f"{strong} carry a value ABSENT from the other side (marked `!`) - work those first.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
