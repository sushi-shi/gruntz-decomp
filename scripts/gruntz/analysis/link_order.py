#!/usr/bin/env python3
"""link_order.py - recover the order the linker processed the object files.

Retail .text is a sequence of contiguous, mostly-disjoint per-object blocks, laid
out in the order the objects were fed to the linker (see
docs/link-order-investigation.md). This recovers that order from the matched retail
RVAs and writes it to config/link-order.tsv (regenerate as coverage grows).

  1. CROSS-TU / MODULE LINK ORDER. Each unit is positioned at the start of its main
     code block. CRUCIAL: we DE-POOL first. A class's constructor/destructor are
     emitted as COMDATs and pooled by the linker into shared low-address runs far
     from the class's code, so a naive min-RVA sort orders units by where their
     *dtor* landed (the pool), not their code - flagging almost everything as
     "conflated". We drop the special members (mangled ??0/??1/??_E/??_G) and
     position each unit by the min-RVA of its largest non-special code cluster. Units
     are grouped into modules (Gruntz, Net, Dsndmgr, ... from config/units.toml); the
     module order and whether modules interleave (smear) is reported. zlib is the
     anchor - it must come out as one clean ordered run.

  2. INTRA-TU ORDER (needs a candidate .map from `gruntz link`). For each TU, is our
     candidate-link function order the same as retail's? A mismatch is a TODO to
     reorder that .cpp's function definitions to retail-RVA order (MSVC emits COMDATs
     in source-definition order).

Usage:
    python3 -m gruntz.analysis.link_order [--map build/exe/GRUNTZ.candidate.map]
"""

import argparse
import csv
import re
import tomllib
from collections import Counter, defaultdict
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])

# A gap this large between a unit's matched funcs means two genuinely distant .text
# regions - the signature of a unit whose code is in >1 object block (truly conflated).
REGION_GAP = 0x40000
# ctor / dtor / vector- & scalar-deleting-dtor mangled prefixes. These are COMDATs the
# linker pools away from the class's code block, so they're excluded when locating a
# unit's position (else the pooled dtor drags it to the pool, not its real block).
SPECIAL = ("??0", "??1", "??_E", "??_G")

MAP_SEG_RE = re.compile(r"0001:[0-9a-f]+$")
RVA_RE = re.compile(r"[0-9a-f]{8}$")


def load_retail(names_csv: Path):
    """by_name {name:(rva,unit)}, by_unit {unit:[(rva,name,size)]} - matched funcs."""
    by_name, by_unit = {}, defaultdict(list)
    with names_csv.open() as f:
        for r in csv.DictReader(ln for ln in f if not ln.lstrip().startswith("#")):
            if (r.get("kind") or "func") != "func":
                continue
            rva = int(r["rva"], 16)
            size = int(r["size"], 16) if r.get("size") else 0
            by_name[r["name"]] = (rva, r["unit"])
            by_unit[r["unit"]].append((rva, r["name"], size))
    for u in by_unit:
        by_unit[u].sort()
    return by_name, by_unit


def load_modules(units_toml: Path):
    """unit -> module (the src/ subdir, or 'zlib'); aggregate src/Stub/ units excluded."""
    mod = {}
    for u in tomllib.load(open(units_toml, "rb"))["unit"]:
        src = u["source"]
        if "/Stub/" in src:
            continue                       # engine_label_stubs/discovered/attributed/... aggregates
        parts = src.split("/")
        mod[u["unit"]] = "zlib" if parts[0] == "vendor" else parts[1]
    return mod


def clusters(items, gap=REGION_GAP):
    """Split [(rva,size),...] (sorted) into runs separated by > gap."""
    out, run = [], []
    for it in items:
        if run and it[0] - run[-1][0] > gap:
            out.append(run)
            run = []
        run.append(it)
    if run:
        out.append(run)
    return out


def depool(funcs):
    """Position a unit at the start of its largest non-special code block.

    funcs: [(rva,name,size)]. Returns (position_rva, [(lo,hi,bytes),...] for every
    block). Drops ctors/dtors (pooled); falls back to all funcs if a unit matched
    only special members.
    """
    items = sorted((rva, size) for rva, name, size in funcs
                   if not name.startswith(SPECIAL))
    if not items:
        items = sorted((rva, size) for rva, _, size in funcs)
    cl = clusters(items)
    blocks = sorted((c[0][0], c[-1][0], sum(s for _, s in c)) for c in cl)
    main = max(cl, key=lambda c: sum(s for _, s in c))      # largest by code bytes
    return main[0][0], blocks


def report_recovered(by_unit, mod, out_tsv):
    print("=" * 72)
    print("1. CROSS-TU / MODULE LINK ORDER  (de-pooled: positioned by main code block)")
    print("=" * 72)
    pos, blocks = {}, {}
    for u, fs in by_unit.items():
        if u not in mod:                   # skip aggregate Stub units
            continue
        pos[u], blocks[u] = depool(fs)
    order = sorted(pos, key=lambda u: (pos[u], u))

    # ---- module order + fragmentation (how interspersed each module is) -------
    seq = [mod[u] for u in order]
    trans = sum(1 for i in range(len(seq) - 1) if seq[i] != seq[i + 1])
    runs, minpos, nunits = Counter(), {}, Counter()
    for i, u in enumerate(order):
        m = mod[u]
        if i == 0 or mod[order[i - 1]] != m:
            runs[m] += 1               # a fresh run of this module starts here
        minpos.setdefault(m, pos[u])
        nunits[m] += 1
    print("  module order (by first code block); runs>1 = interspersed with others:")
    print(f"  {'start':>8} {'units':>5} {'runs':>4}  module")
    for m in sorted(minpos, key=lambda m: minpos[m]):
        print(f"  {minpos[m]:8x} {nunits[m]:5d} {runs[m]:4d}  {m}")
    print(f"\n  {len(order)} units, {trans} module-transitions (low => clean object "
          f"blocks). zlib anchor must be 1 run.")

    # ---- persist the inferred order -> config/link-order.tsv ------------------
    if out_tsv:
        with open(out_tsv, "w", newline="") as fh:
            w = csv.writer(fh, delimiter="\t")
            w.writerow(("# generated by gruntz.analysis.link_order - inferred retail link order",))
            w.writerow(("rank", "rva", "module", "unit", "n_funcs", "n_blocks", "blocks"))
            for i, u in enumerate(order):
                bl = ";".join(f"{lo:x}-{hi:x}" for lo, hi, _ in blocks[u])
                w.writerow((i, f"0x{pos[u]:06x}", mod[u], u, len(by_unit[u]),
                            len(blocks[u]), bl))
        print(f"\n  wrote {len(order)} units in inferred link order -> {out_tsv}")

    conflated = [u for u in order if len(blocks[u]) > 1]
    print(f"  {len(order)} units positioned; {len(conflated)} truly conflated "
          f"(code in >1 block after de-pooling): {', '.join(sorted(conflated)[:12])}"
          f"{' ...' if len(conflated) > 12 else ''}")
    return pos


def report_intra_tu(by_name, cand):
    print("\n" + "=" * 72)
    print("2. INTRA-TU ORDER  (our candidate-link order vs retail; mismatch => reorder .cpp)")
    print("=" * 72)
    if not cand:
        print("  (no candidate .map - run `gruntz link` first)")
        return
    exact = total = 0
    todo = []
    for obj in sorted(cand):
        unit = obj[:-4] if obj.endswith(".obj") else obj
        our = [n for n in cand[obj] if n in by_name]
        if len(our) < 2:
            continue
        ret_order = sorted(our, key=lambda n: by_name[n][0])
        p = {n: i for i, n in enumerate(ret_order)}
        inv = sum(1 for i in range(len(our) - 1) if p[our[i]] > p[our[i + 1]])
        total += 1
        ok = our == ret_order
        exact += ok
        if not ok:
            todo.append((unit, len(our), inv))
    print(f"  {exact}/{total} TUs already in retail source order.")
    if todo:
        print("  reorder these .cpp function definitions to retail-RVA order:")
        for unit, n, inv in sorted(todo, key=lambda t: -t[2])[:20]:
            print(f"     {unit:24} {n:3d} funcs, {inv} inversions")


def load_candidate_order(map_path: Path):
    """object -> [name,...] in OUR link's .text order (== source-definition order)."""
    funcs = []
    for ln in map_path.read_text(errors="replace").splitlines():
        p = ln.split()
        if (len(p) >= 5 and MAP_SEG_RE.match(p[0]) and RVA_RE.match(p[2])
                and p[3] == "f"):
            funcs.append((int(p[2], 16), p[1], p[-1]))
    funcs.sort()
    by_obj = defaultdict(list)
    for _, name, obj in funcs:
        by_obj[obj].append(name)
    return by_obj


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--map", default="build/exe/GRUNTZ.candidate.map")
    ap.add_argument("--names", default="build/gen/symbol_names.csv")
    ap.add_argument("--out", default=str(REPO / "config" / "link-order.tsv"),
                    help="where to persist the inferred order (--out '' to skip)")
    args = ap.parse_args()

    by_name, by_unit = load_retail(Path(args.names))
    mod = load_modules(REPO / "config" / "units.toml")
    report_recovered(by_unit, mod, args.out or None)

    cand = load_candidate_order(Path(args.map)) if Path(args.map).exists() else {}
    report_intra_tu(by_name, cand)


if __name__ == "__main__":
    main()
