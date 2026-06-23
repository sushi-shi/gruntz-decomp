#!/usr/bin/env python3
"""Move flat-only stub classes to include/Stub/<X>.h with real RTTI inheritance +
delta pad + SIZE; rewrite src/Stub/<X>.cpp to #include the header (bodies stay).
Generalised over all base families, processed bottom-up (shortest chain first) so
each base header exists before its leaves. Dry-run unless --write.

Resolves a base B to a stub-world header + size:
  CUserBase=4, CUserLogic=0x54 (spine); else a class already moved to Stub/<B>.h
  with its measured size; root (no base) -> pad to full size (no inheritance).
Skips: ambiguous-size, delta<0 (size inconsistency / base-leak), unresolved base."""
import csv, re, sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
SRC, INC = REPO / "src/Stub", REPO / "include/Stub"
WRITE = "--write" in sys.argv
SPINE = {"CUserBase": 4, "CUserLogic": 0x54}
# bases modeled in include/Stub/ WITHOUT a SIZE (ambiguous own size); use the
# modeled lower-bound size so derived classes compose (their pad absorbs the tail).
MODELED = {"CMovingLogic": 0x54}

rows = list(csv.DictReader(open(REPO / "ctor-survey/inheritance.csv")))
base = {r["class"]: r["base"] for r in rows}
depth = {r["class"]: r["chain"].count(" : ") for r in rows}
sz = {}
for r in csv.DictReader(open(REPO / "ctor-survey/new_sites.csv")):
    if r["method"] in ("vtable", "ctor") and r["size_kind"] == "const" and r["class"] and "+" not in r["class"]:
        sz.setdefault(r["class"], set()).add(int(r["sizeof"], 16))
sz = {k: next(iter(v)) for k, v in sz.items() if len(v) == 1}

def body(text, cls):
    m = re.search(r"\bclass\s+" + re.escape(cls) + r"\b[^:{;]*\{", text)  # flat only
    if not m: return None
    o = text.index("{", m.start()); d = 0; i = o
    while i < len(text):
        d += (text[i] == "{") - (text[i] == "}")
        if d == 0: break
        i += 1
    j = i + 1
    while j < len(text) and text[j] in " \t": j += 1
    return (m.start(), (j + 1 if j < len(text) and text[j] == ";" else i + 1), text[o + 1:i])

def known_size(c):
    return SPINE.get(c) or MODELED.get(c) or sz.get(c)

done, skip = [], []
# bottom-up: shorter chain (closer to root) first, so base headers precede leaves
for X in sorted(sz, key=lambda c: depth.get(c, 99)):
    p = SRC / f"{X}.cpp"
    if not p.exists():
        continue
    t = p.read_text(); b = body(t, X)
    if b is None:
        continue                                   # already inherits / not a flat def
    B = base.get(X, "")
    # resolve base header + size
    if B in SPINE:
        bh, bs = f"Stub/{B}.h", SPINE[B]
    elif B and (INC / f"{B}.h").exists():
        bh, bs = f"Stub/{B}.h", known_size(B)
    elif not B:
        bh, bs = None, 0                            # root -> pad full, no inheritance
    else:
        skip.append((X, f"base {B} unresolved (graduated/MFC/not-yet-moved)")); continue
    if bs is None:
        skip.append((X, f"base {B} size unknown")); continue
    delta = sz[X] - bs
    if delta < 0:
        skip.append((X, f"measured 0x{sz[X]:x} < base {B} 0x{bs:x} (size-leak?)")); continue
    s0, s1, inner = b
    pad = f"    char m_size_pad[0x{delta:x}]; // own region over {B or 'root'} (0x{bs:x})\n" if delta else ""
    inh = f" : public {B}" if B else ""
    inc = f"#include <{bh}>\n" if bh else ""
    hdr = (f"#ifndef GRUNTZ_STUB_{X.upper()}_H\n#define GRUNTZ_STUB_{X.upper()}_H\n"
           f"#include <rva.h>\n{inc}// {X}{(' : ' + B) if B else ' (root)'} (RTTI). sizeof 0x{sz[X]:x}.\n"
           f"class {X}{inh} {{{inner}{pad}}};\nSIZE({X}, 0x{sz[X]:x});\n#endif\n")
    if WRITE:
        (INC / f"{X}.h").write_text(hdr)
        p.write_text(t[:s0] + f"#include <Stub/{X}.h>\n" + t[s1:])
    done.append((X, sz[X], B or "(root)", delta))

for X, s, B, d in done:
    print(f"{'WROTE' if WRITE else 'WOULD':6} {X:28} 0x{s:<5x} : {B:18} delta 0x{d:x}")
print(f"\n{len(done)} moved; {len(skip)} skipped.")
for X, why in skip:
    print(f"  skip {X:28} {why}")
