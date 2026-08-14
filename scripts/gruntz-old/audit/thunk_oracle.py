#!/usr/bin/env python3
"""thunk_oracle.py - read retail's incremental-link thunk band as a TU/library oracle.

Retail was linked `/INCREMENTAL`, so MSVC put a jump table of 5-byte `E9 rel32` thunks
at the top of `.text` and routes calls through it. Two properties make that a decidable
question about every function (see docs/patterns/incremental-thunks-reveal-tu-boundaries.md):

  * the linker thunks only objects ON THE LINK LINE - a `.LIB` member never gets one;
  * inside a thunked object it thunks essentially every call (measured: 6728 thunked
    vs 1 direct), including intra-object ones.

**The test is ASYMMETRIC.** A thunk exists per function that is directly CALLED, so:

    thunk present -> PROOF the function was compiled into a .obj on the link line
    thunk absent  -> proves NOTHING on its own: it is either a .LIB member, or a
                     function nothing calls directly (an address-taken callback such
                     as a window proc, a vtable-only virtual, or dead code)

At UNIT level absence is still strong - a 118-function unit with zero thunks is a
library member, not 118 uncalled functions. At FUNCTION level it is not. So only the
`object-side minority` of a mixed unit is a proven defect; a `library-side minority`
is usually just an uncalled function and is reported separately.

    python -m gruntz.audit.thunk_oracle              # summary + the mixed worklist
    python -m gruntz.audit.thunk_oracle --units      # per-unit table
    python -m gruntz.audit.thunk_oracle --lib-list   # library-side unit names, one per line
"""

import argparse
import bisect
import collections
import csv
import struct
import sys
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
NAMES = REPO / "build" / "gen" / "symbol_names.csv"


def thunk_targets(pe, band_end=None):
    """{rva} every function an `E9` in the leading band jumps to.

    The band is a RESERVED region with growth slack, so it does not end at the last
    `E9`; walk 5-byte slots and only stop after a long non-`E9` run."""
    d = pe.data
    _n, tva, _tvsz, trp, trsz = pe.text
    out, i, last = set(), 0, 0
    while i + 5 <= trsz:
        if d[trp + i] == 0xE9:
            out.add(tva + i + 5 + struct.unpack_from("<i", d, trp + i + 1)[0])
            last = i
        elif band_end is None and i - last > 400:
            break
        elif band_end is not None and tva + i >= band_end:
            break
        i += 5
    return out


def classify(pe=None):
    """{unit: (thunked_fns, library_fns)} plus the raw per-function verdict."""
    from gruntz.core.pe import PE
    pe = pe or PE()
    tg = thunk_targets(pe)
    per = collections.defaultdict(lambda: [[], []])
    if not NAMES.exists():
        sys.exit(f"[thunk-oracle] missing {NAMES} - run `gruntz build` first")
    for r in csv.DictReader(NAMES.open()):
        if r["kind"] != "func":
            continue
        rva = int(r["rva"], 16)
        per[r["unit"]][0 if rva in tg else 1].append((rva, r["name"]))
    return per, tg


def main(argv=None):
    ap = argparse.ArgumentParser(description="incremental-thunk TU/library oracle")
    ap.add_argument("--units", action="store_true", help="per-unit table")
    ap.add_argument("--lib-list", action="store_true",
                    help="print library-side unit names (feed to link.py --lib)")
    args = ap.parse_args(argv)

    per, tg = classify()
    obj = {u for u, (t, n) in per.items() if t and not n}
    lib = {u for u, (t, n) in per.items() if n and not t}
    mixed = {u: (t, n) for u, (t, n) in per.items() if t and n}

    if args.lib_list:
        for u in sorted(lib):
            print(u)
        return 0

    print(f"[thunk-oracle] {len(tg)} distinct thunk targets")
    print(f"  link-line objects : {len(obj):4d} unit(s)")
    print(f"  library members   : {len(lib):4d} unit(s)")
    print(f"  MIXED (defects)   : {len(mixed):4d} unit(s)")

    if args.units:
        for u, (t, n) in sorted(per.items()):
            kind = "MIXED" if t and n else ("object" if t else "library")
            print(f"    {u:28s} {kind:8s} thunked {len(t):3d}  library {len(n):3d}")
        return 0

    # A defect needs the unthunked side to be big enough that "nothing calls ANY of
    # them" is implausible - otherwise absence is just "not directly called". A tie
    # (1 thunked / 1 not) is never proof: the thunk proves that one is object-side and
    # says nothing about the other.
    proven = {u: (t, n) for u, (t, n) in mixed.items() if len(t) < len(n) and len(n) >= 3}
    weak = {u: (t, n) for u, (t, n) in mixed.items()
            if not (len(t) < len(n) and len(n) >= 3)}
    if proven:
        print("\n  PROVEN defects - a thunk proves the function was in a link-line object,\n"
              "  but the rest of its TU is a library member, so the TU spans two compilands:\n")
        for u, (t, n) in sorted(proven.items(), key=lambda kv: len(kv[1][0])):
            print(f"    {u} ({len(t)} object / {len(n)} library) - move {len(t)}:")
            for rva, nm in sorted(t)[:8]:
                print(f"       {rva:#08x}  {nm[:60]}")
            if len(t) > 8:
                print(f"       ... +{len(t)-8} more")
    if weak:
        print("\n  NOT defects - an object-side TU with a few unthunked functions. Absence of a\n"
              "  thunk only means nothing calls them directly (callbacks, vtable-only, dead):\n")
        for u, (t, n) in sorted(weak.items()):
            for rva, nm in sorted(n)[:4]:
                print(f"    {u:22s} {rva:#08x}  {nm[:52]}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
