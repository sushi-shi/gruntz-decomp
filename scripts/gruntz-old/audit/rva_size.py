#!/usr/bin/env python3
"""rva_size.py - does every RVA(addr, size) claim FIT its admitted extent?

The `size` argument is the size AUTHORITY: the delinker carves the TARGET object
with it (the inventory carries starts + kinds only, its extents are derived to
the next admitted start and include trailing jump tables/alignment). Two
directions, and they are not symmetric:

  OVERRUN  ours > derived extent - the claim crosses the NEXT admitted start,
           so the carve would swallow another function's bytes. FATAL.
  MARGIN   ours < derived extent - normal: the tail is the function's own jump
           table / alignment. Reported (large margins are worth a look - a
           truncated claim hides here until the .text partition names its
           jump-table runs explicitly), never gated.

The historical SHORT class (a claim that silently truncated its own body - six
exacts lost that way) is no longer decidable from the inventory alone: the
admitted table stopped storing code-only sizes. It resurfaces as a MARGIN
outlier and, once jump-table runs become partition rows, as an exact check again.

    python -m gruntz.audit.rva_size            # report both directions
    python -m gruntz.audit.rva_size --gate     # exit 1 if any OVERRUN remains
    python -m gruntz.audit.rva_size --margin   # list every MARGIN row too
"""
import argparse
import csv
import re
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
FUNCS = REPO / "config/retail/functions.tsv"
ROOTS = ("src", "include")

# RVA(0x00xxxxxx, 0xNN) - the canonical spelling (gated by gruntz.audit.label_style).
# RVA_COMPGEN(addr, size, symbol) carries the same extent for compiler-generated bodies.
RVA_RE = re.compile(r"\bRVA(?:_COMPGEN)?\s*\(\s*(0x[0-9a-fA-F]+)\s*,\s*(0x[0-9a-fA-F]+)")


def retail_extents():
    if not FUNCS.is_file():
        raise SystemExit("[rva-size] tracked inventory missing: %s" % FUNCS)
    from gruntz.core.retail_functions import by_rva
    return {rva: row["size"] for rva, row in by_rva(FUNCS).items()}


def labels():
    for root in ROOTS:
        for path in sorted((REPO / root).rglob("*")):
            if path.suffix not in (".cpp", ".h"):
                continue
            for i, line in enumerate(path.read_text(errors="replace").split("\n"), 1):
                m = RVA_RE.search(line)
                if m:
                    yield path.relative_to(REPO), i, int(m.group(1), 16), int(m.group(2), 16)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--gate", action="store_true", help="exit 1 if any OVERRUN remains")
    ap.add_argument("--margin", action="store_true", help="list every MARGIN row too")
    a = ap.parse_args()

    extents = retail_extents()
    overrun, margin, unknown, ok = [], [], 0, 0
    for path, line, rva, size in labels():
        real = extents.get(rva)
        if real is None:
            unknown += 1
            continue
        if size > real:
            overrun.append((path, line, rva, size, real))
        elif size < real:
            margin.append((path, line, rva, size, real))
        else:
            ok += 1

    if overrun:
        print("OVERRUN - the claim crosses the next admitted start, the carve would "
              "swallow another function's bytes:")
        for path, line, rva, size, real in sorted(overrun, key=lambda r: -(r[3] - r[4])):
            print("  %s:%d  RVA(0x%08x, 0x%x)  derived extent 0x%x  (over by 0x%x)"
                  % (path, line, rva, size, real, size - real))
        print()
    if margin and a.margin:
        print("MARGIN - claim < derived extent (own jump table / alignment tail):")
        for path, line, rva, size, real in sorted(margin, key=lambda r: -(r[4] - r[3])):
            print("  %s:%d  RVA(0x%08x, 0x%x)  derived 0x%x  (tail 0x%x)"
                  % (path, line, rva, size, real, real - size))
        print()

    print("rva-size: %d exact, %d OVERRUN, %d with tail margin, %d absent from retail inventory"
          % (ok, len(overrun), len(margin), unknown))
    if a.gate and overrun:
        raise SystemExit(1)


main()
