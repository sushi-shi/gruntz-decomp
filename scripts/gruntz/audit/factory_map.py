#!/usr/bin/env python3
"""Decode retail's SerialObjectFactory LogicTypeId -> class map and diff it against src/.

WHY
  Every object in a save game is restored by id: `CDDrawChildGroup::LoadObjects`
  hands `desc.m_logicTypeId` to `SerialObjectFactory`'s SERIAL_CREATE arm, which
  `new`s the class.  One wrong arm builds the WRONG class from a retail file -
  wrong vtable, wrong size, wrong field offsets - and the failure surfaces
  hundreds of frames later.  Nothing in objdiff can see it: a `new CFoo` and a
  `new CBar` of the same size are the same instruction stream apart from two
  reloc-masked addresses.

METHOD (no Ghidra, no decompiler)
  Retail dispatches the arm with

      lea eax,[ecx-0x3e8] ; cmp eax,0x44 ; ja <default> ; jmp [eax*4+0x40eb10]

  so the map is 0x45 dwords at RVA 0xeb10.  For each arm take the LAST
  `mov [esi],<imm32>` before its `mov eax,1` epilogue - that is the most-derived
  class's vptr stamp, and `config/retail/vtables_game.csv` turns the address
  into the class NAME.  Two arms `call` an out-of-line ctor instead of inlining
  it and therefore stamp nothing; they are reported as CALL rows for manual
  adjudication.  Table entries that point at the default arm are ids retail
  DECLINES, and src/ is expected to omit them.

  Measured 2026-08-10: 69/69 arms agree with src/Gruntz/SerialObjectFactory.cpp.

USAGE
  python -m gruntz.audit.factory_map
"""

import argparse
import os
import re
import struct
import sys
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])

IMAGE_BASE = 0x400000
TABLE_RVA = 0xEB10
NARMS = 0x45
FIRST_ID = 0x3E8
VPTR_LO, VPTR_HI = 0x5C0000, 0x600000


def sections(data):
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    nsec = struct.unpack_from("<H", data, pe + 6)[0]
    optsz = struct.unpack_from("<H", data, pe + 20)[0]
    out = []
    for i in range(nsec):
        o = pe + 24 + optsz + 40 * i
        vsz, va, rsz, ra = struct.unpack_from("<IIII", data, o + 8)
        out.append((va, max(vsz, rsz), ra))
    return out


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--exe", default=os.environ.get("GRUNTZ_EXE"),
                    help="retail GRUNTZ.EXE (default $GRUNTZ_EXE)")
    args = ap.parse_args(argv)
    if not args.exe:
        print("set $GRUNTZ_EXE or pass --exe")
        return 2

    data = Path(args.exe).read_bytes()
    secs = sections(data)

    def rva2off(rva):
        for va, sz, ra in secs:
            if va <= rva < va + sz:
                return ra + (rva - va)
        raise KeyError(hex(rva))

    vt = {}
    for line in (REPO / "config/retail/vtables_game.csv").read_text().splitlines():
        if line.startswith("#") or line.startswith("name,"):
            continue
        f = line.split(",")
        if len(f) >= 2:
            try:
                vt[int(f[1], 16)] = f[0]
            except ValueError:
                pass

    arms = struct.unpack_from("<%dI" % NARMS, data, rva2off(TABLE_RVA))
    default_rva = None
    counts = {}
    for a in arms:
        counts[a] = counts.get(a, 0) + 1
    # the default arm is the only target more than one id shares
    for a, n in counts.items():
        if n > 1:
            default_rva = a

    retail = {}
    for i, armva in enumerate(arms):
        tid = FIRST_ID + i
        if armva == default_rva:
            retail[tid] = ("DECLINE", None)
            continue
        blob = data[rva2off(armva - IMAGE_BASE):][:0x2400]
        end = blob.find(b"\xb8\x01\x00\x00\x00")
        scan = blob[:end] if end > 0 else blob
        stamps = [struct.unpack_from("<I", scan, m.start() + 2)[0]
                  for m in re.finditer(rb"\xc7\x06", scan)]
        stamps = [s for s in stamps if VPTR_LO <= s < VPTR_HI]
        if stamps:
            v = stamps[-1] - IMAGE_BASE
            retail[tid] = ("VPTR", vt.get(v, "??_7?@0x%x" % v))
        else:
            retail[tid] = ("CALL", "out-of-line ctor (arm RVA 0x%x)" % (armva - IMAGE_BASE))

    enum = {}
    for line in (REPO / "include/Gruntz/LogicTypeId.h").read_text().splitlines():
        m = re.match(r"\s*(LOGIC_\w+)\s*=\s*(-?0x[0-9a-fA-F]+|-?\d+)", line)
        if m:
            enum[m.group(1)] = int(m.group(2), 0)
    ours = {}
    src = (REPO / "src/Gruntz/SerialObjectFactory.cpp").read_text()
    for m in re.finditer(r"case (LOGIC_\w+):\s*\*result = new (\w+)", src):
        ours[enum[m.group(1)]] = m.group(2)

    print("%-6s %-12s %-34s %-30s %s" % ("id", "kind", "retail", "src/", "verdict"))
    bad = manual = 0
    for tid in sorted(retail):
        kind, r = retail[tid]
        o = ours.get(tid)
        if kind == "DECLINE":
            ok = o is None
        elif kind == "VPTR":
            ok = (o is not None and r == "??_7%s@@6B@" % o)
        else:
            ok = o is not None
            manual += 1
        if not ok:
            bad += 1
        print("0x%03x %-12s %-34s %-30s %s"
              % (tid, kind, r or "-", o or "(absent)", "" if ok else "  <-- MISMATCH"))
    print("\n%d mismatching arm(s) out of %d (%d CALL row(s) need manual "
          "adjudication - chase the ILT thunk)." % (bad, len(retail), manual))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
