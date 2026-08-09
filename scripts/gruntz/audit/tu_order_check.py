#!/usr/bin/env python3
"""tu_order_check.py - the linker-layout acceptance gate.

link.exe 5.10 lays each .obj's .text contribution down as ONE CONTIGUOUS block,
input sections in the obj's section-table order (== cl's emission order == file
order), objs in link-line order, library members after all link-line objs in
resolution order (mechanism read out of link.exe + probe-proven:
docs/link-text-layout.md). So a faithful reconstruction must satisfy:

  INTRA-TU  within each .cpp, the RVA() functions appear in FILE ORDER that
            is strictly increasing in retail RVA, and their [rva, rva+size) spans
            do not overlap.  (file order == link order == RVA order.)

  INTER-TU  each TU occupies ONE contiguous .text block: its [min_start, max_end)
            span must not interleave another TU's span. Gaps between blocks are
            fine (unreconstructed code lives there); overlap/interleave is not -
            it means two .cpp files are really one .obj, or a body is misattributed.

THE ONE LEGITIMATE EXCEPTION - kept-COMDAT exiles. A multi-defined COMDAT (a
header-inline / compiler-adjacent member every including TU emits) is KEPT by the
FIRST obj on the link line that defines it and discarded from the rest, so its
retail address lies inside the KEEPER's contiguous block, not its class TU's.
Our tree homes such bodies with their class (the class-homing convention), which
is correct source structure the linker invariant cannot see. Those bodies are
enumerated - each with evidence - in config/retail/kept-comdat-exiles.tsv,
excluded from both checks, counted explicitly, and VERIFIED every run: a row
whose rva is no longer pinned in its owner unit, or no longer lies within
(+/-0x1800 seam slack of) its host unit's span, FAILS the audit.

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

from gruntz.audit.tu_layout import RVA_RE, SIG_RE, _parse_size, pooled

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
SRC = REPO / "src"

# The kept-COMDAT exile ledger (see module docstring). rva -> (owner, host, name).
EXILES_TSV = REPO / "config" / "retail" / "kept-comdat-exiles.tsv"
# A kept COMDAT sits inside its host's run or at its seam: cl emits the deferred
# inline copies at first-use points and at the TU tail, so the group may start
# just past the host's last *pinned* fn (MenuPage: +0x2ee). Measured max, rounded.
SEAM_SLACK = 0x1800


def load_exiles():
    """rva -> (owner_unit, host_unit, name). Empty dict if no ledger."""
    out = {}
    if EXILES_TSV.is_file():
        for ln in EXILES_TSV.read_text().splitlines():
            if not ln.strip() or ln.startswith("#"):
                continue
            f = ln.split("\t")
            if len(f) >= 4:
                out[int(f[0], 16)] = (f[1], f[2], f[3])
    return out


def verify_exiles(exiles, claimed, spans) -> list:
    """The ledger is evidence, so it is re-proven every run. Returns violation
    strings: a row whose rva is not pinned in owner_unit's .cpp (stale claim),
    or whose rva lies outside host_unit's span +/- SEAM_SLACK (stale host)."""
    bad = []
    for rva, (owner, host, name) in sorted(exiles.items()):
        got = claimed.get(rva)
        if got is None:
            bad.append(f"exile {rva:#010x} {name}: no RVA() pin found in src (owner {owner})")
        elif got != owner:
            bad.append(f"exile {rva:#010x} {name}: pinned in {got}, ledger says {owner}")
        sp = spans.get(host)
        if sp is None:
            bad.append(f"exile {rva:#010x} {name}: host unit {host} has no span")
        elif not (sp[0] - SEAM_SLACK <= rva < sp[1] + SEAM_SLACK):
            bad.append(f"exile {rva:#010x} {name}: outside host {host} "
                       f"[{sp[0]:#x}-{sp[1]:#x}] +/-{SEAM_SLACK:#x}")
    return bad


class Entry:
    __slots__ = ("rva", "size", "line", "name", "tu")

    def __init__(self, rva, size, line, name, tu):
        self.rva, self.size, self.line, self.name, self.tu = rva, size, line, name, tu

    @property
    def end(self) -> int:
        return self.rva + self.size


def load_in_file_order(src: Path, include_stub: bool, exclude_pools: bool):
    """Every RVA() function per .cpp, in FILE (source) order (NOT rva-sorted).

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
            if not m:
                continue
            rva, size = int(m.group(1), 16), _parse_size(m.group(2))
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


def split_exiles(tus, exiles):
    """(tus_without_exile_bodies, claimed rva->tu map, n_dropped). Exiled bodies
    are placement-wise part of their HOST's contribution, so neither the intra
    file-order invariant nor the owner's span may include them."""
    claimed = {e.rva: tu for tu, seq in tus.items() for e in seq}
    if not exiles:
        return tus, claimed, 0
    clean, dropped = {}, 0
    for tu, seq in tus.items():
        keep = [e for e in seq if e.rva not in exiles]
        dropped += len(seq) - len(keep)
        if keep:
            clean[tu] = keep
    return clean, claimed, dropped


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


# A body sitting this far from the rest of its TU, in a group no bigger than
# MAX_OUTLIERS, is not part of that compiland's contiguous run - it is one
# misplaced function (usually a ctor/dtor the linker placed at first use).
OUTLIER_GAP = 0x8000
MAX_OUTLIERS = 3


def tu_cluster(seq):
    """(cluster, outliers) - the TU's dense run, and the bodies far from it.

    THE CAUSE, not the consequence. One function stranded half an image away
    stretches its TU's extent across everything in between, so it alone
    manufactures a pair with every unit in that range - which is why a handful
    of these read as >1000 interleave pairs. Peel the small remote groups off
    and what is left is the compiland's real contribution."""
    ent = sorted((e for e in seq if not pooled(e.rva)), key=lambda e: e.rva)
    outliers = []
    while len(ent) > 1:
        rvas = [e.rva for e in ent]
        gap, i = max((rvas[k + 1] - rvas[k], k) for k in range(len(rvas) - 1))
        if gap < OUTLIER_GAP:
            break
        lo, hi = ent[:i + 1], ent[i + 1:]
        drop = lo if len(lo) <= len(hi) else hi      # the minority side is the stray
        if len(drop) > MAX_OUTLIERS:
            break                                    # a genuine two-cluster TU
        outliers += drop
        ent = [e for e in ent if e not in drop]
    return ent, outliers


def tu_outliers(tus):
    """[(tu, entry, distance_from_cluster)] over the whole tree."""
    out = []
    for tu, seq in sorted(tus.items()):
        cluster, strays = tu_cluster(seq)
        if not cluster:
            continue
        lo, hi = cluster[0].rva, cluster[-1].rva
        for e in strays:
            out.append((tu, e, lo - e.rva if e.rva < lo else e.rva - hi))
    return out


def tu_spans(tus):
    """tu -> (min_start, max_end); the DENSE RUN defines the block extent."""
    spans = {}
    for tu, seq in tus.items():
        own, _strays = tu_cluster(seq)
        if not own:
            continue
        starts = [e.rva for e in own]
        ends = [e.end for e in own if e.size]
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


BASELINE = REPO / "config" / "cleanliness" / "tu-order-baseline.tsv"


def _gate(intra, inter, exile_bad=(), n_exiled=0) -> int:
    """Down-only ratchet vs the committed backlog. A TU whose intra-violation
    count RISES (or a brand-new offender TU, or a rise in the total interleave
    pair count) fails the build; improvements roll the baseline down. Floors
    are never raised by tooling - fixing the layout is the only way down."""
    if exile_bad:
        for b in exile_bad:
            print(f"tu-order EXILE LEDGER STALE: {b}", file=sys.stderr)
        print("kept-comdat-exiles.tsv rows are re-proven every run - fix or delete "
              "the stale row, never widen the slack", file=sys.stderr)
        return 2
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
          f"{pairs} pair(s) (frozen in {BASELINE.name}); "
          f"{n_exiled} kept-COMDAT exile bodies excluded ({EXILES_TSV.name}, verified)")
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
    ap.add_argument("--outliers", action="store_true",
                    help="report the CAUSE: every body sitting far from its TU's "
                         "dense run. These are what inflate the interleave-pair "
                         "count - one stray spans every unit in between - so this "
                         "is the actionable worklist, not the pair total.")
    ap.add_argument("--gate", action="store_true",
                    help="build-tail ratchet: compare per-TU intra violations + the "
                         "interleave-pair count vs config/cleanliness/tu-order-baseline.tsv; any "
                         "RISE fails (exit 2); improvements roll the baseline DOWN "
                         "(the frozen-backlog pattern, like vtable-slot-binding)")
    args = ap.parse_args()

    tus = load_in_file_order(SRC, args.include_stub, args.exclude_pools)
    exiles = load_exiles()
    tus_all = tus                                     # exiles still visible in --tu
    tus, claimed, n_exiled = split_exiles(tus, exiles)
    exile_bad = verify_exiles(exiles, claimed, tu_spans(tus))

    if args.gate:
        return _gate(check_intra(tus), check_inter(tus), exile_bad, n_exiled)

    if args.outliers:
        out = tu_outliers(tus)
        by_tu = {}
        for tu, e, dist in out:
            by_tu.setdefault(tu, []).append((e, dist))
        for tu in sorted(by_tu):
            print(f"{tu}:")
            for e, dist in sorted(by_tu[tu], key=lambda x: x[0].rva):
                print(f"    {e.rva:#08x} +{e.size:#06x}  {dist:#9x} from cluster  {e.name}")
        print(f"\n{len(out)} outlier bod(ies) across {len(by_tu)} TU(s) "
              f"(>= {OUTLIER_GAP:#x} from the run, groups of <= {MAX_OUTLIERS})")
        return 0

    if args.tu:
        seq = tus_all.get(args.tu)
        if not seq:
            print(f"no such TU: {args.tu}")
            return 2
        print(f"{args.tu}  ({len(seq)} functions, file order):")
        prev = None
        for e in seq:
            if e.rva in exiles:
                print(f"  L{e.line:<5} {e.rva:#08x} +{e.size:#06x} -> {e.end:#08x}  {e.name}"
                      f"  [kept-COMDAT exile -> {exiles[e.rva][1]}]")
                continue
            flag = "  <-- NOT ASCENDING" if prev and e.rva <= prev.rva else ""
            print(f"  L{e.line:<5} {e.rva:#08x} +{e.size:#06x} -> {e.end:#08x}  {e.name}{flag}")
            prev = e
        return 0

    intra = {} if args.inter_only else check_intra(tus)
    inter = [] if args.intra_only else check_inter(tus)

    print(f"scanned {len(tus)} TUs, "
          f"{sum(len(s) for s in tus.values())} functions "
          f"({'incl' if args.include_stub else 'excl'} src/Stub/); "
          f"{n_exiled} kept-COMDAT exile bodies excluded ({EXILES_TSV.name})\n")
    if exile_bad:
        print("=== EXILE LEDGER (rows failing re-verification) ===")
        for b in exile_bad:
            print(f"  {b}")
        print()

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
    ok = (nbad_tu == 0 and npair == 0 and not exile_bad)
    print(f"GATE: {'PASS' if ok else 'FAIL'}  "
          f"({nbad_tu} TUs with intra-order violations, {npair} interleaving TU-pairs, "
          f"{len(exile_bad)} stale exile rows)")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
