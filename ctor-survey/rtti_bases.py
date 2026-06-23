#!/usr/bin/env python3
"""Dump the RTTI base-class chain for given vftable VAs (definitive inheritance).
  vftable[-4] -> Complete Object Locator
  COL+0x10    -> Class Hierarchy Descriptor (CHD)
  CHD+0x08    -> numBaseClasses ; CHD+0x0C -> pBaseClassArray
  BaseClassArray[i] -> BaseClassDescriptor ; BCD+0x00 -> TypeDescriptor ; TD+0x08 -> name
"""
import sys, struct
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
data = (REPO / "build/exe/GRUNTZ.EXE").read_bytes()
pe = struct.unpack_from("<I", data, 0x3c)[0]
nsec = struct.unpack_from("<H", data, pe + 6)[0]
opt = struct.unpack_from("<H", data, pe + 20)[0]
secs = []
for i in range(nsec):
    o = pe + 24 + opt + i * 40
    vsize, vaddr, rawsz, rawp = struct.unpack_from("<IIII", data, o + 8)
    secs.append((0x400000 + vaddr, rawp, rawsz))

def rd32(va):
    for base, rawp, rawsz in secs:
        if base <= va < base + rawsz:
            return struct.unpack_from("<I", data, rawp + (va - base))[0]
    return None

def rdstr(va):
    for base, rawp, rawsz in secs:
        if base <= va < base + rawsz:
            o = rawp + (va - base); e = data.index(b"\0", o); return data[o:e].decode("latin1")
    return "?"

def demangle(mn):
    s = mn[4:] if mn[:4] in (".?AV", ".?AU") else mn
    return s[:-2] if s.endswith("@@") and "?$" not in s else mn

def bases(vft):
    col = rd32(vft - 4)
    chd = rd32(col + 0x10)
    n = rd32(chd + 8)
    arr = rd32(chd + 0xC)
    out = []
    for i in range(n):
        bcd = rd32(arr + 4 * i)
        td = rd32(bcd)
        out.append(demangle(rdstr(td + 8)))
    return out

for tok in sys.argv[1:]:
    vft = int(tok, 16)
    chain = bases(vft)
    print(f"0x{vft:x}: {' : '.join(chain)}")
