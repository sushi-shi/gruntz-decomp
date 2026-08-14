#!/usr/bin/env python3
"""Find pure-phantom classes that are cleanly foldable: every declared-only method
resolves (through ILT thunks) to a REAL reconstructed function on ONE real class.
Groups by source file so a nest folds together. Prints class -> {real class: n}."""
import re, csv
from pathlib import Path
from collections import defaultdict
from gruntz.core import vtable_scan as vs
from gruntz.cleanliness.view_debt import pure_phantom_classes, LIBRARY_CLASSES
REPO = next(p for p in Path(__file__).resolve().parents if (p/"flake.nix").exists())
gen = {}
for r in csv.DictReader(open(REPO/"build/gen/symbol_names.csv")):
    try: gen[int(r["rva"],16)] = r["name"]
    except: pass
def real_of(rva):
    b = vs.chase_thunk(rva) or rva
    n = gen.get(b)
    if not n: return None
    m = re.match(r"\?[^@]+@([A-Za-z_]\w*)@", n)
    return m.group(1) if m else None
# index every struct/class def once: name -> (file, [method rvas])
idx = {}
for f in list(REPO.rglob("src/**/*.cpp")) + list(REPO.rglob("include/**/*.h")):
    t = f.read_text(errors="ignore")
    for m in re.finditer(r'\b(?:struct|class)\s+([A-Za-z_]\w*)\b[^{;]*\{(.*?)\n\};', t, re.S):
        name, body = m.group(1), m.group(2)
        rvas = [int(x,16) for x in re.findall(r'\)\s*(?:const)?\s*;\s*//[^\n]*?0x([0-9a-f]{3,7})', body)]
        rvas += [int(x,16) for x in re.findall(r'\b(?:Fn|Method_|H)([0-9a-f]{3,7})\s*\(', body)]
        if rvas and name not in idx: idx[name] = (f.relative_to(REPO).as_posix(), rvas)
ph = pure_phantom_classes()
bysrc = defaultdict(list)
for cls in ph:
    if cls not in idx: continue
    src, rvas = idx[cls]
    tg = defaultdict(int); ok = True
    for rv in rvas:
        rc = real_of(rv)
        if rc and rc != cls and rc not in LIBRARY_CLASSES: tg[rc]+=1
        elif rc in LIBRARY_CLASSES: tg["<lib>"]+=1
        else: ok=False
    if ok and tg: bysrc[src].append((cls, len(rvas), dict(tg)))
tot = sum(len(v) for v in bysrc.values())
print(f"cleanly-foldable phantom classes: {tot}  across {len(bysrc)} file(s)\n")
for src in sorted(bysrc, key=lambda s:-len(bysrc[s])):
    print(f"{src}  ({len(bysrc[src])} class(es))")
    for cls,n,tg in sorted(bysrc[src], key=lambda x:-x[1]):
        print(f"    {cls:24} ({n}) -> {tg}")
