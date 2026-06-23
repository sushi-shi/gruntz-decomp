#!/usr/bin/env python3
"""Propose SIZE() additions: map each new-site's ctor_rva -> the src class that
declares that RVA (the true leaf), then attach the new-site push size. Scoped to
classes that already exist in src/ (only there can a SIZE() be placed)."""
import csv, re
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
SRC = REPO / "src"
NEW = REPO / "ctor-survey/new_sites.csv"

# rva (int) -> (class, file) for every RVA()/RVAU() annotated def in src/
rva_class = {}
rva_pat = re.compile(r"RVAU?\(\s*0x([0-9a-fA-F]+)")
def_pat = re.compile(r"\b([A-Za-z_]\w*)\s*::\s*~?\w+\s*\(")
for p in list(SRC.rglob("*.cpp")) + list(SRC.rglob("*.h")):
    txt = p.read_text(errors="ignore")
    lines = txt.splitlines()
    for i, ln in enumerate(lines):
        m = rva_pat.search(ln)
        if not m:
            continue
        rva = int(m.group(1), 16)
        # the class is the X:: of the def on this or the next few lines
        for j in range(i, min(i + 4, len(lines))):
            d = def_pat.search(lines[j])
            if d:
                rva_class[rva] = (d.group(1), p.relative_to(REPO))
                break

# new-sites: ctor_rva -> sizes (only ctor-confirmed, const size)
by_class = {}   # class -> {size: count}, file
for r in csv.DictReader(open(NEW)):
    if r["after"] != "ctor" or r["size_kind"] != "const" or not r["ctor_rva"]:
        continue
    ctor = int(r["ctor_rva"], 16)
    if ctor not in rva_class:
        continue                      # ctor not declared in src -> no class to size
    cls, f = rva_class[ctor]
    size = int(r["sizeof"], 16)
    e = by_class.setdefault(cls, {"sizes": {}, "file": f})
    e["sizes"][size] = e["sizes"].get(size, 0) + 1

print(f"{'class':32} {'size':>8}  sites  file (ambiguous?)")
clean, ambiguous = [], []
for cls in sorted(by_class):
    szs = by_class[cls]["sizes"]; f = by_class[cls]["file"]
    main = max(szs, key=szs.get)
    tag = "" if len(szs) == 1 else f"  AMBIG {sorted((hex(s),c) for s,c in szs.items())}"
    print(f"{cls:32} 0x{main:<6x} {szs[main]:5d}  {f}{tag}")
    (clean if len(szs) == 1 else ambiguous).append(cls)
print(f"\n{len(clean)} clean (single size), {len(ambiguous)} ambiguous (multi-size).")
print("ambiguous:", ", ".join(ambiguous))
