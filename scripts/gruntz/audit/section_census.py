#!/usr/bin/env python3
"""section_census.py - every byte of the candidate-vs-retail SECTION deltas, bucketed.

`ninja candidate` produces a real link, so the candidate EXE's section table is an
oracle: each `retail vsize - candidate vsize` is a number that must be *explained*,
not merely reported. This tool does the explaining.

    python -m gruntz.audit.section_census                 # the section table + buckets
    python -m gruntz.audit.section_census --bss           # per-object .bss, all models
    python -m gruntz.audit.section_census --reloc         # base-relocation buckets

THE ONE MECHANISM THAT DOMINATES: `/INCREMENTAL:YES` GROWTH PADDING.

Retail IS an incremental link, proven three ways: the E9 ILT thunk band at the top of
`.text`, the separate padded `.idata`, and - decisively - `.CRT$XCA/XCC/XCL/XCU` sitting
at `.data+0x0000/0x0104/0x0210/0x0320`, the EXACT offsets our `/INCREMENTAL:YES` link
produces (a `/INCREMENTAL:NO` relink puts them at +0/+4/+0xc/+0x18).

But the linker pads only what it might have to REPLACE on a relink, and that is objects
named on the LINK LINE - never a member pulled out of a `.lib`. Measured here, `.bss`,
flat vs incremental over the same objs:

    107 command-line objects   570,724 -> 684,724   (+114,000, +19.97%)
     27 library members          9,408 ->   9,408   (      +0,   0.00%)

So a section delta is only meaningful once both sides are modelled with the same
library/loose split. Retail's `.bss` says which side each of its TUs was on, because
the growth pad is visible as slack after the contribution: `imagepolyclip` (235,022 B)
is followed by 2 bytes and `ddsurface` (197,168 B) by 0 - they were LIBRARY members -
while `dircellmethods` carries ~20%. That is the `.bss` corroboration of the standing
"engine modules were static .LIBs" finding, and `gruntz.build.link --engine-lib`
reproduces it: `imagepolyclip` 282,016 -> 235,016, `ddsurface` 236,616 -> 197,176,
both landing on retail's spans.
"""

from __future__ import annotations

import argparse
import bisect
import collections
import csv
import re
import struct
import sys
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
sys.path.insert(0, str(REPO / "scripts"))
from gruntz.core.pe import PE  # noqa: E402

EXES = {
    "candidate": REPO / "build/exe/GRUNTZ.candidate.EXE",
    "englib": REPO / "build/exe/GRUNTZ.englib.EXE",
    "flat": REPO / "build/exe/GRUNTZ.flat.EXE",
}
SECTIONS = (".text", ".rdata", ".data", ".idata", ".rsrc", ".reloc")


def load():
    out = {"retail": PE()}
    for k, p in EXES.items():
        if p.exists():
            out[k] = PE(p)
    return out


def cmd_table(_a):
    pes = load()
    cols = [k for k in ("retail", "candidate", "englib", "flat") if k in pes]
    S = {k: {s["name"]: s for s in pes[k].sections} for k in cols}
    print("VIRTUAL SIZE")
    print(f"{'section':<9}" + "".join(f"{c:>13}" for c in cols))
    for n in SECTIONS:
        print(f"{n:<9}" + "".join(
            f"{S[c].get(n, {}).get('virtual_size', 0):>13,}" for c in cols))
    print(f"{'.data raw':<9}" + "".join(
        f"{S[c]['.data']['raw_size']:>13,}" for c in cols))
    print(f"{'.bss':<9}" + "".join(
        f"{S[c]['.data']['virtual_size'] - S[c]['.data']['raw_size']:>13,}"
        for c in cols))
    print(f"{'FILE':<9}" + "".join(f"{len(pes[c].data):>13,}" for c in cols))
    r = len(pes["retail"].data)
    print(f"{'% retail':<9}" + "".join(
        f"{100.0*len(pes[c].data)/r:>12.2f}%" for c in cols))

    print("\n.data DELTA, bucketed (vs retail)")
    R = S["retail"][".data"]
    for c in cols[1:]:
        C = S[c][".data"]
        draw = C["raw_size"] - R["raw_size"]
        dbss = ((C["virtual_size"] - C["raw_size"])
                - (R["virtual_size"] - R["raw_size"]))
        print(f"  {c:<10} initialized (raw) {draw:>+9,}   "
              f".bss zero-fill {dbss:>+9,}   total {draw+dbss:>+9,}")
    print("\n  `initialized` moves in units of the 0x200 FileAlignment, so a +1,024 row\n"
          "  is at most 1,024 and at least 513 bytes of real content.")

    # the incremental-padding proof: where .CRT$X* land
    d = pes["retail"].data
    o = pes["retail"].off(R["rva"])
    hits = [i for i in range(0, 0x400, 4)
            if struct.unpack_from("<I", d, o + i)[0]]
    print(f"\n  retail's first nonzero .data dwords: "
          f"{', '.join('+0x%04x' % i for i in hits[:6])}")
    print("  ours /INCREMENTAL:YES  .CRT$XCA/XCC/XCL/XCU at +0x0000/+0x0104/+0x0210/+0x0320")
    print("  ours /INCREMENTAL:NO   .CRT$XCA/XCC/XCL/XCU at +0x0000/+0x0004/+0x000c/+0x0018")
    print("  => retail is /INCREMENTAL:YES.")


def map_bss_spans(path: Path):
    secs, syms = [], []
    for ln in open(path, encoding="latin1"):
        m = re.match(r"^ ([0-9a-f]{4}):([0-9a-f]{8}) ([0-9a-f]{8})H (\S+)", ln)
        if m:
            secs.append((int(m.group(1), 16), int(m.group(2), 16),
                         int(m.group(3), 16), m.group(4)))
            continue
        m = re.match(r"^ ([0-9a-f]{4}):([0-9a-f]{8})\s+(\S+)\s+([0-9a-f]{8})\s+(\S.*)$", ln)
        if m:
            syms.append((int(m.group(1), 16), int(m.group(2), 16),
                         m.group(5).split()[-1]))
    bss = next(s for s in secs if s[0] == 3 and s[3] == ".bss")
    b = sorted([s for s in syms if s[0] == 3 and s[1] >= bss[1]], key=lambda x: x[1])
    first = {}
    for _, off, obj in b:
        first.setdefault(obj, off)
    order = sorted(first, key=lambda o: first[o])
    return {o: (first[order[i + 1]] if i + 1 < len(order) else bss[1] + bss[2]) - first[o]
            for i, o in enumerate(order)}, bss[2]


def cmd_bss(_a):
    maps = {}
    for tag, stem in (("flat", "GRUNTZ.flat.map"), ("objs", "GRUNTZ.candidate.map"),
                      ("englib", "GRUNTZ.englib.map")):
        p = REPO / "build/exe" / stem
        if p.exists():
            maps[tag] = map_bss_spans(p)
    if "flat" not in maps or "objs" not in maps:
        sys.exit("need GRUNTZ.flat.map and GRUNTZ.candidate.map "
                 "(python -m gruntz.build.link --no-incremental --out ... --map ...)")
    A, sa = maps["flat"]
    B, sb = maps["objs"]
    lib = [o for o in A if ":" in o]
    own = [o for o in A if ":" not in o]

    def tot(g, M):
        return sum(M[o] for o in g if o in M)
    print("/INCREMENTAL:YES growth padding, .bss, by where the object came from")
    print(f"  {len(own):>4} command-line objects  {tot(own, A):>9,} -> {tot(own, B):>9,}"
          f"   {tot(own, B)-tot(own, A):>+9,}  ({100.0*(tot(own,B)-tot(own,A))/max(tot(own,A),1):.2f}%)")
    print(f"  {len(lib):>4} library members       {tot(lib, A):>9,} -> {tot(lib, B):>9,}"
          f"   {tot(lib, B)-tot(lib, A):>+9,}  ({100.0*(tot(lib,B)-tot(lib,A))/max(tot(lib,A),1):.2f}%)")
    print(f"  section total                {sa:>9,} -> {sb:>9,}   {sb-sa:>+9,}")

    C = maps.get("englib", ({}, 0))[0]
    print(f"\n{'object':<24}{'flat':>10}{'objs/incr':>11}{'englib':>10}"
          "   retail slack after the contribution")
    for k, kk, retail in (("imagepolyclip.obj", "engine:imagepolyclip.obj", "2 B"),
                          ("ddsurface.obj", "engine:ddsurface.obj", "0 B"),
                          ("fileimage.obj", "engine:fileimage.obj", "1,024 B"),
                          ("dircellmethods.obj", "dircellmethods.obj", "~20% (padded)")):
        print(f"{k:<24}{A.get(k, 0):>10,}{B.get(k, 0):>11,}"
              f"{C.get(kk, C.get(k, 0)):>10,}   {retail}")

    print(f"\ntop 15 command-line objects by padding inserted:")
    delta = sorted(((B[o] - A[o], o) for o in own if o in B), reverse=True)
    for d, o in delta[:15]:
        print(f"  {d:>+9,}   {A[o]:>9,} -> {B[o]:>9,}   {o}")


def cmd_reloc(_a):
    pes = load()
    print(f"{'model':<11}{'dir size':>10}{'HIGHLOW':>9}"
          + "".join(f"{s:>10}" for s in (".text", ".rdata", ".data")))
    for tag in ("retail", "candidate", "englib", "flat"):
        pe = pes.get(tag)
        if not pe:
            continue
        d = pe.data
        e = struct.unpack_from("<I", d, 0x3C)[0]
        _rva, size = struct.unpack_from("<II", d, e + 24 + 96 + 5 * 8)
        per = collections.Counter(pe.sec_name(r) for r in pe.reloc_sites)
        print(f"{tag:<11}{size:>10,}{sum(per.values()):>9,}"
              + "".join(f"{per.get(s, 0):>10,}" for s in (".text", ".rdata", ".data")))

    # apples-to-apples: fixups inside function bodies present on BOTH sides
    retail, cand = pes["retail"], pes.get("candidate")
    if not cand:
        return
    rt = sorted(r for r in retail.reloc_sites if retail.sec_name(r) == ".text")
    ct = sorted(r for r in cand.reloc_sites if cand.sec_name(r) == ".text")
    cva = {}
    for ln in open(REPO / "build/exe/GRUNTZ.candidate.map", encoding="latin1"):
        m = re.match(r"^ 0001:([0-9a-f]{8})\s+(\S+)\s+([0-9a-f]{8})\s+", ln)
        if m:
            cva.setdefault(m.group(2), int(m.group(3), 16) - 0x400000)
    tr = tc = paired = 0
    diffs = []
    for r in csv.DictReader(open(REPO / "build/gen/symbol_names.csv")):
        if r.get("kind") != "func" or not r.get("size"):
            continue
        rva, sz = int(r["rva"], 16), int(r["size"], 16)
        va = cva.get(r["name"])
        if not sz or va is None:
            continue
        paired += 1
        a = bisect.bisect_left(rt, rva + sz) - bisect.bisect_left(rt, rva)
        b = bisect.bisect_left(ct, va + sz) - bisect.bisect_left(ct, va)
        tr += a
        tc += b
        if a != b:
            diffs.append((b - a, r["unit"], r["name"]))
    print(f"\n.text fixups inside the {paired:,} function bodies present on BOTH sides:")
    print(f"  retail {tr:,}   candidate {tc:,}   delta {tc-tr:+,}"
          f"   ({len(diffs)} functions differ)")
    print(f"  outside those bodies (CRT/MFC + unattributed): "
          f"retail {len(rt)-tr:,}  candidate {len(ct)-tc:,}  "
          f"delta {(len(ct)-tc)-(len(rt)-tr):+,}")
    print("\n  worst 12 functions:")
    for d, unit, name in sorted(diffs)[:12]:
        print(f"  {d:>5}  {unit:<20} {name[:66]}")


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--bss", action="store_true")
    ap.add_argument("--reloc", action="store_true")
    a = ap.parse_args(argv)
    if a.bss:
        cmd_bss(a)
    elif a.reloc:
        cmd_reloc(a)
    else:
        cmd_table(a)


if __name__ == "__main__":
    main()
