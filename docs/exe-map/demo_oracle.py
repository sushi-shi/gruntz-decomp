#!/usr/bin/env python3
"""Demo-vs-retail placement oracle -> demo_oracle.json (consumed by deep_layout.py).

GruntDem.exe (../../../runtime/GruntDem.exe) is a different LINK of the same build
session, 99.3% code-identical. For every flags.json outlier, find the function in
the demo via a reloc/rel32-masked byte search, map its unit's HOME interval in the
demo the same way, and compare relative placement:

  AT-HOME-IN-DEMO  far from home in retail but at home in demo -> ilink MOVED it
  FAR-IN-BOTH      same relative spot in both links -> BIRTH position (COMDAT
                   emission / genuinely-that-TU), NOT incremental relocation
  FAR-DIFFERENT    far in both but by different amounts (home-estimate noise or
                   a move between two far spots)

Measured result (2026-07-10): 170 FAR-IN-BOTH, 3 AT-HOME-IN-DEMO, 8 FAR-DIFFERENT
-> the incremental linker did NOT cause the scatter. Skips cleanly if the demo
EXE is absent. Slow (~minutes): masked regex over 2MB per probe."""
import bisect
import csv
import json
import os
import re
import struct
import sys
from collections import Counter, defaultdict

DIR = os.path.dirname(os.path.abspath(__file__))
DEMO = os.path.join(DIR, "../../../runtime/GruntDem.exe")
EXE = os.environ.get("GRUNTZ_EXE", "")
if not os.path.isfile(DEMO):
    print("demo_oracle: %s absent - skipping (demo_oracle.json left as-is)" % DEMO)
    sys.exit(0)
RET = open(EXE, "rb").read()
DEM = open(DEMO, "rb").read()
IMG = 0x400000
RTEXT = (0x1000, 0x1e626b)
DEMO_TEXT = DEM[0x400:0x400 + 0x1e01af]


def rfo(rva):
    return rva - 0x1000 + 0x400


relocs = set()
off, end = 0x249a00, 0x249a00 + 0x1b9d1
while off + 8 <= end:
    page, bsz = struct.unpack("<II", RET[off:off + 8])
    if bsz < 8:
        break
    for i in range(8, bsz, 2):
        e, = struct.unpack("<H", RET[off + i:off + i + 2])
        if e >> 12 == 3:
            s = page + (e & 0xFFF)
            if RTEXT[0] <= s < RTEXT[1]:
                relocs.add(s)
    off += bsz


def pattern_for(rva, size):
    """Regex over the function bytes with reloc dwords + rel32 operands masked."""
    b = RET[rfo(rva):rfo(rva) + size]
    mask = bytearray(len(b))
    for s in relocs:
        if rva <= s < rva + size:
            for k in range(4):
                if 0 <= s - rva + k < len(b):
                    mask[s - rva + k] = 1
    i = 0
    while i < len(b):
        if mask[i]:
            i += 1
            continue
        if b[i] in (0xE8, 0xE9) and i + 5 <= len(b):
            for k in range(1, 5):
                mask[i + k] = 1
            i += 5
            continue
        if b[i] == 0x0F and i + 6 <= len(b) and 0x80 <= b[i + 1] <= 0x8F:
            for k in range(2, 6):
                mask[i + k] = 1
            i += 6
            continue
        i += 1
    if len(b) - sum(mask) < 20:
        return None
    out, run = [], b""
    for i in range(len(b)):
        if mask[i]:
            if run:
                out.append(re.escape(run))
                run = b""
            out.append(b".")
        else:
            run += b[i:i + 1]
    if run:
        out.append(re.escape(run))
    return re.compile(b"".join(out), re.S)


def find_in_demo(rva, size):
    pat = pattern_for(rva, size)
    if pat is None:
        return ("thin", None)
    hits = [m.start() + 0x1000 for m in pat.finditer(DEMO_TEXT)]
    if len(hits) == 1:
        return ("ok", hits[0])
    return ("none" if not hits else "multi", None)


def main():
    by_unit = defaultdict(list)
    for r in csv.DictReader(open(os.path.join(DIR, "../../build/gen/symbol_names.csv"))):
        if r["kind"] == "func":
            by_unit[r["unit"]].append((int(r["rva"], 16),
                                       int(r["size"] or "0", 16), r["name"]))
    for v in by_unit.values():
        v.sort()

    def main_cluster(fns):
        cl, cur = [], [fns[0]]
        for f in fns[1:]:
            if f[0] - cur[-1][0] > 0x4000:
                cl.append(cur)
                cur = []
            cur.append(f)
        cl.append(cur)
        return max(cl, key=len)

    home_cache = {}

    def demo_home(unit):
        if unit in home_cache:
            return home_cache[unit]
        fns = by_unit.get(unit)
        if not fns or len(fns) < 3:
            home_cache[unit] = None
            return None
        hits = []
        for rva, sz, name in main_cluster(fns)[:8]:
            if sz < 24:
                continue
            st, d = find_in_demo(rva, min(sz, 512))
            if st == "ok":
                hits.append(d)
            if len(hits) >= 4:
                break
        home_cache[unit] = (min(hits), max(hits)) if len(hits) >= 2 else None
        return home_cache[unit]

    import gruntz.core.exe_map as em
    src2unit = {v: k for k, v in em._load_units().items() if v}
    flags = json.load(open(os.path.join(DIR, "flags.json")))
    results, stats = [], Counter()
    for f in flags["flagged"]:
        unit = src2unit.get(f["file"])
        for o in f["outliers"]:
            if not unit or unit not in by_unit:
                stats["no-unit"] += 1
                continue
            sz = next((s for r, s, n in by_unit[unit] if r == o["rva"]), 0)
            if sz < 24:
                stats["tiny"] += 1
                continue
            st, drva = find_in_demo(o["rva"], min(sz, 512))
            if st != "ok":
                stats["demo-" + st] += 1
                continue
            home = demo_home(unit)
            if home is None:
                stats["no-home"] += 1
                continue
            dd = 0 if home[0] - 0x4000 <= drva <= home[1] + 0x4000 else \
                min(abs(drva - home[0]), abs(drva - home[1]))
            if dd == 0:
                verdict = "AT-HOME-IN-DEMO"
            elif abs(dd - o["dist"]) < 0x8000:
                verdict = "FAR-IN-BOTH"
            else:
                verdict = "FAR-DIFFERENT"
            stats[verdict] += 1
            results.append({"file": f["file"], "name": o["name"], "rva": o["rva"],
                            "demo_rva": drva, "retail_dist": o["dist"],
                            "demo_dist": dd, "verdict": verdict})
    print("demo_oracle:", dict(stats))
    json.dump(results, open(os.path.join(DIR, "demo_oracle.json"), "w"), indent=1)


main()
