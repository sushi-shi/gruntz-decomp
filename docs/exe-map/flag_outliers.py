#!/usr/bin/env python3
"""Misplacement finder: after removing the benign COMDAT-pooled destructors, flag
functions that sit far from their src/ file's main block - candidates for being in
the WRONG file (mis-attribution) or a CONFLATED TU that should be split.

Method (per file, destructors + src/Stub/ removed):
  - split the file's functions into clusters (RVA gap > GAP)
  - main cluster = the largest; everything else is an OUTLIER
  - for each outlier, find the TERRITORY it landed in: which OTHER file owns the
    methods surrounding its RVA -> the likely correct home
  - CONFLATED = the file has >=2 clusters of >=3 methods (two real TUs under one name)

Dumps flags.json for the dashboard + prints a ranked worklist."""
import bisect
import json
from collections import defaultdict

import gruntz.analysis.exe_map as em

import os as _os; DIR = _os.path.dirname(_os.path.abspath(__file__))
GAP = 0x4000       # cluster split threshold
WIN = 0x2000       # territory window around an outlier


import re as _re
# COMDAT-pooled members MSVC5 exiles from the TU block into shared low-RVA pools -
# not misplacements, just linker pooling (same reason dtors are dropped). Besides
# the base ??1 + deleting ??_G/??_E dtors: GetTypeTag / GetRuntimeClass, the
# reconstructed base-class inline virtuals (VslotNN), the msgmap default/stub
# handlers, and Serialize (also pooled). Excluding these keeps the finder from
# counting a correctly-placed pooled virtual as an "outlier".
_POOLED_RE = _re.compile(
    r"\?(GetTypeTag|GetRuntimeClass|Serialize|Vslot[0-9a-f]{2}|"
    r"Wap32GameMgrVfunc[0-9]|SbiSlot[0-9]|DoDefault|UnusedMsgHandler|"
    r"On(DrawItem|MeasureItem|ActionBtn|StubBtn|InitDialog|Ok|OkCommand))@")


def is_dtor(name):
    # dtors + CRT C++ dynamic-init fragments (_$E / ??__E / InitStr, ordered by the
    # init table, inherently far from the TU block) + COMDAT-pooled virtuals.
    return (name.startswith(("??1", "??_G", "??_E", "_$E", "??__E", "?InitStr", "_$S"))
            or "DeletingDtor" in name or "InitStr" in name
            or bool(_POOLED_RE.match(name)))


def _family(r):
    """Which named .text region a function belongs to (game / engine / library / ...)."""
    c = r["category"]
    if c in ("mfc", "crt", "zlib", "eh", "asm", "thunk"):
        return {"thunk": "ILT thunks", "mfc": "MFC", "crt": "CRT", "zlib": "zlib",
                "eh": "EH", "asm": "asm"}[c]
    if c == "unit":
        return "game" if r["source"].startswith("src/Gruntz/") else "engine"
    return "unknown"


def named_regions(funcs, lo, hi, binsz=0x2000, minseg=0x5000):
    """Coalesced [lo, hi, family] bands: the dominant family per bin, short bands
    absorbed into a neighbour so labels stay readable. src/Gruntz/ = game, other
    src/ modules = engine; plus MFC / CRT / zlib / EH / ILT thunks / unknown."""
    nb = (hi - lo) // binsz + 1
    acc = [dict() for _ in range(nb)]
    for r in funcs:
        b = (r["rva"] - lo) // binsz
        if 0 <= b < nb:
            f = _family(r)
            acc[b][f] = acc[b].get(f, 0) + r["size"]
    dom = [max(a, key=a.get) if a else None for a in acc]
    for i in range(nb):
        if dom[i] is None:
            dom[i] = dom[i - 1] if i else "unknown"
    segs = []
    for i, f in enumerate(dom):
        rv = lo + i * binsz
        if segs and segs[-1][2] == f:
            segs[-1][1] = rv + binsz
        else:
            segs.append([rv, rv + binsz, f])
    out = []
    for s in segs:
        if out and (s[1] - s[0] < minseg or out[-1][2] == s[2]):
            out[-1][1] = s[1]
        else:
            out.append(list(s))
    final = []
    for s in out:
        if final and final[-1][2] == s[2]:
            final[-1][1] = s[1]
        else:
            final.append(list(s))
    return final


def clusters(recs):
    rs = sorted(recs, key=lambda r: r["rva"])
    out, cur = [], [rs[0]]
    for a, b in zip(rs, rs[1:]):
        if b["rva"] - a["end"] > GAP:
            out.append(cur)
            cur = [b]
        else:
            cur.append(b)
    out.append(cur)
    return out


def main():
    funcs, meta = em.load()
    funcs = [r for r in funcs if not r["source"].startswith("src/Stub/")]
    meth = [r for r in funcs if not is_dtor(r["name"])]           # dtors removed
    owned = [r for r in meth if r["category"] == "unit" and r["source"]]
    gstarts = sorted(owned, key=lambda r: r["rva"])
    grva = [r["rva"] for r in gstarts]

    def territory(lo, hi, exclude):
        i, j = bisect.bisect_left(grva, lo - WIN), bisect.bisect_right(grva, hi + WIN)
        tally = defaultdict(int)
        for r in gstarts[i:j]:
            if r["source"] != exclude:
                tally[r["source"]] += 1
        return sorted(tally.items(), key=lambda kv: -kv[1])

    by_file = defaultdict(list)
    for r in owned:
        by_file[r["source"]].append(r)

    files_out = []
    for src, recs in by_file.items():
        cl = clusters(recs)
        main_c = max(cl, key=lambda c: (len(c), sum(x["size"] for x in c)))
        mlo, mhi = min(x["rva"] for x in main_c), max(x["end"] for x in main_c)
        big_clusters = [c for c in cl if len(c) >= 3]
        conflated = len(big_clusters) >= 2
        outliers = []
        for c in cl:
            if c is main_c:
                continue
            clo, chi = min(x["rva"] for x in c), max(x["end"] for x in c)
            dist = clo - mhi if clo > mhi else mlo - chi
            terr = territory(clo, chi, src)
            for x in c:
                outliers.append({
                    "rva": x["rva"], "name": x["name"], "dist": dist,
                    "cluster_n": len(c),
                    "home": terr[0][0] if terr else "", "home_n": terr[0][1] if terr else 0,
                })
        files_out.append({
            "file": src, "n": len(recs), "bytes": sum(x["size"] for x in recs),
            "main_lo": mlo, "main_hi": mhi, "main_n": len(main_c),
            "main_share": len(main_c) / len(recs),
            "clusters": len(cl), "conflated": conflated,
            "cluster_list": [{"lo": min(x["rva"] for x in c),
                              "hi": max(x["end"] for x in c), "n": len(c)}
                             for c in sorted(cl, key=lambda c: min(x["rva"] for x in c))],
            "outliers": outliers,
            "rvas": sorted(x["rva"] for x in recs),
        })

    flagged = [f for f in files_out if f["outliers"]]
    # severity: conflated first, then by #outliers, then by farthest outlier
    def sev(f):
        return (f["conflated"], len(f["outliers"]), max(o["dist"] for o in f["outliers"]))
    flagged.sort(key=sev, reverse=True)

    regions = named_regions(funcs, meta["text_lo"], meta["text_hi"])
    json.dump({"text_lo": meta["text_lo"], "text_hi": meta["text_hi"],
               "regions": regions, "flagged": flagged},
              open(DIR + "/flags.json", "w"), indent=1)

    tot_out = sum(len(f["outliers"]) for f in flagged)
    conf = [f for f in flagged if f["conflated"]]
    print("=" * 72)
    print("MISPLACEMENT FLAGS  (destructors removed, src/Stub/ excluded)")
    print("=" * 72)
    print(f"{len(files_out)} files -> {len(flagged)} have outlier methods "
          f"({tot_out} functions possibly in the wrong file); {len(conf)} look CONFLATED.\n")
    print("CONFLATED-TU candidates (>=2 clusters of >=3 methods; split the unit):")
    for f in sorted(conf, key=lambda f: -f["n"])[:12]:
        cs = clusters(by_file[f["file"]])
        spans = "  ".join("%d@0x%x" % (len(c), min(x["rva"] for x in c)) for c in cs)
        print(f"  {f['file']:<38} {f['n']:>3} methods in {f['clusters']} clusters: {spans}")
    print("\nLONE MISPLACEMENT candidates (outlier far from main -> likely correct home):")
    lone = sorted((o | {"file": f["file"], "main_lo": f["main_lo"], "main_hi": f["main_hi"]}
                   for f in flagged for o in f["outliers"] if o["cluster_n"] <= 2),
                  key=lambda o: -o["dist"])
    for o in lone[:18]:
        home = o["home"].rsplit("/", 1)[-1] if o["home"] else "?"
        print(f"  {o['file'].rsplit('/',1)[-1]:<26} {o['name'][:34]:<34} @0x{o['rva']:06x}  "
              f"{em._fmt_size(o['dist']):>8} from main  -> {home} ({o['home_n']} fns)")
    print(f"\nwrote flags.json ({len(flagged)} flagged files)")


if __name__ == "__main__":
    main()
