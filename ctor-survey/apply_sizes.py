#!/usr/bin/env python3
"""Apply pad + SIZE() to the empty src/Stub/ leaf classes whose size we recovered
from operator-new sites. Self-consistent scaffold: pad to N, assert SIZE(N).

Skips: the 7 ambiguous (multi-size) classes (base-attribution leakage), classes
reconstructed elsewhere (double-defined), and anything not a flat empty stub.

Dry-run by default; pass --write to edit files."""
import csv, re, sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
SRC = REPO / "src"
NEW = REPO / "ctor-survey/new_sites.csv"
WRITE = "--write" in sys.argv
SKIP = {"CGruntzMgr"}  # reconstructed in src/Gruntz/GruntzMgr.cpp (double-defined)

# rva -> (class, path)
rva_class = {}
rva_pat = re.compile(r"RVAU?\(\s*0x([0-9a-fA-F]+)")
def_pat = re.compile(r"\b([A-Za-z_]\w*)\s*::\s*~?\w+\s*\(")
for p in list(SRC.rglob("*.cpp")) + list(SRC.rglob("*.h")):
    lines = p.read_text(errors="ignore").splitlines()
    for i, ln in enumerate(lines):
        m = rva_pat.search(ln)
        if not m:
            continue
        for j in range(i, min(i + 4, len(lines))):
            d = def_pat.search(lines[j])
            if d:
                rva_class[int(m.group(1), 16)] = (d.group(1), p)
                break

# class -> {size: count}, path  (ctor-confirmed const-size new-sites)
by_class = {}
for r in csv.DictReader(open(NEW)):
    if r["after"] != "ctor" or r["size_kind"] != "const" or not r["ctor_rva"]:
        continue
    c = int(r["ctor_rva"], 16)
    if c not in rva_class:
        continue
    cls, path = rva_class[c]
    e = by_class.setdefault(cls, {"sizes": {}, "path": path})
    e["sizes"][int(r["sizeof"], 16)] = e["sizes"].get(int(r["sizeof"], 16), 0) + 1


def class_body_end(text, cstart):
    """index just past the class's closing '};' given index of its opening '{'."""
    depth = 0
    i = cstart
    while i < len(text):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                j = i + 1
                while j < len(text) and text[j] in " \t":
                    j += 1
                return j + 1 if j < len(text) and text[j] == ";" else None
        i += 1
    return None


done, skipped = [], []
for cls in sorted(by_class):
    szs = by_class[cls]["sizes"]; path = by_class[cls]["path"]
    if cls in SKIP or "Stub/" not in str(path) or len(szs) != 1:
        skipped.append((cls, "skip/ambiguous/non-stub")); continue
    size = next(iter(szs))
    text = path.read_text()
    if "SIZE(" in text or "m_pad" in text:
        skipped.append((cls, "already has SIZE/pad")); continue
    cm = re.search(r"\bclass\s+" + re.escape(cls) + r"\b[^{;]*\{", text)
    if not cm:
        skipped.append((cls, "class def not found")); continue
    obrace = text.index("{", cm.start())
    end = class_body_end(text, obrace)
    if end is None:
        skipped.append((cls, "brace match failed")); continue
    pad = f"\n    char m_pad[0x{size:x}]; // size 0x{size:x} from operator-new (gruntz.analysis.news); fields TBD"
    new = (text[:obrace + 1] + pad + text[obrace + 1:end]
           + f"\nSIZE({cls}, 0x{size:x});" + text[end:])
    if WRITE:
        path.write_text(new)
    done.append((cls, size, path.relative_to(REPO)))

for cls, size, f in done:
    print(f"{'WROTE' if WRITE else 'WOULD':6} {cls:32} 0x{size:<5x} {f}")
print(f"\n{len(done)} classes {'edited' if WRITE else 'to edit'}; {len(skipped)} skipped.")
for cls, why in skipped:
    print(f"  skip {cls:32} {why}")
