#!/usr/bin/env python3
"""gruntz.audit.data_denominator - what the UNENROLLED data bytes actually are.

`gruntz.core.data_universe` says how much of retail's data is enrolled (coverage).
This says what the REST is, because "about 40% covered" is only actionable once the
remainder is partitioned into work and non-work:

    unenrolled = game data we can pin              (a worklist)
               + library data we can never pin     (a stated exclusion, per lib)
               + compiler-generated data           (cl emits it; we do not pin it)
               + padding
               + what we cannot classify           (stated, never folded into
                                                    "library" to make it tidy)

EVERY VERDICT COMES FROM RETAIL OR THE TOOLCHAIN, NEVER FROM `src/`
-------------------------------------------------------------------
`.reloc`      the HIGHLOW table is exhaustive for absolute address operands, so
              the stored dword at each site is an address the image POINTS AT.
              Map the site back to its containing function and the retail
              function inventory says whether that function is a reconstruction
              target or FID-identified library code. That is the primary
              attribution and it is evidence, not a name heuristic.
`FuncInfo`    every MSVC `/GX` EH table starts `0x19930520`; its maxState /
              nTryBlocks / nCatches fields give the EXACT extent of the unwind
              map, try-block map and handler arrays it owns. 973 records in this
              image, 972 of them in the unenrolled `.rdata` tail.
`dxguid.lib`  a COM GUID is 16 bytes of ground truth. If retail's bytes equal a
`UUID.LIB`    16-byte symbol payload in the SDK's GUID libraries, the datum IS
              that GUID and it belongs to the library, no matter that the code
              which LEAs it is ours. This is why the naming-function test alone
              is not enough: `CDDrawMgr::Init` naming `IID_IDirectDraw2` would
              otherwise make a dxguid table look like game data.
payload       cl pads between contributions with ZERO, so an uncovered all-zero
              run that nothing names is padding, and an uncovered NON-zero run
              that nothing names is a real gap in our knowledge - reported as
              UNCLASSIFIED rather than absorbed.

TRANSITIVE ATTRIBUTION. An RTTI base-class array is named only by its class
hierarchy descriptor, which is named only by the complete object locator, which
is named by a vtable. One pass over `.text` sites seeds the labels; further
passes propagate them along data->data pointers, so the interior of a pointer
chain inherits the owner of its root instead of landing in UNCLASSIFIED.

USAGE
    python -m gruntz.audit.data_denominator             # the partition
    python -m gruntz.audit.data_denominator --worklist  # the game-data runs
    python -m gruntz.audit.data_denominator --tsv PATH  # every run, classified
"""
from __future__ import annotations

import argparse
import bisect
import csv
import os
import struct
import sys
from collections import Counter, defaultdict
from pathlib import Path

from gruntz.core.data_universe import enrolled_runs, regions
from gruntz.core.function_universe import classify as classify_functions
from gruntz.core.pe import PE, REPO

EH_MAGIC = 0x19930520
#: the SDK GUID archives, in the dev shell's toolchain
GUID_LIBS = ("$DXSDK_DIR/Lib/dxguid.lib", "$MSVC_DIR/lib/UUID.LIB")

GAME, LIB = "game data (worklist)", "library data"
EH, EHPAD = "compiler: C++ EH tables", "compiler: EH-table padding"
RTTI = "compiler: RTTI records"
LITERAL = "compiler: pooled literal data"
GUIDV = "library data (SDK GUID)"
PAD = "padding / alignment (zero)"
TGTUNK = "UNCLASSIFIED (target-referenced)"
UNK = "UNCLASSIFIED (non-zero)"
ORDER = [EH, RTTI, LITERAL, LIB, GUIDV, EHPAD, PAD, TGTUNK, UNK, GAME]

# Proven retail contribution-group boundaries (docs/link-section-audit.md).
MFC_LO, MFC_HI = 0x001EB068, 0x001EE5A4
LIB_RDATA_HI = 0x001F1F20
RTTI_LO, RTTI_HI = 0x001F1F20, 0x001F7AD0


# --------------------------------------------------------------------------- #
# the toolchain GUID oracle
# --------------------------------------------------------------------------- #
def _ar_members(data: bytes):
    if data[:8] != b"!<arch>\n":
        return
    off, longnames = 8, b""
    while off + 60 <= len(data):
        hdr = data[off:off + 60]
        name = hdr[0:16].decode("latin1").strip()
        try:
            size = int(hdr[48:58].decode("latin1").strip() or 0)
        except ValueError:
            return
        body = data[off + 60:off + 60 + size]
        if name.startswith("//"):
            longnames = body
        elif name.startswith("/") and name[1:].isdigit():
            n = int(name[1:])
            yield longnames[n:longnames.find(b"\0", n)].decode("latin1"), body
        elif name not in ("/", ""):
            yield name.rstrip("/"), body
        off += 60 + size + (size & 1)


def _coff_data_symbols(b: bytes, width: int = 16):
    """`(name, payload)` for every EXTERNAL symbol with `width` bytes behind it."""
    if len(b) < 20:
        return
    mach, nsec, _, symptr, nsym, osz, _ = struct.unpack_from("<HHIIIHH", b, 0)
    if mach != 0x14C or not symptr:
        return
    secs = []
    for i in range(nsec):
        o = 20 + osz + i * 40
        secs.append(struct.unpack_from("<II", b, o + 16))     # (size, ptr)
    strtab = symptr + nsym * 18
    i = 0
    while i < nsym:
        o = symptr + i * 18
        raw = b[o:o + 8]
        if raw[:4] == b"\0\0\0\0":
            so = struct.unpack_from("<I", raw, 4)[0]
            name = b[strtab + so:b.find(b"\0", strtab + so)].decode("latin1")
        else:
            name = raw.rstrip(b"\0").decode("latin1")
        val, snum, _, cls, naux = struct.unpack_from("<IhHBB", b, o + 8)
        if cls == 2 and 1 <= snum <= len(secs):
            size, ptr = secs[snum - 1]
            if ptr and val + width <= size:
                yield name, b[ptr + val:ptr + val + width]
        i += 1 + naux


def guid_table() -> dict[bytes, tuple[str, str]]:
    """`{16 raw bytes: (lib, symbol)}` from the SDK's GUID archives.

    Empty (with a warning) when the toolchain is not on the environment - the
    partition then reports those bytes under whatever the naming-function test
    says, which is a WEAKER answer, so the absence is stated rather than hidden.
    """
    out: dict[bytes, tuple[str, str]] = {}
    for spec in GUID_LIBS:
        p = Path(os.path.expandvars(spec))
        if "$" in str(p) or not p.is_file():
            continue
        for _, body in _ar_members(p.read_bytes()):
            for name, payload in _coff_data_symbols(body):
                if any(payload):                      # GUID_NULL is not evidence
                    out.setdefault(payload, (p.name, name))
    return out


# --------------------------------------------------------------------------- #
# the /GX EH tables
# --------------------------------------------------------------------------- #
def eh_bytes(pe: PE) -> tuple[set[int], Counter]:
    """Every byte owned by an MSVC `/GX` EH table, and the per-kind census.

    FuncInfo is 8 dwords; its `maxState`/`nTryBlocks`/`nIPMapEntries` counts give
    the exact extent of the arrays it points at, so this is a parse, not a
    heuristic span.
    """
    sec = {s["name"]: s for s in pe.sections}
    rd = sec[".rdata"]
    buf = pe.data[rd["raw_offset"]: rd["raw_offset"] + rd["virtual_size"]]
    ib = pe.image_base

    def dw(rva):
        o = pe.off(rva)
        return struct.unpack_from("<I", pe.data, o)[0] if o is not None else 0

    owned: set[int] = set()
    kinds: Counter = Counter()

    def own(lo, n, kind):
        if lo < rd["rva"] or n <= 0 or lo + n > rd["rva"] + rd["virtual_size"]:
            return
        owned.update(range(lo, lo + n))
        kinds[kind] += n

    i, needle = 0, struct.pack("<I", EH_MAGIC)
    magics = []
    while True:
        j = buf.find(needle, i)
        if j < 0:
            break
        magics.append(rd["rva"] + j)
        i = j + 4
    for m in magics:
        own(m, 0x20, "FuncInfo")
        max_state, p_unwind = dw(m + 4), dw(m + 8)
        n_try, p_try = dw(m + 0xC), dw(m + 0x10)
        n_ip, p_ip = dw(m + 0x14), dw(m + 0x18)
        if p_unwind and max_state < 4096:
            own(p_unwind - ib, 8 * max_state, "UnwindMap")
        if p_try and n_try < 4096:
            own(p_try - ib, 0x14 * n_try, "TryBlockMap")
            for t in range(n_try):
                e = p_try - ib + 0x14 * t
                n_catch, p_hand = dw(e + 0xC), dw(e + 0x10)
                if p_hand and n_catch < 4096:
                    own(p_hand - ib, 0x10 * n_catch, "HandlerArray")
        if p_ip and n_ip < 65536:
            own(p_ip - ib, 8 * n_ip, "IPtoStateMap")
    kinds["records"] = len(magics)
    return owned, kinds


# --------------------------------------------------------------------------- #
# the partition
# --------------------------------------------------------------------------- #
def _complement(runs, lo, hi):
    out, cur = [], lo
    for a, b in runs:
        a2, b2 = max(a, lo), min(b, hi)
        if a2 >= b2:
            continue
        if a2 > cur:
            out.append((cur, a2))
        cur = max(cur, b2)
    if cur < hi:
        out.append((cur, hi))
    return out


def partition():
    pe = PE()
    regs = regions(pe)
    runs = enrolled_runs()
    if not runs:
        sys.exit("no build/gen/delink_data_manifest.tsv - run `gruntz build` first")
    sec = {s["name"]: s for s in pe.sections}
    text_lo = sec[".text"]["rva"]
    text_hi = text_lo + sec[".text"]["virtual_size"]
    ib = pe.image_base

    owned_eh, eh_kinds = eh_bytes(pe)
    eh_lo, eh_hi = (min(owned_eh), max(owned_eh) + 1) if owned_eh else (0, 0)
    guids = guid_table()

    frows = sorted(classify_functions(REPO)[0], key=lambda r: r["rva"])
    fstarts = [r["rva"] for r in frows]

    def fn_at(rva):
        i = bisect.bisect_right(fstarts, rva) - 1
        if i < 0:
            return None
        r = frows[i]
        return r if r["rva"] <= rva < r["rva"] + r["size"] else None

    by_target = defaultdict(list)
    for s in pe.reloc_sites:
        o = pe.off(s)
        if o is not None:
            by_target[struct.unpack_from("<I", pe.data, o)[0] - ib].append(s)
    ntargets = sorted(by_target)

    def named_in(lo, hi):
        i = bisect.bisect_left(ntargets, lo)
        out = []
        while i < len(ntargets) and ntargets[i] < hi:
            out.append(ntargets[i])
            i += 1
        return out

    # unenrolled runs, split at every address the image names (an object start)
    nodes = []
    for key in ("rdata", "data"):
        lo0, hi0 = regs[key]
        for a, b in _complement(runs, lo0, hi0):
            boundaries = (MFC_LO, MFC_HI, LIB_RDATA_HI, RTTI_LO, RTTI_HI)
            pts = sorted(set([a] + named_in(a + 1, b)
                             + [x for x in boundaries if a < x < b])) + [b]
            for i in range(len(pts) - 1):
                nodes.append([pts[i], pts[i + 1], "." + key])
    node_lo = [n[0] for n in nodes]

    def node_at(rva):
        i = bisect.bisect_right(node_lo, rva) - 1
        return i if i >= 0 and nodes[i][0] <= rva < nodes[i][1] else None

    enrolled_lo = [a for a, _ in runs]

    def enrolled(rva):
        i = bisect.bisect_right(enrolled_lo, rva) - 1
        return i >= 0 and runs[i][0] <= rva < runs[i][1]

    # seed: a `.text` operand that names the run, classified by its function
    label: dict[int, str] = {}
    libof: dict[int, Counter] = {}
    for i, (a, b, _) in enumerate(nodes):
        cats, libs = Counter(), Counter()
        for t in [a] + named_in(a + 1, b):
            for s in by_target.get(t, ()):
                if not (text_lo <= s < text_hi):
                    continue
                f = fn_at(s)
                cats[f["category"] if f else "text?"] += 1
                if f and f["category"] == "library":
                    libs[f.get("lib") or "?"] += 1
        if cats.get("target"):
            # A game function naming a datum proves use, not ownership. DirectX
            # GUIDs are a concrete counterexample, so keep this set unresolved
            # until a type/owner oracle identifies it.
            label[i] = TGTUNK
        elif cats.get("library"):
            label[i], libof[i] = LIB, libs

    # propagate along data -> data pointers
    for _ in range(16):
        changed = 0
        for i, (a, b, _) in enumerate(nodes):
            if i in label:
                continue
            cats, libs = Counter(), Counter()
            for t in [a] + named_in(a + 1, b):
                for s in by_target.get(t, ()):
                    if text_lo <= s < text_hi:
                        continue
                    j = node_at(s)
                    if j is not None and j in label:
                        cats[label[j]] += 1
                        libs.update(libof.get(j, ()))
                    elif enrolled(s):
                        cats[TGTUNK] += 1
            if cats.get(TGTUNK):
                label[i] = TGTUNK
                changed += 1
            elif cats.get(LIB):
                label[i], libof[i] = LIB, libs
                changed += 1
        if not changed:
            break

    def payload_nz(a, b):
        return sum(1 for r in range(a, b) if pe.data[pe.off(r)])

    buckets: Counter = Counter()
    libbytes: Counter = Counter()
    guidnames: Counter = Counter()
    rows = []

    def emit(a, b, region, verdict, note, libs=None):
        buckets[verdict] += b - a
        rows.append((a, b, region, verdict, note))
        if libs:
            # Never fractionally spread a run across naming libraries: that
            # manufactures precision. Preserve a joint provenance until archive
            # member evidence can separate it.
            libbytes["+".join(sorted(libs))] += b - a

    for i, (a, b, region) in enumerate(nodes):
        # Carve, in evidence order: EH tables (a parse), then SDK GUIDs (an exact
        # 16-byte identity). Both can sit INSIDE a longer run - a dxguid table is
        # 29 consecutive GUIDs with only its first entry named - so they are cut
        # out of the run rather than tested against the whole of it.
        cut: list[tuple[int, int, str, str]] = []
        r = a
        while r < b:
            if r in owned_eh:
                e = r
                while e < b and e in owned_eh:
                    e += 1
                cut.append((r, e, EH, ""))
                r = e
                continue
            if r % 8 == 0 and r + 16 <= b:
                g = guids.get(pe.data[pe.off(r):pe.off(r) + 16])
                if g:
                    cut.append((r, r + 16, GUIDV, f"{g[0]}:{g[1]}"))
                    guidnames[g[1]] += 1
                    r += 16
                    continue
            r += 1
        # the remainder, between the carve-outs, keeps the node's own verdict
        pieces: list[tuple[int, int]] = []
        cur = a
        for lo, hi, _, _ in cut:
            if lo > cur:
                pieces.append((cur, lo))
            cur = hi
        if cur < b:
            pieces.append((cur, b))
        for lo, hi, verdict, note in cut:
            emit(lo, hi, region, verdict, note,
                 {note.split(":")[0]: 1} if verdict == GUIDV else None)
        for lo, hi in pieces:
            nz = payload_nz(lo, hi)
            lab = label.get(i)
            if RTTI_LO <= lo < RTTI_HI:
                emit(lo, hi, region, RTTI, "retail .rdata$r contribution group")
            elif MFC_LO <= lo < MFC_HI:
                emit(lo, hi, region, LIB, "NAFXCW:MFC message-map/data block",
                     {"NAFXCW": 1})
            elif MFC_HI <= lo < LIB_RDATA_HI:
                emit(lo, hi, region, LIB,
                     "retail library tail; archive member unresolved",
                     {"unresolved static-library member": 1})
            elif region == ".data" and nz:
                # The independent coverage/access sieve proved every residual
                # initialized .data run lies between pooled ??_C@ literals. Its
                # sole game-owned survivor, g_table_20fa78, is already enrolled.
                emit(lo, hi, region, LITERAL,
                     "between retail pooled literals; no source pin")
            elif eh_lo <= lo < eh_hi and nz == 0:
                emit(lo, hi, region, EHPAD, "")
            elif nz == 0:
                emit(lo, hi, region, PAD, "")
            elif lab == TGTUNK:
                emit(lo, hi, region, TGTUNK,
                     f"nz={nz}; use does not prove ownership")
            elif lab == LIB:
                emit(lo, hi, region, LIB, ",".join(sorted(libof.get(i, {}))),
                     libof.get(i) or {"?": 1})
            else:
                emit(lo, hi, region, UNK, f"nz={nz}")
    return {"pe": pe, "regions": regs, "buckets": buckets, "libbytes": libbytes,
            "rows": rows, "eh_kinds": eh_kinds, "guids": bool(guids),
            "guidnames": guidnames}


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--worklist", action="store_true",
                    help="list the game-data runs (the closeable set)")
    ap.add_argument("--unclassified", action="store_true",
                    help="list the runs no oracle explained")
    ap.add_argument("--tsv", help="write every classified run to a TSV")
    ap.add_argument("--limit", type=int, default=40)
    args = ap.parse_args()

    p = partition()
    regs, buckets = p["regions"], p["buckets"]
    init_retail = ((regs["rdata"][1] - regs["rdata"][0])
                   + (regs["data"][1] - regs["data"][0]))
    unenrolled = sum(buckets.values())
    print(f"retail initialized data      {init_retail:>9,} B "
          f"(.rdata {regs['rdata'][1]-regs['rdata'][0]:,} + "
          f".data {regs['data'][1]-regs['data'][0]:,})")
    print(f"enrolled                     {init_retail-unenrolled:>9,} B  "
          f"({100.0*(init_retail-unenrolled)/init_retail:.2f}%)")
    print(f"UNENROLLED                   {unenrolled:>9,} B  "
          f"({100.0*unenrolled/init_retail:.2f}%), partitioned:")
    for k in ORDER:
        if buckets.get(k):
            print(f"    {k:32} {buckets[k]:>9,} B  "
                  f"{100.0*buckets[k]/unenrolled:5.1f}% of unenrolled")
    print()
    ek = p["eh_kinds"]
    print(f"  EH tables: {ek.get('records', 0)} `/GX` FuncInfo records "
          + ", ".join(f"{k} {v:,} B" for k, v in ek.items() if k != "records"))
    if not p["guids"]:
        print("  WARNING: $DXSDK_DIR/$MSVC_DIR not on the environment - the SDK GUID "
              "oracle is OFF, so GUID tables fall back to the (weaker) "
              "naming-function verdict.")
    else:
        print(f"  SDK GUIDs matched byte-for-byte: {sum(p['guidnames'].values())}")
    print("\n  library attribution (bytes, split across the naming libraries):")
    for k, v in p["libbytes"].most_common():
        print(f"    {k:16} {v:>9,.0f} B")

    if args.worklist or args.unclassified:
        want = GAME if args.worklist else UNK
        rows = sorted((r for r in p["rows"] if r[3] == want),
                      key=lambda r: -(r[1] - r[0]))
        print(f"\n{want}: {len(rows)} runs, {sum(r[1]-r[0] for r in rows):,} B")
        for a, b, region, _, note in rows[:args.limit]:
            print(f"  {a:#010x}..{b:#010x} {b-a:>7,} B  {region:<7} {note}")
        if len(rows) > args.limit:
            print(f"  ... {len(rows)-args.limit} more (--limit)")

    if args.tsv:
        with open(args.tsv, "w", newline="") as f:
            w = csv.writer(f, delimiter="\t")
            w.writerow(["rva", "end", "size", "section", "verdict", "evidence"])
            for a, b, region, verdict, note in sorted(p["rows"]):
                w.writerow([f"0x{a:08x}", f"0x{b:08x}", b - a, region, verdict, note])
        print(f"\nwrote {args.tsv}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
