#!/usr/bin/env python3
"""rtti_classify.py - map every MSVC vtable in GRUNTZ.EXE to its RTTI class name,
and characterise the RTTI-LESS `Unknown*` "Harry Potter family" placeholder classes.

Why: GRUNTZ.EXE ships 231 RTTI mangled class names (`.?AV<Class>@@`). For a
polymorphic class WITH RTTI, the real name is recoverable:

    vtable[-1] -> CompleteObjectLocator -> TypeDescriptor -> ".?AV<Class>@@"

The `Unknown*` placeholders (tomalla's Harry-Potter codenames) are precisely the
classes whose vtables carry NO complete-object-locator (vtable[-1] == 0), i.e. a
module compiled WITHOUT /GR - so RTTI cannot name them. This tool proves that and
ties each placeholder to the construction/vtable evidence instead.

Run inside the dev shell (needs $GRUNTZ_EXE; reads build/gen/symbol_names.csv and
build/ghidra-enrich/exports/functions.csv when present for nicer labels):

    nix develop --command python3 scripts/analysis/rtti_classify.py            # summary
    nix develop --command python3 scripts/analysis/rtti_classify.py --orphans  # + unreconstructed RTTI names
    nix develop --command python3 scripts/analysis/rtti_classify.py --vtable 0x1efd00   # dump one vtable
"""
from __future__ import annotations
import struct, os, re, csv, sys, bisect, glob
from collections import defaultdict
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
EXE = os.environ["GRUNTZ_EXE"]
data = open(EXE, "rb").read()

# --- minimal PE parse ---
pe = struct.unpack_from("<I", data, 0x3c)[0]
nsec = struct.unpack_from("<H", data, pe + 6)[0]
opt = pe + 24
IMG = struct.unpack_from("<I", data, opt + 28)[0]
sec0 = opt + struct.unpack_from("<H", data, pe + 20)[0]
SECS = []
for i in range(nsec):
    o = sec0 + i * 40
    name = data[o:o + 8].rstrip(b"\0").decode("latin1")
    vsz = struct.unpack_from("<I", data, o + 8)[0]
    va = struct.unpack_from("<I", data, o + 12)[0]
    rsz = struct.unpack_from("<I", data, o + 16)[0]
    raw = struct.unpack_from("<I", data, o + 20)[0]
    SECS.append((name, va, max(vsz, rsz), raw))
TEXT = [(va, vs) for n, va, vs, raw in SECS if n == ".text"][0]
LO, HI = TEXT[0], TEXT[0] + TEXT[1]


def off(rva):
    for n, va, vs, raw in SECS:
        if va <= rva < va + vs:
            return raw + (rva - va)
    return None


def dw(rva):
    o = off(rva)
    return struct.unpack_from("<I", data, o)[0] if o is not None else None


def is_code(rva):
    return rva is not None and LO <= rva < HI


# --- labels (best-effort) ---
NAMED = {}
sn = REPO / "build" / "gen" / "symbol_names.csv"
if sn.is_file():
    for r in csv.DictReader(open(sn)):
        if r.get("kind") == "func":
            NAMED[int(r["rva"], 16)] = r["name"]
FENTS = []
fc = REPO / "build" / "ghidra-enrich" / "exports" / "functions.csv"
if fc.is_file():
    FENTS = sorted(int(r["entry_rva"], 16) for r in csv.DictReader(open(fc))
                   if r["entry_rva"].startswith("0x"))


def label(rva):
    return NAMED.get(rva, f"FUN_{rva:06x}")


def func_of(rva):
    i = bisect.bisect_right(FENTS, rva) - 1
    return FENTS[i] if (FENTS and i >= 0) else None


# --- RTTI: type descriptors -> COLs -> vtables ---
def build_rtti():
    td_by_va = {}                          # IMG+td_rva -> name
    for n, va, vs, raw in SECS:
        if n not in (".data", ".rdata"):
            continue
        blob = data[raw:raw + vs]
        for m in re.finditer(rb"\.\?A[UV][\x21-\x7e]*?@@", blob):
            td_by_va[IMG + (va + m.start() - 8)] = m.group().decode("latin1")
    col_td = {}                            # col_rva -> name
    for n, va, vs, raw in SECS:
        if n not in (".data", ".rdata"):
            continue
        blob = data[raw:raw + vs]
        for o2 in range(0, len(blob) - 16, 4):
            if struct.unpack_from("<I", blob, o2)[0] == 0:          # signature
                ptd = struct.unpack_from("<I", blob, o2 + 12)[0]    # pTypeDescriptor
                if ptd in td_by_va:
                    col_td[va + o2] = td_by_va[ptd]
    vt_name = {}                           # vtable_rva -> name
    for n, va, vs, raw in SECS:
        if n not in (".data", ".rdata"):
            continue
        blob = data[raw:raw + vs]
        for o2 in range(0, len(blob) - 8, 4):
            colptr = struct.unpack_from("<I", blob, o2)[0] - IMG
            if colptr in col_td:
                if is_code(struct.unpack_from("<I", blob, o2 + 4)[0] - IMG):
                    vt_name[va + o2 + 4] = col_td[colptr]
    return td_by_va, col_td, vt_name


# --- pointer tables (vtables, incl. RTTI-less) ---
def build_ptrtables():
    ptr_at = {}
    for n, va, vs, raw in SECS:
        if n not in (".rdata", ".data"):
            continue
        blob = data[raw:raw + vs]
        for o2 in range(0, len(blob) - 3, 4):
            v = struct.unpack_from("<I", blob, o2)[0]
            if IMG + LO <= v < IMG + HI:
                ptr_at[va + o2] = v - IMG
    pts = defaultdict(list)
    for loc, t in ptr_at.items():
        pts[t].append(loc)
    return ptr_at, pts


PTR_AT, PTS = build_ptrtables()


def vstart(loc):
    s = loc
    while (s - 4) in PTR_AT:
        s -= 4
    return s


def vtable_methods(vt_rva, limit=64):
    out, p = [], vt_rva
    while len(out) < limit:
        r = PTR_AT.get(p)
        if r is None:
            break
        out.append(r)
        p += 4
    return out


def find_ctors(vt_rva):
    """Functions that stamp this vtable (mov [reg], offset vtable)."""
    needle = struct.pack("<I", IMG + vt_rva)
    to, tsz = TEXT
    blob = data[off(to):off(to) + tsz]
    out, start = set(), 0
    while True:
        k = blob.find(needle, start)
        if k < 0:
            break
        f = func_of(to + k)
        if f is not None:
            out.add(f)
        start = k + 1
    return out


def src_unknown_classes():
    """Unknown*/Worker* placeholder class -> list of function RVAs (from src/ RVA macros)."""
    cls = defaultdict(list)
    for f in glob.glob(str(REPO / "src/**/*.cpp"), recursive=True) + \
             glob.glob(str(REPO / "src/**/*.h"), recursive=True):
        lines = open(f).read().splitlines()
        for i, l in enumerate(lines):
            m = re.search(r"\bRVA\((0x[0-9a-fA-F]+)", l)
            if not m:
                continue
            rva = int(m.group(1), 16)
            for j in range(i + 1, min(i + 4, len(lines))):
                dm = re.search(r"\b([A-Za-z_]\w*)::", lines[j])
                if dm:
                    cls[dm.group(1)].append(rva)
                    break
    return cls


def main():
    td_by_va, col_td, vt_name = build_rtti()
    print(f"RTTI: {len(td_by_va)} type descriptors, {len(col_td)} complete-object-locators, "
          f"{len(vt_name)} vtables WITH RTTI.\n")

    if "--vtable" in sys.argv:
        vt = int(sys.argv[sys.argv.index("--vtable") + 1], 16)
        print(f"vtable @0x{vt:x}  vtable[-1]=0x{(dw(vt-4) or 0):08x}  "
              f"RTTI={vt_name.get(vt,'NONE')}")
        for i, r in enumerate(vtable_methods(vt)):
            print(f"   +0x{i*4:02x}  0x{r:06x}  {label(r)}")
        return

    cls = src_unknown_classes()
    print("=== Unknown* placeholder classes: RTTI status + construction ===")
    for c in sorted(cls):
        if not (c.startswith("Unknown") or c in ("WorkerA", "WorkerB")):
            continue
        vts = defaultdict(int)
        for r in cls[c]:
            if r in PTS:
                vts[vstart(PTS[r][0])] += 1
        if not vts:
            print(f"  {c:34s} non-virtual leaves only (no vtable)")
            continue
        vt = max(vts, key=vts.get)
        rtti = vt_name.get(vt, "NONE")
        ctors = sorted(find_ctors(vt))
        cn = ", ".join(label(x) for x in ctors[:3])
        print(f"  {c:34s} vt@0x{vt:x}  RTTI={rtti}  stamped-by: {cn}")

    if "--orphans" in sys.argv:
        rtti_cls = sorted({re.match(r"\.\?A[UV](\w+)@@", s).group(1)
                           for s in td_by_va.values() if re.match(r"\.\?A[UV](\w+)@@", s)})
        matched = set()
        for nm in NAMED.values():
            m = re.search(r"@(\w+)@@", nm)
            if m:
                matched.add(m.group(1))
        orphans = [c for c in rtti_cls if c not in matched]
        print(f"\n=== {len(orphans)} RTTI class names with no matched function in src/ "
              f"(real names; many are MFC/CRT library classes) ===")
        print(", ".join(orphans))


if __name__ == "__main__":
    main()
