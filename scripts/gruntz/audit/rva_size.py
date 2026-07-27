#!/usr/bin/env python3
"""rva_size.py - does every RVA(addr, size) label match the function's real extent?

The `size` argument is not decoration: the delinker carves the TARGET object using it.
Declare it short and the delinked function stops early, so objdiff compares our complete,
byte-perfect body against a TRUNCATED target and scores it below 100 forever. Nothing else
in the pipeline notices - the code is right, the yardstick is wrong.

That cost six exacts before anyone looked (three `IsLoaded` slots at 0x1a where the real
extent is 0x1d, `CMenuItem2::Disable` 0x14 vs 0x17, two accessors 0xa vs 0xe). Each had
quietly lost the function's last basic block. They were found by noticing the target asm was
a strict PREFIX of ours - an expensive way to learn something one subtraction can tell you.

Ground truth is Ghidra's carve (`build/ghidra-enrich/exports/functions.csv`, entry_rva +
byte_size). Two directions, and they are not symmetric:

  SHORT  ours < Ghidra's  - the costly one. The target is truncated and the score is a lie.
  LONG   ours > Ghidra's  - usually benign or a Ghidra under-carve (it splits on shared
                            tails and on data interleaved in .text), so it is reported but
                            does not gate.

Ghidra 12 carves fewer function starts than 11.4.2 did (see docs/gotchas.md), so an address
missing from the CSV is not a finding - it is skipped and counted.

    python -m gruntz.audit.rva_size            # report both directions
    python -m gruntz.audit.rva_size --gate     # exit 1 if any SHORT label remains
    python -m gruntz.audit.rva_size --long     # include the LONG list in full
"""
import argparse
import csv
import re
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
FUNCS = REPO / "build/ghidra-enrich/exports/functions.csv"
ROOTS = ("src", "include")

# RVA(0x00xxxxxx, 0xNN) - the canonical spelling (gated by gruntz.audit.label_style).
# RVA_COMPGEN(addr, size, symbol) carries the same extent for compiler-generated bodies.
RVA_RE = re.compile(r"\bRVA(?:_COMPGEN)?\s*\(\s*(0x[0-9a-fA-F]+)\s*,\s*(0x[0-9a-fA-F]+)")


def ghidra_extents():
    if not FUNCS.is_file():
        raise SystemExit("[rva-size] %s missing - run `gruntz init`" % FUNCS)
    out = {}
    with FUNCS.open() as fh:
        for row in csv.DictReader(fh):
            try:
                out[int(row["entry_rva"], 16)] = int(row["byte_size"])
            except (ValueError, KeyError):
                continue
    return out


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
    ap.add_argument("--gate", action="store_true", help="exit 1 if any SHORT label remains")
    ap.add_argument("--long", action="store_true", help="list every LONG label too")
    a = ap.parse_args()

    extents = ghidra_extents()
    short, long_, unknown, ok = [], [], 0, 0
    for path, line, rva, size in labels():
        real = extents.get(rva)
        if real is None:
            unknown += 1
            continue
        if size < real:
            short.append((path, line, rva, size, real))
        elif size > real:
            long_.append((path, line, rva, size, real))
        else:
            ok += 1

    if short:
        print("SHORT - the delinked target is TRUNCATED, so objdiff scores a complete body "
              "against a partial one:")
        for path, line, rva, size, real in sorted(short, key=lambda r: -(r[4] - r[3])):
            print("  %s:%d  RVA(0x%08x, 0x%x)  real 0x%x  (short by 0x%x)"
                  % (path, line, rva, size, real, real - size))
        print()
    if long_ and a.long:
        print("LONG - ours exceeds Ghidra's carve; usually a Ghidra under-carve "
              "(shared tail / interleaved data), verify before changing:")
        for path, line, rva, size, real in sorted(long_, key=lambda r: -(r[3] - r[4])):
            print("  %s:%d  RVA(0x%08x, 0x%x)  ghidra 0x%x" % (path, line, rva, size, real))
        print()

    print("rva-size: %d matched, %d SHORT, %d long, %d not carved by Ghidra"
          % (ok, len(short), len(long_), unknown))
    if a.gate and short:
        raise SystemExit(1)


main()
