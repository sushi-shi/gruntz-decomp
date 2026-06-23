#!/usr/bin/env python3
"""Append SIZE(Class, N) for the 24 reliable (clean, single-size) classes that
already have a src home but no SIZE yet. These are graduated (real layouts) so
SIZE is genuine validation; placed at end of the file where the ctor is defined
(class is complete there). CGruntzMgr's real layout lives in GruntzMgr.cpp, not
its stub - override accordingly.  Dry-run unless --write."""
import csv, re, sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
SRC = REPO / "src"
NEW = REPO / "ctor-survey/new_sites.csv"
WRITE = "--write" in sys.argv
OVERRIDE = {"CGruntzMgr": SRC / "Gruntz/GruntzMgr.cpp"}  # real layout, not the stub

rva_class, rp, dp = {}, re.compile(r"RVAU?\(\s*0x([0-9a-fA-F]+)"), re.compile(r"\b([A-Za-z_]\w*)\s*::\s*~?\w+\s*\(")
for p in list(SRC.rglob("*.cpp")) + list(SRC.rglob("*.h")):
    L = p.read_text(errors="ignore").splitlines()
    for i, ln in enumerate(L):
        m = rp.search(ln)
        if m:
            for j in range(i, min(i + 4, len(L))):
                d = dp.search(L[j])
                if d: rva_class[int(m.group(1), 16)] = (d.group(1), p); break

have_size = {m.group(1) for p in SRC.rglob("*.cpp")
             for m in re.finditer(r"SIZE\(\s*([A-Za-z_]\w*)\s*,", p.read_text(errors="ignore"))}

by_class = {}
for r in csv.DictReader(open(NEW)):
    if r["after"] == "ctor" and r["size_kind"] == "const" and r["ctor_rva"]:
        c = int(r["ctor_rva"], 16)
        if c in rva_class:
            cls, path = rva_class[c]
            by_class.setdefault(cls, {"sizes": set(), "path": path})["sizes"].add(int(r["sizeof"], 16))

targets = {c: (next(iter(v["sizes"])), OVERRIDE.get(c, v["path"]))
           for c, v in by_class.items() if len(v["sizes"]) == 1 and c not in have_size}

done = []
for cls in sorted(targets):
    size, path = targets[cls]
    txt = path.read_text()
    if re.search(r"SIZE\(\s*" + re.escape(cls) + r"\s*,", txt):
        continue
    add = f"\n// size 0x{size:x} recovered from operator-new sites (gruntz.analysis.news)\nSIZE({cls}, 0x{size:x});\n"
    if WRITE:
        path.write_text((txt if txt.endswith("\n") else txt + "\n") + add)
    done.append((cls, size, path.relative_to(REPO)))

for cls, size, f in done:
    print(f"{'WROTE' if WRITE else 'WOULD':6} SIZE({cls}, 0x{size:x})  -> {f}")
print(f"\n{len(done)} SIZE() {'appended' if WRITE else 'to append'}.")
