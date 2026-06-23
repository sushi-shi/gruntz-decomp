#!/usr/bin/env python3
"""For ctors with a size but no RVA()-matched src class: what class does
ctor_candidates name them, and is that class actually declared in src/?"""
import csv, re
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
SRC = REPO / "src"
NEW = REPO / "ctor-survey/new_sites.csv"
CTORS = REPO / "ctor-survey/ctor_candidates.csv"

# RVA()-annotated ctor -> class (the strict map used before)
rva_class, rp, dp = {}, re.compile(r"RVAU?\(\s*0x([0-9a-fA-F]+)"), re.compile(r"\b([A-Za-z_]\w*)\s*::\s*~?\w+\s*\(")
for p in list(SRC.rglob("*.cpp")) + list(SRC.rglob("*.h")):
    L = p.read_text(errors="ignore").splitlines()
    for i, ln in enumerate(L):
        m = rp.search(ln)
        if m:
            for j in range(i, min(i + 6, len(L))):
                d = dp.search(L[j])
                if d: rva_class[int(m.group(1), 16)] = d.group(1); break

# ctor rva -> class label from ctors.py
cand = {int(r["rva"], 16): r["class"] for r in csv.DictReader(open(CTORS))}

# which class names are declared anywhere in src, and which have a Stub file
declared = set()
for p in SRC.rglob("*"):
    if p.suffix in (".cpp", ".h"):
        for m in re.finditer(r"\bclass\s+([A-Za-z_]\w*)\b", p.read_text(errors="ignore")):
            declared.add(m.group(1))

no_src = []
seen = set()
for r in csv.DictReader(open(NEW)):
    if r["after"] != "ctor" or r["size_kind"] != "const" or not r["ctor_rva"]:
        continue
    c = int(r["ctor_rva"], 16)
    if c in rva_class or c in seen:
        continue
    seen.add(c)
    label = cand.get(c, "?")
    leaves = [x for x in label.split("+")]
    decl = [x for x in leaves if x in declared]
    no_src.append((c, int(r["sizeof"], 16), label, decl))

print(f"{len(no_src)} ctors with size but not RVA-matched to a src class:\n")
print(f"{'ctor_rva':10} {'size':>7}  candidate-class label            declared-in-src?")
for c, sz, label, decl in sorted(no_src):
    print(f"0x{c:06x}   0x{sz:<5x}  {label[:32]:32} {decl if decl else 'NO'}")
