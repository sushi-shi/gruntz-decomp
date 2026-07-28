#!/usr/bin/env python3
"""recomp_islands.py - which retail functions can be EXECUTED as an oracle?

`recomp/harness/pidrun.c` proved something stronger than byte-matching: it maps GRUNTZ.EXE,
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
  + `this` is MEASURED, not assumed. A __thiscall member whose body never dereferences
    the incoming ecx needs no object fabricated - that is exactly why RunDecode1 was
    harnessable - so a disasm pass decides it per function and the listing says
    "no `this`" or "needs `this`". 311 of the 693 self-contained members turn out never
    to touch it. Two cases the pass has to get right, and did not at first: an /Od-ish
    prologue spilling `this` into a frame slot it never reads back (RunDecode1), and an
    intra-function `jmp`, which forwards nothing. `CFaderShape::RenderTile` was the
    standing suspect and is NOT one of them - it moves ecx to edx and then reads
    0x58(%edx), so it wants a real CFaderShape.

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
# The REAL measure of how much state a function needs is not its parameter TYPES - it is
# how many distinct struct offsets it actually dereferences. A CGruntzMgr* parameter looks
# like "the whole engine", but CMapMgr::UpdateDiagonals touches exactly 11 fields off it
# and is a fabricable stub. Ranking by parameter type was as wrong as ranking by code size.
# %esp/%ebp displacements are stack frame, not object fields, so they are excluded.
FIELD = re.compile(r"(?:0x([0-9a-f]+))?\((%e(?!sp|bp)[a-z][a-z])\)")
# ?Name@Cls@@ + access/convention. In the MSVC scheme the char after `@@` is the
# function class (Q public, I protected, A private, S static, ...), then a CV
# modifier, then the CALLING CONVENTION - `E` is __thiscall. Free functions are
# `@@Y<conv>`, so excluding a leading Y is what separates a member from them.
THISCALL_MEMBER = re.compile(r"@@[^Y][A-Z]E")

# ---------------------------------------------------------------------------
# Does the body actually USE `this`?
#
# A __thiscall member whose body never dereferences the incoming ecx is exactly
# as harnessable as a free function - that is why CDDSurface::RunDecode1 could
# be executed at all. Charging every member for a fabricated object was the
# audit's own known under-count; this is the pass that removes it.
#
# llvm-objdump prints AT&T, so the DESTINATION is the last operand. The rule:
#
#   * `this` starts in ecx, and a plain reg->reg move propagates the taint
#     (retail almost always parks it in esi/edi immediately);
#   * any memory operand through a tainted register, or a `push` of one, is a
#     USE - stop, the function needs a real object;
#   * a tainted register written by a pure-write instruction (mov/lea/pop/...)
#     is KILLED - `this` never mattered;
#   * a read-modify-write (add/sub/and/...) of a tainted register is a USE;
#   * a CALL, or a tail `jmp` OUT of the function, while any taint is live is
#     conservatively a USE, since ecx may be forwarded. An intra-function branch
#     is not - llvm-objdump annotates the target `<sym+0x..>`, which is how the
#     two are told apart. Getting that wrong hid RunDecode1 behind its own
#     `jmp` to the epilogue.
#
# One refinement is not optional, because it is the motivating case. An /Od-ish
# prologue spills `this` to the frame (`movl %ecx, -0x24(%ebp)`) whether or not
# it is ever read back - CDDSurface::RunDecode1 does exactly that and then never
# touches the slot again, which is why it was harnessable. A naive reader counts
# that store as a use and hides the one function we already proved reachable. So
# a spill to an EBP-relative slot taints the SLOT instead, and only a later read
# of it counts. ESP-relative spills are still treated as uses: the displacement
# moves with every push, so tracking them here would not be sound.
#
# Deliberately conservative otherwise: it can say "uses this" when the value is
# dead, and it will not say "unused" when it is live. `push %ecx` is counted as
# a use even though MSVC also emits it purely to reserve four bytes of stack,
# because the two are not distinguishable from one instruction.
PURE_WRITE = ("mov", "movl", "movb", "movw", "movzbl", "movzwl", "movsbl", "movswl",
              "lea", "leal", "pop", "popl", "xchg")
REG_IN_MEM = re.compile(r"\(%(e[a-z][a-z])[,)]")
REG_BARE = re.compile(r"^%(e[a-z][a-z])$")
REGMOVE = re.compile(r"^mov[lbw]?\s+%(e[a-z][a-z]),\s*%(e[a-z][a-z])$")
EBP_SLOT = re.compile(r"^-?0x[0-9a-f]+\(%ebp\)$")
SELFZERO = re.compile(r"^(?:xor|sub)[lbw]?\s+%(e[a-z][a-z]),\s*%\1$")
MNEMONIC = re.compile(r"^\s*[0-9a-f]+:\s+(\S+)\s*(.*)$")


class ThisTracker(object):
    """Per-function `this` liveness. Feed it instruction lines in address order."""

    def __init__(self, sym=""):
        self.sym = sym
        self.taint = set(["ecx"])
        self.slots = set()  # ebp-relative frame slots holding a dead copy of `this`
        self.uses = False
        self.done = False

    def feed(self, line):
        if self.done:
            return
        m = MNEMONIC.match(line.rstrip())
        if not m:
            return
        mnem, ops = m.group(1), m.group(2).strip()
        internal = ("<%s+" % self.sym) in ops or ("<%s>" % self.sym) in ops
        # drop the `<sym+0x..>` branch annotation llvm-objdump appends
        ops = ops.split("<")[0].strip()

        if mnem.startswith("j") and internal:
            return  # intra-function control flow forwards nothing
        if mnem.startswith(("call", "jmp")) and (self.taint or self.slots):
            self.uses = self.done = True
            return

        parts = [p.strip() for p in ops.split(",")] if ops else []
        # A spill of `this` into a stable frame slot is not yet a use - only a
        # later read of that slot is. See the note above.
        if len(parts) == 2 and mnem in ("mov", "movl"):
            src, dst = parts
            sm = REG_BARE.match(src)
            if sm and sm.group(1) in self.taint and EBP_SLOT.match(dst):
                self.slots.add(dst)
                return
        for p in parts[:-1] if len(parts) > 1 else parts:
            if p in self.slots:
                self.uses = self.done = True
                return

        for r in REG_IN_MEM.findall(ops):
            if r in self.taint:
                self.uses = self.done = True
                return
        if mnem.startswith("push"):
            if ops.strip("%") in self.taint:
                self.uses = self.done = True
            return

        mm = REGMOVE.match(mnem + " " + ops)
        if mm and mm.group(1) in self.taint:
            self.taint.add(mm.group(2))
            return
        if SELFZERO.match(mnem + " " + ops):
            self.taint.discard(SELFZERO.match(mnem + " " + ops).group(1))
        else:
            for i, p in enumerate(parts):
                bare = REG_BARE.match(p)
                if not bare or bare.group(1) not in self.taint:
                    continue
                last = (i == len(parts) - 1)
                if last and mnem in PURE_WRITE:
                    self.taint.discard(bare.group(1))
                else:
                    self.uses = self.done = True
                    return
        if not self.taint and not self.slots:
            self.done = True  # `this` died untouched

    def verdict(self):
        return self.uses


def harness_cost(name, fields, needs_this=False):
    """-> (cost, why). `fields` is the measured count of distinct struct offsets the body
    dereferences - the number of members a harness must fabricate. That is the honest
    cost; the declared parameter types are not."""
    classes = sorted({c for c in PARAM_CLASS.findall(name) if c not in POD})
    member = bool(THISCALL_MEMBER.search(name))
    tag = ""
    if member:
        tag = ", needs `this`" if needs_this else ", no `this`"
    if not fields:
        return 0, "touches no object fields" + tag
    why = "%d field(s)" % fields
    if classes:
        why += " off " + ", ".join(c + "*" for c in classes[:2])
    return fields, why + tag
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
    fields = set()
    tracker = None

    def flush():
        if cur is None:
            return
        size = (last - first) if (first is not None and last is not None) else 0
        nfields = len(fields)
        # A member that never touches the incoming ecx needs no object at all.
        needs_this = bool(THISCALL_MEMBER.search(cur)) and (
            tracker is None or tracker.verdict())
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
        rows.append((cur, size, kind, nfields, needs_this))

    for line in out.split("\n"):
        m = SYM.match(line)
        if m:
            flush()
            cur, calls, selfcalls, first, last = m.group(1), 0, 0, None, None
            dat = imp = other = 0
            fields = set()
            tracker = ThisTracker(cur) if THISCALL_MEMBER.search(cur) else None
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
        if tracker is not None:
            tracker.feed(line)
        for mf in FIELD.finditer(line):
            fields.add(int(mf.group(1), 16) if mf.group(1) else 0)
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
    ap.add_argument("--max-fields", type=int, default=12,
                    help="how many object fields a harness may fabricate (default 12)")
    a = ap.parse_args()

    pct = scores()
    tally, islands = {}, []
    members = freed = 0
    for obj in sorted(TARGET.glob("*.obj")):
        for name, size, kind, nf, needs_this in classify_obj(obj):
            tally[kind] = tally.get(kind, 0) + 1
            if kind in ("ISLAND", "SELF-CALL", "DATA-ONLY"):
                if THISCALL_MEMBER.search(name):
                    members += 1
                    if not needs_this:
                        freed += 1
                cost, why = harness_cost(name, nf, needs_this)
                islands.append((cost, -size, size, name, kind, pct.get(name), why))

    total = sum(tally.values())
    print("recomp islands: %d functions in the delinked target  |  %s"
          % (total, "  ".join("%s %d" % (k, tally[k]) for k in sorted(tally))))
    reach = [r for r in islands if r[2] >= a.min_size]
    cheap = [r for r in reach if r[0] <= a.max_fields]
    todo = [r for r in cheap if r[5] is not None and r[5] < 100.0]
    print("  self-contained CODE (ISLAND/SELF-CALL/DATA-ONLY): %d, %d of them >= 0x%x"
          % (len(islands), len(reach), a.min_size))
    print("  ...of those, %d touch <= %d object field(s), and %d of those are NOT yet "
          "exact - that is the worklist" % (len(cheap), a.max_fields, len(todo)))
    print("  __thiscall members among the self-contained: %d, of which %d NEVER touch "
          "`this` and so need no object fabricated at all" % (members, freed))

    if not a.summary:
        print("\nOracle candidates - `N field(s)` is how many members a harness must "
              "fabricate:")
        for cost, _neg, size, name, kind, p, why in sorted(todo)[:30]:
            score = "%6.2f%%" % p if p is not None else "   n/a "
            print("   0x%04x  %-9s %s  %-52s  %s"
                  % (size, kind, score, name[:52], why))
    return 0


if __name__ == "__main__":
    sys.exit(main())
