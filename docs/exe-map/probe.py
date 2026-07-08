#!/usr/bin/env python3
"""Ground-truth the 'scatter': is it real linker interleaving, or outliers + COMDAT
exile + our file grouping? Characterize GruntzMgr.cpp + the general outlier effect."""
import re
import gruntz.analysis.exe_map as em

funcs, meta = em.load()
funcs = [r for r in funcs if not r["source"].startswith("src/Stub/")]
by_src = {}
for r in funcs:
    if r["category"] == "unit":
        by_src.setdefault(r["source"], []).append(r)

def clusters(rvas_recs, jump=0x4000):
    """Split a unit's functions into clusters separated by big RVA jumps."""
    rs = sorted(rvas_recs, key=lambda r: r["rva"])
    cl, cur = [], [rs[0]]
    for a, b in zip(rs, rs[1:]):
        if b["rva"] - a["end"] > jump:
            cl.append(cur); cur = [b]
        else:
            cur.append(b)
    cl.append(cur)
    return cl

g = by_src["src/Gruntz/GruntzMgr.cpp"]
cl = clusters(g)
print("GruntzMgr.cpp: %d fns, full span %s" %
      (len(g), em._fmt_size(max(r["end"] for r in g) - min(r["rva"] for r in g))))
print("  clusters (jump>0x4000):")
for c in sorted(cl, key=len, reverse=True):
    lo, hi = min(r["rva"] for r in c), max(r["end"] for r in c)
    print("    %2d fns  0x%06x..0x%06x  span %s" % (len(c), lo, hi, em._fmt_size(hi - lo)))
main = max(cl, key=len)
mlo, mhi = min(r["rva"] for r in main), max(r["end"] for r in main)
outliers = [r for r in g if not (mlo <= r["rva"] < mhi)]
print("  main cluster: %d/%d fns in %s;  %d outlier(s): %s" %
      (len(main), len(g), em._fmt_size(mhi - mlo), len(outliers),
       ", ".join("%s@0x%x" % (r["name"][:24], r["rva"]) for r in outliers)))

# what interleaves the MAIN cluster? ctors/dtors (COMDAT-exiled) vs real methods
inter = [r for r in funcs if mlo <= r["rva"] < mhi and r["source"] != "src/Gruntz/GruntzMgr.cpp"]
ctor_dtor = sum(1 for r in inter if re.match(r"\?\?[01]", r["name"]) or "DeletingDtor" in r["name"])
print("  foreign fns inside the main cluster: %d  (%d are ctors/dtors ??0/??1/ScalarDtor = %d%%)" %
      (len(inter), ctor_dtor, 100 * ctor_dtor // max(len(inter), 1)))

# General: how much of each big unit's span is one main cluster? (outlier sensitivity)
print("\nOutlier sensitivity across units >=20 fns (main-cluster share of span):")
import statistics as st
shares, drops = [], 0
for src, recs in by_src.items():
    if len(recs) < 20:
        continue
    full = max(r["end"] for r in recs) - min(r["rva"] for r in recs)
    cl = clusters(recs)
    m = max(cl, key=len)
    mspan = max(r["end"] for r in m) - min(r["rva"] for r in m)
    shares.append(mspan / full if full else 1.0)
    if mspan < full * 0.5:
        drops += 1
print("  median main-cluster span / full span: %.2f  (1.0 = one block; low = outliers stretch it)"
      % st.median(shares))
print("  units where the main cluster is <50%% of the span (outlier-stretched): %d" % drops)
