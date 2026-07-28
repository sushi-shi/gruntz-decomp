#!/usr/bin/env python3
"""recomp_islands.py - which retail functions can be EXECUTED as an oracle?

`tools/recomp/pidrun.c` proved something stronger than byte-matching: it maps GRUNTZ.EXE,
applies `.reloc`, and CALLS retail's own `CDDSurface::RunDecode1` through inline asm, so
retail itself decides whether our reimplementation is right. That validated 9,821 sprites
at 100% identical pixels.

It works because that function is an ISLAND - self-contained machine code:

  * no relocations at all (no imports, no globals, no calls to other functions)
  * no CALL instruction of any kind
  * it never dereferences `this`; every input arrives as a scalar or a caller-supplied
    buffer, and every output goes to a caller-supplied buffer

Anything holding application state is out of reach - you would have to stand up the CRT,
the import table, DirectDraw, the heap. But "needs state" is not the same as "most of
them", and the question of how many other islands exist is answerable rather than
arguable. This tool answers it from the delinked TARGET objects, which are retail's own
bytes.

  ISLAND     no relocs, no calls - directly executable, an oracle candidate
  SELF-CALL  calls only into itself (a tail-recursive loop) - executable, needs care
  CALLS      calls something else; an island only if its callees are islands too
  RELOC      touches a global or an import - needs the image mapped and its data live

An ISLAND is not automatically WORTH harnessing: a two-instruction accessor proves
nothing. Size and current match% are printed so the payoff is visible - the interesting
rows are big, complex, and NOT yet exact, because those are where our reading is most
likely wrong and where a differential test is worth writing.

    python -m gruntz.audit.recomp_islands             # the ranked candidate list
    python -m gruntz.audit.recomp_islands --summary   # counts only
    python -m gruntz.audit.recomp_islands --min-size 0x40
"""
import argparse
import json
import re
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
TARGET = REPO / "build/objdiff/target"

SYM = re.compile(r"^[0-9a-f]+ <(.+)>:")
RELOC = re.compile(r"IMAGE_REL_")
CALL = re.compile(r"^\s+[0-9a-f]+:\s+calll?\s")
INSN = re.compile(r"^\s+([0-9a-f]+):\s+\S")


def scores():
    path = REPO / "build/objdiff/report.json"
    if not path.is_file():
        raise SystemExit("[islands] %s missing - run `gruntz build`" % path)
    rep = json.loads(path.read_text())
    # report.json omits fuzzy_match_percent when it is exactly 0.0 (serde skip-default)
    return {f["name"]: f.get("fuzzy_match_percent", 0.0)
            for u in rep["units"] for f in u.get("functions", [])}


def classify_obj(obj):
    """-> [(name, size, kind)] for one delinked target object."""
    try:
        out = subprocess.run(
            ["llvm-objdump", "-dr", "--no-show-raw-insn", str(obj)],
            capture_output=True, text=True, check=False).stdout
    except FileNotFoundError:
        raise SystemExit("[islands] llvm-objdump not on PATH - run inside `nix develop`")

    rows, cur, relocs, calls, selfcalls, first, last = [], None, 0, 0, 0, None, None

    def flush():
        if cur is None:
            return
        size = (last - first) if (first is not None and last is not None) else 0
        if relocs:
            kind = "RELOC"
        elif calls and calls == selfcalls:
            kind = "SELF-CALL"
        elif calls:
            kind = "CALLS"
        else:
            kind = "ISLAND"
        rows.append((cur, size, kind))

    for line in out.split("\n"):
        m = SYM.match(line)
        if m:
            flush()
            cur, relocs, calls, selfcalls, first, last = m.group(1), 0, 0, 0, None, None
            continue
        if cur is None:
            continue
        if RELOC.search(line):
            relocs += 1
            continue
        mi = INSN.match(line)
        if mi:
            off = int(mi.group(1), 16)
            if first is None:
                first = off
            last = off
        if CALL.match(line):
            calls += 1
            # a call whose target names THIS symbol is a self-call (a loop), not an
            # external dependency
            if cur is not None and ("<%s+" % cur) in line:
                selfcalls += 1
    flush()
    return rows


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--summary", action="store_true", help="counts only")
    ap.add_argument("--min-size", type=lambda s: int(s, 0), default=0x30,
                    help="ignore islands smaller than this (default 0x30)")
    a = ap.parse_args()

    pct = scores()
    tally, islands = {}, []
    for obj in sorted(TARGET.glob("*.obj")):
        for name, size, kind in classify_obj(obj):
            tally[kind] = tally.get(kind, 0) + 1
            if kind in ("ISLAND", "SELF-CALL"):
                islands.append((size, name, kind, pct.get(name)))

    total = sum(tally.values())
    print("recomp islands: %d functions in the delinked target  |  %s"
          % (total, "  ".join("%s %d" % (k, tally[k]) for k in sorted(tally))))
    big = [r for r in islands if r[0] >= a.min_size]
    print("  executable (ISLAND + SELF-CALL): %d, of which %d are >= 0x%x bytes"
          % (len(islands), len(big), a.min_size))

    if not a.summary:
        print("\nOracle candidates, largest first - a big NON-exact island is where a "
              "differential test pays most:")
        for size, name, kind, p in sorted(big, reverse=True)[:40]:
            score = "%6.2f%%" % p if p is not None else "   n/a "
            print("   0x%04x  %-9s %s  %s" % (size, kind, score, name[:62]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
