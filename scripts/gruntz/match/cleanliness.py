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


# The "placeholder classes" metric: a type whose NAME carries an RVA-like hex run
# (4+ hex incl. a digit: L_fe90, S_104a0, CVtbl_1efc58, Method117e20, Sub150c30, ...)
# is an un-attributed placeholder shell, NOT a real engine class - a fake view. This
# is LOCATION-INDEPENDENT (src AND include): moving such a shell from a .cpp into a
# header does NOT resolve it, so the metric can't be gamed by relocation. Drive to 0
# by recovering each placeholder's REAL class identity (name + shape) or removing it.
_CPP = {".cpp", ".cc", ".cxx"}
_TYPEDEF = re.compile(r"\b(?:struct|class)\s+(\w+)")
_HEXRUN = re.compile(r"[0-9a-f]{4,}")


def _is_placeholder(name: str) -> bool:
    return any(any(c.isdigit() for c in run) for run in _HEXRUN.findall(name))


def _count_placeholders(code: str) -> int:
    return sum(1 for n in _TYPEDEF.findall(code) if _is_placeholder(n))


# The "placeholder vtable slots" metric (GAMEABLE, but tracked per request): a
# virtual whose NAME is a placeholder for an unresolved vtable slot (dummyN / vNN /
# vfunc / SlotN) - a real virtual with an un-recovered identity. Counts the DECL
# sites (one per slot). Drive to 0 via the vtable_hierarchy TOPOLOGICAL override
# analysis (inherit the base's slots, name the rest from the slot RVA), NOT by
# hand-renaming - see `python -m gruntz.analysis.vtable_hierarchy --audit/--coverage`.
_VTSLOT = re.compile(r"virtual\b[^;{}\n]*\b(?:dummy[0-9]+|v[0-9a-f]{2,}|vfunc[0-9]*|[Ss]lot[0-9]+)\s*\(")


# (label, matcher, cpp_only). matcher = compiled regex (findall count) OR a callable
# code->int for structural counts. Occurrences summed over stripped code.
METRICS = (
    ("m_<hex> fields", re.compile(r"\bm_[0-9a-f]{2,}\b"), False),
    ("Unknown ids", re.compile(r"\b\w*[Uu]nknown\w*\b"), False),
    ("g_<hex> globals", re.compile(r"\bg_[0-9a-f]{4,}\b"), False),
    ("Method/Stub/FUN", re.compile(r"\b(?:Method[0-9a-f]{3,}|Stub_[0-9a-f]+|vfunc_[0-9]+|FUN_[0-9a-f]+)\b"), False),
    ("placeholder classes", _count_placeholders, False),
    # --- manual-vtable residue (the de-hack / vtable-review targets) ---
    ("placeholder vtable slots", _VTSLOT, False),
    ("*Vtbl structs", re.compile(r"\b(?:struct|class)\s+\w*Vtbl\w*"), False),
    ("->vtbl accesses", re.compile(r"->\s*\w*[Vv]tbl\w*"), False),
    ("g_*Vtbl globals", re.compile(r"\bg_\w*[Vv]tbl\w*"), False),
    ("m_vtbl/m_vptr members", re.compile(r"\bm_v(?:tbl|ptr)\w*"), False),
    # --- casts ---
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
            for label, matcher, cpp_only in METRICS:
                if cpp_only and not is_cpp:
                    continue
                totals[label] += matcher(code) if callable(matcher) else len(matcher.findall(code))
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
    # three groups: naming (5) | manual-vtable residue (5) | casts (5)
    naming, vtable, casts = rows[:5], rows[5:10], rows[10:]
    width = max(len(lbl) for lbl, _ in rows) + 1
    lines = ["cleanliness (-> 0 where affordable; delta vs baseline, down = good):"]
    for i in range(max(len(naming), len(vtable), len(casts))):
        a = _cell(*naming[i], base, width) if i < len(naming) else ""
        b = _cell(*vtable[i], base, width) if i < len(vtable) else ""
        c = _cell(*casts[i], base, width) if i < len(casts) else ""
        lines.append(f"  {a:<{width + 14}}  {b:<{width + 14}}  {c}")
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
