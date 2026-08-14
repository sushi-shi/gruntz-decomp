#!/usr/bin/env python3
"""gruntz.audit.data_denominator - what the UNENROLLED data bytes actually are.

`gruntz.core.data_universe` says how much of retail's data is enrolled (coverage).
This says what the REST is, because "about 40% covered" is only actionable once the
remainder is partitioned into work and non-work:

    unenrolled = game-visible data                 (a worklist)
               + library-private data              (a stated exclusion, per lib)
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
              that GUID and belongs to that library. Ownership does not decide
              eligibility, though: a GUID directly named by game code remains
              in the coverage denominator; only library-private GUIDs are
              excluded.
payload       cl pads between contributions with ZERO, so an uncovered all-zero
              run that nothing names is padding, and an uncovered NON-zero run
              that nothing names is a real gap in our knowledge - reported as
              UNCLASSIFIED rather than absorbed.

`.bss` is partitioned with the SAME reachability machinery but different
ownership oracles: its bytes are loader zero-fill, so payload proves nothing
and the verdict rests entirely on (a) who references the run - the `.reloc`
HIGHLOW table names every absolute-address operand and the containing
function's game/library classification is evidence - and (b) alignment: a
hole strictly smaller than the ALIGNMENT the delink manifest states for the
claim starting at the hole's end exists BECAUSE of that alignment, and is
slack. Everything else in `.bss` stays eligible (UNCLASSIFIED), fail-closed.

REACHABILITY OVERRIDES OWNERSHIP. A datum directly named by game/compiler code,
or reached through a pointer stored in enrolled/game-visible data, remains in
the reconstructable denominator even when its payload proves CRT/MFC/SDK
ownership. Traversal stops at proven library functions: calling a CRT routine
does not make every private CRT table game-visible.

TRANSITIVE ATTRIBUTION. An RTTI base-class array is named only by its class
hierarchy descriptor, which is named only by the complete object locator, which
is named by a vtable. One pass over `.text` sites seeds the labels; further
passes propagate them along data->data pointers, so the interior of a pointer
chain inherits the owner of its root instead of landing in UNCLASSIFIED.

USAGE
    python -m gruntz.audit.data_denominator             # the partition
    python -m gruntz.audit.data_denominator --worklist  # the game-data runs
    python -m gruntz.audit.data_denominator --tsv PATH  # every run, classified
    python -m gruntz.audit.data_denominator --check     # census agreement gate
    python -m gruntz.audit.data_denominator --check     # re-prove tracked partition
"""
from __future__ import annotations

import argparse
import bisect
import csv
import io
import json
import os
import re
import struct
import sys
from collections import Counter, defaultdict, deque
from pathlib import Path

from gruntz.core.data_universe import CLAIMS, enrolled_runs, regions
from gruntz.core.function_universe import classify as classify_functions
from gruntz.core.pe import PE, REPO

EH_MAGIC = 0x19930520
#: the SDK GUID archives, in the dev shell's toolchain
GUID_LIBS = ("$DXSDK_DIR/Lib/dxguid.lib", "$MSVC_DIR/lib/UUID.LIB")

# The committed census the derived partition is checked against.
CENSUS = REPO / "config/retail/data.tsv"
SYMBOLS = REPO / "build/gen/symbol_names.csv"
BASE_OBJECTS = REPO / "build/objdiff/base"
GLOBAL_TYPES = REPO / "build/gen/globals.json"

VISIBLE = "game-visible unenrolled data"
LIB = "library-private data"
EH, EHPAD = "compiler: C++ EH tables", "compiler: EH-table padding"
RTTI = "compiler: RTTI records"
LITERAL = "compiler: pooled literal data"
GUIDV = "library-private data (SDK GUID)"
PAD = "padding / alignment (zero)"
UNK = "UNCLASSIFIED (non-zero)"
UNKB = "UNCLASSIFIED (.bss)"
ORDER = [VISIBLE, UNK, UNKB, EH, RTTI, LITERAL, LIB, GUIDV, EHPAD, PAD]
ELIGIBLE = {VISIBLE, UNK, UNKB}

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


def _contiguous(values):
    """Maximal ``[lo, hi)`` runs from a set of byte RVAs."""
    out = []
    for value in sorted(values):
        if out and value == out[-1][1]:
            out[-1] = (out[-1][0], value + 1)
        else:
            out.append((value, value + 1))
    return out


def _span_covering(spans, starts, lo, hi):
    """The span containing all of ``[lo, hi)``, or ``None``."""
    i = bisect.bisect_right(starts, lo) - 1
    if i < 0:
        return None
    span = spans[i]
    return span if span[0] <= lo and hi <= span[1] else None


def _merge_rows(rows):
    """Merge adjacent rows carrying identical proof and eligibility."""
    out = []
    for row in sorted(rows):
        if out and out[-1][1] == row[0] and out[-1][2:] == row[2:]:
            out[-1] = (out[-1][0], row[1], *row[2:])
        else:
            out.append(row)
    return out


def propagate_reachability(direct_reasons, enrolled_roots, edges):
    """Return ``(visible, reasons)`` for the game-side data graph.

    ``edges`` contains data-to-data pointer edges only.  References originating
    in library code are deliberately absent, which is the boundary that keeps a
    call into CRT/MFC from pulling all of that library's private tables into the
    coverage denominator.
    """
    why = defaultdict(Counter)
    queue = deque()
    visible = set()

    def root(node, reason, count):
        why[node][reason] += count
        if node not in visible:
            visible.add(node)
            queue.append(node)

    for node, reasons in direct_reasons.items():
        for reason, count in reasons.items():
            root(node, reason, count)
    for node, count in enrolled_roots.items():
        root(node, "pointer from enrolled data", count)

    while queue:
        source = queue.popleft()
        for target, count in edges.get(source, {}).items():
            if target in visible:
                continue
            root(target, "pointer from game-visible data", count)
    return visible, why


def coverage_verdict(attribution, is_visible):
    """The coverage class for an ownership verdict and reachability result."""
    if is_visible:
        return VISIBLE, True
    return attribution, attribution in (UNK, UNKB)


def claim_alignments() -> dict[int, int]:
    """`{claim rva: stated alignment}` from the delink data manifest.

    The manifest's alignment column is c2's own placement rule
    (`data_layout.obj_align`), so a `.bss` hole strictly smaller than the
    alignment of the claim STARTING at its end is proven slack: the hole
    exists because that claim had to move up.
    """
    out: dict[int, int] = {}
    if not CLAIMS.is_file():
        return out
    with CLAIMS.open() as f:
        for r in csv.DictReader(f, delimiter="\t"):
            try:
                out[int(r["rva"], 16)] = int(r["alignment"], 0)
            except (KeyError, ValueError):
                continue
    return out


def _one_past_addend(anchor, addend):
    _rva, size, kind, element_size = anchor
    return (kind == "data" and size
            and size <= addend < size + max(1, element_size))


def _paired_one_past_sites(mine, theirs, text, pe, known):
    """Retail relocation sites proven to spell ``known datum + sizeof(datum)``.

    ``mine`` and ``theirs`` are the positionally corresponding DIR32 relocation
    lists for one candidate/retail function.  Every named candidate anchor must
    agree with the address written into retail before any result is accepted.
    This is the same self-corroborating pairing used by the FP-pool oracle; the
    returned *sites*, rather than target addresses, let another genuine reference
    to the same address remain visible.
    """
    if not mine or len(mine) != len(theirs):
        return set()
    found = set()
    for (site, symbol), target_site in zip(mine, theirs):
        at = pe.off(target_site)
        if at is None or site + 4 > len(text):
            return set()
        value = struct.unpack_from("<I", pe.data, at)[0] - pe.image_base
        anchor = known.get(symbol)
        if anchor is None:
            continue
        rva, _size, _kind, _element_size = anchor
        addend = struct.unpack_from("<I", text, site)[0]
        if value != rva + addend:
            return set()
        if _one_past_addend(anchor, addend):
            found.add(target_site)
    return found


def _unique_one_past_sites(mine, theirs, text, pe, known):
    """Fallback for a non-matching function: unique same-function identities.

    Structural reconstruction can add or remove other relocations, defeating the
    positional proof above.  A one-past identity is still unambiguous when exactly
    one candidate relocation spells ``datum + sizeof(datum)`` and exactly one
    retail relocation in the same function contains that resulting address.
    """
    candidate = Counter()
    retail = defaultdict(list)
    for site, symbol in mine:
        anchor = known.get(symbol)
        if anchor is None or site + 4 > len(text):
            continue
        rva, _size, _kind, _element_size = anchor
        addend = struct.unpack_from("<I", text, site)[0]
        if _one_past_addend(anchor, addend):
            candidate[rva + addend] += 1
    for target_site in theirs:
        at = pe.off(target_site)
        if at is not None:
            value = struct.unpack_from("<I", pe.data, at)[0] - pe.image_base
            retail[value].append(target_site)
    return {retail[value][0] for value, count in candidate.items()
            if count == 1 and len(retail.get(value, ())) == 1}


def paired_one_past_sites(pe, symbols=SYMBOLS, base_dir=BASE_OBJECTS,
                          global_types=GLOBAL_TYPES):
    """Code relocation sites whose candidate COFF proves a one-past bound.

    A PE HIGHLOW target at the byte immediately following an enrolled array is
    otherwise indistinguishable from a reference naming a missing adjacent datum.
    Candidate COFF preserves the required distinction as ``array + sizeof(array)``.
    Pair only functions with the same DIR32 count, corroborate every known anchor,
    and return the retail relocation *site* so the denominator can ignore only the
    bound expression that supplied the false reachability root.
    """
    if not Path(symbols).is_file() or not Path(base_dir).is_dir():
        return set()

    array_counts = {}
    try:
        for row in json.loads(Path(global_types).read_text()):
            match = re.search(r"\[([1-9][0-9]*)\]\s*$", row.get("type") or "")
            if match:
                array_counts[int(row["rva"], 0)] = int(match.group(1))
    except (OSError, json.JSONDecodeError, KeyError, TypeError, ValueError):
        pass

    known, fn_extent = {}, {}
    with Path(symbols).open(newline="") as stream:
        for row in csv.DictReader(line for line in stream
                                  if not line.lstrip().startswith("#")):
            try:
                rva = int(row["rva"], 0)
                size = int(row["size"], 0) if (row.get("size") or "").strip() else 0
            except (KeyError, ValueError):
                continue
            kind = (row.get("kind") or "").strip()
            count = array_counts.get(rva, 0)
            element_size = size // count if count and size % count == 0 else 1
            known[row["name"]] = (rva, size, kind, element_size)
            if kind == "func" and size:
                fn_extent[row["name"]] = (rva, size)

    sys.path.insert(0, str(REPO / "scripts/gruntz/build"))
    from coff_oracle import _Coff

    sites = pe.reloc_sites
    proven = set()
    for obj in sorted(Path(base_dir).glob("*.obj")):
        try:
            coff = _Coff(obj)
        except Exception:
            continue
        for sec in coff.section_table:
            if not sec["characteristics"] & 0x20000000:
                continue
            ptr, count, first = sec["reloc_offset"], sec["reloc_count"], 0
            if not ptr:
                continue
            if sec["characteristics"] & 0x01000000 and count == 0xFFFF:
                count = struct.unpack_from("<I", coff.buf, ptr)[0]
                first = 1
            rel = {}
            for i in range(first, count):
                site, index, typ = struct.unpack_from("<IIH", coff.buf, ptr + i * 10)
                if typ == 0x0006:  # IMAGE_REL_I386_DIR32
                    rel[site] = coff.sym_name(index)
            if not rel:
                continue
            text = coff.section_payload(sec["index"])
            for offset, name in coff.defined_symbols(sec["index"]):
                extent = fn_extent.get(name)
                if extent is None:
                    continue
                rva, size = extent
                mine = sorted((site, symbol) for site, symbol in rel.items()
                              if offset <= site < offset + size)
                lo = bisect.bisect_left(sites, rva)
                hi = bisect.bisect_left(sites, rva + size)
                theirs = sites[lo:hi]
                proven.update(_paired_one_past_sites(
                    mine, theirs, text, pe, known))
                proven.update(_unique_one_past_sites(
                    mine, theirs, text, pe, known))
    return proven


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
    one_past_sites = paired_one_past_sites(pe)

    def named_in(lo, hi):
        i = bisect.bisect_left(ntargets, lo)
        out = []
        while i < len(ntargets) and ntargets[i] < hi:
            out.append(ntargets[i])
            i += 1
        return out

    # Start with the exact gaps.  Evidence-backed object extents (EH records and
    # GUIDs) are inserted as boundaries BEFORE reachability is computed.  Without
    # this, one reference to the first GUID in a packed table would incorrectly
    # make every following private GUID game-visible.
    gaps = []
    for key in ("rdata", "data", "bss"):
        lo0, hi0 = regs[key]
        for a, b in _complement(runs, lo0, hi0):
            gaps.append((a, b, "." + key))

    eh_spans = _contiguous(owned_eh)
    guid_spans = []
    for a, b, region in gaps:
        if region == ".bss":               # loader zero-fill: no payload to scan
            continue
        r = a
        while r + 16 <= b:
            if r % 8 == 0:
                g = guids.get(pe.data[pe.off(r):pe.off(r) + 16])
                if g:
                    guid_spans.append((r, r + 16, g[0], g[1]))
                    r += 16
                    continue
            r += 1
    guid_spans.sort()

    evidence_bounds = sorted({x for a, b in eh_spans for x in (a, b)} |
                             {x for a, b, _, _ in guid_spans for x in (a, b)})
    fixed_bounds = (MFC_LO, MFC_HI, LIB_RDATA_HI, RTTI_LO, RTTI_HI)
    nodes = []
    for a, b, region in gaps:
        j = bisect.bisect_right(evidence_bounds, a)
        k = bisect.bisect_left(evidence_bounds, b)
        pts = sorted(set([a] + named_in(a + 1, b)
                         + evidence_bounds[j:k]
                         + [x for x in fixed_bounds if a < x < b])) + [b]
        for i in range(len(pts) - 1):
            nodes.append([pts[i], pts[i + 1], region])
    node_lo = [n[0] for n in nodes]

    def node_at(rva):
        i = bisect.bisect_right(node_lo, rva) - 1
        return i if i >= 0 and nodes[i][0] <= rva < nodes[i][1] else None

    enrolled_lo = [a for a, _ in runs]

    def enrolled(rva):
        i = bisect.bisect_right(enrolled_lo, rva) - 1
        return i >= 0 and runs[i][0] <= rva < runs[i][1]

    # Seed visibility from game/compiler code.  Library functions are a hard
    # traversal boundary: their private implementation references establish
    # ownership, not game visibility.
    direct_reasons: dict[int, Counter] = defaultdict(Counter)
    libof: dict[int, Counter] = {}
    for i, (a, b, _) in enumerate(nodes):
        libs = Counter()
        for t in [a] + named_in(a + 1, b):
            for s in by_target.get(t, ()):
                if not (text_lo <= s < text_hi):
                    continue
                if s in one_past_sites:
                    continue
                f = fn_at(s)
                category = f["category"] if f else "text?"
                if category == "text?":
                    # Un-inventoried .text: the $E initializer funclets live in
                    # inventory GAPS. A gap flanked by library functions on BOTH
                    # sides is library code (the MFC funclets constructing the
                    # static CWnd/CMemoryException objects live exactly there);
                    # a gap with a game-side neighbour stays unattributed.
                    k = bisect.bisect_right(fstarts, s) - 1
                    prev_f = frows[k] if k >= 0 else None
                    next_f = frows[k + 1] if k + 1 < len(frows) else None
                    if (prev_f and next_f
                            and prev_f["category"] == "library"
                            and next_f["category"] == "library"):
                        category = "library"
                        f = prev_f
                if category in ("target", "compiler", "eh"):
                    direct_reasons[i]["direct game/compiler code"] += 1
                elif category == "library":
                    libs[f.get("lib") or "?"] += 1
                elif category == "thunk":
                    # These bodies are outside the reconstruction target.  Their
                    # absolute operands are linker/library state unless an
                    # independent game-side root reaches the same datum.
                    libs["MSVC-THUNK"] += 1
        if libs:
            libof[i] = libs

    # Build data-to-data edges, then traverse only from game-side roots.  An
    # enrolled datum is a root because the source/delink manifests explicitly
    # rebuild it.  Nothing propagates out of an un-enrolled library-private node.
    enrolled_roots = Counter()
    edges = defaultdict(Counter)
    for i, (a, b, _) in enumerate(nodes):
        for t in [a] + named_in(a + 1, b):
            for s in by_target.get(t, ()):
                if text_lo <= s < text_hi:
                    continue
                if enrolled(s):
                    enrolled_roots[i] += 1
                    continue
                j = node_at(s)
                if j is not None:
                    edges[j][i] += 1
    visible, why_visible = propagate_reachability(
        direct_reasons, enrolled_roots, edges)

    def payload_nz(a, b):
        return sum(1 for r in range(a, b) if pe.data[pe.off(r)])

    rsites = pe.reloc_sites

    def relocs_in(a, b):
        """True when a HIGHLOW site's 4-byte cell overlaps ``[a, b)``."""
        i = bisect.bisect_left(rsites, a - 3)
        return i < len(rsites) and rsites[i] < b

    #: `??_7type_info@@6B@` - the vptr every `??_R0` TypeDescriptor carries.
    TYPEINFO_VTBL = 0x001EE5AC

    def is_typedesc(rva):
        """A TypeDescriptor at `rva`: type_info vptr, spare 0, a mangled
        `.`-tag (`.?AV...` for classes, `.PAX` etc. for EH catch types)."""
        o = pe.off(rva)
        if o is None or rva not in reloc_set:
            return False
        return (struct.unpack_from("<I", pe.data, o)[0] - pe.image_base
                == TYPEINFO_VTBL
                and pe.data[o + 8:o + 9] == b"."
                and 32 < pe.data[o + 9] < 127)

    reloc_set = set(rsites)

    def rtti_content(a, b, depth=0):
        """RTTI/EH-type records recognised by SHAPE outside the proven bands.

        cl scatters `??_R0` TypeDescriptors into `.data`, per-TU `??_R1..3`
        descriptors into `.rdata$r`, and the throw-type machinery
        (`_CatchableType` / `_CatchableTypeArray` / `_ThrowInfo`) into
        `.xdata$x`; the band constants cover only the biggest contribution
        group. A run that IS a TypeDescriptor, or that reaches one through
        one or two relocated cells, is such a record wherever it lives.
        """
        if is_typedesc(a):
            return "TypeDescriptor by content (type_info vptr + mangled tag)"
        i = bisect.bisect_left(rsites, a)
        if i < len(rsites) and rsites[i] < b:
            t = struct.unpack_from("<I", pe.data, pe.off(rsites[i]))[0] - pe.image_base
            if is_typedesc(t):
                return "RTTI/EH descriptor by content (cell -> TypeDescriptor)"
            # The record graph is shallow: COL -> CHD -> base-class array ->
            # ??_R1 -> TypeDescriptor is the longest path, every hop within
            # the record's first 0x10 bytes.
            if depth < 4 and pe.off(t) is not None \
                    and rtti_content(t, t + 0x10, depth + 1):
                return "RTTI/EH record by content (cell chain -> TypeDescriptor)"
        return None

    eh_starts = [a for a, _ in eh_spans]
    guid_starts = [a for a, _, _, _ in guid_spans]

    aligns = claim_alignments()
    lib_reached: dict[int, Counter] = {}   # filled by the pre-pass below

    def ownership(i, a, b, region):
        """(category, evidence, naming libraries) independent of reachability."""
        if region == ".bss":
            # Zero-fill: no payload oracle exists. Ownership comes from the
            # referencing functions (direct, or through pointers stored in data
            # already proven library-private - the CRT stdio buffers hang off
            # `__iob` this way); slack from the next claim's alignment.
            if libof.get(i):
                libs = libof[i]
                return LIB, ",".join(sorted(libs)), libs
            if i in lib_reached:
                libs = lib_reached[i]
                return (LIB, "pointer from library-private data ("
                        + ",".join(sorted(libs)) + ")", libs)
            align = aligns.get(b, 0)
            if b - a < align:
                return (PAD, f"slack below the 0x{align:x}-aligned claim "
                        f"at 0x{b:08x}", None)
            return UNKB, "no oracle names this zero-fill run", None
        eh = _span_covering(eh_spans, eh_starts, a, b)
        if eh:
            return EH, "parsed MSVC /GX table extent", None
        guid = _span_covering(guid_spans, guid_starts, a, b)
        if guid:
            lib, name = guid[2], guid[3]
            return GUIDV, f"{lib}:{name}", {lib: 1}
        nz = payload_nz(a, b)
        if RTTI_LO <= a < RTTI_HI:
            return RTTI, "retail .rdata$r contribution group", None
        rtti = rtti_content(a, b)
        if rtti:
            return RTTI, rtti, None
        if MFC_LO <= a < MFC_HI:
            return LIB, "NAFXCW:MFC message-map/data block", {"NAFXCW": 1}
        if MFC_HI <= a < LIB_RDATA_HI:
            return (LIB, "retail library tail; archive member unresolved",
                    {"unresolved static-library member": 1})
        if region == ".data" and nz:
            # The independent coverage/access sieve proved every residual
            # initialized .data run lies between pooled ??_C@ literals. Its
            # sole game-owned survivor, g_table_20fa78, is already enrolled.
            # A run CONTAINING RELOC SITES is the one disproof that needs no
            # sieve: no FP/string pool carries a relocation, so such a run is
            # a real pointer-bearing table and must be attributed, not
            # absorbed into the literal exclusion.
            if relocs_in(a, b):
                if libof.get(i):
                    libs = libof[i]
                    return LIB, ",".join(sorted(libs)), libs
                return (UNK, f"reloc-bearing run between pooled literals; "
                        f"nz={nz}", None)
            return (LITERAL, "between retail pooled literals; no source pin", None)
        if eh_lo <= a < eh_hi and nz == 0:
            return EHPAD, "inside parsed EH-table envelope", None
        if nz == 0:
            return PAD, "zero bytes with no inbound game-visible reference", None
        if libof.get(i):
            libs = libof[i]
            return LIB, ",".join(sorted(libs)), libs
        return UNK, f"nz={nz}", None

    # Library-side pointer propagation into `.bss`. A pointer WRITTEN INSIDE a
    # node already proven library-private is that library's own reference, so
    # the zero-fill it targets is library state unless something game-visible
    # also reaches it (visibility always wins; propagation never enters a
    # visible node). Initialized targets are NOT relabeled - their verdicts
    # stay content-proven. `.bss` holds no bytes, so no edge can originate
    # there and the propagation is single-hop by construction.
    for j, (a, b, region) in enumerate(nodes):
        if region == ".bss" or j in visible:
            continue
        base, _, libs = ownership(j, a, b, region)
        if base not in (LIB, GUIDV) or not libs:
            continue
        for t in edges.get(j, {}):
            if t in visible or t in lib_reached or nodes[t][2] != ".bss":
                continue
            lib_reached[t] = Counter(libs)

    buckets: Counter = Counter()
    libbytes: Counter = Counter()
    visible_libbytes: Counter = Counter()
    visible_owners: Counter = Counter()
    guidnames: Counter = Counter()
    rows = []

    def emit(a, b, region, verdict, eligible, attribution, evidence,
             reachability="", libs=None):
        buckets[verdict] += b - a
        rows.append((a, b, region, verdict,
                     "eligible" if eligible else "excluded",
                     attribution, evidence, reachability))
        if libs:
            key = "+".join(sorted(libs))
            if eligible:
                visible_libbytes[key] += b - a
            else:
                libbytes[key] += b - a

    for i, (a, b, region) in enumerate(nodes):
        base, evidence, libs = ownership(i, a, b, region)
        if base == GUIDV:
            guidnames[evidence.split(":", 1)[1]] += 1
        verdict, eligible = coverage_verdict(base, i in visible)
        if eligible and verdict == VISIBLE:
            reasons = ", ".join(f"{k} ({v})" for k, v in
                                sorted(why_visible[i].items()))
            emit(a, b, region, verdict, True, base, evidence, reasons, libs)
            visible_owners[base] += b - a
        elif eligible:
            # Unclassified bytes are deliberately eligible.  Uncertainty can
            # never be converted into a score improvement.
            emit(a, b, region, verdict, True, base, evidence)
        else:
            emit(a, b, region, verdict, False, base, evidence, libs=libs)

    rows = _merge_rows(rows)
    return {"pe": pe, "regions": regs, "buckets": buckets,
            "libbytes": libbytes, "visible_libbytes": visible_libbytes,
            "visible_owners": visible_owners, "rows": rows,
            "eh_kinds": eh_kinds, "guids": bool(guids),
            "guidnames": guidnames}


def summary(p=None):
    """The eligibility summary `gruntz.core.data_universe.measures()` consumes.

    ``{"regions": {rdata|data|bss: {"eligible_unenrolled": B, "excluded": B}},
       "categories": {verdict: B}}`` - derived LIVE from the image and the
    current enrolment; nothing here is stored, so nothing can go stale."""
    p = p or partition()
    by_region = {k: {"eligible_unenrolled": 0, "excluded": 0}
                 for k in ("rdata", "data", "bss")}
    categories: Counter = Counter()
    for a, b, region, verdict, *_ in p["rows"]:
        key = region.lstrip(".")
        field = "eligible_unenrolled" if verdict in ELIGIBLE else "excluded"
        by_region[key][field] += b - a
        categories[verdict] += b - a
    return {"regions": by_region, "categories": dict(categories)}


#: verdict -> the census kinds its bytes may carry. Content-proven classes bind
#: tightly (an RTTI run may touch the vtable its COL chain roots at); ownership
#: and eligibility verdicts are live judgments over any real datum row, so they
#: exclude only `pad`; PAD additionally tolerates `copy` - the rawsize-edge
#: GruntDirStatics copies are zero-payload data deliberately withheld from the
#: manifest (docs/data-attribution.md §2), so the derivation reads their bytes
#: as alignment zeros while the copies device claims them.
_DATUM = {"", "string", "fppool", "vtable", "rtti", "copy", "common", "guard"}
VERDICT_KINDS = {EH: {"ehtable"}, EHPAD: {"pad"}, RTTI: {"rtti", "vtable"},
                 LITERAL: {"string", "fppool"}, PAD: {"pad", "copy"},
                 VISIBLE: _DATUM, LIB: _DATUM, GUIDV: _DATUM,
                 UNK: _DATUM, UNKB: _DATUM}


def _claim_kind(rva, name, compgen):
    cls = compgen.get(rva)
    if cls:
        return cls
    if name.startswith("??_C@"):
        return "string"
    if name.startswith("??_7"):
        return "vtable"
    if name.startswith("??_R"):
        return "rtti"
    if name.startswith("??_B"):
        return "guard"
    if name.startswith("$T") or name.startswith("__real"):
        return "fppool"
    return ""


def census_check(p=None) -> list[str]:
    """Errors that fail the census gate.

    1. every enrolled claim's rva must be an admitted `data.tsv` row whose kind
       matches the claim's name class (the data analog of "every provider rva is
       a functions.tsv start");
    2. every census row overlapping a derived unenrolled run must carry a kind
       compatible with the run's verdict (content-proven rtti/ehtable/pad/
       string classes bind tightly; eligibility verdicts bind to plain rows);
    3. each region's first byte must be a row - the census is a partition, not
       a sample.
    """
    import bisect

    from gruntz.build.labels import compgen_rows
    from gruntz.core.retail_data import REGIONS, all_rows

    p = p or partition()
    errors = []
    census = all_rows()
    starts = [r["rva"] for r in census]
    by_rva = {r["rva"]: r for r in census}
    for key, (lo, _hi) in REGIONS.items():
        if lo not in by_rva:
            errors.append(f"census: region {key} does not open with a row at 0x{lo:08x}")

    compgen = {rva: cls for rva, _s, _n, _o, cls in compgen_rows()}
    claims = REPO / "build/gen/delink_data_manifest.tsv"
    if claims.is_file():
        with claims.open(newline="") as f:
            for r in csv.DictReader(f, delimiter="\t"):
                if str(r.get("provenance", "")).startswith("provisional-band-gap"):
                    continue
                rva = int(r["rva"], 16)
                row = by_rva.get(rva)
                if row is None:
                    errors.append(
                        f"claim 0x{rva:08x} {r['name']} is not an admitted "
                        f"config/retail/data.tsv row - admit the datum start")
                    continue
                want = _claim_kind(rva, r["name"], compgen)
                if row["kind"] != want:
                    errors.append(
                        f"claim 0x{rva:08x} {r['name']}: census kind "
                        f"{row['kind']!r}, expected {want!r}")

    for a, b, _region, verdict, *_ in p["rows"]:
        allowed = VERDICT_KINDS[verdict]
        i = bisect.bisect_right(starts, a) - 1
        while i < len(starts) and (i < 0 or starts[i] < b):
            if i >= 0:
                row = census[i]
                if max(row["rva"], a) < min(row["rva"] + row["size"], b) \
                        and row["kind"] not in allowed:
                    errors.append(
                        f"census row 0x{row['rva']:08x} kind {row['kind']!r} "
                        f"overlaps a derived {verdict!r} run 0x{a:08x}..0x{b:08x} "
                        f"(allowed: {sorted(allowed)})")
            i += 1
    return errors


def render_tsv(rows) -> str:
    """Canonical tracked partition text."""
    out = io.StringIO(newline="")
    w = csv.writer(out, delimiter="\t", lineterminator="\n")
    # Keep the always-populated evidence column last.  A final empty
    # reachability field would serialize as a trailing tab on every private row.
    w.writerow(["rva", "end", "size", "section", "verdict", "coverage",
                "attribution", "reachability", "evidence"])
    for (a, b, region, verdict, eligibility, attribution, evidence,
         reachability) in rows:
        w.writerow([f"0x{a:08x}", f"0x{b:08x}", b - a, region, verdict,
                    eligibility, attribution, reachability, evidence])
    return out.getvalue()


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--worklist", action="store_true",
                    help="list the game-data runs (the closeable set)")
    ap.add_argument("--unclassified", action="store_true",
                    help="list the runs no oracle explained")
    ap.add_argument("--tsv", help="write every classified run to a TSV")
    ap.add_argument("--check", action="store_true",
                     help="fail unless config/retail/data.tsv agrees with the "
                          "derived partition and the current enrolment")
    ap.add_argument("--limit", type=int, default=40)
    args = ap.parse_args()

    p = partition()
    regs = p["regions"]
    per_scope = {"initialized data": Counter(), ".bss (zero fill)": Counter()}
    for a, b, region, verdict, *_ in p["rows"]:
        scope = ".bss (zero fill)" if region == ".bss" else "initialized data"
        per_scope[scope][verdict] += b - a
    for scope, keys in (("initialized data", ("rdata", "data")),
                        (".bss (zero fill)", ("bss",))):
        buckets = per_scope[scope]
        retail = sum(regs[k][1] - regs[k][0] for k in keys)
        unenrolled = sum(buckets.values())
        eligible_unenrolled = sum(buckets[k] for k in ELIGIBLE if k in buckets)
        excluded = unenrolled - eligible_unenrolled
        enrolled = retail - unenrolled
        eligible_retail = enrolled + eligible_unenrolled
        parts = " + ".join(f".{k} {regs[k][1]-regs[k][0]:,}" for k in keys)
        print(f"retail {scope:21} {retail:>9,} B  ({parts})")
        print(f"enrolled                     {enrolled:>9,} B  "
              f"({100.0*enrolled/retail:.2f}% gross coverage)")
        print(f"UNENROLLED                   {unenrolled:>9,} B  "
              f"({100.0*unenrolled/retail:.2f}%), partitioned:")
        for k in ORDER:
            if buckets.get(k):
                print(f"    {k:32} {buckets[k]:>9,} B  "
                      f"{100.0*buckets[k]/unenrolled:5.1f}% of unenrolled")
        print(f"proven private/excluded      {excluded:>9,} B")
        print(f"eligible retail denominator  {eligible_retail:>9,} B")
        print(f"RECONSTRUCTABLE COVERAGE      {enrolled:>9,} / "
              f"{eligible_retail:,} B  ({100.0*enrolled/eligible_retail:.2f}%)")
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
    print("\n  excluded library-private attribution:")
    for k, v in p["libbytes"].most_common():
        print(f"    {k:32} {v:>9,.0f} B")
    if p["visible_libbytes"]:
        print("\n  game-visible library data (KEPT in coverage):")
        for k, v in p["visible_libbytes"].most_common():
            print(f"    {k:32} {v:>9,.0f} B")

    if args.worklist or args.unclassified:
        want = ELIGIBLE if args.worklist else {UNK, UNKB}
        rows = sorted((r for r in p["rows"] if r[3] in want),
                      key=lambda r: -(r[1] - r[0]))
        title = ("eligible unenrolled worklist" if args.worklist
                 else "UNCLASSIFIED")
        print(f"\n{title}: {len(rows)} runs, {sum(r[1]-r[0] for r in rows):,} B")
        for a, b, region, verdict, _, attribution, evidence, reach in rows[:args.limit]:
            proof = "; ".join(x for x in (attribution, evidence, reach) if x)
            print(f"  {a:#010x}..{b:#010x} {b-a:>7,} B  {region:<7} "
                  f"{verdict}: {proof}")
        if len(rows) > args.limit:
            print(f"  ... {len(rows)-args.limit} more (--limit)")

    if args.tsv:
        Path(args.tsv).write_text(render_tsv(p["rows"]))
        print(f"\nwrote {args.tsv}")
    if args.check:
        if not p["guids"]:
            print("cannot check the census without the pinned SDK GUID "
                  "archives", file=sys.stderr)
            return 1
        errors = census_check(p)
        if errors:
            print(f"data census: {len(errors)} disagreement(s) with "
                  "config/retail/data.tsv:", file=sys.stderr)
            for e in errors[:60]:
                print(f"  {e}", file=sys.stderr)
            if len(errors) > 60:
                print(f"  ... {len(errors) - 60} more", file=sys.stderr)
            return 1
        print(f"\ndata census: {len(p['rows']):,} derived ranges agree with "
              "config/retail/data.tsv")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
