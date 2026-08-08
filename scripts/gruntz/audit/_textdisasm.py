#!/usr/bin/env python3
"""_textdisasm.py - one linear objdump pass over retail `.text`, shared by the
layout audits.

Byte-pattern scanning is fine for a stereotyped pair (`push imm` + `call`) but
wrong the moment anything sits between them, and there is no length decoder in
the package. objdump decodes the whole section in under a second, so the audits
read a real instruction stream instead.

CAVEAT, stated once here: this is a LINEAR sweep from the section start, so an
inline jump table or a data island desynchronises it until it happens to realign.
Measured against the recovered function inventory, 10681 of 10701 function starts
(99.8%) ARE instruction boundaries in the stream - so callers must check
`start in insn` and skip the function otherwise, never assume.
"""
from __future__ import annotations

import re
import subprocess
import tempfile
from pathlib import Path

from gruntz.core import get_context

_LINE = re.compile(r"^\s*([0-9a-f]+):\t([0-9a-f ]+)\t(\S+)\s*(.*)$")
_CACHE = None


def text_insns():
    """{rva: (mnemonic, operands)} for the whole .text, decoded once per process."""
    global _CACHE
    if _CACHE is not None:
        return _CACHE
    ctx = get_context()
    _n, tva, _vsz, rp, rsz = ctx.pe.text
    with tempfile.NamedTemporaryFile(suffix=".bin", delete=False) as f:
        f.write(ctx.pe.data[rp:rp + rsz])
        path = f.name
    try:
        out = subprocess.run(
            ["objdump", "-D", "-b", "binary", "-m", "i386", "-Mintel",
             f"--adjust-vma=0x{tva:x}", path],
            capture_output=True, text=True).stdout
    finally:
        Path(path).unlink(missing_ok=True)
    insn = {}
    for line in out.splitlines():
        m = _LINE.match(line)
        if m:
            insn[int(m.group(1), 16)] = (m.group(3), m.group(4))
    _CACHE = insn
    return insn


def preceding(insn, rva, n):
    """The <=n decoded instruction addresses immediately before `rva`, nearest
    first. Walks backwards one byte at a time and keeps only addresses whose
    decode LANDS on `rva` - i.e. the real predecessor chain, not stray bytes."""
    out, cur = [], rva
    for _ in range(n * 16):
        cur -= 1
        if cur in insn:
            out.append(cur)
            if len(out) >= n:
                break
    return out
