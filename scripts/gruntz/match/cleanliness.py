#!/usr/bin/env python3
"""Cleanliness scoreboard - cast / placeholder / view counts that should trend to
0 as the reconstruction's type/call/name layer is cleaned up.

Printed by ``gruntz build`` below the match summary; runnable as
``python -m gruntz.match.cleanliness [--csv]``. See docs/cleanliness-metrics.md.

Counts OCCURRENCES over ``src/`` + ``include/`` C++ sources with comments AND
string/char literals stripped first, so the extensive ``//`` RVA/analysis
annotations (which mention "unknown", casts, ``m_<hex>`` offsets in prose) and any
string data do NOT inflate the counts - only real code tokens are counted.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
ROOTS = ("src", "include")
EXTS = {".cpp", ".cc", ".cxx", ".h", ".hpp", ".inl"}

# Strip order matters: block comments, then line comments, then string/char
# literals (so a "//" or "(char*)" inside a string is already gone). Replace with
# a space to avoid gluing tokens across the removed span.
_BLOCK = re.compile(r"/\*.*?\*/", re.DOTALL)
_LINE = re.compile(r"//[^\n]*")
_STR = re.compile(r'"(?:\\.|[^"\\\n])*"')
_CHR = re.compile(r"'(?:\\.|[^'\\\n])*'")


def _strip(text: str) -> str:
    text = _BLOCK.sub(" ", text)
    text = _LINE.sub("", text)
    text = _STR.sub(" ", text)
    text = _CHR.sub(" ", text)
    return text


# (label, regex). Occurrences summed over the stripped code. Patterns are kept
# tight to avoid false positives (a member cast `(T*)m_x` is written tight by
# clang-format, so `)m_` does not collide with a spaced `if (c) m_x`).
METRICS = (
    ("m_<hex> fields", re.compile(r"\bm_[0-9a-f]{2,}\b")),
    ("Unknown ids", re.compile(r"\b\w*[Uu]nknown\w*\b")),
    ("g_<hex> globals", re.compile(r"\bg_[0-9a-f]{4,}\b")),
    ("Method/Stub/FUN", re.compile(r"\b(?:Method[0-9a-f]{3,}|Stub_[0-9a-f]+|vfunc_[0-9]+|FUN_[0-9a-f]+)\b")),
    (")this casts", re.compile(r"\)this\b")),
    (")m_ casts", re.compile(r"\)m_[A-Za-z0-9_]")),
    ("(char*) casts", re.compile(r"\(char ?\*\)")),
    ("(const char*) casts", re.compile(r"\(const char ?\*\)")),
    ("void* m_ members", re.compile(r"\bvoid ?\* m_")),
)


def count() -> list[tuple[str, int]]:
    totals = {label: 0 for label, _ in METRICS}
    for root in ROOTS:
        base = REPO / root
        if not base.is_dir():
            continue
        for path in base.rglob("*"):
            if path.suffix not in EXTS or not path.is_file():
                continue
            try:
                code = _strip(path.read_text(errors="ignore"))
            except OSError:
                continue
            for label, rx in METRICS:
                totals[label] += len(rx.findall(code))
    return [(label, totals[label]) for label, _ in METRICS]


def main() -> int:
    rows = count()
    if "--csv" in sys.argv:
        for label, n in rows:
            print(f"{label},{n}")
        return 0
    # Two grouped columns so the block stays compact under the match summary.
    naming = rows[:4]
    casts = rows[4:]
    print("cleanliness (-> 0 where affordable; see docs/cleanliness-metrics.md):")
    width = max(len(lbl) for lbl, _ in rows) + 1
    for i in range(max(len(naming), len(casts))):
        left = f"{naming[i][0]:<{width}}{naming[i][1]:>7}" if i < len(naming) else ""
        right = f"{casts[i][0]:<{width}}{casts[i][1]:>7}" if i < len(casts) else ""
        print(f"  {left:<{width + 9}}  {right}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
