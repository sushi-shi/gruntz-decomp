#!/usr/bin/env python3
"""globals_attribute.py - attribute each global to its owning module via reloc xrefs.

The DATA-side twin of the function re-homing. The linker lays each .obj's globals
contiguously in .data/.rdata/.bss just like its functions in .text, and a
FILE-STATIC global (`static T g_x;` in the original source) is referenced ONLY from
its own .obj. We have no cross-ref DB, but GRUNTZ.EXE keeps its .reloc: every
absolute-address operand in code is a HIGHLOW reloc. So:

  scan .reloc -> every site in .text holding an absolute VA -> if the VA lands in a
  DATA section (.data/.rdata/.bss) it's a global reference. Resolve the site to its
  containing function -> unit (xref._owner, size-bounded). Group by the referenced
  global.

  * global referenced from exactly ONE unit  -> PRIVATE to that unit: define it there
    (move its DATA() out of the src/Globals.cpp pool into that TU). High confidence,
    needs no anchor - this is the "few xrefs, all one module" rule.
  * global referenced from N units           -> SHARED/public: needs a contiguity or
    declaration-site tie-break (reported, not auto-attributed).

Reuses xref.py's EXE/section load + size-bounded function->unit attribution.

Usage:
    python3 -m gruntz.audit.globals_attribute            # summary + private worklist
    python3 -m gruntz.audit.globals_attribute --pooled   # only the Globals.cpp pool
    python3 -m gruntz.audit.globals_attribute --csv OUT  # full DB (global,units,refs)
    python3 -m gruntz.audit.globals_attribute 0x0020a454 # one global's referencing units
"""
from __future__ import annotations

import argparse
import re
import struct
import sys
from collections import defaultdict
from pathlib import Path

import statistics

from gruntz.core.pe import EXE, ILT_HI, ILT_LO, load as _load
from gruntz.core.symbols import SYMCSV, load_names as _names, owner as _owner

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
POOL = REPO / "src" / "Globals.cpp"
DATA_SECS = {".data", ".rdata", ".bss"}
DATA_RE = re.compile(r"\bDATA\s*\(\s*(0x[0-9a-fA-F]+)")


def _rva_to_off(secs, rva):
    for name, va, vsz, rp, rsz in secs:
        if va <= rva < va + max(vsz, rsz):
            off = rva - va
            return rp + off if off < rsz else None  # in .bss tail -> no file bytes
    return None


def _sec_of(secs, rva):
    for name, va, vsz, rp, rsz in secs:
        if va <= rva < va + max(vsz, rsz):
            return name
    return None


def _image_base_and_reloc(d):
    e = struct.unpack_from("<I", d, 0x3c)[0]
    optsz = struct.unpack_from("<H", d, e + 20)[0]
    opt = e + 24
    image_base = struct.unpack_from("<I", d, opt + 28)[0]        # PE32 ImageBase
    nrva = struct.unpack_from("<I", d, opt + 92)[0]
    reloc_rva = reloc_sz = 0
    if nrva > 5:                                                 # dir[5] = base reloc
        reloc_rva, reloc_sz = struct.unpack_from("<II", d, opt + 96 + 5 * 8)
    return image_base, reloc_rva, reloc_sz


def reloc_sites(d, secs, image_base, reloc_rva, reloc_sz):
    """Yield (site_rva, target_rva) for every HIGHLOW reloc whose site is in .text."""
    text = next(s for s in secs if s[0] == ".text")
    t_lo, t_hi = text[1], text[1] + text[2]
    base = _rva_to_off(secs, reloc_rva)
    if base is None:
        return
    end = base + reloc_sz
    p = base
    while p + 8 <= end:
        page, blksz = struct.unpack_from("<II", d, p)
        if blksz < 8:
            break
        for i in range((blksz - 8) // 2):
            entry = struct.unpack_from("<H", d, p + 8 + i * 2)[0]
            if entry >> 12 != 3:                                # 3 = IMAGE_REL_BASED_HIGHLOW
                continue
            site = page + (entry & 0xFFF)
            if not (t_lo <= site < t_hi):                       # only code->? refs
                continue
            off = _rva_to_off(secs, site)
            if off is None:
                continue
            va = struct.unpack_from("<I", d, off)[0]
            yield site, va - image_base
        p += blksz


def build_db():
    """global_rva -> {unit -> ref_count}, restricted to data-section targets."""
    d, secs = _load()
    names, _byname, fstarts, fsize = _names()
    image_base, reloc_rva, reloc_sz = _image_base_and_reloc(d)
    db = defaultdict(lambda: defaultdict(int))
    for site, target in reloc_sites(d, secs, image_base, reloc_rva, reloc_sz):
        if _sec_of(secs, target) not in DATA_SECS:              # keep only globals
            continue
        owner = _owner(site, fstarts, fsize)
        if owner is None or ILT_LO <= owner < ILT_HI:           # gap / ILT thunk
            unit = "(unrecovered)"
        else:
            unit = (names.get(owner) or ("?", "?"))[1] or "?"
        db[target][unit] += 1
    return db, names


def stacking_report():
    """Verify the linker's one-order-per-section stacking, anchored ONLY on globals
    with a single owning module (confident file-statics) in .data/.rdata - shared/
    multi-xref globals pollute a module's data position and fake inversions. Sort units
    by median .text RVA, check each data section's private-anchor median is
    correspondingly ascending. Residual inversions = conflated god-TUs (median is an
    average over the real .objs they lump) or a mis-homed private. Same invariant as
    tu_order_check's inter-TU .text contiguity, on the data side."""
    db, names = build_db()
    d, secs = _load()

    def sec_of(rva):
        for name, va, vsz, rp, rsz in secs:
            if va <= rva < va + max(vsz, rsz):
                return name
        return None

    # per-unit .text funcs (from symbol_names) + private-only .data/.rdata anchors (from db).
    tfunc = defaultdict(list)
    for r in _csv_rows():
        try:
            rva = int(r["rva"], 16)
        except Exception:
            continue
        u = r.get("unit", "") or "?"
        if u in ("globals", "?", "engine_label_stubs"):
            continue
        if (r.get("kind") or "func") == "func" and sec_of(rva) == ".text":
            tfunc[u].append(rva)

    priv = defaultdict(lambda: defaultdict(list))  # unit -> section -> [rva]
    for g, units in db.items():
        real = [u for u in units if u not in ("(unrecovered)", "?", "ghidra", "")]
        # SINGLE-XREF only: exactly one reference site, resolving to one real unit.
        # These are the gold file-static anchors (.data 89% / .rdata 98.8% concordance
        # vs 68% / 94.5% for the looser single-module filter). Shared or multi-ref
        # globals pollute a module's data-median and fake inversions.
        if sum(units.values()) != 1 or len(real) != 1:
            continue
        s = sec_of(g)
        if s in (".data", ".rdata"):
            priv[real[0]][s].append(g)

    tmed = {u: int(statistics.median(v)) for u, v in tfunc.items() if len(v) >= 2}
    tspan = {u: max(v) - min(v) for u, v in tfunc.items()}
    print("stacking (private-anchored): does each data section follow the .text order?\n")
    for datasec in (".data", ".rdata"):
        dmed = {u: int(statistics.median(ss[datasec])) for u, ss in priv.items()
                if len(ss.get(datasec, [])) >= 2}
        pairs = sorted([(u, tmed[u], dmed[u]) for u in dmed if u in tmed],
                       key=lambda x: x[1])
        n = len(pairs)
        inv = sum(1 for i in range(n) for j in range(i + 1, n) if pairs[i][2] > pairs[j][2])
        mx = n * (n - 1) // 2 or 1
        print(f"  {datasec:<7} {n:>3} units (private-anchored) | concordance "
              f"{100*(1-inv/mx):5.1f}%  ({inv}/{mx} inversions)")
        prev, bad = 0, []
        for u, t, dd in pairs:
            if dd < prev:
                bad.append((u, tspan.get(u, 0)))
            prev = max(prev, dd)
        god = sum(1 for u, sp in bad if sp > 0x8000)
        if bad:
            print(f"          inversions: {len(bad)} ({god} god-TUs) -> "
                  + ", ".join(u for u, _ in bad[:10]) + ("..." if len(bad) > 10 else ""))
    return 0


def _csv_rows():
    import csv
    if not SYMCSV.exists():
        return []
    with open(SYMCSV) as f:
        return list(csv.DictReader(f))


def _pooled_rvas():
    if not POOL.exists():
        return set()
    return {int(m, 16) for m in DATA_RE.findall(POOL.read_text())}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("rva", nargs="?", help="one global RVA (0x..) to explain")
    ap.add_argument("--pooled", action="store_true",
                    help="restrict to the src/Globals.cpp catch-all pool")
    ap.add_argument("--stacking", action="store_true",
                    help="verify .data/.rdata follow the .text module order "
                         "(private-anchored); inversions = god-TUs / mis-homes")
    ap.add_argument("--csv", help="write global,owner_unit,kind,n_units,total_refs,units")
    args = ap.parse_args()

    if args.stacking:
        return stacking_report()

    db, names = build_db()
    pooled = _pooled_rvas()

    if args.rva:
        t = int(args.rva, 16)
        units = db.get(t)
        nm = (names.get(t) or ("?",))[0]
        print(f"{t:#010x} {nm}")
        if not units:
            print("  no code references found (unreferenced / data-to-data only)")
        for u, c in sorted(units.items(), key=lambda kv: -kv[1]):
            print(f"  {c:>4} refs  {u}")
        return 0

    rows = []
    priv = shared = unref = 0
    for g, units in db.items():
        real = {u: c for u, c in units.items() if u not in ("(unrecovered)", "?", "")}
        total = sum(units.values())
        if not real:
            unref += 1
            kind, owner = "unresolved", ""
        elif len(real) == 1:
            priv += 1
            kind, owner = "private", next(iter(real))
        else:
            shared += 1
            kind, owner = "shared", ""
        rows.append((g, owner, kind, len(real), total,
                     ";".join(f"{u}:{c}" for u, c in sorted(real.items(), key=lambda kv: -kv[1]))))

    scope = [r for r in rows if r[0] in pooled] if args.pooled else rows
    scope.sort(key=lambda r: (r[2] != "private", -r[4]))

    print(f"global xref DB: {len(db)} referenced globals "
          f"({'Globals.cpp pool only' if args.pooled else 'whole image'})")
    print(f"  PRIVATE (1 unit -> home there): {sum(1 for r in scope if r[2]=='private')}")
    print(f"  SHARED  (N units):              {sum(1 for r in scope if r[2]=='shared')}")
    print(f"  unresolved (only gap/ILT refs): {sum(1 for r in scope if r[2]=='unresolved')}")
    if pooled:
        placed = sum(1 for r in scope if r[2] == "private")
        print(f"  -> {placed}/{len(scope)} pooled globals are confidently attributable now\n")

    print("  PRIVATE worklist (global -> owning unit, most-referenced first):")
    for g, owner, kind, nu, tot, us in scope:
        if kind == "private":
            nm = (names.get(g) or ("",))[0]
            print(f"    {g:#010x} -> {owner:<22} ({tot} refs)  {nm}")

    if args.csv:
        out = ["global,owner_unit,kind,n_units,total_refs,units"]
        for g, owner, kind, nu, tot, us in sorted(rows, key=lambda r: r[0]):
            out.append(f"{g:#010x},{owner},{kind},{nu},{tot},{us}")
        Path(args.csv).write_text("\n".join(out) + "\n")
        print(f"\nwrote {args.csv}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
