#!/usr/bin/env python3
"""gruntz.analysis.discover_gaps - find (and stub) functions Ghidra never carved.

The delink/objdiff pipeline only sees functions that SOMETHING carved: Ghidra's
functions.csv or src/'s RVA()+size claims. Anything else in .text is invisible - it
never enters the denominator, never gets diffed, never lands on a worklist. This
tool measures that blind spot and optionally carves it.

METHOD (pure coverage arithmetic - no disassembly guesswork for membership):
  1. Mark every byte of .text covered by a KNOWN function: functions.csv
     (entry_rva+byte_size) UNION symbol_names.csv (rva+size, kind==func).
  2. Walk the UNCOVERED bytes; split into code blocks delimited by >=2-byte
     int3/nop alignment-padding runs (the linker's inter-function gaps).
  3. Keep a block as a real lost FUNCTION iff:
       - size >= MIN_BODY (default 32; smaller = ILT thunk / adjustor / tiny stub),
       - starts with a plausible MSVC prologue byte,
       - ENDS with a terminator (ret / ret-imm / tail jmp rel32 into .text),
       - its start RVA is not in config/library_labels.csv (FID carve-outs).
  Blocks that fail (thunks, data/jump-tables, EH funclets) are counted but not stubbed.

    python -m gruntz.analysis.discover_gaps                 # report: coverage + counts
    python -m gruntz.analysis.discover_gaps --list          # + every kept RVA/size
    python -m gruntz.analysis.discover_gaps --emit FILE     # write RVA()+size stubs to FILE
"""
import csv, sys
from pathlib import Path

from gruntz.analysis import vtable_scan as vs

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parent)
FUNCS = REPO / "build/ghidra-enrich/exports/functions.csv"
GEN = REPO / "build/gen/symbol_names.csv"
LIBS = REPO / "config/library_labels.csv"
IB = vs.IMAGEBASE

MIN_BODY = 32
MAX_BODY = 0x8000  # bigger = almost certainly an embedded data/jump table, not a fn
# MSVC function prologue first-bytes (frame setup / callee-save / arg load / SEH).
PROLOGUE = {0x55, 0x53, 0x56, 0x57, 0x51, 0x50, 0x83, 0x81, 0x8b, 0x64, 0xa1,
            0x6a, 0x68, 0xb8, 0x33, 0x8d, 0xa0, 0xc7, 0x2b, 0x89, 0xf6, 0xff}


def _text_bounds():
    for (nm, va, vsz, rsz, rp, ch) in vs.SECS:
        if nm == ".text":
            return va, va + max(vsz, rsz)
    raise SystemExit("no .text section")


def _byte(rva):
    o = vs.off(rva)
    return vs.d[o] if o is not None else None


def _covered_bitmap(lo, hi):
    """1 = byte belongs to a KNOWN (Ghidra or src) function."""
    cov = bytearray(hi - lo)
    n_ghidra = n_src = 0

    def mark(start, size):
        if size <= 0:
            return
        a, b = max(start, lo), min(start + size, hi)
        for i in range(a - lo, b - lo):
            cov[i] = 1

    for r in csv.DictReader(FUNCS.open()):
        try:
            rva, sz = int(r["entry_rva"], 16), int(r["byte_size"])
        except (ValueError, KeyError):
            continue
        if lo <= rva < hi:
            mark(rva, sz)
            n_ghidra += 1
    for r in csv.DictReader(GEN.open()):
        try:
            rva = int(r["rva"], 16)
        except (ValueError, KeyError):
            continue
        try:
            sz = int((r.get("size") or "").strip(), 0)
        except ValueError:
            sz = 0
        if (r.get("kind") or "func") == "func" and sz and lo <= rva < hi:
            mark(rva, sz)
            n_src += 1
    return cov, n_ghidra, n_src


def _library_rvas():
    out = set()
    if LIBS.exists():
        for r in csv.DictReader(LIBS.open()):
            try:
                out.add(int(r["rva"], 16))
            except (ValueError, KeyError):
                pass
    return out


def _is_pad(b):
    return b in (0xCC, 0x90)


def _blocks(lo, hi, cov):
    """Uncovered code blocks, delimited by >=2-byte int3/nop padding runs."""
    out = []
    i = lo
    while i < hi:
        b = _byte(i)
        if cov[i - lo] or b is None or _is_pad(b):
            i += 1
            continue
        start = last = i
        j = i
        while j < hi and not cov[j - lo]:
            bj = _byte(j)
            if bj is None:
                break
            if _is_pad(bj):
                k = j
                while k < hi and not cov[k - lo] and _is_pad(_byte(k) or 0):
                    k += 1
                if k - j >= 2:  # alignment gap -> block boundary
                    break
                j = k
                continue
            last = j
            j += 1
        out.append((start, last + 1))
        i = max(j, last + 1)
    return out


def _terminates(start, end):
    """Block ends in a real function terminator: ret / ret-imm / tail jmp rel32."""
    last = _byte(end - 1)
    if last == 0xC3:                      # ret
        return True
    if end - 3 >= start and _byte(end - 3) == 0xC2:  # ret imm16
        return True
    if end - 5 >= start and _byte(end - 5) == 0xE9:   # tail jmp rel32
        return True
    return False


def find_lost(lo, hi, cov, libs):
    """(kept, thunks, tiny, rejected) - kept = real lost-fn (start,size) list."""
    kept, thunks, tiny, rejected = [], 0, 0, 0
    for s, e in _blocks(lo, hi, cov):
        n = e - s
        b0 = _byte(s)
        if n <= 6 and b0 == 0xE9:
            thunks += 1
            continue
        if n < MIN_BODY:
            tiny += 1
            continue
        if s in libs or b0 not in PROLOGUE or n > MAX_BODY or not _terminates(s, e):
            rejected += 1
            continue
        kept.append((s, n))
    return kept, thunks, tiny, rejected


def emit(path, kept):
    L = ['// GapFunctions.cpp - GENERATED by `python -m gruntz.analysis.discover_gaps',
         '// --emit`. DO NOT EDIT BY HAND; re-run the tool to refresh.',
         '//',
         '// Functions Ghidra never carved, recovered by .text coverage-gap analysis',
         '// (real prologue + terminator, >=32B, not FID). Each is an RVA()+size stub so',
         '// the delinker extracts its retail bytes and objdiff tracks it (initially ~0%).',
         '// These are the matching worklist: reconstruct each and RE-HOME it into its',
         '// real class TU, deleting the row here. See gruntz.analysis.discover_gaps.',
         '#include <rva.h>', '']
    for s, n in kept:
        L.append("RVA(0x%08x, 0x%x)" % (s, n))
        L.append("i32 Gap_%06x(void) { return 0; } // @stub" % s)
    Path(path).write_text("\n".join(L) + "\n")


def main():
    args = sys.argv[1:]
    do_list = "--list" in args
    emit_to = None
    if "--emit" in args:
        emit_to = args[args.index("--emit") + 1]
    lo, hi = _text_bounds()
    cov, ng, nsrc = _covered_bitmap(lo, hi)
    libs = _library_rvas()
    kept, thunks, tiny, rejected = find_lost(lo, hi, cov, libs)
    covered = sum(cov)
    total = hi - lo
    print(".text 0x%06x..0x%06x (%d B)" % (lo, hi, total))
    print("  carved: %d Ghidra + %d src fns  ->  %d B covered (%.1f%%), %d B uncovered (%.1f%%)"
          % (ng, nsrc, covered, 100 * covered / total, total - covered, 100 * (total - covered) / total))
    print("  uncovered blocks: %d thunks, %d tiny(<%dB), %d rejected(data/EH/lib), "
          "%d KEPT lost fns" % (thunks, tiny, MIN_BODY, rejected, len(kept)))
    kb = sum(n for _, n in kept)
    print("  kept: %d functions, %d B (%.0f KB), avg %d B"
          % (len(kept), kb, kb / 1024, kb / max(len(kept), 1)))
    if do_list:
        for s, n in kept:
            print("    0x%08x  0x%-4x" % (s, n))
    if emit_to:
        emit(emit_to, kept)
        print("  emitted %d stubs -> %s" % (len(kept), emit_to))


if __name__ == "__main__":
    main()
