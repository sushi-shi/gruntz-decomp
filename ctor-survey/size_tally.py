#!/usr/bin/env python3
"""Tally how many class sizes we know, and how many are still unset."""
import csv, re
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
SRC = REPO / "src"
NEW = REPO / "ctor-survey/new_sites.csv"

# rva -> (class, path) for RVA()-annotated defs in src
rva_class, rva_pat, def_pat = {}, re.compile(r"RVAU?\(\s*0x([0-9a-fA-F]+)"), re.compile(r"\b([A-Za-z_]\w*)\s*::\s*~?\w+\s*\(")
for p in list(SRC.rglob("*.cpp")) + list(SRC.rglob("*.h")):
    lines = p.read_text(errors="ignore").splitlines()
    for i, ln in enumerate(lines):
        m = rva_pat.search(ln)
        if m:
            for j in range(i, min(i + 4, len(lines))):
                d = def_pat.search(lines[j])
                if d: rva_class[int(m.group(1), 16)] = (d.group(1), p); break

# which classes already carry a SIZE() in src
have_size = set()
for p in SRC.rglob("*.cpp"):
    for m in re.finditer(r"SIZE\(\s*([A-Za-z_]\w*)\s*,", p.read_text(errors="ignore")):
        have_size.add(m.group(1))

ctor_sites = [r for r in csv.DictReader(open(NEW)) if r["after"] == "ctor" and r["size_kind"] == "const" and r["ctor_rva"]]
in_src, no_src = {}, set()
for r in ctor_sites:
    c = int(r["ctor_rva"], 16)
    if c in rva_class:
        cls = rva_class[c][0]
        in_src.setdefault(cls, set()).add(int(r["sizeof"], 16))
    else:
        no_src.add(c)

clean = {c: s for c, s in in_src.items() if len(s) == 1}
ambig = {c: s for c, s in in_src.items() if len(s) > 1}
clean_set = {c for c in clean if c in have_size}
clean_unset = {c for c in clean if c not in have_size}

print(f"ctor-confirmed const-size new-sites          : {len(ctor_sites)}")
print(f"distinct classes with a size + src home       : {len(in_src)}")
print(f"  clean (single size)                          : {len(clean)}")
print(f"     already SIZE()-set                         : {len(clean_set)}")
print(f"     still UNSET (reliable, not yet stamped)    : {len(clean_unset)}")
print(f"  ambiguous (multi-size, base-leak - unreliable): {len(ambig)}  {sorted(ambig)}")
print(f"distinct ctors with a size but NO src class yet : {len(no_src)}  (sizes known, nowhere to put SIZE)")
print()
print("clean & still unset (the reliable 'more we know'):")
for c in sorted(clean_unset):
    print(f"  {c:30} 0x{next(iter(clean[c])):x}")
