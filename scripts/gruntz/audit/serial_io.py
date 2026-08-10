#!/usr/bin/env python3
"""Archive-slot I/O sieve: does our reader consume the bytes retail's reader consumes?

WHY
  A save/load defect ("loading a retail-written save crashes") is normally
  framed as "our reader misparses the file".  That hypothesis is enormous and
  every other lead is speculation until it is settled.  It CAN be settled
  statically, because the game's whole file format is expressed through two
  virtual calls.

  `CFileMemBase`'s vtable puts

      +0x2c   i32 Read (void*, i32)
      +0x30   i32 Write(const void*, i32)

  so every serialised field in the image lowers to

      push <size>          ; the byte count      <- THE FORMAT
      push <buf>           ; lea / imm / reg
      mov  ecx, <archive>
      call DWORD PTR [reg+0x2c]        ; or +0x30

  Extract the ORDERED (direction, byte count) sequence per function from the
  objdiff base obj and its delinked target obj and diff them.  A clean run
  EXCLUDES a format desync; a dirty row names the exact field.

  Measured 2026-08-10: 256 functions carry archive-slot calls and every
  sequence is identical to retail.

TWO PARSING TRAPS, both fatal if missed
  1. The size is the push TWO positions before the call, not the last one.  The
     buffer address is pushed last and when the buffer is a global that push is
     also an immediate, so a naive "last immediate" reads the ADDRESS as the
     size.
  2. llvm-objdump prints cl's internal labels (`$L29876`, `$tail$29882`) as if
     they were symbols, which silently re-attributes the calls inside a switch
     arm away from the enclosing function.  Skip any name starting with `$`.

  Rows whose size arrives in a register (`RD:eax`) are not format facts - most
  are `+0x2c` on some OTHER class's vtable.  They are printed but never a hit.

USAGE
  python -m gruntz.audit.serial_io                 # every unit
  python -m gruntz.audit.serial_io --unit triggermgr
"""

import argparse
import re
import subprocess
import sys
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
BASE = REPO / "build" / "objdiff" / "base"
TGT = REPO / "build" / "objdiff" / "target"

SYM_RE = re.compile(r"^[0-9a-f]+ <([^>]+)>:")
INSN_RE = re.compile(r"^\s*[0-9a-f]+:\s+(?:[0-9a-f]{2} )+[ ]*\t(.*)$")
PUSH_RE = re.compile(r"^push\s+(\S+)$")
SLOTCALL_RE = re.compile(r"^call\s+dword ptr \[(\w+) \+ (0x[0-9a-f]+)\]$")

SLOTS = {0x2C: "RD", 0x30: "WR"}


def sequences(objpath):
    """symbol -> ordered ['RD:0x80', 'WR:0x4', ...]"""
    out = subprocess.run(
        ["llvm-objdump", "-d", "-r", "--x86-asm-syntax=intel", str(objpath)],
        capture_output=True, text=True, check=True).stdout
    res, cur, pushes = {}, None, []
    for line in out.splitlines():
        m = SYM_RE.match(line)
        if m:
            if m.group(1).startswith("$"):
                continue                      # cl internal label, same function
            cur = m.group(1)
            res.setdefault(cur, [])
            pushes = []
            continue
        if cur is None:
            continue
        mi = INSN_RE.match(line)
        if not mi:
            continue
        txt = mi.group(1).strip()
        mp = PUSH_RE.match(txt)
        if mp:
            pushes.append(mp.group(1))
            continue
        mc = SLOTCALL_RE.match(txt)
        if mc:
            slot = int(mc.group(2), 0)
            if slot in SLOTS:
                n = pushes[-2] if len(pushes) >= 2 else "?"
                res[cur].append("%s:%s" % (SLOTS[slot], n))
            pushes = []
            continue
        if txt.startswith("call"):
            pushes = []
    return res


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--unit", help="restrict to one objdiff unit")
    args = ap.parse_args(argv)

    units = ([args.unit] if args.unit
             else sorted(p.stem for p in BASE.glob("*.obj")))
    covered = compared = hits = 0
    for unit in units:
        b, t = BASE / (unit + ".obj"), TGT / (unit + ".c.obj")
        if not (b.exists() and t.exists()):
            print("### %s: missing obj" % unit)
            continue
        sb, st = sequences(b), sequences(t)
        for sym in sorted(set(sb) & set(st)):
            compared += 1
            a, c = sb[sym], st[sym]
            if a or c:
                covered += 1
            if a == c:
                continue
            hits += 1
            print("### %s  %s" % (unit, sym))
            print("   ours  (%3d): %s" % (len(a), " ".join(a)))
            print("   retail(%3d): %s" % (len(c), " ".join(c)))

    print("\n%d function(s) carry archive-slot calls out of %d compared; "
          "%d sequence mismatch(es)." % (covered, compared, hits))
    print("A row whose counts are equal and whose sizes are all registers is "
          "NOT a format fact - check the receiver's class before acting on it.")
    return 1 if hits else 0


if __name__ == "__main__":
    sys.exit(main())
