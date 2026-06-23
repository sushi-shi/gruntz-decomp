#!/usr/bin/env python3
"""Revert every pad this survey added (m_pad / m_size_pad lines tagged
gruntz.analysis.news). Keeps SIZE() asserts. Clean slate for inheritance-based
regeneration."""
import re
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
PAD = re.compile(r"[ \t]*char m_(?:size_)?pad\[0x[0-9a-fA-F]+\];[^\n]*gruntz\.analysis\.news[^\n]*\n")
n = 0
for p in list((REPO / "src").rglob("*.cpp")) + list((REPO / "src").rglob("*.h")) + list((REPO / "include").rglob("*.h")):
    t = p.read_text()
    t2, k = PAD.subn("", t)
    if k:
        p.write_text(t2); n += k; print(f"  -{k}  {p.relative_to(REPO)}")
print(f"reverted {n} pad lines")
