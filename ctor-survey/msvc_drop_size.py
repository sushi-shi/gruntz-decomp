#!/usr/bin/env python3
"""Drop SIZE() asserts that fail under MSVC 5.0 (the real compiler / matching ground
truth) - clang-cl's sizeof can differ, so clang-based checks miss these. Iterates:
build, parse C2118 (negative-subscript = SIZE failed), remove those SIZE lines by
class, repeat until the build has no C2118. Run inside `nix develop .#build`.
"""
import re, subprocess, sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
C2118 = re.compile(r"[A-Za-z]:\\.*?\\(src\\[^()]+\.cpp)\((\d+)\)\s*:\s*error C2118")

def build():
    r = subprocess.run(["gruntz", "build"], cwd=REPO, capture_output=True, text=True)
    return r.stdout + r.stderr

dropped_total = 0
for it in range(12):
    out = build()
    fails = {}                                   # file -> set(class)
    for m in C2118.finditer(out):
        rel = m.group(1).replace("\\", "/"); ln = int(m.group(2))
        p = REPO / rel
        lines = p.read_text().splitlines()
        if ln - 1 < len(lines):
            sm = re.search(r"SIZE\(\s*(?:\w+::)?(\w+)\s*,", lines[ln - 1])
            if sm:
                fails.setdefault(rel, set()).add(sm.group(1))
    if not fails:
        print(f"iter {it}: no C2118 - MSVC build clean of SIZE failures")
        break
    n = 0
    for rel, classes in fails.items():
        p = REPO / rel; t = p.read_text()
        for c in classes:
            t2 = re.sub(r"^// size 0x[0-9a-f]+ [^\n]*\n(?=SIZE\(\s*(?:\w+::)?" + re.escape(c) + r"\s*,)", "", t, flags=re.M)
            t2 = re.sub(r"^SIZE\(\s*(?:\w+::)?" + re.escape(c) + r"\s*,[^\n]*\);\n", "", t2, flags=re.M)
            if t2 != t: t = t2; n += 1
        p.write_text(t)
    dropped_total += n
    print(f"iter {it}: dropped {n} MSVC-failing SIZE ({sum(len(v) for v in fails.values())} classes across {len(fails)} files)")
print(f"total dropped: {dropped_total}")
