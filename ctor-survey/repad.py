#!/usr/bin/env python3
"""Recompute each include/Stub/*.h leaf pad = SIZE - sizeof(base) against the
corrected spine (CUserLogic=0x40). Iterates to a fixpoint so base sizes (which may
themselves be other leaf headers) settle. Thin leaves drop their pad; field-adders
get pad = own size - base size."""
import re
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
INC = REPO / "include/Stub"
FIXED = {"CUserBase": 4, "CUserLogic": 0x40, "CMovingLogic": 0x40}

def size_of(cls):
    if cls in FIXED:
        return FIXED[cls]
    h = INC / f"{cls}.h"
    if h.exists():
        m = re.search(r"SIZE\(\s*" + re.escape(cls) + r"\s*,\s*0x([0-9a-fA-F]+)", h.read_text())
        if m:
            return int(m.group(1), 16)
    return None

PAD = re.compile(r"[ \t]*char m_size_pad\[0x[0-9a-fA-F]+\];[^\n]*\n")
changed = 0
for _ in range(6):  # fixpoint over base dependencies
    for h in sorted(INC.glob("*.h")):
        t = h.read_text()
        cm = re.search(r"\bclass\s+(\w+)\s*:\s*public\s+(\w+)", t)
        sm = re.search(r"SIZE\(\s*(\w+)\s*,\s*0x([0-9a-fA-F]+)\)", t)
        if not cm or not sm:
            continue
        cls, base, N = cm.group(1), cm.group(2), int(sm.group(2), 16)
        bs = size_of(base)
        if bs is None:
            continue
        pad = N - bs
        t2 = PAD.sub("", t)                       # strip any existing pad
        if pad > 0:
            ins = f"    char m_size_pad[0x{pad:x}]; // own region over {base} (0x{bs:x})\n"
            t2 = re.sub(r"\n\};", "\n" + ins + "};", t2, count=1)
        if t2 != t:
            h.write_text(t2); changed += 1
print(f"repad: rewrote {changed} header(s)")
