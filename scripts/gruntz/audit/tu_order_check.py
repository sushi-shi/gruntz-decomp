#!/usr/bin/env python3
"""tu_order_check.py - the linker-layout acceptance gate.

MSVC emits one COMDAT per function and the linker lays each .obj (== one source
file) down as ONE CONTIGUOUS .text run, functions in source order. So a faithful
reconstruction must satisfy two invariants (verified by isle/reccmp and th06, the
established MSVC matching decomps that build byte-identical the same way):

  INTRA-TU  within each .cpp, the RVA()/RVAU() functions appear in FILE ORDER that
            is strictly increasing in retail RVA, and their [rva, rva+size) spans
            do not overlap.  (file order == link order == RVA order.)

  INTER-TU  each TU occupies ONE contiguous .text block: its [min_start, max_end)
            span must not interleave another TU's span. Gaps between blocks are
            fine (unreconstructed code lives there); overlap/interleave is not -
            it means two .cpp files are really one .obj, or a body is misattributed.

Header RVA() inlines are ignored (we glob *.cpp only). DATA() lives in a different
section and is not checked here. src/Stub/ is the un-homed backlog: excluded by
default (pass --include-stub to see it interleave everything, which is expected
until re-homing drains it).

Exit 0 = clean (gate PASS); exit 1 = violations (gate FAIL).

Usage:
    python3 -m gruntz.audit.tu_order_check              # gate, real TUs
    python3 -m gruntz.audit.tu_order_check --tu trigger_mgr   # one TU detail
    python3 -m gruntz.audit.tu_order_check --inter-only # only cross-TU blocks
    python3 -m gruntz.audit.tu_order_check --csv report.csv
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

from gruntz.audit.tu_layout import RVA_RE, RVAU_RE, SIG_RE, _psize, pooled

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
SRC = REPO / "src"


class Entry:
    __slots__ = ("rva", "size", "line", "name", "tu")

    def __init__(self, rva, size, line, name, tu):
        self.rva, self.size, self.line, self.name, self.tu = rva, size, line, name, tu

    @property
    def end(self) -> int:
        return self.rva + self.size


def load_in_file_order(src: Path, include_stub: bool, exclude_pools: bool):
    """Every RVA()/RVAU() function per .cpp, in FILE (source) order (NOT rva-sorted).

    exclude_pools drops functions living in the COMDAT dtor/ctor pools (tu_layout
    POOLS) - the linker places those away from their class's main run, so they are
    the direct analogue of the inline/FOLDED functions isle's reccmp linter skips
    in its own order check. Excluding them measures the ordinary out-of-line gate."""
    tus: dict[str, list[Entry]] = {}
    for path in sorted(src.rglob("*.cpp")):
        if not include_stub and "Stub" in path.parts[len(src.parts):]:
            continue
        tu = path.stem
        rel = path.relative_to(REPO)
        lines = path.read_text(errors="replace").splitlines()
        seq: list[Entry] = []
        for i, ln in enumerate(lines):
            m = RVA_RE.search(ln)
            if m:
                rva, size = int(m.group(1), 16), _psize(m.group(2))
            else:
                mu = RVAU_RE.search(ln)
                if not mu:
                    continue
                rva, size = int(mu.group(1), 16), 0  # unsized thunk: order-check only
            if exclude_pools and pooled(rva):
                continue
            name = None
            for j in range(i, min(i + 4, len(lines))):
                sm = SIG_RE.search(lines[j])
                if sm:
                    name = f"{sm.group(1)}::{sm.group(2)}"
                    break
            seq.append(Entry(rva, size, i + 1, name or "?", f"{rel}"))
        if seq:
            tus[tu] = seq
    return tus


def check_intra(tus):
    """(tu -> list of violation strings). Descents + overlaps in file order."""
    viol = {}
    for tu, seq in tus.items():
        vs = []
        for a, b in zip(seq, seq[1:]):
            if b.rva <= a.rva:
                vs.append(f"  L{a.line} {a.rva:#08x} {a.name}  ->  "
                          f"L{b.line} {b.rva:#08x} {b.name}   [file order not ascending]")
            elif a.size and a.end > b.rva:
                vs.append(f"  L{a.line} {a.rva:#08x}+{a.size:#x}={a.end:#08x} {a.name}  overlaps  "
                          f"L{b.line} {b.rva:#08x} {b.name}")
        if vs:
            viol[tu] = vs
    return viol


def tu_spans(tus):
    """tu -> (min_start, max_end); sized entries define the block extent."""
    spans = {}
    for tu, seq in tus.items():
        starts = [e.rva for e in seq]
        ends = [e.end for e in seq if e.size]
        spans[tu] = (min(starts), max(ends) if ends else max(starts))
    return spans


def check_inter(tus):
    """List of (tuA, spanA, tuB, spanB) whose .text blocks interleave/overlap."""
    spans = tu_spans(tus)
    ordered = sorted(spans.items(), key=lambda kv: kv[1][0])
    out = []
    for i in range(len(ordered)):
        ta, (sa, ea) = ordered[i]
        for j in range(i + 1, len(ordered)):
            tb, (sb, eb) = ordered[j]
            if sb >= ea:          # sorted by start: once past, no more overlaps
                break
            if sa < eb and sb < ea:   # intervals intersect
                out.append((ta, (sa, ea), tb, (sb, eb)))
    return out


BASELINE = REPO / "config" / "tu-order-baseline.tsv"


def _gate(intra, inter) -> int:
    """Down-only ratchet vs the committed backlog. A TU whose intra-violation
    count RISES (or a brand-new offender TU, or a rise in the total interleave
    pair count) fails the build; improvements roll the baseline down. Floors
    are never raised by tooling - fixing the layout is the only way down."""
    cur = {tu: len(v) for tu, v in intra.items()}
    pairs = len(inter)
    base_tu, base_pairs = {}, None
    if BASELINE.is_file():
        for ln in BASELINE.read_text().splitlines():
            if not ln.strip():
                continue
            k, _, v = ln.partition("\t")
            if k == "(interleave-pairs)":
                base_pairs = int(v)
            else:
                base_tu[k] = int(v)

    def save():
        rows = [f"{tu}\t{n}" for tu, n in sorted(cur.items())]
        rows.append(f"(interleave-pairs)\t{pairs}")
        BASELINE.write_text("\n".join(rows) + "\n")

    if base_pairs is None:                 # no baseline yet: freeze the backlog
        save()
        print(f"tu-order: baseline frozen - {len(cur)} TU(s) with intra violations, "
              f"{pairs} interleave pair(s) ({BASELINE.name})")
        return 0
    risen = [(tu, base_tu.get(tu, 0), n) for tu, n in sorted(cur.items())
             if n > base_tu.get(tu, 0)]
    if risen or pairs > base_pairs:
        for tu, fl, n in risen:
            print(f"tu-order RATCHET VIOLATED: {tu}  {fl} -> {n} intra violation(s)",
                  file=sys.stderr)
        if pairs > base_pairs:
            print(f"tu-order RATCHET VIOLATED: interleave pairs  {base_pairs} -> {pairs}",
                  file=sys.stderr)
        print("a re-home/move broke the linker-order invariant (strictly-ascending, "
              "one contiguous block per TU) - fix the layout, never bless it up "
              f"(`python -m gruntz.audit.tu_order_check --tu <name>` for detail)",
              file=sys.stderr)
        return 2
    if cur != base_tu or pairs < base_pairs:
        save()                             # down-only roll
    print(f"tu-order: no new wiring defects; backlog {len(cur)} TU(s) / "
          f"{pairs} pair(s) (frozen in {BASELINE.name})")
    return 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--include-stub", action="store_true",
                    help="include src/Stub/ (expected to interleave until drained)")
    ap.add_argument("--exclude-pools", action="store_true",
                    help="drop COMDAT dtor/ctor-pool fns (isle-style FOLDED/inline skip)")
    ap.add_argument("--tu", help="show one TU's functions in file order")
    ap.add_argument("--inter-only", action="store_true")
    ap.add_argument("--intra-only", action="store_true")
    ap.add_argument("--csv", help="write per-TU span + violation counts")
    ap.add_argument("--gate", action="store_true",
                    help="build-tail ratchet: compare per-TU intra violations + the "
                         "interleave-pair count vs config/tu-order-baseline.tsv; any "
                         "RISE fails (exit 2); improvements roll the baseline DOWN "
                         "(the frozen-backlog pattern, like vtable-slot-binding)")
    args = ap.parse_args()

    tus = load_in_file_order(SRC, args.include_stub, args.exclude_pools)

    if args.gate:
        return _gate(check_intra(tus), check_inter(tus))

    if args.tu:
        seq = tus.get(args.tu)
        if not seq:
            print(f"no such TU: {args.tu}")
            return 2
        print(f"{args.tu}  ({len(seq)} functions, file order):")
        prev = None
        for e in seq:
            flag = "  <-- NOT ASCENDING" if prev and e.rva <= prev.rva else ""
            print(f"  L{e.line:<5} {e.rva:#08x} +{e.size:#06x} -> {e.end:#08x}  {e.name}{flag}")
            prev = e
        return 0

    intra = {} if args.inter_only else check_intra(tus)
    inter = [] if args.intra_only else check_inter(tus)

    print(f"scanned {len(tus)} TUs, "
          f"{sum(len(s) for s in tus.values())} functions "
          f"({'incl' if args.include_stub else 'excl'} src/Stub/)\n")

    if not args.inter_only:
        print(f"=== INTRA-TU (file order must be strictly ascending, no overlap) ===")
        if intra:
            for tu in sorted(intra):
                print(f"{tu}:  {len(intra[tu])} violation(s)")
                for v in intra[tu][:12]:
                    print(v)
                if len(intra[tu]) > 12:
                    print(f"  ... +{len(intra[tu]) - 12} more")
        else:
            print("  clean - every TU ascends in file order without overlap")
        print()

    if not args.intra_only:
        print(f"=== INTER-TU (each TU = one contiguous non-interleaving .text block) ===")
        if inter:
            for ta, (sa, ea), tb, (sb, eb) in inter:
                print(f"  {ta} [{sa:#08x}-{ea:#08x}]  INTERLEAVES  {tb} [{sb:#08x}-{eb:#08x}]")
        else:
            print("  clean - no two TUs' .text blocks interleave")
        print()

    if args.csv:
        spans = tu_spans(tus)
        rows = ["tu,start,end,funcs,intra_violations"]
        for tu, seq in sorted(tus.items()):
            s, e = spans[tu]
            rows.append(f"{tu},{s:#08x},{e:#08x},{len(seq)},{len(intra.get(tu, []))}")
        Path(args.csv).write_text("\n".join(rows) + "\n")
        print(f"wrote {args.csv}")

    nbad_tu = len(intra)
    npair = len(inter)
    ok = (nbad_tu == 0 and npair == 0)
    print(f"GATE: {'PASS' if ok else 'FAIL'}  "
          f"({nbad_tu} TUs with intra-order violations, {npair} interleaving TU-pairs)")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
