#!/usr/bin/env python3
"""evidence.py - per-SITE evidence dump for declared-only rows.

For every baseline symbol: every base reloc site, its enclosing fn, the fn's
fuzzy%, the LCS-aligned retail target (v3 method), that target's csv name, and
the 12 retail bytes at the target. One line per site, grouped per symbol.
"""
import json
import subprocess
import sys
from collections import defaultdict
from pathlib import Path



from gruntz.audit.assert_relocs import (REPO, _D, _fo, target_at, load_symbols, resolve)
from gruntz.cleanliness.view_debt import _current_objs
from gruntz.audit.declared_v3 import fn_relocs, align, REL_REL32

TEXT = (0x1000, 0x1E626B)
BASELINE = Path(REPO) / "config/declared-only-baseline.tsv"
TARGET_DIR = Path(REPO) / "build/delink/named"


def main():
    targets = {ln.split("\t")[0].strip()
               for ln in BASELINE.read_text().splitlines()
               if ln.strip() and not ln.startswith("#")}
    sym, data, dups = load_symbols()
    import csv as _csv
    rva2name = {}
    for r in _csv.reader(open(Path(REPO) / "build/gen/symbol_names.csv", encoding="latin-1")):
        if len(r) >= 2:
            try:
                v = int(r[0], 16)
            except ValueError:
                continue
            if v not in rva2name or (r[1].startswith("?") and not rva2name[v].startswith("?")):
                rva2name[v] = r[1]
    fuzzy = {}
    rep = json.load(open(Path(REPO) / "build/objdiff/report.json"))
    for u in rep["units"]:
        for f in u.get("functions", []):
            fuzzy[f["name"]] = f.get("fuzzy_match_percent", 0.0)

    rows = defaultdict(list)
    tgt_cache = {}
    for o in _current_objs():
        try:
            fns = fn_relocs(o)
        except Exception:
            continue
        hits = {f: rl for f, rl in fns.items() if any(r[2] in targets for r in rl[1])}
        if not hits:
            continue
        stem = Path(o).stem
        tpath = TARGET_DIR / f"{stem}.c.obj"
        if stem not in tgt_cache:
            try:
                tgt_cache[stem] = fn_relocs(str(tpath)) if tpath.exists() else {}
            except Exception:
                tgt_cache[stem] = {}
        tfns = tgt_cache[stem]
        for fname, (foff, rl) in hits.items():
            frva = sym.get(fname)
            fz = fuzzy.get(fname)
            if frva is None or fname not in tfns:
                for off, ty, nm in rl:
                    if nm in targets:
                        rows[nm].append((stem, fname, fz, None, None))
                continue
            tfoff, trl = tfns[fname]
            base_vals = []
            for off, ty, nm in rl:
                typ = "REL32" if ty == REL_REL32 else "DIR32"
                base_vals.append(None if nm in targets else resolve(sym, data, typ, nm, 0))
            tgt_vals = [target_at(frva + (off - tfoff), "REL32" if ty == REL_REL32 else "DIR32")
                        for off, ty, nm in trl]
            mapping = align(base_vals, tgt_vals)
            exact = sum(1 for k, j in enumerate(mapping)
                        if j is not None and base_vals[k] is not None and base_vals[k] == tgt_vals[j])
            resolvable = sum(1 for v in base_vals if v is not None)
            for k, (off, ty, nm) in enumerate(rl):
                if nm not in targets:
                    continue
                j = mapping[k]
                t = tgt_vals[j] if j is not None else None
                rows[nm].append((stem, fname, fz, t, f"{exact}/{resolvable}anchors"))
    # also vtable/data-section sites from the trusted classifier pass
    for t in sorted(targets):
        print(f"== {t}")
        for (stem, fname, fz, tgt, anch) in rows.get(t, []):
            if tgt is None:
                print(f"   site {stem}:{fname} fuzzy={fz} -> UNALIGNED")
                continue
            name = rva2name.get(tgt, "")
            b12 = _D[_fo(tgt):_fo(tgt) + 12].hex() if _D is not None and TEXT[0] <= tgt < 0x260000 else ""
            kind = "DATA" if not (TEXT[0] <= tgt < TEXT[1]) else "text"
            print(f"   site {stem}:{fname} fuzzy={fz} -> 0x{tgt:06x} [{kind}] {name} {b12} ({anch})")
        if not rows.get(t):
            print("   (no aligned sites - vtable-only or nosite; see classified.tsv)")


if __name__ == "__main__":
    main()
