#!/usr/bin/env python3
"""gruntz.core.exe_map - a queryable map of retail GRUNTZ.EXE's .text space.

Answers "what lives where" across the whole binary: every carved function in RVA
order, HOW IT IS CLASSIFIED (a src/ TU / MFC / CRT / zlib / compiler helper /
an EH funclet / a linker jump thunk / still-unknown), and where the GAPS between
functions are.

Data sources (all products of `gruntz build`):
  config/retail/functions.tsv               - admitted function starts/extents;
      the master ordered list of the .text function layout.
  build/gen/symbol_names.csv                 - what src/ CLAIMS (rva->mangled,unit):
      the "owned by a TU" set, regenerated every build from the `RVA()` annotations.
  config/retail/library_labels.csv           - FID-identified CRT/MFC/zlib/EH library
      code (rva->name,lib,confidence).
  config/retail/compiler-generated-functions.tsv - compiler-private `$E<n>` helpers.
  config/retail/compiler-helper-functions.tsv - proven forwarding helpers.
  config/units.toml                          - unit -> source path (the FILENAME an
      owned function maps back to).

Classification precedence mirrors gruntz.match.status.engine_universe, so the
categories here line up with the README's carve-out rows:
  1. claimed by a src/ unit  -> "unit"   (owner = its source file)
  2. linker jump thunk       -> "thunk"  (leading ILT jmp-table + thunk_* + tiny FF25/glue)
  3. proven compiler helper  -> "compiler"
  4. FID library             -> mfc/crt/zlib/eh/asm  (split by the FID `lib` column)
  5. Unwind@ EH funclet      -> "eh"
  6. otherwise               -> "unknown" (a real game/engine body not yet reconstructed)

Usage:
  python -m gruntz.core.exe_map                        # whole-binary overview
  python -m gruntz.core.exe_map range 0x24000 0x25000  # functions in an RVA window
  python -m gruntz.core.exe_map range 0x24000-0x25000  # (same; single-arg form)
  python -m gruntz.core.exe_map file GruntzMgr.cpp     # a file's functions + gap occupancy
  python -m gruntz.core.exe_map file src/Net --gaps    # + enumerate the inter-fn gaps
  python -m gruntz.core.exe_map at 0x2494c0            # the function covering one RVA
  python -m gruntz.core.exe_map gaps --min 32          # uncovered holes in .text
  python -m gruntz.core.exe_map units --top 30         # per-source-file breakdown
  python -m gruntz.core.exe_map find CGrunt            # functions whose name matches

Add --json to any command for machine-readable output.
"""
import argparse
import json
import re
import sys
from pathlib import Path

from gruntz.core.function_universe import classify as classify_function_universe

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parent)
FUNCS_CSV = REPO / "config/retail/functions.tsv"
UNITS_TOML = REPO / "config/units.toml"

# FID `lib` column -> our category. NAFXCW is the static MFC lib; LIBCMT/LIBCIMT the
# C/C++ runtime; the rest are self-descriptive. Anything unmapped falls back to "crt".
LIB_CATEGORY = {
    "NAFXCW": "mfc",
    "LIBCMT": "crt",
    "LIBCIMT": "crt",
    "zlib": "zlib",
    "MSVC-EH": "eh",
    "MSVC-THUNK": "thunk",
    "GAME-ASM": "asm",
}

# Fixed-width tag + one-line meaning for each category, in report order.
CATEGORIES = {
    "unit":    ("UNIT",  "reconstructed in a src/ TU"),
    "unknown": ("????",  "real body, not yet reconstructed"),
    "mfc":     ("MFC",   "MFC library (NAFXCW), statically linked"),
    "crt":     ("CRT",   "C/C++ runtime (LIBCMT), statically linked"),
    "zlib":    ("ZLIB",  "zlib, statically linked"),
    "asm":     ("ASM",   "hand-written game asm (FID GAME-ASM)"),
    "eh":      ("EH",    "compiler /GX EH unwind funclet"),
    "compiler":("COMP",  "private static-object lifecycle/cleanup helper"),
    "thunk":   ("THUNK", "linker ILT jmp-table / import glue"),
}

# Compact owner word for the per-row "owner" column (non-unit rows).
OWNER_WORD = {
    "unknown": "unknown (unreconstructed)", "mfc": "MFC library", "crt": "CRT library",
    "zlib": "zlib", "asm": "game asm", "eh": "EH funclet",
    "compiler": "private lifecycle/cleanup helper", "thunk": "linker jump-thunk",
}


def _rint(s):
    s = str(s).strip()
    if not s:
        raise ValueError("empty")
    return int(s, 16) if s.lower().startswith("0x") else int(s)


def _text_bounds(funcs):
    """(.text lo, .text hi). Prefer the real PE section table (via vtable_scan, which
    reads GRUNTZ.EXE) so the trailing gap/padding to section end is accurate; fall
    back to the span of carved functions when the EXE isn't present."""
    try:
        from gruntz.core import vtable_scan as vs
        for (nm, va, vsz, rsz, rp, ch) in vs.SECS:
            if nm == ".text":
                return va, va + max(vsz, rsz)
    except Exception:
        pass
    lo = min(f["rva"] for f in funcs)
    hi = max(f["end"] for f in funcs)
    return lo, hi


def _sections():
    """Full PE section table [(name, lo, hi, exec)] or None if the EXE is absent."""
    try:
        from gruntz.core import vtable_scan as vs
        return [(nm, va, va + max(vsz, rsz), bool(ch & 0x20000000))
                for (nm, va, vsz, rsz, rp, ch) in vs.SECS]
    except Exception:
        return None


def _load_units():
    """unit-stem -> source path, from config/units.toml."""
    out = {}
    if not UNITS_TOML.is_file():
        return out
    try:
        from gruntz.core import manifest
        for u in manifest.units():
            if u.get("unit"):
                out[u["unit"]] = u.get("source", "")
        return out
    except Exception:
        pass
    # Fallback parser: pair the `unit`/`source` keys inside each [[unit]] block.
    unit = None
    for line in UNITS_TOML.read_text().splitlines():
        m = re.match(r'\s*unit\s*=\s*"([^"]+)"', line)
        if m:
            unit = m.group(1)
            out.setdefault(unit, "")
            continue
        m = re.match(r'\s*source\s*=\s*"([^"]+)"', line)
        if m and unit:
            out[unit] = m.group(1)
    return out


def load():
    """Build the ordered, classified function list.

    Returns (funcs, meta) where funcs is a list of records sorted by rva:
      {rva,end,size,retail_name,category,tag,name,owner,unit,source,lib,confidence}
    and meta carries text bounds + the unit->source table + FID/symbol maps."""
    if not FUNCS_CSV.is_file():
        raise SystemExit("missing %s - run `gruntz build` first" % FUNCS_CSV)

    universe, universe_meta = classify_function_universe(REPO)
    units = _load_units()

    funcs = []
    for row in universe:
        rva, sz, rname = row["rva"], row["size"], row["retail_name"]
        rec = {"rva": rva, "size": sz, "end": rva + sz, "retail_name": rname,
               "unit": "", "source": "", "lib": "", "confidence": "", "name": rname}

        if row["category"] == "target" and row["claimed"]:
            cat = "unit"
            rec["unit"] = row["unit"]
            rec["source"] = units.get(row["unit"], "")
            rec["name"] = row["source_name"] or rname
        elif row["category"] == "target":
            cat = "unknown"
        elif row["category"] == "thunk":
            cat = "thunk"
        elif row["category"] == "compiler":
            cat = "compiler"
            rec["name"] = row["role"] or rname
        elif row["category"] == "library":
            cat = LIB_CATEGORY.get(row["lib"], "crt")
            rec["lib"] = row["lib"]
            rec["confidence"] = row["confidence"]
            rec["name"] = row["source_name"] or rname
        elif row["category"] == "eh":
            cat = "eh"
        else:
            raise ValueError("unknown function-universe category %r" % row["category"])

        rec["category"] = cat
        rec["tag"] = CATEGORIES[cat][0]
        funcs.append(rec)

    lo, hi = _text_bounds(funcs)
    meta = {"text_lo": lo, "text_hi": hi, "ilt_end": universe_meta["ilt_end"],
            "units": units, "sections": _sections()}
    return funcs, meta


def owner_str(rec):
    """One-column human owner: the source file for a unit, else a compact category word
    (with the FID lib/confidence appended for library-identified rows)."""
    if rec["category"] == "unit":
        return rec["source"] or (rec["unit"] + " (unit)")
    word = OWNER_WORD[rec["category"]]
    if rec["lib"]:
        return "%s · %s/%s" % (word, rec["lib"], rec["confidence"])
    return word


# --------------------------------------------------------------------------- gaps
def compute_gaps(funcs, lo, hi):
    """Uncovered [start,end) holes in .text, in RVA order. Handles overlaps by
    tracking a running max end (Ghidra occasionally emits nested/overlapping fns)."""
    gaps = []
    cursor = lo
    for rec in funcs:
        if rec["rva"] > cursor:
            gaps.append((cursor, rec["rva"]))
        cursor = max(cursor, rec["end"])
    if hi > cursor:
        gaps.append((cursor, hi))
    return gaps


# ------------------------------------------------------------------------- output
def _fmt_size(n):
    return "%d B" % n if n < 1024 else "%.1f KB" % (n / 1024)


def _print_func_rows(recs, gaps=None):
    """Aligned `rva size TAG name owner` rows, optionally interleaving GAP markers."""
    gap_iter = iter(gaps or [])
    next_gap = next(gap_iter, None)

    def gap(g):
        print("  0x%08x  %9s  ---- gap ----" % (g[0], _fmt_size(g[1] - g[0])))
    for rec in recs:
        while next_gap and next_gap[0] < rec["rva"]:
            gap(next_gap)
            next_gap = next(gap_iter, None)
        print("  0x%08x  %9s  %-5s  %-40s  %s"
              % (rec["rva"], _fmt_size(rec["size"]), rec["tag"],
                 rec["name"][:40], owner_str(rec)))
    while next_gap:
        gap(next_gap)
        next_gap = next(gap_iter, None)


def cmd_overview(funcs, meta, args):
    lo, hi = meta["text_lo"], meta["text_hi"]
    total = hi - lo
    by_cat = {}
    for rec in funcs:
        c = by_cat.setdefault(rec["category"], [0, 0])
        c[0] += 1
        c[1] += rec["size"]
    covered = sum(rec["size"] for rec in funcs)
    gaps = compute_gaps(funcs, lo, hi)
    gap_bytes = sum(b - a for a, b in gaps)

    if args.json:
        print(json.dumps({
            "text_lo": lo, "text_hi": hi, "text_bytes": total,
            "functions": len(funcs), "covered_bytes": covered,
            "gap_bytes": gap_bytes, "gap_count": len(gaps),
            "categories": {c: {"functions": v[0], "bytes": v[1]}
                           for c, v in by_cat.items()},
        }, indent=2))
        return

    print("GRUNTZ.EXE .text map")
    print("  range   0x%08x .. 0x%08x   (%s)" % (lo, hi, _fmt_size(total)))
    print("  carved  %d functions, %s covered (%.1f%%)"
          % (len(funcs), _fmt_size(covered), 100 * covered / total))
    print("  gaps    %d holes, %s uncovered (%.1f%%)"
          % (len(gaps), _fmt_size(gap_bytes), 100 * gap_bytes / total))

    if meta["sections"]:
        print("\nSections")
        for nm, s_lo, s_hi, ex in meta["sections"]:
            print("  %-10s 0x%08x .. 0x%08x  %10s  %s"
                  % (nm, s_lo, s_hi, _fmt_size(s_hi - s_lo), "exec" if ex else ""))

    print("\nBy category      functions        bytes   %.text")
    for cat, (tag, note) in CATEGORIES.items():
        n, b = by_cat.get(cat, [0, 0])
        if not n:
            continue
        print("  %-6s %9d  %11s   %5.1f%%   %s"
              % (tag, n, _fmt_size(b), 100 * b / total, note))

    biggest = sorted(gaps, key=lambda g: g[1] - g[0], reverse=True)[:args.top]
    if biggest:
        print("\nLargest gaps")
        for a, b in biggest:
            print("  0x%08x .. 0x%08x   %s" % (a, b, _fmt_size(b - a)))

    print("\nQuery: python -m gruntz.core.exe_map range 0x%08x 0x%08x   (or `gaps`, "
          "`units`, `at <rva>`, `find <re>`; add --json)" % (lo, min(lo + 0x1000, hi)))


def _parse_range(tokens):
    """Accept `START END`, `START-END`, or `START +LEN` (hex or dec)."""
    if len(tokens) == 1:
        if "-" not in tokens[0]:
            raise SystemExit("range needs START END, START-END, or START +LEN")
        a, b = tokens[0].split("-", 1)
        return _rint(a), _rint(b)
    if len(tokens) == 2 and tokens[1].startswith("+"):
        s = _rint(tokens[0])
        return s, s + _rint(tokens[1][1:])
    if len(tokens) == 2:
        return _rint(tokens[0]), _rint(tokens[1])
    raise SystemExit("range needs START END, START-END, or START +LEN")


def cmd_range(funcs, meta, args):
    lo, hi = _parse_range(args.window)
    sel = [r for r in funcs if r["rva"] < hi and r["end"] > lo]
    gaps = [g for g in compute_gaps(funcs, meta["text_lo"], meta["text_hi"])
            if g[0] < hi and g[1] > lo]
    if args.json:
        print(json.dumps({"lo": lo, "hi": hi,
                          "functions": [_json_rec(r) for r in sel],
                          "gaps": [{"start": a, "end": b} for a, b in gaps]}, indent=2))
        return
    print("RVA 0x%08x .. 0x%08x   %d functions, %d gaps" % (lo, hi, len(sel), len(gaps)))
    _print_func_rows(sel, gaps)


def _file_match(rec, q):
    """True if a UNIT-owned function belongs to the queried file/unit (matches the
    unit stem exactly, the source basename exactly, or the source path as a substring
    so `Net` / `src/Net` scope a whole directory)."""
    if rec["category"] != "unit":
        return False
    ql = q.lower()
    src = rec["source"].lower()
    return ql == rec["unit"].lower() or ql == src.rsplit("/", 1)[-1] or ql in src


def cmd_file(funcs, meta, args):
    q = args.file
    owned = sorted((r for r in funcs if _file_match(r, q)), key=lambda r: r["rva"])
    if not owned:
        near = sorted({r["source"] for r in funcs
                       if r["category"] == "unit" and q.lower() in r["source"].lower()})
        if args.json:
            print(json.dumps({"query": q, "owned_functions": 0,
                              "did_you_mean": near[:10]}, indent=2))
            return
        print("no reconstructed src file matches %r" % q)
        if near:
            print("did you mean:")
            for s in near[:10]:
                print("  " + s)
        else:
            print("(try a source basename like GruntzMgr.cpp, a unit stem, or a dir "
                  "like src/Net; `units` lists every owner)")
        return

    lo = min(r["rva"] for r in owned)
    hi = max(r["end"] for r in owned)
    own_bytes = sum(r["size"] for r in owned)
    owned_rvas = {r["rva"] for r in owned}
    # Everything else carved into this file's RVA span - the functions that sit in the
    # GAPS between this file's functions (how interleaved / scattered the file is).
    others = [r for r in funcs
              if r["rva"] < hi and r["end"] > lo and r["rva"] not in owned_rvas]
    span_gaps = [g for g in compute_gaps(funcs, meta["text_lo"], meta["text_hi"])
                 if g[0] < hi and g[1] > lo]
    uncovered = sum(min(g[1], hi) - max(g[0], lo) for g in span_gaps)
    ocat = {}  # other-owner functions in the span, by category
    for r in others:
        c = ocat.setdefault(r["category"], [0, 0])
        c[0] += 1
        c[1] += r["size"]
    files = sorted({r["source"] or r["unit"] for r in owned})

    if args.json:
        print(json.dumps({
            "query": q, "files": files,
            "owned_functions": len(owned), "owned_bytes": own_bytes,
            "span_lo": lo, "span_hi": hi, "span_bytes": hi - lo,
            "gap_functions": len(others),
            "gap_function_bytes": sum(v[1] for v in ocat.values()),
            "gap_uncovered_bytes": uncovered,
            "gap_by_category": {c: {"functions": v[0], "bytes": v[1]}
                                for c, v in ocat.items()},
            "functions": [_json_rec(r) for r in owned],
        }, indent=2))
        return

    print("File: %s" % (files[0] if len(files) == 1 else "%d files matching %r" % (len(files), q)))
    for f in files[1:]:
        print("   + %s" % f)
    print("  owns %d functions, %s" % (len(owned), _fmt_size(own_bytes)))
    print("  span 0x%08x .. 0x%08x  (%s)" % (lo, hi, _fmt_size(hi - lo)))
    print("  in the gaps between them: %d functions from other owners (%s) + %s uncovered"
          % (len(others), _fmt_size(sum(v[1] for v in ocat.values())), _fmt_size(uncovered)))
    for cat, (n, b) in sorted(ocat.items(), key=lambda kv: -kv[1][1]):
        print("      %-6s %6d fns  %s" % (CATEGORIES[cat][0], n, _fmt_size(b)))

    if args.gaps:
        # The file's own functions in RVA order (named anchors); between each
        # consecutive PAIR, the gap that separates them - so every gap reads as
        # "between the function above and the function below". The gap line only
        # SUMMARIZES the space (size + how many foreign fns of what kind interleave),
        # it does not name the occupants.
        print("\nthe file's functions in RVA order; between each consecutive pair, the gap\n"
              "that separates them (size + how many foreign functions interleave):\n")
        prev_end = None
        for r in owned:
            if prev_end is not None and r["rva"] > prev_end:
                seg = [o for o in others if o["rva"] < r["rva"] and o["end"] > prev_end]
                region = r["rva"] - prev_end
                if not seg:
                    # Pure padding between two of the file's functions - they are
                    # effectively adjacent; hide the noise unless --verbose.
                    if args.verbose:
                        print("        gap %s (uncovered)" % _fmt_size(region))
                else:
                    unc = region - sum(o["size"] for o in seg)
                    cats = {}
                    for o in seg:
                        cats[o["category"]] = cats.get(o["category"], 0) + 1
                    catstr = ", ".join("%d %s" % (v, CATEGORIES[k][0])
                                       for k, v in sorted(cats.items(), key=lambda kv: -kv[1]))
                    unc_s = ", +%s uncovered" % _fmt_size(unc) if unc >= 16 else ""
                    print("        gap %s (%d foreign fns: %s%s)"
                          % (_fmt_size(region), len(seg), catstr, unc_s))
            print("  0x%08x  %9s  %-5s  %s"
                  % (r["rva"], _fmt_size(r["size"]), r["tag"], r["name"]))
            prev_end = max(prev_end or 0, r["end"])
    else:
        print()
        _print_func_rows(owned)


def cmd_at(funcs, meta, args):
    rva = _rint(args.rva)
    hit = [r for r in funcs if r["rva"] <= rva < r["end"]]
    if args.json:
        print(json.dumps([_json_rec(r) for r in hit], indent=2))
        return
    if not hit:
        gaps = compute_gaps(funcs, meta["text_lo"], meta["text_hi"])
        g = next((g for g in gaps if g[0] <= rva < g[1]), None)
        if g:
            print("0x%08x is in a GAP  0x%08x .. 0x%08x  (%s)"
                  % (rva, g[0], g[1], _fmt_size(g[1] - g[0])))
        else:
            print("0x%08x: no function or gap (outside .text?)" % rva)
        return
    _print_func_rows(hit)


def cmd_gaps(funcs, meta, args):
    gaps = compute_gaps(funcs, meta["text_lo"], meta["text_hi"])
    gaps = [g for g in gaps if g[1] - g[0] >= args.min]
    if args.sort:
        gaps.sort(key=lambda g: g[1] - g[0], reverse=True)
    if args.top:
        gaps = gaps[:args.top]
    if args.json:
        print(json.dumps([{"start": a, "end": b, "size": b - a} for a, b in gaps],
                         indent=2))
        return
    print("%d gaps >= %d B" % (len(gaps), args.min))
    for a, b in gaps:
        print("  0x%08x .. 0x%08x   %s" % (a, b, _fmt_size(b - a)))


def cmd_units(funcs, meta, args):
    agg = {}  # key -> [funcs, bytes]
    for rec in funcs:
        if rec["category"] == "unit":
            key = rec["source"] or rec["unit"]
        else:
            key = "<%s>" % rec["category"]
        a = agg.setdefault(key, [0, 0])
        a[0] += 1
        a[1] += rec["size"]
    ordered = sorted(agg.items(), key=lambda kv: kv[1][1], reverse=True)
    if args.top:
        ordered = ordered[:args.top]
    if args.json:
        print(json.dumps([{"owner": k, "functions": v[0], "bytes": v[1]}
                          for k, v in ordered], indent=2))
        return
    print("%-46s %8s %11s" % ("owner", "funcs", "bytes"))
    for k, v in ordered:
        print("%-46s %8d %11s" % (k[:46], v[0], _fmt_size(v[1])))


def cmd_find(funcs, meta, args):
    rx = re.compile(args.pattern, re.I)
    sel = [r for r in funcs if rx.search(r["name"]) or rx.search(r["retail_name"])]
    if args.json:
        print(json.dumps([_json_rec(r) for r in sel], indent=2))
        return
    print("%d functions match /%s/" % (len(sel), args.pattern))
    _print_func_rows(sel)


def _json_rec(r):
    return {k: r[k] for k in ("rva", "size", "end", "category", "name",
                              "retail_name", "unit", "source", "lib", "confidence")}


def main(argv=None):
    try:  # die silently on a closed pipe (`| head`) instead of a teardown traceback
        import signal
        signal.signal(signal.SIGPIPE, signal.SIG_DFL)
    except (ImportError, AttributeError, ValueError):
        pass
    # --json on a shared parent so it is accepted in ANY position (before OR after the
    # subcommand): `exe_map --json range ...` and `exe_map range ... --json` both work.
    common = argparse.ArgumentParser(add_help=False)
    common.add_argument("--json", action="store_true", help="machine-readable output")

    p = argparse.ArgumentParser(prog="gruntz.core.exe_map",
                                description=__doc__.splitlines()[0], parents=[common])
    sub = p.add_subparsers(dest="cmd")

    sp = sub.add_parser("overview", parents=[common], help="whole-binary summary (default)")
    sp.add_argument("--top", type=int, default=15, help="how many largest gaps to show")

    sp = sub.add_parser("range", parents=[common], help="functions in an RVA window")
    sp.add_argument("window", nargs="+", help="START END | START-END | START +LEN")

    sp = sub.add_parser("file", parents=[common],
                        help="all functions a src file owns + how many functions sit "
                             "in the gaps between them")
    sp.add_argument("file", help="source basename (GruntzMgr.cpp), unit stem, or dir (src/Net)")
    sp.add_argument("--gaps", action="store_true",
                    help="list the file's functions in order with the gap that "
                         "separates each consecutive pair (which two functions each "
                         "gap is between); empty padding gaps are hidden unless -v")
    sp.add_argument("-v", "--verbose", action="store_true",
                    help="with --gaps: also show empty (pure-padding) gaps")

    sp = sub.add_parser("at", parents=[common], help="the function covering one RVA")
    sp.add_argument("rva")

    sp = sub.add_parser("gaps", parents=[common], help="uncovered holes in .text")
    sp.add_argument("--min", type=lambda s: _rint(s), default=1, help="minimum gap size")
    sp.add_argument("--top", type=int, default=0, help="cap to N gaps")
    sp.add_argument("--sort", action="store_true", help="order by size (default: by RVA)")

    sp = sub.add_parser("units", parents=[common], help="per-source-file / per-category breakdown")
    sp.add_argument("--top", type=int, default=0, help="cap to N owners")

    sp = sub.add_parser("find", parents=[common], help="functions whose name matches a regex")
    sp.add_argument("pattern")

    args = p.parse_args(argv)
    funcs, meta = load()
    dispatch = {"overview": cmd_overview, "range": cmd_range, "file": cmd_file,
                "at": cmd_at, "gaps": cmd_gaps, "units": cmd_units, "find": cmd_find}
    if args.cmd is None:
        args.cmd, args.top = "overview", 15
    dispatch[args.cmd](funcs, meta, args)


if __name__ == "__main__":
    import os
    try:
        main()
    except BrokenPipeError:  # piping into `head`/`less` closes stdout early
        os.dup2(os.open(os.devnull, os.O_WRONLY), sys.stdout.fileno())
        sys.exit(0)
