#!/usr/bin/env python3
"""exe_diff.py - whole-EXE comparison: our candidate GRUNTZ.EXE vs retail.

objdiff scores each *object* in isolation (reloc-masked). This tool goes one
level up: it links our base objs into a candidate GRUNTZ.EXE (see
`gruntz.build.link`) and compares that whole image against retail GRUNTZ.EXE.
It answers three questions the per-object score can't:

  A. HEADERS / SECTION TABLE - do the PE headers and sections line up?
  B. .text LAYOUT FIDELITY   - does each function land at its retail RVA? Broken
     down into the levers that drive it: intra-TU function order, intra-TU
     byte-exact block layout, and absolute placement (needs link order + full
     coverage). This is what the RVA-reorder campaign moves.
  C. .text BYTE FIDELITY      - name-align every reconstructed function to its
     retail twin and compare the *linked* bytes (relocations applied). Reported
     both over all functions and over the objdiff-exact subset (which isolates
     "unresolved-extern displacement noise" from real code differences).

The candidate is a PARTIAL reconstruction linked with /FORCE, so it is not
runnable and most externals are unresolved (resolved to 0) - that depresses the
raw byte number; the exact-subset number is the honest code-fidelity signal.

Run in `nix develop` (no MSVC needed - reads existing EXEs). Regenerate the
candidate first with `nix develop .#build --command python -m gruntz.build.link`.

Usage:
    python -m gruntz.analysis.exe_diff
    python -m gruntz.analysis.exe_diff --json          # machine-readable summary
"""

import argparse
import csv
import json
import os
import re
import struct
from collections import defaultdict
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
REPO = next((p for p in SCRIPT_DIR.parents if (p / "flake.nix").exists()), SCRIPT_DIR)

MAP_SEG_RE = re.compile(r"0001:[0-9a-f]+$")
RVA_RE = re.compile(r"[0-9a-f]{8}$")


# --------------------------------------------------------------------------- PE
def parse_pe(path: Path):
    """Minimal PE32 reader: image base + {section: (vaddr, vsize, rawptr, rawsz)}."""
    d = path.read_bytes()
    if d[:2] != b"MZ":
        raise ValueError(f"{path}: not MZ")
    pe = struct.unpack_from("<I", d, 0x3C)[0]
    if d[pe:pe + 4] != b"PE\x00\x00":
        raise ValueError(f"{path}: no PE signature")
    coff = pe + 4
    nsec = struct.unpack_from("<H", d, coff + 2)[0]
    opt_sz = struct.unpack_from("<H", d, coff + 16)[0]
    opt = coff + 20
    lmaj, lmin = d[opt + 2], d[opt + 3]
    entry = struct.unpack_from("<I", d, opt + 16)[0]
    imgbase = struct.unpack_from("<I", d, opt + 28)[0]
    secs = {}
    order = []
    base = opt + opt_sz
    for i in range(nsec):
        o = base + i * 40
        name = d[o:o + 8].rstrip(b"\x00").decode("latin1")
        vsz, vaddr, rawsz, rawptr = struct.unpack_from("<IIII", d, o + 8)
        secs[name] = (vaddr, vsz, rawptr, rawsz)
        order.append(name)
    return dict(data=d, secs=secs, order=order, imgbase=imgbase, entry=entry,
                linker=(lmaj, lmin), nsec=nsec, size=len(d))


def read_at(pe, rva, n):
    """Read n bytes at image-RVA `rva` from a parsed PE (via its section map)."""
    for _, (va, vs, rp, rs) in pe["secs"].items():
        if va <= rva < va + max(vs, rs):
            off = rp + (rva - va)
            return pe["data"][off:off + n]
    return b""


# ------------------------------------------------------------------ input joins
def load_candidate_map(map_path: Path, imgbase: int):
    """name -> (rva, obj) and an RVA-sorted list -> per-func candidate size."""
    rows = []
    for ln in map_path.read_text(errors="replace").splitlines():
        p = ln.split()
        if len(p) >= 5 and MAP_SEG_RE.match(p[0]) and RVA_RE.match(p[2]) and p[3] == "f":
            rows.append((int(p[2], 16) - imgbase, p[1], p[-1]))
    rows.sort()
    cand, csize = {}, {}
    for i, (rva, name, obj) in enumerate(rows):
        cand[name] = (rva, obj)
        nxt = rows[i + 1][0] if i + 1 < len(rows) else rva
        csize[name] = max(0, nxt - rva)
    return cand, csize


def load_retail_names(names_csv: Path):
    """name -> (rva, unit, size), func kind only."""
    ret = {}
    with names_csv.open() as f:
        for r in csv.DictReader(l for l in f if not l.lstrip().startswith("#")):
            if (r.get("kind") or "func") != "func":
                continue
            sz = int(r["size"], 16) if r.get("size") else 0
            ret[r["name"]] = (int(r["rva"], 16), r["unit"], sz)
    return ret


def load_exact_set(report_json: Path):
    """Names objdiff scored 100% (reloc-masked exact), for the byte-subset stat."""
    exact = set()
    if not report_json.exists():
        return exact
    rep = json.loads(report_json.read_text())
    for u in rep.get("units", []):
        for fn in u.get("functions", []):
            if (fn.get("fuzzy_match_percent") or 0) >= 100.0:
                exact.add(fn["name"])
    return exact


# ----------------------------------------------------------------------- report
def hexk(n):
    return f"{n:#x}"


def main() -> None:
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--cand", default=str(REPO / "build/exe/GRUNTZ.candidate.EXE"))
    ap.add_argument("--retail", default=os.environ.get(
        "GRUNTZ_EXE", str(REPO / "build/exe/GRUNTZ.EXE")))
    ap.add_argument("--map", default=str(REPO / "build/exe/GRUNTZ.candidate.map"))
    ap.add_argument("--names", default=str(REPO / "build/gen/symbol_names.csv"))
    ap.add_argument("--report", default=str(REPO / "build/objdiff/report.json"))
    ap.add_argument("--json", action="store_true", help="emit JSON summary only.")
    args = ap.parse_args()

    cand_pe = parse_pe(Path(args.cand))
    ret_pe = parse_pe(Path(args.retail))
    cand, csize = load_candidate_map(Path(args.map), cand_pe["imgbase"])
    ret = load_retail_names(Path(args.names))
    exact_set = load_exact_set(Path(args.report))
    common = set(cand) & set(ret)

    # --- A. headers / sections ------------------------------------------------
    sec_rows = []
    for name in sorted(set(ret_pe["order"]) | set(cand_pe["order"])):
        r = ret_pe["secs"].get(name)
        c = cand_pe["secs"].get(name)
        sec_rows.append((name, r, c))

    # --- B. layout fidelity ---------------------------------------------------
    abs_ok = sum(1 for n in common if cand[n][0] == ret[n][0])
    byobj = defaultdict(list)
    for n in common:
        byobj[cand[n][1]].append(n)
    order_tus = order_ok_tus = order_ok_funcs = 0
    block_tus = block_funcs = 0
    multi_funcs = 0
    reorder_worklist = []
    for obj, names in byobj.items():
        if len(names) < 2:
            continue
        order_tus += 1
        multi_funcs += len(names)
        ret_order = sorted(names, key=lambda n: ret[n][0])
        cand_order = sorted(names, key=lambda n: cand[n][0])
        ok = ret_order == cand_order
        order_ok_tus += ok
        if ok:
            order_ok_funcs += len(names)
        else:
            pos = {n: i for i, n in enumerate(ret_order)}
            inv = sum(1 for i in range(len(cand_order) - 1)
                      if pos[cand_order[i]] > pos[cand_order[i + 1]])
            reorder_worklist.append((obj, len(names), inv))
        a = ret_order[0]
        if all((cand[n][0] - cand[a][0]) == (ret[n][0] - ret[a][0]) for n in ret_order):
            block_tus += 1
            block_funcs += len(names)
    singletons = sum(1 for n in common if len(byobj[cand[n][1]]) == 1)

    # --- C. byte fidelity (name-aligned, linked bytes) ------------------------
    tot = same = 0
    ex_tot = ex_same = 0
    linked_exact = 0
    for n in common:
        crva = cand[n][0]
        rrva, _unit, rsz = ret[n]
        L = rsz if rsz else csize[n]
        if L <= 0:
            continue
        cb = read_at(cand_pe, crva, L)
        rb = read_at(ret_pe, rrva, L)
        L = min(len(cb), len(rb))
        if L <= 0:
            continue
        m = sum(1 for a, b in zip(cb[:L], rb[:L]) if a == b)
        tot += L
        same += m
        if m == L and csize[n] == rsz:
            linked_exact += 1
        if n in exact_set:
            ex_tot += L
            ex_same += m

    pct = lambda a, b: (100.0 * a / b) if b else 0.0
    summary = {
        "candidate_bytes": cand_pe["size"], "retail_bytes": ret_pe["size"],
        "common_functions": len(common),
        "layout": {
            "abs_rva_correct": abs_ok,
            "intra_tu_order_ok_tus": order_ok_tus, "intra_tu_order_tus": order_tus,
            "intra_tu_order_ok_funcs": order_ok_funcs,
            "block_exact_tus": block_tus, "block_exact_funcs": block_funcs,
            "singleton_funcs": singletons, "multi_tu_funcs": multi_funcs,
            "order_pct": pct(order_ok_funcs, multi_funcs),
        },
        "bytes": {
            "linked_identical": same, "linked_total": tot,
            "linked_pct": pct(same, tot),
            "linked_exact_funcs": linked_exact,
            "exact_subset_identical": ex_same, "exact_subset_total": ex_tot,
            "exact_subset_pct": pct(ex_same, ex_tot),
        },
    }

    if args.json:
        print(json.dumps(summary, indent=2))
        return

    W = 74
    print("=" * W)
    print("EXE DIFF - candidate GRUNTZ.EXE vs retail")
    print("=" * W)
    print(f"  candidate {cand_pe['size']:>9,} B   retail {ret_pe['size']:>9,} B "
          f"({pct(cand_pe['size'], ret_pe['size']):.1f}% of retail; partial recon)")

    print("\n" + "-" * W)
    print("A. HEADERS & SECTION TABLE")
    print("-" * W)
    print(f"  image base  cand={hexk(cand_pe['imgbase'])}  retail={hexk(ret_pe['imgbase'])}"
          f"   linker cand={cand_pe['linker']} retail={ret_pe['linker']}")
    print(f"  entry point cand={hexk(cand_pe['entry'])}  retail={hexk(ret_pe['entry'])}"
          f"   (cand /ENTRY unresolved under /FORCE)")
    print(f"  sections    cand={cand_pe['nsec']}  retail={ret_pe['nsec']}")
    print(f"    {'section':10} {'retail vaddr/vsize':>24}   {'candidate vaddr/vsize':>24}")
    for name, r, c in sec_rows:
        rs = f"{r[0]:#08x} / {r[1]:#8x}" if r else "-- absent --"
        cs = f"{c[0]:#08x} / {c[1]:#8x}" if c else "-- absent --"
        print(f"    {name:10} {rs:>24}   {cs:>24}")
    print("  (candidate lacks .idata/.rsrc/.reloc: no imports table / resources /"
          " base relocs\n   are emitted for a /FORCE partial link - expected.)")

    print("\n" + "-" * W)
    print("B. .text LAYOUT FIDELITY  (name-aligned; drives absolute RVA correctness)")
    print("-" * W)
    print(f"  functions in both candidate & retail : {len(common)}")
    print(f"  at CORRECT absolute retail RVA        : {abs_ok}/{len(common)} "
          f"({pct(abs_ok, len(common)):.2f}%)")
    print(f"     -> absolute RVA needs correct cross-TU LINK ORDER *and* full")
    print(f"        coverage (every preceding object present at retail size);")
    print(f"        with sorted-by-name order + ~partial objs it is expected ~0.")
    print(f"  intra-TU function ORDER correct       : {order_ok_tus}/{order_tus} TUs"
          f"  ({order_ok_funcs} funcs, {pct(order_ok_funcs, multi_funcs):.1f}%)")
    print(f"     -> mismatch = reorder that .cpp's defs to retail-RVA order.")
    print(f"  intra-TU BYTE-EXACT block             : {block_tus}/{order_tus} TUs"
          f"  ({block_funcs} funcs)")
    print(f"     -> order correct AND every func at retail offset-from-anchor;")
    print(f"        the gap vs order-correct is FUNCTION SIZE mismatch (not yet")
    print(f"        byte-exact bodies). A byte-exact block slots in at one shift.")
    print(f"  singleton TUs (1 common func)         : {singletons} funcs")
    if reorder_worklist:
        print("  top intra-TU reorder targets (obj, funcs, adjacent inversions):")
        for obj, n, inv in sorted(reorder_worklist, key=lambda t: -t[2])[:12]:
            print(f"     {obj:28} {n:3d} funcs, {inv} inversions")

    print("\n" + "-" * W)
    print("C. .text BYTE FIDELITY  (linked bytes, relocations APPLIED)")
    print("-" * W)
    print(f"  name-aligned linked byte-identical    : {same:,}/{tot:,} "
          f"= {pct(same, tot):.2f}%")
    print(f"  functions byte-EXACT in linked image  : {linked_exact}")
    print(f"  same, over objdiff-EXACT funcs only   : {ex_same:,}/{ex_tot:,} "
          f"= {pct(ex_same, ex_tot):.2f}%")
    print(f"     -> objdiff-exact funcs match retail reloc-masked; their residual")
    print(f"        here ({100 - pct(ex_same, ex_tot):.2f}%) is purely unresolved-extern")
    print(f"        displacement bytes (/FORCE resolves them to 0). Real code")
    print(f"        differences live in the non-exact functions.")

    print("\n" + "-" * W)
    print("D. PROPOSED TRACKED EXE-MATCH NUMBERS  (`gruntz exe-diff`)")
    print("-" * W)
    print(f"  exe-layout-order% = {pct(order_ok_funcs, multi_funcs):5.1f}%  "
          f"(funcs in retail intra-TU order)   [reorder lever]")
    print(f"  exe-layout-block% = {pct(block_funcs, len(common)):5.1f}%  "
          f"(funcs in a byte-exact retail block) [reorder+match]")
    print(f"  exe-byte%         = {pct(same, tot):5.1f}%  "
          f"(name-aligned linked .text bytes)  [match+coverage]")
    print(f"  exe-byte(exact)%  = {pct(ex_same, ex_tot):5.1f}%  "
          f"(same, exact subset = code purity) [reloc-resolution]")
    print(f"  abs-rva%          = {pct(abs_ok, len(common)):5.1f}%  "
          f"(final target; rises near full coverage + correct link order)")


if __name__ == "__main__":
    main()
