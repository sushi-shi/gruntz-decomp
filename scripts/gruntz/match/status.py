#!/usr/bin/env python3
"""gruntz.match.status - make matching progress and REGRESSIONS queriable.

The matching loop grinds GRUNTZ.EXE functions into byte-exact src/. objdiff gives
us a fuzzy% per function (build/objdiff/report.json). This tool remembers the
BEST-EVER fuzzy% per function in a small, git-tracked text file
(config/match_baseline.tsv) so we can answer one question cheaply:

    "did anything REGRESS?"  ->  python -m gruntz.match.status check

A regression is a function whose freshly-built fuzzy% is below its recorded best.
We keep the *max* (not just the previous value) so a drop caused by something
UNRELATED to the current edit - a shared header, a flag tweak, a target-side
delink change - stays visible until someone looks at it. We keep the best-ever
score without the database/PDB machinery a bigger project needs: there are no
PDBs for the target and only a few dozen units, so a flat text file diffed
against a fresh build (with git as the history store) is enough.

Two ideas:
  * keep the max %            - so unrelated regressions are at least visible.
  * gate the max by a source  - each function carries a fingerprint. On `update`,
    fingerprint               if a function's fingerprint changed, best<-current
                              (the old peak belonged to different source); if
                              unchanged, best<-max(best,current). So deliberately
                              editing a function clears its stale peak, but its
                              siblings keep guarding their high-water marks.

The fingerprint is PER FUNCTION, not per .cpp: gruntz.match.fingerprints asks
clangd for each function's source extent and hashes that range. That way editing
one function does not reset its siblings (so a collateral regression in a sibling
stays visible). When that cache is missing or stale for a unit, we fall back to
the unit's whole-.cpp hash - coarser, but always available (e.g. fresh worktree).

Baseline file (config/match_baseline.tsv), two TAB-separated sections, sorted,
deterministic - hand-greppable and trivial to parse from py/awk:

    # [units]      unit  module  cpp_hash  source
    adler32        zlib-1.0.4  7a7a0bcb03b5  vendor/zlib-1.0.4/adler32.c
    # [functions]  unit  function  best_pct  src_hash
    adler32        _adler32  100.0000  3f9c1a2b4d5e

Commands (run inside nix develop):
    python -m gruntz.match.status check    # regressions vs baseline (non-fatal)
    python -m gruntz.match.status status [...]  # per-function current %, filterable
    python -m gruntz.match.status summary  # overall + per-module/per-unit rollup
    python -m gruntz.match.status update   # recompute best+fingerprint, write baseline

    nix develop --command python3 -m gruntz.match.status check
"""
from __future__ import annotations

import argparse
import json
import subprocess
import sys
import tomllib
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
MANIFEST = REPO / "config" / "units.toml"
BASELINE = REPO / "config" / "match_baseline.tsv"
DEFAULT_REPORT = REPO / "build" / "objdiff" / "report.json"

# Reconstruction-target universe (the match-% denominator) + the carve-out
# categories that are NOT independent reconstruction targets, so are EXCLUDED from
# the %: linker jump thunks (the leading ILT jmp-table + Ghidra thunk_*), FID-
# identified CRT/MFC library code, and compiler-emitted EH unwind funclets (named
# Unwind@*, which reappear automatically when their parent function matches, just
# as the ILT thunks do when their target body does). objdiff's report.json only
# totals the functions we've pulled into units; for honest progress we weigh
# `exact` against the real
# engine and surface the carve-outs as their own README rows (counted, not hidden).
# functions.csv is the delinker's Ghidra input (present after `gruntz build`); the
# FID list is committed. Absent (fresh worktree, no build) -> None and callers
# fall back to objdiff's started-unit scope.
FUNCS_CSV = REPO / "build" / "ghidra-enrich" / "exports" / "functions.csv"
FID_CSV = REPO / "config" / "library_labels.csv"

README = REPO / "README.md"
RM_START = "<!-- match-score:start -->"
RM_END = "<!-- match-score:end -->"

EPS = 0.01  # ignore sub-0.01% jitter


def _pct(num: float, den: float) -> float:
    return 100.0 * num / den if den else 0.0


def engine_universe():
    """Reconstruction-target universe + the excluded carve-out categories.

    Returns {"real_fn","real_code","categories":[(label,fn,code,note),...]} or None
    when the Ghidra/FID exports are absent (callers then fall back to objdiff's
    started-unit totals). `real_*` is the match-% denominator; each category is
    generated/library code carved out of it (shown in the README for transparency,
    not counted in the %)."""
    if not (FUNCS_CSV.is_file() and FID_CSV.is_file()):
        return None
    import csv

    def rint(s):
        s = str(s).strip()
        return int(s, 16) if s.lower().startswith("0x") else int(s)

    rows = []  # (rva, size, name, is_thunk); in-.text only
    with open(FUNCS_CSV) as f:
        for r in csv.DictReader(f):
            try:
                rva, sz = rint(r["entry_rva"]), int(r["byte_size"])
            except Exception:
                continue
            # honor an older export's in_text column; drop anything off .text.
            if r.get("in_text", "1") != "1":
                continue
            name = r.get("name", "")
            # jump thunks are named thunk_* (the enriched export carries no is_thunk
            # column; honor it when an older export still does).
            is_thunk = name.startswith("thunk_") or r.get("is_thunk", "0") == "1"
            rows.append((rva, sz, name, is_thunk))
    lib: set[int] = set()
    with open(FID_CSV) as f:
        for r in csv.DictReader(f):
            try:
                lib.add(rint(r["rva"]))
            except Exception:
                pass

    # The leading contiguous run of <=5-byte functions is the linker's ILT jump
    # table - 5-byte `jmp rel32` forwarders, each named after the body it targets.
    # They aren't named thunk_*, so a name filter misses them; instead bound the
    # table by RVA: real code begins at the first >5-byte function, so anything
    # below that is a thunk (verified: no real fn lives in the leading block, while
    # the few legit <=5-byte vtable-dispatch methods sit far above it, kept real).
    ilt_end = min((rva for rva, sz, _n, _t in rows if sz > 5), default=0)

    # Disjoint carve-outs (first match wins); everything else is a real target.
    acc = {"thunk": [0, 0], "lib": [0, 0], "eh": [0, 0]}
    real_fn = real_code = 0
    for rva, sz, name, is_thunk in rows:
        if rva < ilt_end or is_thunk:      # ILT jmp-table + Ghidra thunk_*
            k = "thunk"
        elif rva in lib:
            k = "lib"
        elif name.startswith("Unwind@"):   # compiler /GX EH unwind funclet
            k = "eh"
        else:
            real_fn += 1
            real_code += sz
            continue
        acc[k][0] += 1
        acc[k][1] += sz
    categories = [
        ("EH unwind funclets", *acc["eh"],
         "compiler /GX EH; match with their parent function"),
        ("CRT/MFC library", *acc["lib"], "FID-identified, statically linked"),
        ("jump thunks", *acc["thunk"],
         "linker ILT jmp-table (RVA <%#x) + thunk_*" % ilt_end),
    ]
    return {"real_fn": real_fn, "real_code": real_code, "categories": categories}


def _md_table(headers: list[str], aligns: str, rows: list[list[str]]) -> list[str]:
    """GitHub markdown table with pipes padded to column widths so the raw source
    reads cleanly in an editor. `aligns`: one char/col, 'l' left or 'r' right."""
    widths = [len(h) for h in headers]
    for r in rows:
        for i, c in enumerate(r):
            widths[i] = max(widths[i], len(c))

    def cell(text: str, i: int) -> str:
        return text.rjust(widths[i]) if aligns[i] == "r" else text.ljust(widths[i])

    def row(cells: list[str]) -> str:
        return "| " + " | ".join(cell(c, i) for i, c in enumerate(cells)) + " |"

    sep = ["-" * (w - 1) + ":" if a == "r" else ":" + "-" * (w - 1)
           for w, a in zip(widths, aligns)]
    return [row(headers), "| " + " | ".join(sep) + " |", *(row(r) for r in rows)]


def write_readme(block: str) -> None:
    text = README.read_text()
    if RM_START in text and RM_END in text:
        pre = text[: text.index(RM_START)]
        post = text[text.index(RM_END) + len(RM_END):]
        README.write_text(pre + block + post)
    else:  # first install: drop it right before the first "## " heading
        i = text.index("\n## ")
        README.write_text(text[:i] + "\n" + block + "\n" + text[i:])

from gruntz.match.fingerprints import cpp_hash, load_cache as load_fp_cache


# --------------------------------------------------------------------------- #
# manifest: unit -> source path + module                                      #
# --------------------------------------------------------------------------- #
def load_units() -> dict[str, dict]:
    """unit stem -> {source, module} from config/units.toml."""
    with open(MANIFEST, "rb") as f:
        data = tomllib.load(f)
    out: dict[str, dict] = {}
    for u in data.get("unit", []):
        name, source = u.get("unit"), u.get("source", "")
        out[name] = {"source": source, "module": module_of(source)}
    return out


def module_of(source: str) -> str:
    """Group units for the summary rollup by the meaningful path component."""
    parts = Path(source).parts
    if not parts:
        return "?"
    if parts[0] in ("src", "vendor") and len(parts) > 1:
        return parts[1]
    return parts[0]


FALLBACK = "cpp:"  # marks a fingerprint we could NOT resolve per-function


def is_fallback(h: str) -> bool:
    """A fingerprint with no per-function resolution (whole-.cpp hash stand-in)."""
    return h.startswith(FALLBACK)


def real_edit(prev_fp: str, cur_fp: str) -> bool:
    """True only when BOTH sides are real (non-fallback) fingerprints that differ -
    a genuine source edit, not a cache hash-domain change. A fallback on either
    side means "unknown", which must NOT count as an edit (else a stale cache hides
    regressions as TOUCHED and corrupts the baseline on update)."""
    return not is_fallback(prev_fp) and not is_fallback(cur_fp) and prev_fp != cur_fp


def fingerprinter():
    """Return fp(unit, mangled) -> the CURRENT per-function source fingerprint.

    Returns the func_fingerprints cache's per-function range hash when the cache is
    fresh for the unit AND has that function. Otherwise returns a FALLBACK-tagged
    whole-.cpp hash (`cpp:<sha>`) - meaning "no per-function fingerprint available"
    (cache stale/absent, or the function isn't fingerprinted). Callers must NOT
    treat a fallback as a real source-change signal: a `cpp:` hash differing from a
    stored range hash is a *hash-domain* change, not an edit, so check/update only
    detect a true edit when BOTH sides are real (non-fallback) and differ.
    """
    manifest = load_units()
    cache_units, cache_funcs = load_fp_cache()
    cur_cpp: dict[str, str] = {}

    def cpp_of(unit: str) -> str:
        if unit not in cur_cpp:
            cur_cpp[unit] = cpp_hash(manifest.get(unit, {}).get("source", ""))
        return cur_cpp[unit]

    stale: set = set()  # units whose cache is stale/absent (the dangerous fallback)

    def fp(unit: str, mangled: str) -> str:
        h = cpp_of(unit)
        cu = cache_units.get(unit)
        if cu and cu.get("cpp_hash") == h:
            real = cache_funcs.get((unit, mangled))
            if real is not None:
                return real                      # real per-function range hash
            return FALLBACK + h                  # fresh unit, fn inherently unmappable
        stale.add(unit)                          # whole unit's cache is stale/absent
        return FALLBACK + h

    return fp, cpp_of, stale


# --------------------------------------------------------------------------- #
# report.json: current per-function fuzzy%                                    #
# --------------------------------------------------------------------------- #
def load_report(path: Path):
    """Return (overall measures, {(unit,fn): pct}, {unit: unit measures})."""
    if not path.is_file():
        sys.exit(f"no report: {path}\n  build it first: gruntz build")
    rep = json.loads(path.read_text())
    funcs: dict[tuple[str, str], float] = {}
    umeas: dict[str, dict] = {}
    for u in rep.get("units", []):
        unit = u.get("name", "")
        umeas[unit] = u.get("measures", {})
        for fn in u.get("functions", []):
            funcs[(unit, fn["name"])] = float(fn.get("fuzzy_match_percent") or 0.0)
    return rep.get("measures", {}), funcs, umeas


def func_code_sizes() -> dict[tuple[str, str], int]:
    """{(unit, mangled_name): byte_size} for the matched set.

    Joins build/gen/symbol_names.csv (rva,name,unit - the generated label map) to
    build/ghidra-enrich/exports/functions.csv (entry_rva,byte_size). Used to
    code-weight the per-function best-ever churn for the `Fuzzy Max` column so it
    is directly comparable to the (code-weighted) `Fuzzy` column."""
    import csv
    names = REPO / "build" / "gen" / "symbol_names.csv"
    funcs_csv = REPO / "build" / "ghidra-enrich" / "exports" / "functions.csv"
    size_by_rva: dict[int, int] = {}
    if funcs_csv.is_file():
        with funcs_csv.open() as f:
            for r in csv.DictReader(f):
                try:
                    size_by_rva[int(r["entry_rva"], 16)] = int(r["byte_size"])
                except (ValueError, KeyError):
                    pass
    out: dict[tuple[str, str], int] = {}
    if names.is_file():
        for line in names.read_text().splitlines():
            line = line.strip()
            if not line or line.startswith("#") or line.startswith("rva"):
                continue
            p = line.split(",")
            if len(p) < 3:
                continue
            try:
                rva = int(p[0], 16)
            except ValueError:
                continue
            out[(p[2], p[1])] = size_by_rva.get(rva, 0)   # (unit, name) -> size
    return out


def func_rvas() -> dict[tuple[str, str], int]:
    """{(unit, mangled_name): retail_rva} from build/gen/symbol_names.csv.

    The retail RVA is the STABLE identity of a function: changing its signature
    renames the mangled symbol but the RVA holds. The regression check uses this to
    tell a rename (same RVA, new name) from a genuine loss - so a signature edit
    (e.g. the stub-signature passes) no longer shows up as a phantom regression."""
    import csv
    names = REPO / "build" / "gen" / "symbol_names.csv"
    out: dict[tuple[str, str], int] = {}
    if names.is_file():
        for line in names.read_text().splitlines():
            line = line.strip()
            if not line or line.startswith("#") or line.startswith("rva"):
                continue
            p = line.split(",")
            if len(p) < 3:
                continue
            try:
                out[(p[2], p[1])] = int(p[0], 16)   # (unit, name) -> rva
            except ValueError:
                continue
    return out


# --------------------------------------------------------------------------- #
# baseline file I/O                                                           #
# --------------------------------------------------------------------------- #
EXACT = 99.995  # objdiff scores a byte-exact function at 100% (allow float slop)

HEADER = (
    "# gruntz match baseline - generated by gruntz.match.status. DO NOT hand-edit.\n"
    "#   units rows:     unit  n_functions  matched   (matched = at 100% fuzzy now;\n"
    "#                    the README's per-module 'exact' uses objdiff's byte-exact count)\n"
    "#   function rows:  unit  function  best_pct  cur_pct  tries  src_hash  rva\n"
    "#   rva      = retail RVA (stable identity): a vanished row whose rva is still\n"
    "#              occupied is a rename, not a loss - so signature edits don't regress.\n"
    "#   best_pct = best-ever (max) objdiff fuzzy% - the REGRESSION gate (may be 100%).\n"
    "#   cur_pct  = fuzzy% at THIS commit - `diff <revA> [<revB>]` two commits to see\n"
    "#              the moves (e.g. unit 5->10 fns, a fn 10%->40%) while best_pct holds.\n"
    "#   tries    = times this function's source fingerprint changed (how much it has\n"
    "#              been worked on - high = hard to match).\n"
    "#   src_hash = per-function source fingerprint (editing a fn bumps tries + resets best).\n"
    "# Query: python -m gruntz.match.status check | status | summary | diff <revA> [<revB>]\n"
)


def unit_counts(funcs: dict[tuple[str, str], dict]) -> dict[str, dict]:
    """Per-unit {n: total functions, matched: functions at 100% now} from the rows."""
    out: dict[str, dict] = {}
    for (unit, _), f in funcs.items():
        a = out.setdefault(unit, {"n": 0, "matched": 0})
        a["n"] += 1
        if f["cur"] >= EXACT:
            a["matched"] += 1
    return out


def load_baseline(text: str | None = None):
    """Parse the [functions] section into {(unit,fn): {best, cur, tries, fp}}. The
    [units] counts are derived (unit_counts), so we skip them here. `text` overrides
    the on-disk file (used by `diff` on `git show <rev>:...` output)."""
    funcs: dict[tuple[str, str], dict] = {}
    if text is None:
        if not BASELINE.is_file():
            return funcs
        text = BASELINE.read_text()
    section = None
    bad = 0
    for line in text.splitlines():
        if line.startswith("# [units]\t"):     # tab-precise: don't match HEADER prose
            section = "units"
            continue
        if line.startswith("# [functions]\t"):
            section = "functions"
            continue
        if not line or line.startswith("#") or section != "functions":
            continue
        c = line.split("\t")
        try:
            if len(c) < 6:
                raise ValueError("too few columns")
            # rva (7th col) is optional: pre-rename-aware baselines lack it, and a row
            # whose function had no symbol_names entry writes it empty. None -> the
            # rename check can't vouch for that row (falls back to fingerprint).
            addr = int(c[6], 16) if len(c) > 6 and c[6] else None
            funcs[(c[0], c[1])] = {"best": float(c[2]), "cur": float(c[3]),
                                   "tries": int(c[4]), "fp": c[5], "addr": addr}
        except ValueError:
            bad += 1
    if bad:  # surface corruption / an old-format (pre-this-PR) commit instead of crashing
        print(f"warning: {bad} malformed/old-format baseline row(s) skipped",
              file=sys.stderr)
    return funcs


def write_baseline(funcs: dict[tuple[str, str], dict]) -> None:
    units = unit_counts(funcs)
    lines = [HEADER, "# [units]\tunit\tn_functions\tmatched\n"]
    for unit in sorted(units):
        u = units[unit]
        lines.append(f"{unit}\t{u['n']}\t{u['matched']}\n")
    lines.append("# [functions]\tunit\tfunction\tbest_pct\tcur_pct\ttries\tsrc_hash\trva\n")
    for key in sorted(funcs):
        unit, fn = key
        f = funcs[key]
        addr = f.get("addr")
        addr_s = f"0x{addr:x}" if addr is not None else ""
        lines.append(f"{unit}\t{fn}\t{f['best']:.4f}\t{f['cur']:.4f}\t{f['tries']}\t{f['fp']}\t{addr_s}\n")
    BASELINE.write_text("".join(lines))


# --------------------------------------------------------------------------- #
# update: recompute best + fingerprint, rewrite baseline                      #
# --------------------------------------------------------------------------- #
def cmd_update(args) -> int:
    _, cur, _ = load_report(Path(args.report))
    base_funcs = load_baseline()
    fp, _, stale = fingerprinter()
    rvas = func_rvas()

    raised = touched = added = 0
    new_funcs: dict[tuple[str, str], dict] = {}
    for key, pct in cur.items():
        cur_fp = fp(*key)
        prev = base_funcs.get(key)
        if prev is None:
            new_funcs[key] = {"best": pct, "cur": pct, "tries": 1, "fp": cur_fp}  # 1st try
            added += 1
        elif real_edit(prev["fp"], cur_fp):
            # genuine source edit -> bump tries; old peak is stale, reset best to current
            best = max(prev["best"], pct) if args.keep_max else pct
            new_funcs[key] = {"best": best, "cur": pct,
                              "tries": prev["tries"] + 1, "fp": cur_fp}
            touched += 1
        else:
            # unchanged, OR a fallback on either side (fingerprint unknown): never bump
            # tries, never LOWER best; adopt a real fp if we now have one, else keep it.
            best = max(prev["best"], pct)
            keep_fp = cur_fp if not is_fallback(cur_fp) else prev["fp"]
            new_funcs[key] = {"best": best, "cur": pct,
                              "tries": prev["tries"], "fp": keep_fp}
            if best - prev["best"] > EPS:
                raised += 1

    for key, f in new_funcs.items():  # stamp the stable RVA (for rename-vs-loss in check)
        f["addr"] = rvas.get(key)

    if args.accept_regressions:  # bless ONLY regressed functions as their new floor
        for f in new_funcs.values():
            if f["cur"] < f["best"] - EPS:
                f["best"] = f["cur"]

    if stale:
        print(f"WARNING: fingerprint cache stale/absent for {len(stale)} unit(s) "
              f"({', '.join(sorted(stale))}); their functions kept prior fingerprints "
              f"(not refreshed). Run `gruntz build` / `gruntz.match.fingerprints` first for "
              f"per-function accuracy.", file=sys.stderr)

    lost = sorted(k for k in base_funcs if k not in cur)
    write_baseline(new_funcs)
    print(f"baseline written: {BASELINE.relative_to(REPO)}")
    print(f"  {len(new_funcs)} functions across {len({u for (u, _) in new_funcs})} units")
    print(f"  raised best: {raised}  tried(touched): {touched}  new: {added}  dropped: {len(lost)}")
    if lost and args.verbose:
        for u, fn in lost:
            print(f"    dropped {u}/{fn}")
    return 0


# --------------------------------------------------------------------------- #
# check: regressions vs baseline                                              #
# --------------------------------------------------------------------------- #
def classify(cur, base_funcs, fp, cur_rvas=frozenset()):
    """Yield (kind, unit, fn, cur_pct, best_pct) for every interesting delta.

    TOUCHED/REMOVED require a *real* edit (both fingerprints real and differing); a
    fallback fingerprint is "unknown", so it can't excuse a drop - the function
    still gets gated against best (REGRESS) or counted as LOST. This is what keeps a
    stale/absent cache from silently hiding regressions.

    `cur_rvas` is the set of (unit, rva) present in this build. A vanished baseline
    row whose recorded rva is still in it is a RENAME (same function, new mangled
    name - e.g. a signature change), classed REMOVED not LOST, so it doesn't gate."""
    for key, pct in sorted(cur.items()):
        unit, fn = key
        prev = base_funcs.get(key)
        if prev is None:
            yield ("NEW", unit, fn, pct, None)
        elif real_edit(prev["fp"], fp(*key)):
            yield ("TOUCHED", unit, fn, pct, prev["best"])
        elif pct < prev["best"] - EPS:
            yield ("REGRESS", unit, fn, pct, prev["best"])
        elif pct > prev["best"] + EPS:
            yield ("IMPROVE", unit, fn, pct, prev["best"])

    for key, prev in sorted(base_funcs.items()):
        if key in cur:
            continue
        unit, fn = key
        addr = prev.get("addr")
        # vanished: a rename (rva still occupied) or a real source edit -> REMOVED;
        # otherwise a genuine loss of a previously-matched function (LOST, gated).
        renamed = addr is not None and (unit, addr) in cur_rvas
        yield ("REMOVED" if (renamed or real_edit(prev["fp"], fp(*key))) else "LOST",
               unit, fn, None, prev["best"])


def cmd_check(args) -> int:
    _, cur, _ = load_report(Path(args.report))
    base_funcs = load_baseline()
    if not base_funcs:
        sys.exit("no baseline yet - seed it: python -m gruntz.match.status update")
    fp, _, stale = fingerprinter()
    # (unit, rva) present now - lets a vanished baseline row be recognised as a rename.
    rvas = func_rvas()
    cur_rvas = {(u, rvas[(u, fn)]) for (u, fn) in cur if (u, fn) in rvas}

    buckets: dict[str, list] = {}
    for kind, unit, fn, pct, best in classify(cur, base_funcs, fp, cur_rvas):
        buckets.setdefault(kind, []).append((unit, fn, pct, best))

    # `stale` is now populated (classify called fp for every function). A stale/absent
    # unit cache means none of its functions have a fresh per-function fingerprint, so
    # a real edit can't be told from a regression - surface it loudly so a clean
    # "no regressions" is never silently trusted (the failure this PR exists to catch).
    if stale:
        print(f"WARNING: fingerprint cache stale/absent for {len(stale)} unit(s) "
              f"({', '.join(sorted(stale))}) - gating degraded; run `gruntz build` or "
              f"`python -m gruntz.match.fingerprints` to refresh.", file=sys.stderr)

    regress = buckets.get("REGRESS", [])
    lost = buckets.get("LOST", [])
    n = len(regress) + len(lost)

    if args.json:
        print(json.dumps({"degraded_units": sorted(stale), **{k: [
            {"unit": u, "function": f, "cur": p, "best": b,
             "delta": (None if p is None else round(p - b, 4) if b is not None else None)}
            for (u, f, p, b) in v] for k, v in buckets.items()}}, indent=2))
        return 1 if (n and args.strict) else 0

    def show(kind, rows, arrow):
        if not rows:
            return
        print(f"\n{kind} ({len(rows)}):")
        for u, f, p, b in sorted(rows, key=lambda r: (r[2] - r[3]) if (r[2] is not None and r[3] is not None) else 0):
            if p is None:
                print(f"  {u:<16} {f}\n      was best {b:.4f}, now absent")
            elif b is None:
                print(f"  {u:<16} {f}   {p:.4f}")
            else:
                print(f"  {u:<16} {f}\n      {b:.4f} {arrow} {p:.4f}   (Δ {p - b:+.4f})")

    show("REGRESS", regress, "->")
    show("LOST", lost, "->")
    if args.all:
        show("IMPROVE", buckets.get("IMPROVE", []), "->")
        show("TOUCHED", buckets.get("TOUCHED", []), "~>")
        show("NEW", buckets.get("NEW", []), "")
        show("REMOVED", buckets.get("REMOVED", []), "->")

    if n:
        # Non-fatal by design: a fuzzy% drop is often NOT something the matcher
        # controls (matching one fn shifts a shared TU's codegen and nudges a
        # sibling; the target/delink side moves). This is a review signal, not a
        # build failure. Use --strict to opt into a non-zero exit (CI gate).
        print(f"\n{n} regression(s) to review. If a drop is uncontrollable/intended, "
              f"bless it: python -m gruntz.match.status update --accept-regressions")
    else:
        extra = ""
        if buckets.get("IMPROVE"):
            extra = f"  ({len(buckets['IMPROVE'])} unblessed improvement(s) - run `update`)"
        if stale:
            extra += f"  [DEGRADED: {len(stale)} unit(s) without a fresh fingerprint cache]"
        print(f"no regressions vs baseline.{extra}")
    return 1 if (n and args.strict) else 0


# --------------------------------------------------------------------------- #
# status: per-function current %, filterable                                  #
# --------------------------------------------------------------------------- #
def cmd_status(args) -> int:
    _, cur, _ = load_report(Path(args.report))
    base_funcs = load_baseline()
    rows = []
    for (unit, fn), pct in cur.items():
        if args.unit and unit != args.unit:
            continue
        if args.grep and args.grep not in fn:
            continue
        if args.below is not None and pct >= args.below:
            continue
        prev = base_funcs.get((unit, fn))
        best = prev["best"] if prev else None
        tries = prev["tries"] if prev else None
        rows.append((unit, fn, pct, best, tries))
    # worst first; --by-tries surfaces the most-worked-on functions
    rows.sort(key=lambda r: (-(r[4] or 0),) if args.by_tries else (r[2], r[0], r[1]))

    if args.json:
        print(json.dumps([
            {"unit": u, "function": f, "pct": p, "best": b, "tries": t}
            for (u, f, p, b, t) in rows], indent=2))
        return 0

    print(f"{'unit':<16} {'fuzzy%':>9}  {'best%':>9}  {'tries':>5}  function")
    for u, f, p, b, t in rows:
        bs = f"{b:9.4f}" if b is not None else f"{'--':>9}"
        ts = f"{t:>5}" if t is not None else f"{'--':>5}"
        flag = "  <- REGRESS" if (b is not None and p < b - EPS) else ""
        print(f"{u:<16} {p:9.4f}  {bs}  {ts}  {f}{flag}")
    print(f"\n{len(rows)} function(s).")
    return 0


# --------------------------------------------------------------------------- #
# summary: overall + per-module/per-unit rollup                               #
# --------------------------------------------------------------------------- #
def collect_modules(manifest, umeas):
    """Aggregate per-module measures from the report; also return the started-unit
    fuzzy-weighted sum + code so the headline can normalise against the engine."""
    mods: dict[str, dict] = {}
    units: dict[str, dict] = {}
    started_fzw = 0.0
    started_code = 0
    for unit, m in umeas.items():
        mod = manifest.get(unit, {}).get("module", "?")
        tc = int(m.get("total_code") or 0)
        mc = int(m.get("matched_code") or 0)
        fz = float(m.get("fuzzy_match_percent") or 0.0)
        tf = int(m.get("total_functions") or 0)
        mf = int(m.get("matched_functions") or 0)
        units[unit] = {"module": mod, "fuzzy": fz, "tf": tf, "mf": mf, "tc": tc, "mc": mc}
        a = mods.setdefault(mod, {"tc": 0, "mc": 0, "fzw": 0.0, "tf": 0, "mf": 0, "units": 0})
        a["tc"] += tc
        a["mc"] += mc
        a["fzw"] += fz * tc
        a["tf"] += tf
        a["mf"] += mf
        a["units"] += 1
        started_fzw += fz * tc
        started_code += tc
    return mods, units, started_fzw, started_code


def render_report(overall, mods, started_fzw, started_code) -> str:
    """The README score block: three metrics per started module + an (unmatched)
    row, with totals weighed against the FULL ENGINE (everything not matched yet)
    as the denominator, not just the units we've started."""
    matched_fn = int(overall.get("matched_functions") or 0)
    matched_code = int(overall.get("matched_code") or 0)
    started_fn = int(overall.get("total_functions") or 0)

    eng = engine_universe()
    if eng:
        tot_fn, tot_code = eng["real_fn"], eng["real_code"]
        scope = "full engine"
    else:
        tot_fn, tot_code = started_fn, started_code
        scope = "started units"

    rows = []
    for mod in sorted(mods, key=lambda k: -mods[k]["tf"]):
        a = mods[mod]
        fz = (a["fzw"] / a["tc"] if a["tc"] else 0.0)   # fzw is already %*bytes
        fzmax = fz + (a.get("cw", 0.0) / a["tc"] if a["tc"] else 0.0)
        rows.append([
            f"`{mod}`", f"{a['units']}",
            f"{a['mf']:,} / {a['tf']:,} ({_pct(a['mf'], a['tf']):.1f}%)",
            f"{fz:.1f}%",
            f"{fzmax:.1f}%",
        ])
    if eng:
        un_fn = max(0, tot_fn - started_fn)
        rows.append(["`(unmatched)`", "—", f"0 / {un_fn:,} (0.0%)", "0.0%", "0.0%"])
    table = _md_table(
        ["Module", "Units", "Functions exact", "Fuzzy", "Fuzzy Max"], "lrrrr", rows)

    # Carve-out categories: generated/library code excluded from the % above, shown
    # for transparency (counted, not hidden). Empty when the engine scope is absent.
    excl_lines: list[str] = []
    if eng:
        ex_rows = [[f"`{label}`", f"{fn:,}", f"{code:,}", note]
                   for (label, fn, code, note) in eng["categories"]]
        excl_lines = [
            "",
            "_Excluded from the % above — generated/library code, not independent "
            "reconstruction targets:_",
            "",
            *_md_table(["Category", "Functions", "Code (B)", "Why excluded"],
                       "lrrl", ex_rows),
        ]

    # fuzzy_weighted is already sum(percent * bytes); divide by bytes for the
    # weighted percent - do NOT _pct it (that would multiply by 100 again).
    overall_fuzzy = started_fzw / tot_code if tot_code else 0.0
    started_fuzzy = started_fzw / started_code if started_code else 0.0
    overall_cw = sum(a.get("cw", 0.0) for a in mods.values())
    overall_fuzzy_max = overall_fuzzy + (overall_cw / tot_code if tot_code else 0.0)
    block = [
        RM_START,
        "## Match status",
        "",
        "_Auto-generated by `python -m gruntz.match.status summary --write-readme` "
        "(refreshed by `gruntz build`); do not hand-edit. Diff this block across "
        "commits to spot regressions._",
        "",
        f"**Overall (vs {scope}): {matched_fn:,} / {tot_fn:,} functions exact "
        f"({_pct(matched_fn, tot_fn):.2f}%) &middot; {overall_fuzzy:.2f}% fuzzy "
        f"&middot; {overall_fuzzy_max:.2f}% fuzzy max.**",
        "",
        "_Totals are vs the whole engine = every in-`.text` reconstruction-target "
        "function; the generated/library categories tabled below (compiler EH "
        "funclets, CRT/MFC library, jump thunks) are excluded from the denominator. "
        "The bulk we have not started is the `(unmatched)` row. `Fuzzy` = code-"
        "weighted partial credit (how close); `Fuzzy Max` = the same with every "
        "function at its best-ever fuzzy% - a gap above `Fuzzy` is entropy churn "
        "since the last `update`._",
        "",
        f"_Started units alone: {matched_fn:,}/{started_fn:,} fns exact, "
        f"{started_fuzzy:.2f}% fuzzy over {started_code:,} of {tot_code:,} engine "
        f"code bytes._",
        "",
        *table,
        *excl_lines,
        RM_END,
    ]
    return "\n".join(block)


def cmd_summary(args) -> int:
    manifest = load_units()
    overall, funcs, umeas = load_report(Path(args.report))
    mods, units, started_fzw, started_code = collect_modules(manifest, umeas)
    # Per-module best-ever churn (code-weighted), for the `Fuzzy Max` column:
    # sum (best_ever - current) * bytes over functions now below their peak. Added
    # to Fuzzy gives Fuzzy Max; Fuzzy Max > Fuzzy means entropy churn since the
    # last `update`. Zero gap = no churn.
    sizes = func_code_sizes()
    base = load_baseline()
    for a in mods.values():
        a["cw"] = 0.0
    for (unit, fn), cur in funcs.items():
        b = base.get((unit, fn))
        if not b:
            continue
        churn = b["best"] - cur
        if churn <= EPS:
            continue
        mod = manifest.get(unit, {}).get("module", "?")
        if mod in mods:
            mods[mod]["cw"] += churn * sizes.get((unit, fn), 0)

    if args.json:
        eng = engine_universe()
        print(json.dumps({
            "overall": overall,
            "engine_universe": ({
                "real_functions": eng["real_fn"], "real_code": eng["real_code"],
                "excluded_categories": [
                    {"label": l, "functions": fn, "code": c, "note": note}
                    for (l, fn, c, note) in eng["categories"]],
            } if eng else None),
            "modules": {k: {**v, "fuzzy": (v["fzw"] / v["tc"] if v["tc"] else 0.0)}
                        for k, v in mods.items()},
            "units": units,
        }, indent=2, default=float))
        return 0

    block = render_report(overall, mods, started_fzw, started_code)
    if args.write_readme:
        write_readme(block)
        print(block)
        print(f"\n[match_status] wrote score block to {README.relative_to(REPO)}")
    else:
        print(block)

    if args.per_unit:
        print(f"\n{'unit':<16} {'exact fn':>10} {'fuzzy%':>9} module")
        for unit in sorted(units, key=lambda u: units[u]["fuzzy"]):
            u = units[unit]
            print(f"{unit:<16} {u['mf']:>4}/{u['tf']:<5} {u['fuzzy']:9.4f} {u['module']}")
    return 0


# --------------------------------------------------------------------------- #
# diff: compare two commits' baselines                                        #
# --------------------------------------------------------------------------- #
def _git_show(rev: str) -> str:
    rel = BASELINE.relative_to(REPO).as_posix()
    r = subprocess.run(["git", "show", f"{rev}:{rel}"], cwd=str(REPO),
                       capture_output=True, text=True)
    if r.returncode != 0:
        sys.exit(f"git show {rev}:{rel} failed: {r.stderr.strip()}")
    return r.stdout


def cmd_diff(args) -> int:
    a = load_baseline(_git_show(args.revA))
    b = load_baseline(_git_show(args.revB)) if args.revB else load_baseline()
    label_b = args.revB or "working tree"
    # guard BOTH sides: an empty parse (absent or pre-this-format baseline) would
    # otherwise mislabel every function as all-NEW or all-LOST, implying a disaster
    # that is really a format/absence artifact.
    if not a:
        sys.exit(f"no usable baseline at {args.revA} (absent or pre-this-format)")
    if not b:
        sys.exit(f"no usable baseline at {label_b} (absent or pre-this-format)")

    # unit-level: function-count / matched-count moves (e.g. 5 -> 10)
    ca, cb = unit_counts(a), unit_counts(b)
    unit_rows = []
    for u in sorted(set(ca) | set(cb)):
        na, nb = ca.get(u, {}).get("n", 0), cb.get(u, {}).get("n", 0)
        ma, mb = ca.get(u, {}).get("matched", 0), cb.get(u, {}).get("matched", 0)
        if na != nb or ma != mb:
            unit_rows.append((u, na, nb, ma, mb))

    # function-level: cur% moves, with max + tries context
    buckets: dict[str, list] = {}
    for key in sorted(set(a) | set(b)):
        unit, fn = key
        pa, pb = a.get(key), b.get(key)
        if pa is None:
            buckets.setdefault("NEW", []).append((unit, fn, None, pb["cur"], pb["best"], None, pb["tries"]))
        elif pb is None:
            buckets.setdefault("LOST", []).append((unit, fn, pa["cur"], None, pa["best"], pa["tries"], None))
        else:
            kind = ("TOUCHED" if real_edit(pa["fp"], pb["fp"])
                    else "REGRESS" if pb["cur"] < pa["cur"] - EPS
                    else "IMPROVE" if pb["cur"] > pa["cur"] + EPS else None)
            if kind:
                buckets.setdefault(kind, []).append(
                    (unit, fn, pa["cur"], pb["cur"], pb["best"], pa["tries"], pb["tries"]))

    if args.json:
        print(json.dumps({
            "from": args.revA, "to": label_b,
            "units": [{"unit": u, "n": [na, nb], "matched": [ma, mb]}
                      for (u, na, nb, ma, mb) in unit_rows],
            "functions": {k: [{"unit": u, "function": f, "from": x, "to": y, "max": m,
                               "tries": [ta, tb]} for (u, f, x, y, m, ta, tb) in v]
                          for k, v in buckets.items()},
        }, indent=2))
        return 0

    print(f"diff {args.revA} -> {label_b}")
    if unit_rows:
        print("\nUnits (function count):")
        for u, na, nb, ma, mb in unit_rows:
            print(f"  {u:<16} {na} -> {nb} functions   ({ma} -> {mb} matched)")

    def pp(x):
        return f"{x:.2f}%" if x is not None else "--"

    def show(kind):
        rows = buckets.get(kind, [])
        if not rows:
            return
        print(f"\n{kind} ({len(rows)}):")
        for u, f, x, y, m, ta, tb in rows:
            tries = (f"tries {ta}->{tb}" if ta is not None and tb is not None
                     else f"tries {tb if tb is not None else ta}")
            print(f"  {u:<16} {f}\n      {pp(x)} -> {pp(y)}   (max {m:.2f}%, {tries})")

    for kind in ("REGRESS", "LOST", "IMPROVE", "NEW"):
        show(kind)
    if args.all:
        show("TOUCHED")

    n = len(buckets.get("REGRESS", [])) + len(buckets.get("LOST", []))
    untouched = len(buckets.get("TOUCHED", []))
    extra = f"  ({untouched} touched - use --all)" if untouched and not args.all else ""
    print(f"\n{n} regression(s)/loss(es) {args.revA} -> {label_b}.{extra}")
    return 0


# --------------------------------------------------------------------------- #
def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--report", default=str(DEFAULT_REPORT),
                    help="objdiff report.json (default: build/objdiff/report.json)")
    sub = ap.add_subparsers(dest="cmd", required=True)

    c = sub.add_parser("check", help="list regressions vs baseline (non-fatal)")
    c.add_argument("--json", action="store_true")
    c.add_argument("--all", action="store_true", help="also show improve/touched/new")
    c.add_argument("--strict", action="store_true",
                   help="exit 1 if regressions/lost (opt-in CI gate); default non-fatal")
    c.set_defaults(func=cmd_check)

    s = sub.add_parser("status", help="per-function current %, filterable")
    s.add_argument("--unit")
    s.add_argument("--grep", help="substring match on (mangled) function name")
    s.add_argument("--below", type=float, help="only functions under this fuzzy%")
    s.add_argument("--by-tries", action="store_true",
                   help="sort by tries desc (most-worked-on first)")
    s.add_argument("--json", action="store_true")
    s.set_defaults(func=cmd_status)

    d = sub.add_parser("diff", help="compare two commits' baselines (cur% + unit + tries moves)")
    d.add_argument("revA", help="git rev (e.g. HEAD~1, a sha, a tag)")
    d.add_argument("revB", nargs="?", help="git rev (default: the working-tree baseline)")
    d.add_argument("--all", action="store_true", help="also show TOUCHED (source changed)")
    d.add_argument("--json", action="store_true")
    d.set_defaults(func=cmd_diff)

    r = sub.add_parser("summary", help="3-metric report vs the full engine (README block)")
    r.add_argument("--write-readme", action="store_true",
                   help="refresh the score block in README.md between markers")
    r.add_argument("--per-unit", action="store_true")
    r.add_argument("--json", action="store_true")
    r.set_defaults(func=cmd_summary)

    u = sub.add_parser("update", help="recompute best+fingerprint, write baseline")
    u.add_argument("--accept-regressions", action="store_true",
                   help="bless current as the new floor (lower best to current)")
    u.add_argument("--keep-max", action="store_true",
                   help="do NOT reset best when a function's source changed")
    u.add_argument("-v", "--verbose", action="store_true")
    u.set_defaults(func=cmd_update)

    args = ap.parse_args()
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
