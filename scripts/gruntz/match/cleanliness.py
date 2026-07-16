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


# The ".cpp-local views" metric enforces matcher.md rule 0: a struct/class DEFINITION inside
# a .cpp is a per-TU view of a class whose one true shape belongs in a header - regardless of
# its NAME (unlike "placeholder classes", which is name-based). Counts DEFINITIONS (name then
# a body brace, optional base-clause), NOT forward-decls / elaborated uses / anonymous aggs.
# cpp_only + scaffolding-excluded => only real main-tree .cpp files count; homing a view into
# one trips it. Drive to ~0 by moving the type to include/<Module>/.
_TYPEDEF_DEF = re.compile(r"\b(?:struct|class)\s+(\w+)\b(?:\s+final)?\s*(?::[^;{]*)?\{")


def _count_cpp_local_defs(code: str) -> int:
    return len(_TYPEDEF_DEF.findall(code))


# The "placeholder vtable slots" metric (GAMEABLE, but tracked per request): a
# virtual whose NAME is a placeholder for an unresolved vtable slot (dummyN / vNN /
# vfunc / SlotN) - a real virtual with an un-recovered identity. Counts the DECL
# sites (one per slot). Drive to 0 via the vtable_hierarchy TOPOLOGICAL override
# analysis (inherit the base's slots, name the rest from the slot RVA), NOT by
# hand-renaming - see `python -m gruntz.analysis.vtable_hierarchy --audit/--coverage`.
_VTSLOT = re.compile(
    r"virtual\b[^;{}\n]*\b(?:dummy[0-9]+|vfunc[0-9]*|[Ss]lot[0-9]+"
    r"|[sv][0-9a-f]{2,3}(?![0-9a-z])"  # v04 / s04 offset-named fake slots (both are placeholders)
    r"|[SsVv]f[0-9a-f]+)\s*\("          # Sf1 / vf10 slot-forwarders
)


# ")m_ casts" (a C-style cast applied to a member, `(CFoo*)m_54`) is a VECTOR of fake-view
# propagation - a caller casts an untyped/placeholder member to a fake view. RATCHETED down.
# EXCLUDES the legit string casts (char*)/(const char*)m_x (a member byte-buffer): those are the
# allowed exception and already counted under "(char*) casts". ")this casts" (casting `this` to a
# fake view) is ALWAYS the vector - ratcheted with no exception. Killing these dissolves the views.
_M_CAST = re.compile(r"\)m_[A-Za-z0-9_]")
_STR_M_CAST = re.compile(r"\((?:const |unsigned |signed )*char ?\*\)m_[A-Za-z0-9_]")


def _count_nonstring_m_casts(code: str) -> int:
    return len(_M_CAST.findall(code)) - len(_STR_M_CAST.findall(code))


# Offset-access cast-hiding macros: `#define F(p,o) (*(i32*)((char*)(p)+(o)))` (also P/PTR/I32/DBL/
# M/W/WTS_*/PB_*/GREG_*/FEC_*/I32AT). Each expands to the raw-offset cast the ratchet counts but
# HIDES every call-site (the define counts once as a `(char*)` cast; the N uses vanish at
# preprocessing). Count each such macro's def + every call-site -> the true hidden-cast footprint.
# Cast-ratchet EVASION; dissolve into real typed member access `p->m_field`.
_OFFSET_MACRO_DEF = re.compile(
    r"#define\s+(\w+)\s*\([^)]*\)\s*\(\s*\*\s*\(\s*\w[\w ]*\*\s*\)\s*\(\s*\(\s*char\s*\*\s*\)"
)


def _count_offset_macro_casts(code: str) -> int:
    total = 0
    for m in _OFFSET_MACRO_DEF.finditer(code):
        total += len(re.findall(r"\b" + re.escape(m.group(1)) + r"\s*\(", code))
    return total


# Per-TU `extern` decls: a global/function re-declared in a consumer .cpp instead of living in its
# OWNER's header (which consumers #include). extern "C" DATA-array globals that need it for mangling
# are the residual; the rest belong in owner headers. .cpp-only (a header `extern` IS the owner decl).
_CPP_EXTERN = re.compile(r"^\s*extern\b", re.MULTILINE)


# (label, matcher, cpp_only). matcher = compiled regex (findall count) OR a callable
# code->int for structural counts. Occurrences summed over stripped code.
METRICS = (
    ("m_<hex> fields", re.compile(r"\bm_[0-9a-f]{2,}\b"), False),
    ("Unknown ids", re.compile(r"\b\w*[Uu]nknown\w*\b"), False),
    ("g_<hex> globals", re.compile(r"\bg_[0-9a-f]{4,}\b"), False),
    ("Method/Stub/FUN", re.compile(r"\b(?:Method[0-9a-f]{3,}|Stub_[0-9a-f]+|vfunc_[0-9]+|FUN_[0-9a-f]+)\b"), False),
    ("placeholder classes", _count_placeholders, False),
    (".cpp-local views", _count_cpp_local_defs, True),
    # --- manual-vtable residue (the de-hack / vtable-review targets) ---
    ("placeholder vtable slots", _VTSLOT, False),
    ("*Vtbl structs", re.compile(r"\b(?:struct|class)\s+\w*Vtbl\w*"), False),
    ("->vtbl accesses", re.compile(r"->\s*\w*[Vv]tbl\w*"), False),
    ("g_*Vtbl globals", re.compile(r"\bg_\w*[Vv]tbl\w*"), False),
    ("m_vtbl/m_vptr members", re.compile(r"\bm_v(?:tbl|ptr)\w*"), False),
    # --- casts ---
    (")this casts", re.compile(r"\)this\b"), False),
    (")m_ casts", _count_nonstring_m_casts, False),  # string-cast-excluded; ratcheted
    ("(char*) casts", re.compile(r"\(char ?\*\)"), False),
    ("(const char*) casts", re.compile(r"\(const char ?\*\)"), False),
    ("void* m_ members", re.compile(r"\bvoid ?\* m_"), False),
    # --- metric-evasion / placeholder hacks (2026-07-14 de-hack campaign; MAX-fuzzy gate) ---
    ("offset-cast macros", _count_offset_macro_casts, False),
    ("cpp extern decls", _CPP_EXTERN, True),
)


# The RATCHET set: metrics that only go DOWN (main-tree). The stub backlog (src/Stub/ TUs) and
# their *Views.h view-scaffolding are reconstruction machinery, NOT main-tree code - their
# fake-view/placeholder shells (and their casts) are EXPECTED, so they don't count here. A view
# (or a `)this`/`)m_` cast that props a view) only counts once it lands in a real main-tree TU,
# where it must resolve to a proper class. Includes the two cast VECTORS of fake-view propagation:
# `)this casts` (always) and `)m_ casts` (string-cast-excluded). Other metrics (m_<hex>, string
# casts, ...) still count everywhere and are tracked, not ratcheted.
_VIEW_METRICS = {"placeholder classes", ".cpp-local views", "placeholder vtable slots",
                 "*Vtbl structs", "->vtbl accesses", "g_*Vtbl globals", "m_vtbl/m_vptr members",
                 ")this casts", ")m_ casts"}


def _is_scaffolding(path) -> bool:
    return "/Stub/" in path.as_posix() or path.name.endswith("Views.h")


# The caller_callee reconciliation metric: retail call-graph edges (reconstructed->reconstructed)
# our source FAILS to reproduce - it reaches the real callee through a fake-view / wrong-signature
# name (FAKE-VIEW), or omits the edge (MISSING). NOT a text scan (reads clang IR + the retail
# graph), so it needs a build; shelled here so the SAME baseline tracks + ratchets it to 0 like
# the source metrics. Gracefully absent (rows omitted) when no build IR is available.
_CALLER_CALLEE_LABELS = ("caller-callee unreconciled", "caller-callee FAKE-VIEW")


def _caller_callee_counts() -> dict[str, int]:
    import subprocess
    try:
        out = subprocess.run(
            [sys.executable, "-m", "gruntz.analysis.caller_callee", "--metric"],
            capture_output=True, text=True, timeout=600, cwd=str(REPO),
        ).stdout
    except Exception:
        return {}
    res: dict[str, int] = {}
    for line in out.splitlines():
        m = re.search(r"UNRECONCILED.*:\s*(\d+)", line)  # "... drive to 0): 2366" -> 2366
        if m:
            res["caller-callee unreconciled"] = int(m.group(1))
        m = re.search(r"(\d+)\s+FAKE-VIEW", line)
        if m:
            res["caller-callee FAKE-VIEW"] = int(m.group(1))
    return res


# Ratchet set: metrics that only go DOWN. The caller_callee edges join the view/cast metrics.
_RATCHET = _VIEW_METRICS | set(_CALLER_CALLEE_LABELS)


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
            scaffold = _is_scaffolding(path)   # stub/Views: excluded from the VIEW metrics
            try:
                code = _strip(path.read_text(errors="ignore"))
            except OSError:
                continue
            for label, matcher, cpp_only in METRICS:
                if cpp_only and not is_cpp:
                    continue
                if scaffold and label in _VIEW_METRICS:
                    continue
                totals[label] += matcher(code) if callable(matcher) else len(matcher.findall(code))
    rows = [(label, totals[label]) for label, _, _ in METRICS]
    cc = _caller_callee_counts()  # build-derived; omitted if no IR
    for lbl in _CALLER_CALLEE_LABELS:
        if lbl in cc:
            rows.append((lbl, cc[lbl]))
    return rows


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


def merge_baseline_downonly(rows: list[tuple[str, int]]) -> list[tuple[str, int]]:
    """Rows to persist during a build's baseline roll: the RATCHETED metrics
    (_VIEW_METRICS - views + the fake-view-propagating casts) never RISE. Keep
    min(new count, committed floor) so a regression stays visible as debt on every
    later build instead of being silently blessed into the floor; other (tracked,
    non-ratcheted) metrics roll forward as before. Manual ``--update`` still writes
    the true counts (the one deliberate way to bless a floor - only ever lower)."""
    base = load_baseline()
    out = []
    for label, n in rows:
        if label in _RATCHET and label in base:
            out.append((label, min(n, base[label])))
        else:
            out.append((label, n))
    return out


def _cell(label: str, n: int, base: dict[str, int], width: int) -> str:
    """`label   NNN (+d)` - delta vs baseline (down = good). Blank delta if 0/new."""
    d = n - base[label] if label in base else 0
    tag = f" ({d:+d})" if d else ""
    return f"{label:<{width}}{n:>7}{tag:>7}"


def report_lines(rows: list[tuple[str, int]] | None = None) -> list[str]:
    """Formatted scoreboard lines (two columns) for the build report."""
    rows = rows if rows is not None else count()
    base = load_baseline()
    # three groups: naming/views (6) | manual-vtable residue (5) | casts (5)
    naming, vtable, casts = rows[:6], rows[6:11], rows[11:]
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
