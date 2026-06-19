#!/usr/bin/env python3
"""harvest_locals.py - per-RVA local-variable names for the Ghidra enrichment.

Compiles each src/ TU a SECOND time with `/Z7` (codegen-neutral CodeView debug
info; the .text bytes are byte-identical to the matching base obj, verified), then
reads the named stack/register locals from the resulting object via codeview.py
and joins each procedure's DEMANGLED qualified name to the RVA that
build/gen/functions.json already derived for it. Writes build/gen/locals.json:

    [{rva, name, locals:[{name, kind:"stack"|"reg", offset|reg, type}]}]

apply.py injects these as named Ghidra stack/register variables (they surface in
the on-demand decompiler exactly like the function prototypes do). This is kept
OUT of the matching build hot-path: locals only feed the Ghidra DB, which is
refreshed occasionally, so the second /Z7 compile runs in `gruntz ghidra-refresh`
(cached in build/debug/, recompiled only when the source is newer).

The matching base objs stay PRISTINE (no /Z7): /Z7 is .text-identical but not
symbol-table-identical (it injects .bf/.ef debug symbols and drops C-static
symbols), which would break labels.py's nm authority check - so the debug objs
are a separate, parallel artifact used only here.

Degrades gracefully: if wine/cl is unavailable (default dev shell), or a TU fails
to compile, that unit is skipped with a warning and apply.py simply finds no
locals for it.
"""

import argparse
import json
import os
import subprocess
import sys
import tomllib
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
REPO = next((p for p in SCRIPT_DIR.parents if (p / "flake.nix").exists()), SCRIPT_DIR)
CC_WRAP = SCRIPT_DIR / "cc_wrap.py"

sys.path.insert(0, str(SCRIPT_DIR))
import codeview  # noqa: E402


def log(msg):
    print(f"[harvest_locals] {msg}", file=sys.stderr)


def load_units(toml_path):
    """[(unit, source, [flags])] for src/ TUs only (skip vendored C)."""
    with open(toml_path, "rb") as f:
        data = tomllib.load(f)
    profiles = data.get("flags", {})
    out = []
    for u in data.get("unit", []):
        src = u.get("source", "")
        if not src.startswith("src/"):
            continue  # vendored zlib: C statics, no source-level locals to name
        flags = profiles.get(u.get("flags", ""), [])
        out.append((u["unit"], src, list(flags)))
    return out


def matched_rvas(symnames_path, report_path):
    """Set of rva-hex strings whose function is byte-EXACT (fuzzy==100) in objdiff.

    Locals describe the /Z7 compilation's frame layout, which equals the retail
    function's ONLY when the code is byte-exact; for WIP functions the offsets
    would be wrong, so we inject locals for matched functions only. Returns None
    when no report exists yet (caller decides whether to proceed unfiltered).
    """
    import csv
    if not (Path(symnames_path).exists() and Path(report_path).exists()):
        return None
    sym = {}   # rva-hex -> (mangled, unit)
    with open(symnames_path) as f:
        for row in csv.DictReader([l for l in f if not l.lstrip().startswith("#")]):
            if (row.get("kind") or "func").strip() == "data":
                continue
            rva = (row.get("rva") or "").strip()
            if rva:
                sym[rva] = ((row.get("name") or "").strip(),
                            (row.get("unit") or "").strip())
    rep = json.loads(Path(report_path).read_text())
    matched = set()
    for u in rep.get("units", []):
        un = u.get("name", "")
        for fn in u.get("functions", []) or []:
            if float(fn.get("fuzzy_match_percent") or 0.0) >= 99.995:
                matched.add((un, fn.get("name")))
    return {rva for rva, (m, u) in sym.items() if (u, m) in matched}


def compile_debug_obj(unit, source, flags, debug_dir):
    """cl <flags> /Z7 -> build/debug/<unit>.obj (cached on source mtime). Path or None."""
    obj = debug_dir / f"{unit}.obj"
    src = REPO / source
    if not src.exists():
        log(f"WARN {unit}: source missing {source}")
        return None
    if obj.exists() and obj.stat().st_mtime >= src.stat().st_mtime:
        return obj  # up to date
    debug_dir.mkdir(parents=True, exist_ok=True)
    cmd = [sys.executable, str(CC_WRAP), "--out", str(obj), "--src", str(src),
           "--", *flags, "/Z7"]
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0 or not obj.exists():
        log(f"WARN {unit}: /Z7 compile failed (wine/cl missing?) - skipping\n"
            f"      {res.stderr.strip()[-200:]}")
        return None
    return obj


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--units-toml", default=str(REPO / "config/units.toml"))
    ap.add_argument("--functions", default=str(REPO / "build/gen/functions.json"),
                    help="rva<-qualified-name join source (labels.py output).")
    ap.add_argument("--debug-dir", default=str(REPO / "build/debug"))
    ap.add_argument("--symbol-names", default=str(REPO / "build/gen/symbol_names.csv"))
    ap.add_argument("--report", default=str(REPO / "build/objdiff/report.json"),
                    help="objdiff report.json; locals are kept only for byte-exact "
                         "(100%%) functions (their /Z7 frame layout == retail).")
    ap.add_argument("--out", default=str(REPO / "build/gen/locals.json"))
    args = ap.parse_args()

    if not Path(args.functions).exists():
        log(f"ERROR no functions.json at {args.functions} - run `gruntz build` first")
        return 1
    funcs = json.loads(Path(args.functions).read_text())
    keep_rvas = matched_rvas(args.symbol_names, args.report)
    if keep_rvas is None:
        log("WARN no objdiff report - emitting locals UNFILTERED (offsets may be "
            "wrong for not-yet-matched functions); build first for the 100% filter")
    else:
        log(f"100%-match filter: {len(keep_rvas)} byte-exact function(s)")
    # (unit, qualified-name) -> rva ; the unit disambiguates same-named methods.
    qual_to_rva = {}
    for d in funcs:
        qual_to_rva.setdefault((d.get("unit"), d.get("name")), d["rva"])

    units = load_units(args.units_toml)
    debug_dir = Path(args.debug_dir)
    out = {}                  # rva -> {name, locals}
    n_units = n_skip = n_locals = 0
    miss = 0
    for unit, source, flags in units:
        obj = compile_debug_obj(unit, source, flags, debug_dir)
        if obj is None:
            n_skip += 1
            continue
        n_units += 1
        for qual, locs in codeview.parse_locals(str(obj)).items():
            rva = qual_to_rva.get((unit, qual))
            if rva is None:
                if locs:
                    miss += 1
                continue
            if keep_rvas is not None and rva not in keep_rvas:
                continue  # only inject locals for byte-exact functions
            # Keep the true LOCALS - negative-offset stack slots + enregistered
            # locals - and drop: `this` (the prototype types it), positive-offset
            # stack slots (those are PARAMETERS, named by the prototype), and
            # unnamed temporaries. Dedup by storage: a slot reused across lexical
            # blocks emits multiple records, but a Ghidra frame holds one variable
            # per offset, so keep the first name per storage.
            keep = []
            seen = set()
            for l in locs:
                if not l.name or l.name == "this":
                    continue
                if l.kind == "stack":
                    if l.offset is None or l.offset >= 0:
                        continue
                    key = ("stack", l.offset)
                    if key in seen:
                        continue
                    seen.add(key)
                    keep.append({"name": l.name, "kind": "stack",
                                 "offset": l.offset, "type": l.type_index})
                else:  # enregistered local
                    key = ("reg", l.reg)
                    if key in seen:
                        continue
                    seen.add(key)
                    keep.append({"name": l.name, "kind": "reg", "reg": l.reg,
                                 "type": l.type_index})
            if keep:
                out[rva] = {"name": qual, "locals": keep}
                n_locals += len(keep)

    rows = [{"rva": rva, **v} for rva, v in sorted(out.items())]
    outp = Path(args.out)
    outp.parent.mkdir(parents=True, exist_ok=True)
    outp.write_text(json.dumps(rows, indent=1))
    log(f"compiled {n_units} debug obj(s) ({n_skip} skipped), wrote "
        f"{len(rows)} function(s) / {n_locals} local(s) -> {outp}"
        + (f"  ({miss} proc(s) had locals but no rva match)" if miss else ""))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
