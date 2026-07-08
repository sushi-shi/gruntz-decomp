#!/usr/bin/env python3
"""Retail .text scatter analysis: how fragmented each src file's functions are.

For every source file (unit), walk the GLOBAL RVA-ordered function list and count
how many maximal contiguous RUNS its functions form:
  fragments == 1        -> the file's functions are one contiguous block
  fragments == n_funcs  -> every function is isolated (maximally scattered)

Produces TWO datasets so the COMDAT effect is visible:
  scatter.json          - all functions (minus the src/Stub/ backlog)
  scatter_methods.json  - the same, but DESTRUCTORS removed (base ??1 + deleting
                          ??_G/??_E): MSVC pools those dtor COMDATs away from the TU
                          block, so they inflate scatter. Constructors are KEPT (they
                          cluster with the methods) - this shows the real TU layout.
Prints a summary for each + dumps per-file stats JSON for the charts. Run under
`nix develop` (scripts/ on PYTHONPATH); reads via exe_map.load()."""
import json
import re
import statistics as st
from collections import defaultdict

import gruntz.analysis.exe_map as em

import os as _os; DIR = _os.path.dirname(_os.path.abspath(__file__))


def is_dtor(name):
    """A destructor MSVC COMDAT-pools away from the TU block: the base destructor
    (??1) and the vtable-referenced deleting destructors (??_G scalar, ??_E vector,
    and our reconstruction's ScalarDeletingDtor). Constructors are NOT pooled, so
    they are kept."""
    return name.startswith(("??1", "??_G", "??_E")) or "DeletingDtor" in name


def rows_for(funcs):
    """Per-file fragment stats over the GLOBAL RVA-ordered `funcs` list."""
    stats = defaultdict(lambda: {"n": 0, "bytes": 0, "lo": None, "hi": 0, "frags": 0})
    prev = None
    for r in funcs:
        key = r["source"] if (r["category"] == "unit" and r["source"]) else None
        if key is None:
            prev = "<foreign>"
            continue
        s = stats[key]
        s["n"] += 1
        s["bytes"] += r["size"]
        s["lo"] = r["rva"] if s["lo"] is None else min(s["lo"], r["rva"])
        s["hi"] = max(s["hi"], r["end"])
        if key != prev:
            s["frags"] += 1
        prev = key
    out = []
    for f, s in stats.items():
        span = s["hi"] - s["lo"]
        out.append({"file": f, "n": s["n"], "bytes": s["bytes"], "lo": s["lo"],
                    "hi": s["hi"], "span": span, "frags": s["frags"],
                    "avg_cluster": s["n"] / s["frags"],
                    "frag_ratio": s["frags"] / s["n"],
                    "occupancy": s["bytes"] / span if span else 1.0,
                    "spread": span / s["bytes"] if s["bytes"] else 1.0})
    out.sort(key=lambda r: r["frags"], reverse=True)
    return out


def summarize(label, rows):
    def line(name, sub):
        if not sub:
            return
        fr = [r["frag_ratio"] for r in sub]
        ac = [r["avg_cluster"] for r in sub]
        sp = [r["spread"] for r in sub]
        cg = sum(1 for r in sub if r["frags"] == 1)
        print(f"  {name:<22} n={len(sub):<4} frag-ratio med {st.median(fr):.2f}  "
              f"avg-run med {st.median(ac):.2f}  spread med {st.median(sp):.0f}x  "
              f"contiguous {100*cg//len(sub)}%")
    print(f"\n{label}: {len(rows)} files, {sum(r['n'] for r in rows)} functions")
    line("all files", rows)
    line("files >=5 fns", [r for r in rows if r["n"] >= 5])
    line("files >=20 fns", [r for r in rows if r["n"] >= 20])


def main():
    funcs, _ = em.load()
    funcs = [r for r in funcs if not r["source"].startswith("src/Stub/")]  # drop backlog

    all_rows = rows_for(funcs)
    methods = [r for r in funcs if not is_dtor(r["name"])]
    n_dtor = len(funcs) - len(methods)
    meth_rows = rows_for(methods)

    json.dump(all_rows, open(DIR + "/scatter.json", "w"), indent=1)
    json.dump(meth_rows, open(DIR + "/scatter_methods.json", "w"), indent=1)

    print("=" * 70)
    print("RETAIL .text SCATTER  (src/Stub/ backlog excluded)")
    print("=" * 70)
    summarize("ALL functions", all_rows)
    summarize(f"DESTRUCTORS REMOVED (dropped {n_dtor} ??1/??_G; ctors kept)", meth_rows)
    print(f"\nwrote scatter.json ({len(all_rows)} files) + "
          f"scatter_methods.json ({len(meth_rows)} files)")


if __name__ == "__main__":
    main()
