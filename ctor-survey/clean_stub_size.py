#!/usr/bin/env python3
"""Remove stale SIZE() lines (and their // size-comment) from src/Stub/*.cpp left
by the pre-inheritance passes. Canonical SIZE now lives in the include/Stub/*.h
headers (for inheritance-modeled classes); un-modeled stubs carry none."""
import re
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
n = 0
for p in (REPO / "src/Stub").glob("*.cpp"):
    t = p.read_text()
    t2 = re.sub(r"^// size 0x[0-9a-f]+ [^\n]*\(gruntz\.analysis\.news\)\n", "", t, flags=re.M)
    t2 = re.sub(r"^SIZE\([^\n]*\);\n", "", t2, flags=re.M)
    if t2 != t:
        p.write_text(t2); n += 1
print(f"cleaned SIZE cruft from {n} stub .cpp files")
