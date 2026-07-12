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
    """Functions MSVC COMDAT-pools away from the TU block - not scatter, just
    linker pooling. Two families:
      - destructors: the base ??1 + vtable-referenced deleting dtors (??_G scalar,
        ??_E vector, our reconstruction's ScalarDeletingDtor). Ctors are NOT pooled.
      - C++ dynamic-init fragments: the raw _$E<n> / ??__E<sym> initializer thunks
        and our reconstructed InitStr<addr>, ordered by the CRT init table (the
        .data:0x208000 table) so they sit far from the TU's code by construction."""
    import re as _re
    return (name.startswith(("??1", "??_G", "??_E", "_$E", "??__E", "?InitStr", "_$S"))
            or "DeletingDtor" in name or "InitStr" in name
            # pooled vtable-slot virtuals (GetTypeTag/Serialize*/Slot/GetClassId/IsLoaded
            # + any leaf-class override) - COMDATs the linker pools, not scatter:
            or bool(_re.search(r"@@[UME]AE", name))
            or bool(_re.match(r"\?(GetTypeTag|GetRuntimeClass|GetClassId|IsLoaded|"
                              r"Serialize[A-Za-z]*|V?[Ss]lot[0-9a-f]{2})@", name)))


def is_pooled(name):
    """The FULL irreducible-scatter set (superset of is_dtor): every function the
    /Gy retail link places by first-use / init order rather than in the TU block,
    which our build CANNOT relocate (multi-emit trips the dup-RVA guard). On top of
    is_dtor's dtors + init-fragments + pooled vtable-slots, this also drops every
    MSVC special-name COMDAT: constructors (??0), operators (??2 new, ??3 delete,
    ??4 =, ??5.. etc) and the rest of the ?? special-member family. What REMAINS is
    the genuine per-TU BODY - the named non-special methods, which after the
    contiguity drain form one run per file (the scatter_core.html flatline). ?? DATA
    symbols (??_7 vtable, ??_R RTTI) never reach .text, so this only trims code."""
    return name.startswith("??") or is_dtor(name)


def interleaver_rvas(src_root):
    """RVAs of functions flagged `// @interleaver` or `// @orphan` in src/ - proven
    irreducible /Gy scatter our build cannot home without tripping the dup-RVA guard:
      @interleaver = boundary COMDAT the linker placed between two OTHER units (retail
        neighbours differ on each side);
      @orphan = a pooled COMDAT whose real class identity is unrecovered (interior to
        a holding-TU span; homing it needs the class first).
    Both are excluded from the core-body flatline. Anchor: the flag comment sits just
    above the function's RVA(0x...) macro."""
    import os
    rvas = set()
    for dirpath, _, files in os.walk(src_root):
        for fn in files:
            if not fn.endswith((".cpp", ".h")):
                continue
            lines = open(os.path.join(dirpath, fn), errors="ignore").read().splitlines()
            for i, ln in enumerate(lines):
                if "@interleaver" not in ln and "@orphan" not in ln:
                    continue
                for j in range(i + 1, min(i + 20, len(lines))):  # next RVA(0x...) below
                    m = re.search(r"\bRVA\(\s*0x0*([0-9a-fA-F]+)", lines[j])
                    if m:
                        rvas.add(int(m.group(1), 16))
                        break
    return rvas


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
    il = interleaver_rvas(DIR + "/../../src")  # proven @interleaver boundary COMDATs
    core = [r for r in funcs if not is_pooled(r["name"]) and r["rva"] not in il]
    n_pooled = len(funcs) - len(core)
    core_rows = rows_for(core)

    json.dump(all_rows, open(DIR + "/scatter.json", "w"), indent=1)
    json.dump(meth_rows, open(DIR + "/scatter_methods.json", "w"), indent=1)
    json.dump(core_rows, open(DIR + "/scatter_core.json", "w"), indent=1)

    print("=" * 70)
    print("RETAIL .text SCATTER  (src/Stub/ backlog excluded)")
    print("=" * 70)
    summarize("ALL functions", all_rows)
    summarize(f"DESTRUCTORS REMOVED (dropped {n_dtor} ??1/??_G; ctors kept)", meth_rows)
    summarize(f"CORE BODY (dropped {n_pooled}: ?? COMDATs + init-frags + pooled vslots "
              f"+ {len(il)} @interleaver boundary COMDATs)", core_rows)
    non1 = [r for r in core_rows if r["frags"] > 1]
    print(f"\n  core-view files still fragmented (frags>1): {len(non1)}/{len(core_rows)}"
          + (":" if non1 else " -- FLATLINE"))
    for r in sorted(non1, key=lambda r: r["frags"], reverse=True)[:40]:
        print(f"    {r['frags']:>3} frags  {r['n']:>3} fns  {r['file']}")
    print(f"\nwrote scatter.json ({len(all_rows)}) + scatter_methods.json ({len(meth_rows)}) + "
          f"scatter_core.json ({len(core_rows)})")


if __name__ == "__main__":
    main()
