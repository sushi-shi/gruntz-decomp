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

  ISLAND     no relocs, no calls - the code is self-contained
  SELF-CALL  calls only into itself (a loop) - self-contained, needs care
  DATA-ONLY  relocs, but ONLY to data symbols: constant tables and scratch buffers.
             Still harnessable - map the data beside the code. NOT the same problem as
             an import or a live global, which is why lumping all relocs together
             understated the reachable set.
  CALLS      calls another function; reachable only if the callees are reachable too
  IMPORTS    touches __imp_ / the CRT - needs the import table stood up

**Self-contained code is only half the question. The other half is the STATE PASSED IN.**
A function with zero relocations that takes a `CGruntzMgr*` still needs an entire object
graph synthesized before it can be called, while one taking `(int, int)` can be called
immediately. So the ranking here is by HARNESS COST, not by code size:

  cheap   only scalars, or pointers to small PODs (RECT, POINT, PALETTEENTRY, buffers)
  costly  one or more pointers to engine classes (PAV.../PAU... of a C-prefixed type)
  + `this` counts as an argument for a __thiscall member. KNOWN UNDER-COUNT: RunDecode1
    is a member whose body never dereferences `this`, which is exactly why it was
    harnessable - and this tool would still charge it 1 and hide it. Detecting that needs
    a disasm pass (does the body read off the this-register before overwriting it?).
    `CFaderShape::RenderTile(int,int)` at 59% is the obvious suspect. So the cost-0 set
    below is a LOWER BOUND on what is reachable, not the whole of it.

Sizing the two axes separately is the point. Ranking by code size alone produced a
61-row "worklist" of which most were unreachable: `CMapMgr::UpdateDiagonals` has zero
relocations and 0x1a3 bytes of pure computation, but takes a `CGruntzMgr*` and so pulls
in most of the engine. Self-contained CODE and cheap STATE are different questions and
both have to hold.

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
RELOC = re.compile(r"IMAGE_REL_\w+\s+(\S+)")

# A reloc to one of these is CHEAP: map the bytes beside the code. A reloc to an import
# or to another function is not. `?g_pal@@3PAU...` / `?g_255@@3MA` are data symbols -
# MSVC encodes a variable as `?name@@3<type><cv>` where the segment char is `3`.
DATA_SYM = re.compile(r"^\?[\w@?$]+@@3")
IMPORT_SYM = re.compile(r"^(__imp_|_[a-z]|__)")

# Parameter types that cost nothing to synthesize: scalars and small PODs the harness can
# fill in by hand. Anything else that is a pointer-to-class drags its object graph in -
# a CGruntzMgr* or CGrunt* pulls in most of the engine, which is why size is the WRONG
# ranking axis for harness cost.
POD = {"tagRECT", "tagPOINT", "tagPALETTEENTRY", "tagRGBQUAD", "tagSIZE",
       "ClipVtx", "Coord", "GruntCoord", "BrickzCell", "PidHeader", "PcxHeader",
       "WwdPlaneHeader", "BmpFileImage", "Bmp256Info", "SbiRect", "WwdRect",
       "DDSURFACEDESC", "WAVEFORMATEX", "LevelDims"}
PARAM_CLASS = re.compile(r"PA[VU](\w+)@@")
# ?Name@Cls@@ + access/convention. S* = static (no `this`), everything else with A/E is
# a member and `this` is an implicit argument.
STATIC_MEMBER = re.compile(r"@@S[A-Z]")


def harness_cost(name):
    """-> (cost, why). cost 0 = callable now, higher = more state to synthesize."""
    if not name.startswith("?"):
        return 0, "free function, C linkage"
    body = name.split("@@", 1)[1] if "@@" in name else name
    classes = [c for c in PARAM_CLASS.findall(name) if c not in POD]
    cost = len(classes)
    why = []
    if classes:
        why.append("takes " + ", ".join(c + "*" for c in sorted(set(classes))))
    is_member = "@" in name.split("@@")[0] and not STATIC_MEMBER.search(name)
    if is_member:
        cost += 1
        why.append("`this`")
    return cost, "; ".join(why) or "scalars/PODs only"
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

    rows, cur, calls, selfcalls, first, last = [], None, 0, 0, None, None
    dat = imp = other = 0

    def flush():
        if cur is None:
            return
        size = (last - first) if (first is not None and last is not None) else 0
        if imp:
            kind = "IMPORTS"
        elif other:
            kind = "RELOC"
        elif dat and not calls:
            kind = "DATA-ONLY"
        elif dat:
            kind = "CALLS"
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
            cur, calls, selfcalls, first, last = m.group(1), 0, 0, None, None
            dat = imp = other = 0
            continue
        if cur is None:
            continue
        mr = RELOC.search(line)
        if mr:
            t = mr.group(1)
            if IMPORT_SYM.match(t):
                imp += 1
            elif DATA_SYM.match(t):
                dat += 1
            else:
                other += 1
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
            if kind in ("ISLAND", "SELF-CALL", "DATA-ONLY"):
                cost, why = harness_cost(name)
                islands.append((cost, -size, size, name, kind, pct.get(name), why))

    total = sum(tally.values())
    print("recomp islands: %d functions in the delinked target  |  %s"
          % (total, "  ".join("%s %d" % (k, tally[k]) for k in sorted(tally))))
    reach = [r for r in islands if r[2] >= a.min_size]
    cheap = [r for r in reach if r[0] == 0]
    todo = [r for r in cheap if r[5] is not None and r[5] < 100.0]
    print("  self-contained CODE (ISLAND/SELF-CALL/DATA-ONLY): %d, %d of them >= 0x%x"
          % (len(islands), len(reach), a.min_size))
    print("  ...of those, %d need NO synthesized state (cost 0), and %d of those are "
          "NOT yet exact - that is the worklist" % (len(cheap), len(todo)))

    if not a.summary:
        print("\nOracle candidates - cost 0 means callable with scalars/PODs alone:")
        for cost, _neg, size, name, kind, p, why in sorted(todo)[:30]:
            score = "%6.2f%%" % p if p is not None else "   n/a "
            print("   0x%04x  %-9s %s  %-52s  %s"
                  % (size, kind, score, name[:52], why))
    return 0


if __name__ == "__main__":
    sys.exit(main())
