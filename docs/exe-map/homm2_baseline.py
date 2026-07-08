#!/usr/bin/env python3
"""HoMM2 (Heroes of Might & Magic II, HEROES2W) ground-truth scatter baseline.

The "PoL" build ships a real NB09 CodeView debug stream, so the homm2-decomp's
symbol_names.csv carries the TRUE function -> source-file grouping (not a guess).
That lets us measure how much the MSVC linker scatters a KNOWN-correct grouping -
the control for our Gruntz numbers - and split it by /O2 vs /Od (COMDAT vs not).

READ-ONLY on the homm2 project: reads its symbol_names.csv + units.toml, snapshots
the VAs into docs/exe-map/homm2_va.csv, computes the same fragment metric as
scatter.py, and (below) builds homm2.html. Nothing in homm2-decomp is modified."""
import csv
import os
import statistics as st
import tomllib
from collections import defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
HOMM2 = "/home/sheep/Projects/homm2/homm2-decomp"
SYM = os.path.join(HOMM2, "build/gen/symbol_names.csv")
UNITS = os.path.join(HOMM2, "config/units.toml")
SNAP = os.path.join(HERE, "homm2_va.csv")


def rint(s):
    s = s.strip()
    return int(s, 16) if s.lower().startswith("0x") else int(s)


def load():
    # unit -> (source, opt)  from the ground-truth manifest
    umeta = {}
    with open(UNITS, "rb") as f:
        for u in tomllib.load(f).get("unit", []):
            fl = u.get("flags", "")
            opt = "O2" if fl == "o2" else "Od"
            umeta[u["unit"]] = (u.get("source", ""), opt)
    # func rows from the CodeView-derived symbol map (ground truth)
    rows = []
    with open(SYM) as f:
        for r in csv.DictReader(f):
            if (r.get("kind") or "") != "func":
                continue
            try:
                rva, sz = rint(r["rva"]), rint(r.get("size") or "0")
            except ValueError:
                continue
            rows.append({"rva": rva, "size": sz, "name": r["name"],
                         "unit": r["unit"]})
    rows.sort(key=lambda r: r["rva"])
    return rows, umeta


def snapshot(rows, umeta):
    with open(SNAP, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["rva", "unit", "opt", "size", "name"])
        for r in rows:
            opt = umeta.get(r["unit"], ("", ""))[1]
            w.writerow(["0x%x" % r["rva"], r["unit"], opt, "0x%x" % r["size"], r["name"]])


def frag_rows(rows, umeta):
    """Per-unit fragment stats over global RVA order (same metric as scatter.py).
    Only real TUs (present in units.toml, i.e. SOURCE/BASE) are reported; imports
    and CRT still interrupt fragments but are not themselves scored."""
    stats = defaultdict(lambda: {"n": 0, "bytes": 0, "lo": None, "hi": 0, "frags": 0})
    prev = None
    for r in rows:
        key = r["unit"]
        s = stats[key]
        s["n"] += 1
        s["bytes"] += r["size"]
        s["lo"] = r["rva"] if s["lo"] is None else min(s["lo"], r["rva"])
        s["hi"] = max(s["hi"], r["rva"] + r["size"])
        if key != prev:
            s["frags"] += 1
        prev = key
    out = []
    for u, s in stats.items():
        if u not in umeta:           # skip imports (*.dll) / CRT (not real TUs)
            continue
        span = s["hi"] - s["lo"]
        out.append({"file": umeta[u][0] or u, "unit": u, "opt": umeta[u][1],
                    "n": s["n"], "bytes": s["bytes"], "span": span, "frags": s["frags"],
                    "avg_cluster": s["n"] / s["frags"],
                    "frag_ratio": s["frags"] / s["n"],
                    "spread": span / s["bytes"] if s["bytes"] else 1.0})
    out.sort(key=lambda r: r["frags"], reverse=True)
    return out


def summarize(label, rows):
    def line(nm, sub):
        if not sub:
            return
        fr = [r["frag_ratio"] for r in sub]
        ac = [r["avg_cluster"] for r in sub]
        cg = sum(1 for r in sub if r["frags"] == 1)
        print(f"  {nm:<20} n={len(sub):<3} frag-ratio med {st.median(fr):.2f}  "
              f"avg-run med {st.median(ac):.2f}  contiguous {100*cg//len(sub)}%")
    print(f"\n{label}: {len(rows)} TUs, {sum(r['n'] for r in rows)} functions")
    line("all", rows)
    line(">=5 fns", [r for r in rows if r["n"] >= 5])
    line(">=5 fns  /O2", [r for r in rows if r["n"] >= 5 and r["opt"] == "O2"])
    line(">=5 fns  /Od", [r for r in rows if r["n"] >= 5 and r["opt"] == "Od"])


if __name__ == "__main__":
    if not os.path.isfile(SYM):
        print(f"homm2-decomp not found at {HOMM2} - keeping existing snapshot "
              f"({'present' if os.path.isfile(SNAP) else 'ABSENT'}); skipping re-extract.")
        raise SystemExit(0)
    rows, umeta = load()
    snapshot(rows, umeta)
    fr = frag_rows(rows, umeta)
    print("=" * 68)
    print("HoMM2 (HEROES2W) GROUND-TRUTH SCATTER  (CodeView source-file grouping)")
    print("=" * 68)
    print(f"snapshot: {os.path.relpath(SNAP)}  ({len(rows)} funcs)")
    summarize("real TUs", fr)
