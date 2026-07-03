#!/usr/bin/env python3
"""Cleanliness scoreboard - cast / placeholder / view counts that should trend to
0 as the reconstruction's type/call/name layer is cleaned up, shown WITH a delta
vs the committed baseline so a matcher can steer on its own change.

Printed by ``gruntz build`` in the report block (below the match summary); runnable
as ``python -m gruntz.match.cleanliness`` (shows counts + delta),
``--update`` (bless: write the baseline), ``--csv``. See docs/cleanliness-metrics.md.

Counts OCCURRENCES over ``src/`` + ``include/`` C++ sources with comments AND
string/char literals stripped first, so the extensive ``//`` RVA/analysis
annotations and string data do NOT inflate the counts - only real code tokens.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
ROOTS = ("src", "include")
EXTS = {".cpp", ".cc", ".cxx", ".h", ".hpp", ".inl"}
BASELINE = REPO / "config" / "cleanliness-baseline.tsv"

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


# (label, regex, cpp_only). Occurrences summed over stripped code. Tight patterns so
# a member cast `(T*)m_x` (clang-format writes it tight) doesn't collide with
# `if (c) m_x`. cpp_only=True counts ONLY in .cpp TUs (not headers): a `struct/class
# X {...}` DEFINITION inside a .cpp is a per-TU "fake view" of a real engine class -
# the mandate is to reduce every one to the real struct/class in a header, so this
# metric drives to ~0 (headers are where types belong; `.cpp` files define methods).
_CPP = {".cpp", ".cc", ".cxx"}
METRICS = (
    ("m_<hex> fields", re.compile(r"\bm_[0-9a-f]{2,}\b"), False),
    ("Unknown ids", re.compile(r"\b\w*[Uu]nknown\w*\b"), False),
    ("g_<hex> globals", re.compile(r"\bg_[0-9a-f]{4,}\b"), False),
    ("Method/Stub/FUN", re.compile(r"\b(?:Method[0-9a-f]{3,}|Stub_[0-9a-f]+|vfunc_[0-9]+|FUN_[0-9a-f]+)\b"), False),
    (".cpp-local views", re.compile(r"\b(?:struct|class)\s+\w+\s*(?::[^{;]*)?\{"), True),
    (")this casts", re.compile(r"\)this\b"), False),
    (")m_ casts", re.compile(r"\)m_[A-Za-z0-9_]"), False),
    ("(char*) casts", re.compile(r"\(char ?\*\)"), False),
    ("(const char*) casts", re.compile(r"\(const char ?\*\)"), False),
    ("void* m_ members", re.compile(r"\bvoid ?\* m_"), False),
)


def count() -> list[tuple[str, int]]:
    totals = {label: 0 for label, _, _ in METRICS}
    for root in ROOTS:
        base = REPO / root
        if not base.is_dir():
            continue
        for path in base.rglob("*"):
            if path.suffix not in EXTS or not path.is_file():
                continue
            is_cpp = path.suffix in _CPP
            try:
                code = _strip(path.read_text(errors="ignore"))
            except OSError:
                continue
            for label, rx, cpp_only in METRICS:
                if cpp_only and not is_cpp:
                    continue
                totals[label] += len(rx.findall(code))
    return [(label, totals[label]) for label, _, _ in METRICS]


def load_baseline() -> dict[str, int]:
    if not BASELINE.is_file():
        return {}
    out: dict[str, int] = {}
    for line in BASELINE.read_text().splitlines():
        if "\t" in line:
            lbl, n = line.rsplit("\t", 1)
            try:
                out[lbl] = int(n)
            except ValueError:
                pass
    return out


def save_baseline(rows: list[tuple[str, int]]) -> None:
    BASELINE.write_text("".join(f"{lbl}\t{n}\n" for lbl, n in rows))


def _cell(label: str, n: int, base: dict[str, int], width: int) -> str:
    """`label   NNN (+d)` - delta vs baseline (down = good). Blank delta if 0/new."""
    d = n - base[label] if label in base else 0
    tag = f" ({d:+d})" if d else ""
    return f"{label:<{width}}{n:>7}{tag:>7}"


def report_lines(rows: list[tuple[str, int]] | None = None) -> list[str]:
    """Formatted scoreboard lines (two columns) for the build report."""
    rows = rows if rows is not None else count()
    base = load_baseline()
    naming, casts = rows[:5], rows[5:]
    width = max(len(lbl) for lbl, _ in rows) + 1
    lines = ["cleanliness (-> 0 where affordable; delta vs baseline, down = good):"]
    for i in range(max(len(naming), len(casts))):
        left = _cell(*naming[i], base, width) if i < len(naming) else ""
        right = _cell(*casts[i], base, width) if i < len(casts) else ""
        lines.append(f"  {left:<{width + 14}}  {right}")
    return lines


def main() -> int:
    rows = count()
    if "--update" in sys.argv:
        save_baseline(rows)
        print(f"cleanliness baseline updated: {sum(n for _, n in rows)} total across {len(rows)} metrics")
        return 0
    if "--csv" in sys.argv:
        base = load_baseline()
        for label, n in rows:
            print(f"{label},{n},{n - base.get(label, n)}")
        return 0
    for line in report_lines(rows):
        print(line)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
