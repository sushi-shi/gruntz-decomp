#!/usr/bin/env python3
"""Shared helpers for the FID masked-byte matcher stages (classify, unanchored).

Factored out of the original build/fid scripts so the PE `.text` parse + the
trailing-padding trim live in one place.
"""
import struct


def pe_text(path):
    """Return (.text bytes, .text RVA, image base) for a PE on disk."""
    d = open(path, 'rb').read()
    e = struct.unpack_from('<I', d, 0x3c)[0]
    coff = e + 4
    nsec = struct.unpack_from('<H', d, coff + 2)[0]
    opthdr = struct.unpack_from('<H', d, coff + 16)[0]
    base = struct.unpack_from('<I', d, coff + 20 + 28)[0]
    st = coff + 20 + opthdr
    for i in range(nsec):
        o = st + i * 40
        name = d[o:o + 8].rstrip(b'\0')
        vsize, vaddr, rawsize, rawptr = struct.unpack_from('<IIII', d, o + 8)
        if name == b'.text':
            return d[rawptr:rawptr + rawsize], vaddr, base
    raise RuntimeError("no .text section in %s" % path)


def trim_pad(body, mask):
    """Trailing-padding-trimmed length: drop trailing fixed 0x90/0xCC bytes
    (alignment NOP / INT3 are not part of the function body)."""
    L = len(body)
    while L > 0 and mask[L - 1] == 0xff and body[L - 1] in (0x90, 0xCC):
        L -= 1
    return L
