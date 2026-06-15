#!/usr/bin/env python3
"""match_status.py - make matching progress and REGRESSIONS queriable.

The matching loop grinds GRUNTZ.EXE functions into byte-exact src/. objdiff gives
us a fuzzy% per function (build/objdiff/report.json). This tool remembers the
BEST-EVER fuzzy% per function in a small, git-tracked text file
(config/match_baseline.tsv) so we can answer one question cheaply:

    "did anything REGRESS?"  ->  scripts/match_status.py check

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

The fingerprint is PER FUNCTION, not per .cpp: scripts/func_fingerprints.py asks
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
    scripts/match_status.py check          # regressions vs baseline (non-fatal)
    scripts/match_status.py status [...]    # per-function current %, filterable
    scripts/match_status.py summary         # overall + per-module/per-unit rollup
    scripts/match_status.py update          # recompute best+fingerprint, write baseline

    nix develop --command python3 scripts/match_status.py check
"""
from __future__ import annotations

import argparse
import json
import subprocess
import sys
import tomllib
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
MANIFEST = REPO / "config" / "units.toml"
BASELINE = REPO / "config" / "match_baseline.tsv"
DEFAULT_REPORT = REPO / "build" / "objdiff" / "report.json"

# Full-target universe (the denominator: "everything not matched yet"). objdiff's
# report.json only totals the functions we've pulled into units; for honest
# progress we weigh against the whole engine - every in-.text non-thunk function
# minus the FID-identified CRT/MFC library code. functions.csv is the delinker's
# Ghidra input (present after `gruntz build`); the FID list is committed. Absent
# (fresh worktree, no build) -> the report falls back to started-unit scope.
FUNCS_CSV = REPO / "build" / "ghidra-enrich" / "exports" / "functions.csv"
FID_CSV = REPO / "config" / "library_labels.csv"

README = REPO / "README.md"
RM_START = "<!-- match-score:start -->"
RM_END = "<!-- match-score:end -->"

EPS = 0.01  # ignore sub-0.01% jitter


def _pct(num: float, den: float) -> float:
    return 100.0 * num / den if den else 0.0


def engine_universe():
    """(n_functions, total_code) of the reversing target: all in-.text non-thunk
    functions minus FID library code. None when the Ghidra/FID exports are absent
    (callers then fall back to objdiff's started-unit totals)."""
    if not (FUNCS_CSV.is_file() and FID_CSV.is_file()):
        return None
    import csv

    def rint(s):
        s = str(s).strip()
        return int(s, 16) if s.lower().startswith("0x") else int(s)

    funcs: dict[int, int] = {}
    with open(FUNCS_CSV) as f:
        for r in csv.DictReader(f):
            try:
                rva, sz = rint(r["entry_rva"]), int(r["byte_size"])
            except Exception:
                continue
            if r.get("in_text", "1") == "1" and r.get("is_thunk", "0") != "1":
                funcs[rva] = sz
    lib: set[int] = set()
    with open(FID_CSV) as f:
        for r in csv.DictReader(f):
            try:
                lib.add(rint(r["rva"]))
            except Exception:
                pass
    eng = [s for rva, s in funcs.items() if rva not in lib]
    return (len(eng), sum(eng))


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

sys.path.insert(0, str(REPO / "scripts"))
from func_fingerprints import CACHE as FP_CACHE, cpp_hash, load_cache as load_fp_cache  # noqa: E402


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


def fingerprinter():
    """Return fp(unit, mangled) -> the CURRENT per-function source fingerprint.

    Uses the func_fingerprints cache when it is fresh for the unit (its recorded
    cpp_hash still matches the working tree); otherwise falls back to the unit's
    whole-.cpp hash - which also means "this unit was edited since the cache was
    built", the safe (touched) direction.
    """
    manifest = load_units()
    cache_units, cache_funcs = load_fp_cache()
    cur_cpp: dict[str, str] = {}

    def cpp_of(unit: str) -> str:
        if unit not in cur_cpp:
            cur_cpp[unit] = cpp_hash(manifest.get(unit, {}).get("source", ""))
        return cur_cpp[unit]

    def fp(unit: str, mangled: str) -> str:
        h = cpp_of(unit)
        cu = cache_units.get(unit)
        if cu and cu.get("cpp_hash") == h:
            return cache_funcs.get((unit, mangled), h)  # per-fn, else unit fallback
        return h  # cache stale/missing -> unit-level

    return fp, cpp_of


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


# --------------------------------------------------------------------------- #
# baseline file I/O                                                           #
# --------------------------------------------------------------------------- #
EXACT = 99.995  # objdiff scores a byte-exact function at 100% (allow float slop)

HEADER = (
    "# gruntz match baseline - generated by scripts/match_status.py. DO NOT hand-edit.\n"
    "#   units rows:     unit  n_functions  matched   (matched = functions at 100% now)\n"
    "#   function rows:  unit  function  best_pct  cur_pct  tries  src_hash\n"
    "#   best_pct = best-ever (max) objdiff fuzzy% - the REGRESSION gate (may be 100%).\n"
    "#   cur_pct  = fuzzy% at THIS commit - `diff <revA> [<revB>]` two commits to see\n"
    "#              the moves (e.g. unit 5->10 fns, a fn 10%->40%) while best_pct holds.\n"
    "#   tries    = times this function's source fingerprint changed (how much it has\n"
    "#              been worked on - high = hard to match).\n"
    "#   src_hash = per-function source fingerprint (editing a fn bumps tries + resets best).\n"
    "# Query: scripts/match_status.py check | status | summary | diff <revA> [<revB>]\n"
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
    for line in text.splitlines():
        if line.startswith("# [units]\t"):     # tab-precise: don't match HEADER prose
            section = "units"
            continue
        if line.startswith("# [functions]\t"):
            section = "functions"
            continue
        if not line or line.startswith("#"):
            continue
        c = line.split("\t")
        if section == "functions" and len(c) >= 6:
            funcs[(c[0], c[1])] = {"best": float(c[2]), "cur": float(c[3]),
                                   "tries": int(c[4]), "fp": c[5]}
    return funcs


def write_baseline(funcs: dict[tuple[str, str], dict]) -> None:
    units = unit_counts(funcs)
    lines = [HEADER, "# [units]\tunit\tn_functions\tmatched\n"]
    for unit in sorted(units):
        u = units[unit]
        lines.append(f"{unit}\t{u['n']}\t{u['matched']}\n")
    lines.append("# [functions]\tunit\tfunction\tbest_pct\tcur_pct\ttries\tsrc_hash\n")
    for key in sorted(funcs):
        unit, fn = key
        f = funcs[key]
        lines.append(f"{unit}\t{fn}\t{f['best']:.4f}\t{f['cur']:.4f}\t{f['tries']}\t{f['fp']}\n")
    BASELINE.write_text("".join(lines))


# --------------------------------------------------------------------------- #
# update: recompute best + fingerprint, rewrite baseline                      #
# --------------------------------------------------------------------------- #
def cmd_update(args) -> int:
    _, cur, _ = load_report(Path(args.report))
    base_funcs = load_baseline()
    fp, _ = fingerprinter()

    raised = touched = added = 0
    new_funcs: dict[tuple[str, str], dict] = {}
    for key, pct in cur.items():
        cur_fp = fp(*key)
        prev = base_funcs.get(key)
        if prev is None:
            # first sighting -> one try so far
            new_funcs[key] = {"best": pct, "cur": pct, "tries": 1, "fp": cur_fp}
            added += 1
        elif prev["fp"] != cur_fp:
            # source changed -> bump tries; old peak is stale, reset best to current
            best = max(prev["best"], pct) if args.keep_max else pct
            new_funcs[key] = {"best": best, "cur": pct,
                              "tries": prev["tries"] + 1, "fp": cur_fp}
            touched += 1
        else:
            best = max(prev["best"], pct)
            new_funcs[key] = {"best": best, "cur": pct,
                              "tries": prev["tries"], "fp": cur_fp}
            if best - prev["best"] > EPS:
                raised += 1

    if args.accept_regressions:  # bless current as the new floor (keep cur/tries/fp)
        for key in new_funcs:
            new_funcs[key]["best"] = new_funcs[key]["cur"]

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
def classify(cur, base_funcs, fp):
    """Yield (kind, unit, fn, cur_pct, best_pct) for every interesting delta."""
    for key, pct in sorted(cur.items()):
        unit, fn = key
        prev = base_funcs.get(key)
        if prev is None:
            yield ("NEW", unit, fn, pct, None)
        elif prev["fp"] != fp(*key):
            yield ("TOUCHED", unit, fn, pct, prev["best"])
        elif pct < prev["best"] - EPS:
            yield ("REGRESS", unit, fn, pct, prev["best"])
        elif pct > prev["best"] + EPS:
            yield ("IMPROVE", unit, fn, pct, prev["best"])

    for key, prev in sorted(base_funcs.items()):
        if key in cur:
            continue
        unit, fn = key
        # a function that vanished while its source is unchanged is a real loss;
        # if its fingerprint moved (source edited), it was likely renamed/removed
        changed = prev["fp"] != fp(*key)
        yield ("REMOVED" if changed else "LOST", unit, fn, None, prev["best"])


def cmd_check(args) -> int:
    _, cur, _ = load_report(Path(args.report))
    base_funcs = load_baseline()
    if not base_funcs:
        sys.exit("no baseline yet - seed it: scripts/match_status.py update")
    fp, _ = fingerprinter()

    # HONESTY GUARD: the baseline stores per-function fingerprints, but if the
    # clangd cache is missing/stale fp() degrades to the unit's whole-.cpp hash
    # for every function in that unit. That makes every baseline row look TOUCHED
    # (fp mismatch) and SILENTLY HIDES real regressions. Warn loudly so a green
    # "no regressions" is never mistaken for a real all-clear. Refresh with
    # `gruntz build` (runs func_fingerprints) or `scripts/func_fingerprints.py`.
    if not FP_CACHE.is_file():
        print("WARNING: func-fingerprint cache absent "
              f"({FP_CACHE.relative_to(REPO)}) - regression gate is DEGRADED to "
              "whole-.cpp hashing; edited units mask ALL their functions as "
              "TOUCHED, so regressions there are NOT flagged. Build the cache: "
              "scripts/func_fingerprints.py", file=sys.stderr)

    buckets: dict[str, list] = {}
    for kind, unit, fn, pct, best in classify(cur, base_funcs, fp):
        buckets.setdefault(kind, []).append((unit, fn, pct, best))

    regress = buckets.get("REGRESS", [])
    lost = buckets.get("LOST", [])
    n = len(regress) + len(lost)

    if args.json:
        print(json.dumps({k: [
            {"unit": u, "function": f, "cur": p, "best": b,
             "delta": (None if p is None else round(p - b, 4) if b is not None else None)}
            for (u, f, p, b) in v] for k, v in buckets.items()}, indent=2))
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
              f"bless it: scripts/match_status.py update --accept-regressions")
    else:
        extra = ""
        if buckets.get("IMPROVE"):
            extra = f"  ({len(buckets['IMPROVE'])} unblessed improvement(s) - run `update`)"
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
        tot_fn, tot_code = eng
        scope = "full engine"
    else:
        tot_fn, tot_code = started_fn, started_code
        scope = "started units"

    rows = []
    for mod in sorted(mods, key=lambda k: -mods[k]["tf"]):
        a = mods[mod]
        rows.append([
            f"`{mod}`", f"{a['units']}",
            f"{a['mf']:,} / {a['tf']:,} ({_pct(a['mf'], a['tf']):.1f}%)",
            f"{(a['fzw'] / a['tc'] if a['tc'] else 0):.1f}%",  # fzw is already %*bytes
            f"{_pct(a['mc'], a['tc']):.1f}%",
        ])
    if eng:
        un_fn = max(0, tot_fn - started_fn)
        rows.append(["`(unmatched)`", "—", f"0 / {un_fn:,} (0.0%)", "0.0%", "0.0%"])
    table = _md_table(
        ["Module", "Units", "Functions exact", "Fuzzy", "Code matched"], "lrrrr", rows)

    # fuzzy_weighted is already sum(percent * bytes); divide by bytes for the
    # weighted percent - do NOT _pct it (that would multiply by 100 again).
    overall_fuzzy = started_fzw / tot_code if tot_code else 0.0
    started_fuzzy = started_fzw / started_code if started_code else 0.0
    block = [
        RM_START,
        "## Match status",
        "",
        "_Auto-generated by `scripts/match_status.py summary --write-readme` "
        "(refreshed by `gruntz build`); do not hand-edit. Diff this block across "
        "commits to spot regressions._",
        "",
        f"**Overall (vs {scope}): {matched_fn:,} / {tot_fn:,} functions exact "
        f"({_pct(matched_fn, tot_fn):.2f}%) &middot; {_pct(matched_code, tot_code):.2f}% "
        f"code matched &middot; {overall_fuzzy:.2f}% fuzzy.**",
        "",
        "_Totals are vs the whole engine = every in-`.text` non-thunk function "
        "minus FID-identified CRT/MFC library code; the bulk we have not started "
        "is the `(unmatched)` row. `Fuzzy` = code-weighted partial credit (how "
        "close); `Code matched` = byte-exact only._",
        "",
        f"_Started units alone: {matched_fn:,}/{started_fn:,} fns exact, "
        f"{started_fuzzy:.2f}% fuzzy over {started_code:,} of {tot_code:,} engine "
        f"code bytes._",
        "",
        *table,
        RM_END,
    ]
    return "\n".join(block)


def cmd_summary(args) -> int:
    manifest = load_units()
    overall, _, umeas = load_report(Path(args.report))
    mods, units, started_fzw, started_code = collect_modules(manifest, umeas)

    if args.json:
        eng = engine_universe()
        print(json.dumps({
            "overall": overall,
            "engine_universe": ({"functions": eng[0], "code": eng[1]} if eng else None),
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
    if not a:
        sys.exit(f"no baseline at {args.revA}")
    label_b = args.revB or "working tree"

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
            kind = ("TOUCHED" if pa["fp"] != pb["fp"]
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
