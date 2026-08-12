#!/usr/bin/env python3
"""image_diff.py - a per-segment DIFF of the candidate EXE against retail.

Three questions, three tools, and only the third one is answered here:

  objdiff              does each OBJECT match the object carved out of retail?
  audit.link_sections  is each SECTION the SIZE retail shipped?
  audit.image_diff     WHICH BYTES of each section differ, and WHY?

A section size delta is a number, not an explanation, and size parity is not a
byte match - two images can agree on every section length and share almost no
content.  So every section gets BOTH halves:

  * a SIZE attribution - named buckets that must sum to the observed delta.
    The sum is asserted in code, and whatever is left over is printed as an
    `unattributed` row rather than quietly dropped.  That remainder is the
    worklist; understating it would be worse than a large one.

  * a BYTE SIMILARITY - measured the way objdiff measures a function, lifted to
    the linked image.  Regions are PAIRED BY SYMBOL (never by file offset: our
    .text is ~26 KB short, so everything downstream sits at a different RVA and
    a linear differ would report ~0% for the whole image, which is true and
    useless).  Inside a pair, address operands are MASKED: a relocated dword or
    an E8/E9 rel32 field is resolved to a SYMBOL on each side and the two names
    are compared, so pointing at the same thing at a different address counts as
    a match and pointing at the WRONG thing counts as a mismatch.

The headline per section is `retail reproduced` = matching bytes / retail's
section size.  It charges us for retail bytes we never paired (the ~103 KB of
never-carved .text is precisely what per-object objdiff structurally cannot see:
an unclaimed hole has no object to score) and it cannot be inflated by emitting
content retail does not have - that shows up in the separate `candidate-only`
row.

Sections whose content is STRUCTURED rather than symbol-addressed get a
structural pairing instead: `.idata` pairs by (dll, import name), `.rsrc` by
resource (type, name, language) path, `.reloc` by the site's offset inside its
paired region.  Where nothing can be aligned honestly - `.bss` zero fill, the
`.text$x` unwind funclets, which are not public symbols in either image - the
row says UNMEASURED and says why.  An unmeasured row is a result; a fabricated
percentage is not.

Usage:
    python -m gruntz.audit.image_diff                # the report
    python -m gruntz.audit.image_diff --section .text --detail 20
    python -m gruntz.audit.image_diff --selftest     # plant a defect, find it
    python -m gruntz.audit.image_diff --tsv out.tsv --json out.json

Inputs (all products of `gruntz build` + `ninja candidate`):
    build/exe/GRUNTZ.EXE, GRUNTZ.candidate.EXE, GRUNTZ.candidate.map
    config/retail/functions.tsv, config/retail/library_labels.csv
    build/gen/symbol_names.csv, build/gen/delink_data_manifest.tsv

Companion write-ups: docs/link-section-audit.md (the size half),
docs/image-diff.md (this tool's method and current numbers).
"""
import argparse
import csv
import json
import struct
import sys
from collections import Counter, defaultdict
from pathlib import Path

from gruntz.core import exe_map
from gruntz.core.pe import PE

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
RETAIL = REPO / "build/exe/GRUNTZ.EXE"
CAND = REPO / "build/exe/GRUNTZ.candidate.EXE"
CMAP = REPO / "build/exe/GRUNTZ.candidate.map"
DATAMAN = REPO / "build/gen/delink_data_manifest.tsv"

IMAGEBASE = 0x400000
FILLER = (0xCC, 0x90)

# What may appear in a C string literal.  `\t\n\r` are text, and testing for
# printable-only demoted every literal carrying one to a raw byte window.
TEXT_BYTES = frozenset(range(0x20, 0x7F)) | {0x09, 0x0A, 0x0D}
_ESCAPES = ((("\\", "\\\\")), ("\n", "\\n"), ("\r", "\\r"), ("\t", "\\t"))

# Suppression classes: things we deliberately do NOT count as a byte defect.
# Every one is counted and printed on every run so it can be re-argued.
SUPPRESSIONS = {
    "reloc-slot symbol match": "a relocated dword naming the same symbol on both sides",
    "rel32 slot symbol match": "an E8/E9 displacement reaching the same symbol",
    "slot content match": "an unnamed target (pool constant / literal) whose bytes agree",
    "trailing filler": "0xCC / 0x90 the linker leaves after a contribution",
    "placement shift": "a stored RVA differing by exactly the section placement delta",
    "relocated pool window": "an unnamed target whose bytes ARE relocated addresses:"
                             " a placement fact, so UNDECIDABLE, not a mismatch",
    "interior self-reference": "an operand naming its OWN region at an offset (a"
                               " switch jump table): same referent, codegen offset",
}


# --------------------------------------------------------------------- helpers
def sect(pe, name):
    for s in pe.sections:
        if s["name"] == name:
            return s
    return None


def raw(pe, s):
    """The section's FILE bytes (never the virtual tail, which has none)."""
    return pe.data[s["raw_offset"]: s["raw_offset"] + s["raw_size"]]


def strip_filler(buf, lo, hi):
    """`hi` pulled back over a trailing 0xCC/0x90 run in `buf[lo:hi]`."""
    while hi > lo and buf[hi - 1] in FILLER:
        hi -= 1
    return hi


# ------------------------------------------------------------------- the sides
class Side:
    """One image plus everything needed to name an address inside it.

    Both sides are built through the same interface so retail and candidate are
    always measured by identical code - the rule the link audit already follows.
    """

    def __init__(self, tag, pe):
        self.tag = tag
        self.pe = pe
        self.base = pe.image_base
        self.reloc = set(pe.reloc_sites)
        self.syms = []                 # sorted [(rva, end, primary_name)]
        self.names = defaultdict(list)  # name -> [rva]
        self.aka = defaultdict(set)    # rva -> {all names at that rva}
        self._starts = []
        self.ilt_lo = self.ilt_hi = 0
        # cross-side name canonicalisation: one address can carry several names
        # (`??_E`/`??_G`), and the two images need not pick the same one as the
        # primary.  Every paired region maps the candidate's aliases onto the
        # retail primary so a masked slot naming the same thing tokenises alike.
        self.canon = {}
        # NAMING extent, separate from the pairing extent.  The candidate's only
        # size source is "up to the next public", which swallows every unnamed
        # neighbour - a reference to an FP pool constant then reads as
        # `??_7CUserBase@@6B@+0x14` on our side and `$T1994952` on retail's, and
        # a symmetric method must not turn that into a mismatch.  A paired
        # symbol therefore takes the OTHER image's extent (same object, same
        # size), and anything past it resolves as unnamed on both sides.
        self.label_end = {}
        # names whose region actually paired: only those can decide a mismatch
        self.pairable = set()

    # --- construction ----------------------------------------------------
    def add(self, rva, end, name):
        self.aka[rva].add(name)
        self.names[name].append(rva)

    def seal(self):
        """Freeze the symbol table: one region per address, names deduped."""
        by_rva = {}
        for rva, end, name in self._pending:
            if rva in by_rva:
                by_rva[rva] = (max(by_rva[rva][0], end), by_rva[rva][1])
            else:
                by_rva[rva] = (end, name)
        self.syms = sorted((r, e, n) for r, (e, n) in by_rva.items())
        self._starts = [s[0] for s in self.syms]

    # --- naming ----------------------------------------------------------
    def sym_at(self, rva):
        """(start, name) of the symbol covering `rva`, or None."""
        import bisect
        i = bisect.bisect_right(self._starts, rva) - 1
        if i < 0:
            return None
        st, en, nm = self.syms[i]
        en = self.label_end.get(st, en)
        return (st, nm) if rva < en else None

    def import_thunk(self, rva):
        """`FF 25 <IAT slot>` - the import stub.  Named from the IMPORT, not
        from a symbol table, so the two images agree without one."""
        o = self.pe.off(rva)
        if o is None or self.pe.data[o] != 0xFF or self.pe.data[o + 1] != 0x25:
            return None
        slot = struct.unpack_from("<I", self.pe.data, o + 2)[0] - self.base
        return self.iat.get(slot)

    def ilt_target(self, rva):
        """Follow a leading-band `E9 rel32` forwarder; None if it is not one."""
        if not (self.ilt_lo <= rva < self.ilt_hi):
            return None
        o = self.pe.off(rva)
        if o is None or self.pe.data[o] != 0xE9:
            return None
        return rva + 5 + struct.unpack_from("<i", self.pe.data, o + 1)[0]

    def label(self, va):
        """('sym'|'anon'|'off', text, via_thunk) for an absolute address.

        'sym'  - resolved to a named region (with +offset when interior)
        'anon' - inside the image but unnamed (a pool constant, a literal, an
                 unclaimed hole); the caller falls back to content comparison
        'off'  - not an address in this image at all

        An incremental-link thunk is FOLLOWED and does not change the name.  It
        is a link artifact: retail reaches a static-library body directly while
        we reach the same body through a thunk (only command-line objects get
        one), and calling the same function is not a difference.  `via_thunk`
        records the indirection so the asymmetry is still counted.
        """
        rva = va - self.base
        s = self.pe.sec_of(rva)
        if s is None:
            return ("off", "%#x" % va, False)
        t = self.ilt_target(rva)
        if t is not None:
            k, n, _ = self.label(t + self.base)
            return (k, n, True)
        imp = self.import_thunk(rva)
        if imp is not None:
            return ("sym", "__imp_" + imp, True)
        hit = self.sym_at(rva)
        if hit is None:
            return ("anon", s[0], False)
        st, nm = hit
        nm = self.canon.get(nm, nm)
        return ("sym", nm if st == rva else "%s+%#x" % (nm, rva - st), False)

    def relocated_window(self, va, n=8, lead=3):
        """Does a base relocation overlap [va-lead, va+n)?

        If it does, the bytes there are a PLACEMENT fact - the linker wrote our
        section addresses into them - so they can never agree across the two
        images and are no evidence about the referent at all.  `lead=0` asks
        only about the window itself, which is what the string test wants: a
        literal butted up against a preceding pointer is still a literal.
        """
        rva = va - self.base
        return any((rva + k) in self.reloc for k in range(-lead, max(n, 1)))

    def content(self, va, n=8, smax=256):
        """A comparable fingerprint of what an unnamed address points AT.

        A NUL-terminated printable run compares as its TEXT - string literals
        are pooled and land at different addresses in the two images, and their
        `??_C@...` names exist only on the retail side.  The window for finding
        the terminator has to be long (a 7-char cap silently demoted every real
        literal to a raw byte compare); the raw fallback stays short, because a
        long window drags in whatever neighbour the pool happened to place next.

        A literal may carry `\\n`/`\\t`/`\\r`, and a printable-ONLY test demoted
        every one of those to an 8-byte raw window - on ONE side, because the
        other image's copy is named.  The escapes are text.
        """
        rva = va - self.base
        o = self.pe.off(rva)
        if o is None:
            return None
        probe = self.pe.data[o:o + smax]
        z = probe.find(b"\0")
        if z > 0 and all(b in TEXT_BYTES for b in probe[:z]):
            return ("str", bytes(probe[:z]))
        # An address can name the terminating NUL itself.  MSVC emits this for
        # the final byte load while materialising a local char array.  Treating
        # it as an 8-byte raw object compares the unrelated datum placed after
        # the literal; RegistryHelper::Open's `"Software"` terminator was the
        # decisive false positive.  A contiguous printable prefix proves that
        # the byte is only the terminator; one zero byte carries no identity,
        # so return a one-byte raw window and let `resolve()` mark it unknown.
        if z == 0 and _is_string_terminator(self.pe.data, o, smax):
            return ("raw", b"\0")
        return ("raw", bytes(probe[:n]))


def _is_string_terminator(data, off, limit=256):
    if off <= 0 or off >= len(data) or data[off] != 0:
        return False
    lo = max(0, off - limit)
    p = off - 1
    if data[p] not in TEXT_BYTES:
        return False
    while p >= lo and data[p] in TEXT_BYTES:
        p -= 1
    return p < lo or data[p] == 0


def build_retail():
    R = Side("retail", PE(str(RETAIL)))
    R._pending = []
    funcs, meta = exe_map.load()
    R.ilt_lo, R.ilt_hi = sect(R.pe, ".text")["rva"], meta["ilt_end"]
    cat = {}
    for f in funcs:
        if f["category"] == "thunk" and f["rva"] < R.ilt_hi:
            continue                      # the ILT band is its own bucket
        R._pending.append((f["rva"], f["end"], f["name"]))
        cat[f["rva"]] = f["category"]
    R.category = cat
    _add_manifest(R)
    _add_iat(R)
    R.seal()
    return R


class MissingInput(RuntimeError):
    """A build product this audit reads is absent.  A plain exception, NOT
    SystemExit: link_sections embeds this tool in the README edge and an audit
    must never be able to fail the link."""


def _add_manifest(side):
    if not DATAMAN.is_file():
        raise MissingInput(
            "missing %s - it is a `gruntz build` product (the delink edge writes "
            "it, and configure.py's orphan prune deletes it); regenerate with "
            "`gruntz build` or `python -m gruntz.build.data_manifest`" % DATAMAN)
    best = {}
    with DATAMAN.open() as f:
        for r in csv.DictReader(f, delimiter="\t"):
            a, s = int(r["rva"], 16), int(r["size"], 16)
            if a not in best or s > best[a][0]:
                best[a] = (s, r["name"])
    for a, (s, n) in best.items():
        side._pending.append((a, a + s, n))


def _add_iat(side):
    """Name every IAT slot `__imp_<import>` so an indirect call resolves."""
    pe = side.pe
    d = pe.data
    rva = struct.unpack_from("<I", d, pe._opt + 96 + 8)[0]
    o = pe.off(rva) if rva else None
    side.imports = []
    side.iat = {}
    while o is not None:
        olt, _ts, _fc, nm, fta = struct.unpack_from("<IIIII", d, o)
        if not (olt or nm or fta):
            break
        dll = pe.cstr(nm)
        ents = []
        t, slot = pe.off(olt or fta), fta
        while True:
            v = struct.unpack_from("<I", d, t)[0]
            if v == 0:
                break
            name = ("#%d" % (v & 0xFFFF) if v & 0x80000000
                    else pe.cstr((v & 0x7FFFFFFF) + 2))
            ents.append((name, slot, v & 0x7FFFFFFF if not (v & 0x80000000) else 0))
            side._pending.append((slot, slot + 4, "__imp_%s" % name))
            side.iat[slot] = name
            t += 4
            slot += 4
        side.imports.append(dict(dll=dll, olt=olt, name_rva=nm, iat=fta, entries=ents,
                                 desc=pe.off(o)))
        o += 20


MAP_GRP = __import__("re").compile(
    r"^\s*([0-9a-f]{4}):([0-9a-f]{8})\s+([0-9a-f]{8})H\s+(\S+)\s+(\S+)\s*$")
MAP_PUB = __import__("re").compile(
    r"^\s*([0-9a-f]{4}):([0-9a-f]{8})\s+(\S+)\s+([0-9a-f]{8})\s*(.*)$")


def read_map(path, pe):
    """(groups, publics) from a link .map, RVAs resolved through the section table."""
    seg = {i: s["rva"] for i, s in enumerate(pe.sections, 1)}
    groups, pubs, mode = [], [], None
    for ln in path.read_text(errors="replace").splitlines():
        if "Publics by Value" in ln:
            mode = "p"
            continue
        if ln.strip().startswith("Start") and "Length" in ln:
            mode = "g"
            continue
        if ln.strip().startswith("entry point"):
            mode = None
            continue
        if mode == "g":
            m = MAP_GRP.match(ln)
            if m and int(m.group(1), 16) in seg:
                st = seg[int(m.group(1), 16)] + int(m.group(2), 16)
                groups.append((m.group(4), st, st + int(m.group(3), 16)))
        elif mode == "p":
            m = MAP_PUB.match(ln)
            if m:
                rest = m.group(5).split()
                pubs.append((m.group(3), int(m.group(4), 16) - IMAGEBASE,
                             rest[-1] if rest else ""))
    return groups, pubs


def build_candidate():
    C = Side("candidate", PE(str(CAND)))
    C._pending = []
    groups, pubs = read_map(CMAP, C.pe)
    C.groups = {n: (lo, hi) for n, lo, hi in groups}
    C.pubs = pubs
    t = sect(C.pe, ".text")
    C.ilt_lo, C.ilt_hi = t["rva"], _ilt_end(C.pe)

    # publics -> regions: many names can share one address (?_E/?_G aliases), and
    # an extent runs to the next DISTINCT address, minus the linker's filler.
    at = defaultdict(list)
    for nm, rva, obj in pubs:
        at[rva].append((nm, obj))
    addrs = sorted(at)
    C.owner = {}
    for i, a in enumerate(addrs):
        s = C.pe.sec_of(a)
        if s is None:
            continue
        hi = min(addrs[i + 1], s[1] + max(s[2], s[4])) if i + 1 < len(addrs) \
            else s[1] + max(s[2], s[4])
        o = C.pe.off(a)
        if o is not None and s[0] in (".text", ".rdata", ".data"):
            end_off = C.pe.off(hi - 1)
            if end_off is not None:
                hi = a + (strip_filler(C.pe.data, o, end_off + 1) - o)
        for nm, obj in at[a]:
            C._pending.append((a, max(hi, a + 1), nm))
        C.owner[a] = at[a][0][1]
    _add_iat(C)
    C.seal()
    # every alias name at an address, for cross-side name matching
    for nm, rva, _ in pubs:
        C.aka[rva].add(nm)
        C.names[nm].append(rva)
    return C


def _ilt_end(pe):
    """End of the leading `E9 rel32` incremental-link thunk band (link_sections')."""
    t = sect(pe, ".text")
    d = pe.data[t["raw_offset"]: t["raw_offset"] + t["virtual_size"]]
    i = end = 0
    while i + 5 <= len(d):
        if d[i] == 0xE9:
            i += 5
            end = i
        elif d[i] in FILLER:
            i += 1
        else:
            break
    while end < len(d) and d[end] in FILLER:
        end += 1
    return t["rva"] + end


# ------------------------------------------------------------------- the pairs
class Cmp:
    """Accumulated byte verdicts for one section's paired regions.

    The denominator is always RETAIL's bytes: every byte of every paired retail
    region lands in exactly one of the buckets below, so `check()` can assert it.
    Bytes we emit inside a pair that retail does not have are `cand_extra`, and
    they never improve a score.
    """

    def __init__(self):
        self.compared = 0        # retail bytes inside a paired region
        self.equal = 0           # aligned + identical, unmasked
        self.slot_sym = 0        # masked slot, both sides name the same symbol
        self.slot_content = 0    # masked slot, unnamed but the targets agree
        self.slot_bad = 0        # masked slot, the two sides name DIFFERENT things
        self.slot_unres = 0      # masked slot, not aligned
        self.slot_undec = 0      # masked slot whose referent neither image can name
        self.differ = 0          # retail bytes with no aligned counterpart
        self.cand_extra = 0      # candidate bytes inside a pair that are not matched
        self.reloc_ok = 0        # base-reloc sites reproduced (feeds .reloc)
        self.reloc_bad = 0
        self.ref_undec = 0       # ... whose referent neither image can name
        self.ref_total = 0       # decidable address operands in paired regions
        self.ref_ok = 0          # ... reaching the same referent, same order
        self.ref_pairs_clean = 0
        self.ref_pairs_reordered = 0
        self.ref_pairs_multiplicity = 0
        self.ref_pairs_bad = 0
        self.ref_bad = []        # [(region, [retail refs], [candidate refs])]
        self.ref_reordered = []  # same shape, for the ordering-only pairs
        self.ref_multiplicity = []  # same identities, different occurrence counts
        self.classes = Counter()
        self.worst = []          # [(unmatched, name, retail_len)] for --detail

    @property
    def matching(self):
        return self.equal + self.slot_sym + self.slot_content

    def check(self):
        got = (self.equal + self.slot_sym + self.slot_content + self.slot_bad
               + self.slot_unres + self.slot_undec + self.differ)
        assert got == self.compared, (got, self.compared)


# --------------------------------------------------------------- the alignment
def align(a, b, k=8):
    """Matching runs between two byte strings, insertion/deletion tolerant.

    Fixed-offset comparison is the trap this exists to avoid: ONE extra
    instruction early in a body shifts every byte after it, and a positional
    differ then calls the whole tail different.  Measured here: objdiff scores
    `CGruntzMgr::HandleCommand` 97.4% while a positional byte diff of the same
    pair calls 90% of it different.

    Patience-style: k-grams unique on BOTH sides are anchors, the longest
    increasing subsequence of those anchors is the skeleton, and each anchor is
    then extended maximally in both directions.  O(n log n), where `difflib` is
    O(n*m) and takes 20 s on the pair above.
    """
    n, m = len(a), len(b)
    if not n or not m:
        return []
    if a == b:
        return [(0, 0, n)]
    # common prefix / suffix first - cheap and usually most of the body
    p = 0
    lim = min(n, m)
    while p < lim and a[p] == b[p]:
        p += 1
    s = 0
    while s < lim - p and a[n - 1 - s] == b[m - 1 - s]:
        s += 1
    runs = []
    if p:
        runs.append((0, 0, p))
    mid = _anchor_runs(a[p:n - s], b[p:m - s], k)
    runs += [(i + p, j + p, ln) for i, j, ln in mid]
    if s:
        runs.append((n - s, m - s, s))
    return runs


def _anchor_runs(a, b, k):
    n, m = len(a), len(b)
    if n < k or m < k:
        return []
    ia, ib = {}, {}
    for i in range(n - k + 1):
        g = a[i:i + k]
        ia[g] = i if g not in ia else -1
    for j in range(m - k + 1):
        g = b[j:j + k]
        ib[g] = j if g not in ib else -1
    anchors = sorted((i, ib[g]) for g, i in ia.items()
                     if i >= 0 and ib.get(g, -1) >= 0)
    if not anchors:
        return []
    # longest increasing subsequence on the candidate coordinate (patience)
    import bisect
    tails, prev, idx = [], [None] * len(anchors), []
    for t, (_i, j) in enumerate(anchors):
        p = bisect.bisect_left(tails, j)
        if p == len(tails):
            tails.append(j)
            idx.append(t)
        else:
            tails[p] = j
            idx[p] = t
        prev[t] = idx[p - 1] if p else None
    chain, t = [], idx[len(tails) - 1]
    while t is not None:
        chain.append(anchors[t])
        t = prev[t]
    chain.reverse()

    runs, pi, pj = [], 0, 0
    for i, j in chain:
        if i < pi or j < pj:
            continue
        si, sj = i, j
        while si > pi and sj > pj and a[si - 1] == b[sj - 1]:
            si -= 1
            sj -= 1
        ei, ej = i + k, j + k
        while ei < n and ej < m and a[ei] == b[ej]:
            ei += 1
            ej += 1
        runs.append((si, sj, ei - si))
        pi, pj = ei, ej
    return runs


# --------------------------------------------------------------- masked slots
def slots_of(side, lo, buf, code):
    """[(offset, kind, token, resolved, target_width)] - address operands.

    `token` is a 4-byte stand-in that depends only on WHAT the operand names, so
    the same referent at two different addresses tokenises identically and the
    aligner treats it as a match.  `resolved` says how the name was recovered, so
    the class counters can stay honest about which matches were earned by a
    symbol and which only by comparing the bytes at the target.
    """
    import zlib
    out = []
    n = len(buf)
    i = 0
    while i < n - 3:
        if (lo + i) in side.reloc:
            v = struct.unpack_from("<I", buf, i)[0]
            width = _target_window_width(buf, i, code)
            out.append((i, "reloc") + _token(side, v, zlib, width) + (width,))
            i += 4
            continue
        if code and i + 5 <= n and buf[i] in (0xE8, 0xE9):
            t = lo + i + 5 + struct.unpack_from("<i", buf, i + 1)[0]
            kind, nm, _via = side.label(t + side.base)
            # only a call/jmp to a symbol START is an inter-region reference; an
            # intra-body branch keeps its displacement, which is already
            # position-independent and compares on its own.
            if kind == "sym" and "+" not in nm:
                out.append((i + 1, "rel32")
                           + _token(side, t + side.base, zlib, 8) + (8,))
                i += 5
                continue
        i += 1
    return out


def _target_window_width(buf, off, code):
    """Bytes of an unnamed target that the containing instruction can read.

    The raw-content fallback is identity evidence only for bytes the operand
    actually consumes.  In particular, `fmul DWORD PTR ds:addr` reads one
    four-byte float: fingerprinting eight bytes drags in the next pooled
    literal and manufactures a wrong referent when pool order differs.

    This decoder is intentionally narrow.  Unrecognised instructions retain
    the conservative historical eight-byte window; the direct x87 real-memory
    forms below have an unambiguous width in the opcode itself.
    """
    if not code or off < 2:
        return 8
    opcode, modrm = buf[off - 2], buf[off - 1]
    if modrm & 0xC7 != 0x05:  # not a direct disp32 memory operand
        return 8
    reg = (modrm >> 3) & 7
    if opcode == 0xD8:       # m32real arithmetic
        return 4
    if opcode == 0xDC:       # m64real arithmetic
        return 8
    if opcode == 0xD9 and reg in (0, 2, 3):  # fld/fst/fstp m32real
        return 4
    if opcode == 0xDD and reg in (0, 2, 3):  # fld/fst/fstp m64real
        return 8
    return 8


UNDECIDABLE = "<?>"


def resolve(side, va, weak_n=8):
    """(descriptor, how) - WHAT this operand names, computed from ONE image.

    The precedence exists because the two images do not have the same symbol
    coverage, and an asymmetry must never be reported as a difference:

      1. `off`     - not an address in this image: the literal value.
      2. code      - a call/jmp resolves by SYMBOL only.  If the target has no
                     name both images know, the reference is UNDECIDABLE:
                     fingerprinting a function by its first bytes would match
                     every `push ebp; mov ebp,esp` in the binary.
      3. a paired non-literal symbol - the name.  ONLY a name both images know
                     can decide a mismatch; one-sided names prove nothing.
                     This must beat content sniffing: a RECT field containing
                     92 is the paired aggregate's `.top`, not the one-character
                     string `"\\"` merely because its little-endian bytes look
                     printable.  Compiler `??_C@` literal symbols continue to
                     compare by text so pooling/name asymmetry cannot matter.
      4. a string  - the TEXT, at ANY length.  Retail's manifest names literals
                     `??_C@_0M@NCPH@LogicAttack?$AA@` and our .map does not
                     publish them at all, so comparing text is both symmetric
                     and stronger than comparing either name.  The old 4-char
                     floor was one-sided in exactly the same way: retail names
                     `"A"` `??_C@_01PFH@A?$AA@`, we name it nothing, and every
                     one-character literal in the image read as a wrong
                     referent.  A RELOCATED word is never text - `6b 38 5d 00`
                     is the address 0x5d386b, not the string "k8]" - so the
                     relocation table, not the length, is what guards this.
      5. other data - the instruction's proven read width at the target (a pool
                     constant), or 8 bytes when unknown; marked weak. UNLESS
                     those bytes are themselves RELOCATED: a window over stored
                     addresses is a placement fact and must differ.
      6. no file bytes (.bss) -> UNDECIDABLE.

    UNDECIDABLE is a real answer.  It is counted, never silently folded into
    either "match" or "differ".
    """
    kind, nm, _via = side.label(va)
    if kind == "off":
        return ("%#x" % va, "constant")
    named = kind == "sym" and nm.split("+")[0] in side.pairable
    if side.pe.is_exec(va - side.base):
        return (nm, "symbol") if named else (UNDECIDABLE, "undecidable")
    # An IAT slot is named from the IMPORT TABLE on both sides, so its identity
    # is symmetric by construction and beats any content test.  It also DEFEATS
    # one: the slot holds the hint/name RVA until the loader overwrites it, it
    # carries no base relocation, and `36 55 2c 00` reads as the string "6U,".
    if named and nm.startswith("__imp_"):
        return (nm, "symbol")
    # A cross-image paired name is stronger evidence than a byte-pattern
    # heuristic.  In particular, integer fields of typed aggregates frequently
    # begin with a printable byte followed by NULs.  Content-first resolution
    # misnamed g_levelMsgRectsB.top (92) as `"\\"` and g_sndCueTag (100) as
    # `"d"`, erasing the typed identity carried by correct relocations.
    if named and not nm.split("+")[0].startswith("??_C@"):
        return (nm, "symbol")
    c = side.content(va, n=weak_n)
    if c is not None and c[0] == "str" and not side.relocated_window(va, len(c[1]), 0):
        return (_text(c[1]), "string")
    if named:
        return (nm, "symbol")
    if c is not None and any(c[1]):
        if side.relocated_window(va, len(c[1])):
            return (UNDECIDABLE, "undecidable")
        return ("<%s>" % c[1].hex(), "weak")
    # An all-zero window is not evidence: every zeroed slot in the image would
    # "agree" with every other one.  Say UNDECIDABLE instead of banking a match
    # nothing supports.
    return (UNDECIDABLE, "undecidable")


def _text(b, cap=48):
    """A literal as a one-line descriptor, escapes visible."""
    s = b.decode("latin1")[:cap]
    for a, e in _ESCAPES:
        s = s.replace(a, e)
    return '"%s"' % s


def _decidable(seq):
    """Referent sequence with the explicitly unknowable operands removed."""
    return [x for x in seq if x != UNDECIDABLE]


def _align_undecidable(rseq, cseq):
    """Remove only operands whose cross-image counterpart is unknowable.

    An operand can be named on one side while the corresponding target has no
    pairable public name on the other.  Dropping `<?>` independently leaves the
    named operand behind and manufactures a wrong-referent row.  Align the raw
    streams first, treating `<?>` as a wildcard, then discard both sides of
    that one slot.  Known-vs-known differences and known operands aligned to a
    gap remain in the streams and therefore remain actionable.

    The dynamic program strongly prefers exact known anchors, then wildcard
    pairs, then known mismatches, and gaps last.  Referent streams are small;
    doing this per paired region is cheap compared with decoding the image.
    The third return value counts retail operands that became undecidable.
    """
    # Identity is already decided when the complete DECIDABLE multisets agree.
    # Do not let wildcard placement overturn that stronger fact.  A large
    # source-order permutation can put the unknown operands beside different
    # known anchors; the dynamic program below then discards different known
    # items from the two sides and manufactures an identity defect.  The
    # LoadPickupSprites switch was the decisive counterexample: all 131 named
    # referents agree, but its differently ordered case bands made HEALTH/
    # CONVERSION appear to be replaced by DEATHTOUCH/REACTIVEARMOR.
    rknown, cknown = _decidable(rseq), _decidable(cseq)
    if Counter(rknown) == Counter(cknown):
        return rknown, cknown, max(rseq.count(UNDECIDABLE), cseq.count(UNDECIDABLE))

    n, m = len(rseq), len(cseq)
    score = [[0] * (m + 1) for _ in range(n + 1)]
    step = [[None] * (m + 1) for _ in range(n + 1)]
    for i in range(1, n + 1):
        score[i][0] = score[i - 1][0] - 1
        step[i][0] = "r"
    for j in range(1, m + 1):
        score[0][j] = score[0][j - 1] - 1
        step[0][j] = "c"
    for i in range(1, n + 1):
        for j in range(1, m + 1):
            a, b = rseq[i - 1], cseq[j - 1]
            pair = 5 if a == b and a != UNDECIDABLE else \
                1 if UNDECIDABLE in (a, b) else 0
            choices = ((score[i - 1][j - 1] + pair, "p"),
                       (score[i - 1][j] - 1, "r"),
                       (score[i][j - 1] - 1, "c"))
            score[i][j], step[i][j] = max(choices, key=lambda x: x[0])

    aligned = []
    i, j = n, m
    while i or j:
        s = step[i][j]
        if s == "p":
            aligned.append((rseq[i - 1], cseq[j - 1]))
            i -= 1
            j -= 1
        elif s == "r":
            aligned.append((rseq[i - 1], None))
            i -= 1
        else:
            aligned.append((None, cseq[j - 1]))
            j -= 1
    aligned.reverse()

    rout, cout, rundec = [], [], 0
    for a, b in aligned:
        if a == UNDECIDABLE or b == UNDECIDABLE:
            if a is not None:
                rundec += 1
            continue
        if a is not None:
            rout.append(a)
        if b is not None:
            cout.append(b)
    return rout, cout, rundec


def _reconcile_candidate_tail(rseq, cseq, tail):
    """Admit only candidate-tail referents missing from retail's multiset.

    Candidate symbol extents run to the next public and can therefore include
    compiler tables after the retail carve.  Those candidate-only tail operands
    have never been part of the referent metric.  A real call site can also move
    beyond retail's byte length when our body is longer, though; discarding it
    fabricates a missing referent.  Pull from the tail only while retail still
    has a deficit for that same identity.  A wrong or surplus tail referent
    cannot satisfy the deficit and remains outside the established scope.
    """
    need = Counter(rseq)
    have = Counter(cseq)
    out = list(cseq)
    for desc in tail:
        if have[desc] < need[desc]:
            out.append(desc)
            have[desc] += 1
    return out


def _seq_divergences(rseq, cseq, sm=None):
    """Non-equal segments of two referent sequences, in sequence order.

    Each row is (retail segment, candidate segment).  A referent that only
    MOVED appears twice, once per position - deleted where retail has it,
    inserted where we emit it - so a single transposition reads as two
    one-sided rows and 2 displaced slots in total.
    """
    if sm is None:
        import difflib
        sm = difflib.SequenceMatcher(None, rseq, cseq, autojunk=False)
    out = []
    for tag, i1, i2, j1, j2 in sm.get_opcodes():
        if tag == "equal":
            continue
        rr = rseq[i1:i2]
        cc = cseq[j1:j2]
        if rr or cc:
            out.append((rr, cc))
    return out


def _referent_relation(rseq, cseq):
    """Identity/order verdict for two complete decidable referent sequences."""
    if rseq == cseq:
        return "clean"
    if Counter(rseq) == Counter(cseq):
        return "ordering"
    # Repeating a referent a different number of times is a code-shape fact,
    # not evidence that either operand names the WRONG object.  VC5 routinely
    # changes this through tail merging and algebraic factoring.  Keep it as a
    # separately gated worklist: call/load multiplicity can still matter, but
    # it must not be described as an identity substitution.
    if set(rseq) == set(cseq):
        return "multiplicity"
    return "wrong"


def _token(side, va, zlib, weak_n):
    desc, how = resolve(side, va, weak_n)
    return (_hash(desc, zlib), how)


def _hash(s, zlib):
    return struct.pack("<I", zlib.crc32(s.encode("utf-8", "replace")) | 0x80000000)


def region_diff(R, C, rspan, cspan, cmp_, code, name="", clamp=False):
    """objdiff's method on one region pair, lifted to the linked image.

    Mask the address operands on each side independently (a relocated dword or a
    call/jmp to a symbol becomes a token naming its referent), align the two
    masked streams, then attribute every RETAIL byte: matched outside a slot,
    matched inside a slot (by symbol or by target content), or unmatched.
    """
    (rlo, rhi), (clo, chi) = rspan, cspan
    ro, co = R.pe.off(rlo), C.pe.off(clo)
    if ro is None or co is None:
        return
    rn = min(rhi - rlo, len(R.pe.data) - ro)
    cn = min(chi - clo, len(C.pe.data) - co)
    if clamp:
        # A DATUM's size is a property of the object, and pairing by name says
        # the two ARE the same object.  Our only size source is "up to the next
        # public", so an over-long candidate extent would drag the NEXT datum's
        # first word into this one's referent sequence and report it as a
        # divergence.  Code is not clamped: a body of ours may legitimately be
        # longer, and seeing that is the point.
        cn = min(cn, rn)
    if rn <= 0 or cn <= 0:
        return
    rb = R.pe.data[ro:ro + rn]
    cb = C.pe.data[co:co + cn]

    rsl = slots_of(R, rlo, rb, code)
    csl = slots_of(C, clo, cb, code)
    rm = _apply(rb, rsl)
    cm = _apply(cb, csl)
    runs = align(rm, cm)

    cover = bytearray(rn)
    for i, _j, ln in runs:
        for x in range(i, i + ln):
            cover[x] = 1
    matched = sum(ln for _i, _j, ln in runs)

    slot_bytes = 0
    unmatched_here = 0
    for off, kind, _tok, res, _width in rsl:
        if off + 4 > rn:
            continue
        slot_bytes += 4
        ok = all(cover[off + t] for t in range(4))
        if kind == "reloc":
            if ok:
                cmp_.reloc_ok += 1
            else:
                cmp_.reloc_bad += 1
        if res == "undecidable":
            cmp_.slot_undec += 4
            cmp_.classes["%s slot referent UNDECIDABLE (neither image names it)"
                         % kind] += 1
            continue
        if ok:
            if res == "symbol":
                cmp_.slot_sym += 4
                cmp_.classes["%s slot -> same symbol" % kind] += 1
            else:
                cmp_.slot_content += 4
                cmp_.classes["%s slot -> same %s" % (kind, res)] += 1
        else:
            unmatched_here += 4
            # NOT "a different referent": a correct operand inside a body whose
            # neighbouring code diverged simply falls outside every matching
            # run.  Whether the referent is right is decided by the sequence
            # test below, which does not depend on the byte alignment at all.
            if res == "symbol":
                cmp_.slot_bad += 4
                cmp_.classes["%s slot not aligned (named)" % kind] += 1
            else:
                cmp_.slot_unres += 4
                cmp_.classes["%s slot not aligned (%s)" % (kind, res)] += 1

    # --- the byte verdict, before anything can return --------------------
    plain_matched = matched - sum(
        4 for off, _k, _t, _r, _w in rsl
        if off + 4 <= rn and all(cover[off + t] for t in range(4)))
    plain_diff = rn - slot_bytes - plain_matched
    cmp_.compared += rn
    cmp_.equal += plain_matched
    cmp_.differ += plain_diff
    cmp_.cand_extra += cn - matched
    bad = plain_diff + unmatched_here
    if bad and name:
        cmp_.worst.append((bad, name, rn))

    # --- the referent SEQUENCE test ------------------------------------------
    # Independent of where the bytes landed: does this body reach the same
    # things, in the same order?  A body can be byte-shifted throughout and
    # still have a perfect referent sequence (correct code, different
    # scheduling), and it can be byte-identical in the parts that align while
    # calling the WRONG function - only this test separates the two.
    # Retail's complete carve is the identity denominator.  Candidate bytes
    # beyond that length are normally only an unmeasured size tail: our public
    # extent can swallow compiler tables that retail's carve excludes.  But a
    # genuine call site can move into that tail when our body is longer.  The
    # decisive counterexample was BlitShadedMirrored: both complete bodies have
    # 3 ConvertRowDouble and 4 ConvertRowFlip calls, while the old min(rn, cn)
    # cutoff hid one candidate pair and promoted an ordering-only region to a
    # false wrong-referent result.  Reconcile only tail identities needed to
    # fill a retail multiset deficit; surplus or wrong tail operands stay out of
    # the established metric scope.
    # An operand naming its OWN region at an interior offset is a switch jump
    # table (or a computed-goto label).  Its referent is "this function" on both
    # sides and the offset is pure codegen, so it carries no identity evidence -
    # while retail's carve routinely ENDS before its table and ours does not,
    # which made every table read as a wrong referent.  A self-call (offset 0)
    # is a real referent and stays.
    lim = min(rn, cn)
    interior = (name + "+") if name else None
    rraw = [d for d in (_desc(R, rlo, rb, sl) for sl in rsl if sl[0] + 4 <= rn)
            if not (interior and d.startswith(interior))]
    craw = [d for d in (_desc(C, clo, cb, sl) for sl in csl if sl[0] + 4 <= lim)
            if not (interior and d.startswith(interior))]
    ctail = [d for d in (_desc(C, clo, cb, sl) for sl in csl
                         if lim < sl[0] + 4 <= cn)
             if not (interior and d.startswith(interior))]
    # UNKNOWN IS NOT A REFERENT.  Keeping `<?>` in the sequence let one side's
    # extra unnamed operand split an otherwise identical decidable sequence and
    # manufactured a "different referent" row.  LoadPickupSprites was the
    # decisive counterexample: both sides reach the same complete asset-key set,
    # but two additional undecidable retail operands turned its source-order-only
    # difference into ten alleged wrong assets.  Count unknowns, then remove them
    # before making any identity or ordering claim.
    rseq, cseq, rundec = _align_undecidable(rraw, craw)
    cmp_.ref_undec += rundec
    cseq = _reconcile_candidate_tail(rseq, cseq, _decidable(ctail))
    cmp_.ref_total += len(rseq)
    if not rseq and not cseq:
        return
    relation = _referent_relation(rseq, cseq)
    if relation == "clean":
        cmp_.ref_ok += len(rseq)
        cmp_.ref_pairs_clean += 1
    else:
        import difflib
        sm = difflib.SequenceMatcher(None, rseq, cseq, autojunk=False)
        same = 0
        for b in sm.get_matching_blocks():
            same += b.size
        cmp_.ref_ok += same
        rows = [(name, rr, cc) for rr, cc in _seq_divergences(rseq, cseq, sm)]
        if relation == "ordering":
            cmp_.ref_pairs_reordered += 1
            cmp_.ref_reordered += rows
        elif relation == "multiplicity":
            cmp_.ref_pairs_multiplicity += 1
            cmp_.ref_multiplicity += rows
        else:
            cmp_.ref_pairs_bad += 1
            cmp_.ref_bad += rows


def _desc(side, lo, buf, slot):
    """The printable referent of one address operand - the sequence test's unit."""
    off, kind, _tok, _res, width = slot
    if off + 4 > len(buf):
        return UNDECIDABLE
    if kind == "reloc":
        va = struct.unpack_from("<I", buf, off)[0]
    else:
        va = lo + off + 4 + struct.unpack_from("<i", buf, off)[0] + side.base
    return resolve(side, va, width)[0]


def _apply(buf, slots):
    out = bytearray(buf)
    n = len(out)
    for off, _kind, tok, _res, _width in slots:
        if off + 4 <= n:
            out[off:off + 4] = tok
    return bytes(out)


def pair_by_name(R, C, rlo, rhi, clo, chi):
    """Regions of [rlo,rhi) and [clo,chi) matched by symbol name.

    Returns (pairs, r_unpaired_spans, c_unpaired_spans, notes).  A name present
    more than once on a side is not paired - it is counted as ambiguous, because
    guessing which copy is which would fabricate a match.
    """
    rreg = [(a, e, n) for a, e, n in R.syms if rlo <= a < rhi]
    creg = [(a, e, n) for a, e, n in C.syms if clo <= a < chi]
    rby = defaultdict(set)
    for a, _e, n in rreg:
        for nm in R.aka[a] | {n}:
            rby[nm].add(a)
    cby = defaultdict(set)
    for a, e, n in creg:
        for nm in C.aka[a] | {n}:
            cby[nm].add(a)
    cspan = {a: (a, e) for a, e, n in creg}
    used, pairs, notes = set(), [], Counter()
    for a, e, n in rreg:
        cands = set()
        for nm in R.aka[a] | {n}:
            # A repeated retail FID name is just as ambiguous as a repeated
            # candidate symbol.  The old one-sided check paired our single
            # CWinApp scalar dtor with whichever of dozens of retail lookalikes
            # happened to occur first, manufacturing a wrong-referent row.
            if rby[nm] != {a}:
                continue
            cands.update(cby.get(nm, ()))
        cands = {x for x in cands if x in cspan}
        if not cands:
            notes["retail region with no candidate symbol"] += 1
            continue
        if len(cands) > 1:
            notes["ambiguous name (not paired)"] += 1
            continue
        c = cands.pop()
        if c in used:
            notes["candidate region claimed twice (not paired)"] += 1
            continue
        used.add(c)
        pairs.append(((a, e), cspan[c], n))
    rleft = _complement(rlo, rhi, [p[0] for p in pairs])
    cleft = _complement(clo, chi, [p[1] for p in pairs])
    return pairs, rleft, cleft, notes


def _complement(lo, hi, spans):
    """[lo,hi) minus the (possibly unsorted, possibly overlapping) spans."""
    out, cur = [], lo
    for a, e in sorted(spans):
        a, e = max(a, lo), min(e, hi)
        if a >= hi or e <= cur:
            continue
        if a > cur:
            out.append((cur, a))
        cur = max(cur, e)
    if cur < hi:
        out.append((cur, hi))
    return out


def classify_leftover(side, spans, code):
    """Split unpaired bytes into filler / zero / real content."""
    fill = zero = content = 0
    for a, e in spans:
        o = side.pe.off(a)
        if o is None:
            zero += e - a          # virtual tail: no file bytes, reads as zero
            continue
        n = min(e - a, side.pe.off(e - 1) + 1 - o if side.pe.off(e - 1) else e - a)
        b = side.pe.data[o:o + n]
        f = b.count(b"\xcc") + b.count(b"\x90") if code else 0
        z = b.count(b"\x00")
        fill += f
        zero += z
        content += len(b) - f - z
        if e - a > n:
            zero += (e - a) - n
    return fill, zero, content


# ----------------------------------------------------------------- the report
class SecReport:
    def __init__(self, name, rsize, csize):
        self.name = name
        self.rsize, self.csize = rsize, csize
        self.delta = csize - rsize
        self.rows = []              # (bucket, retail_bytes, cand_bytes)
        self.cmp = Cmp()
        self.method = "unmeasured"
        self.why = ""
        self.paired_r = self.paired_c = 0
        self.unmeasured_r = 0
        self.padding_r = 0          # linker filler: retail's bytes, not content
        self.notes = Counter()
        self.detail = []

    def row(self, name, r, c):
        self.rows.append((name, r, c))

    def close(self):
        """Force the size arithmetic to balance; the shortfall becomes a row."""
        sr = sum(r for _, r, _ in self.rows)
        sc = sum(c for _, _, c in self.rows)
        if sr != self.rsize or sc != self.csize:
            self.rows.append(("unattributed", self.rsize - sr, self.csize - sc))
        assert sum(r for _, r, _ in self.rows) == self.rsize, self.name
        assert sum(c for _, _, c in self.rows) == self.csize, self.name
        assert (sum(c - r for _, r, c in self.rows) == self.delta), self.name
        self.cmp.check()
        # Every retail byte lands in exactly one place, so the bytes we compared
        # plus the bytes we never paired must BE the section.  This assert is
        # not decoration: an early return added to the region differ silently
        # dropped 1,340 B of operand-free .rdata regions until it fired.
        assert self.cmp.compared + (self.rsize - self.paired_r) == self.rsize, \
            "%s: compared %d + unpaired %d != %d" % (
                self.name, self.cmp.compared, self.rsize - self.paired_r,
                self.rsize)

    @property
    def reproduced(self):
        return self.cmp.matching / self.rsize if self.rsize else 0.0


# ------------------------------------------------------------------ the plan
def make_plan(R, C):
    """Every region-set pairing, computed BEFORE any byte is compared.

    Two reasons the pairing has to come first: the masked-slot tokens are built
    from symbol NAMES, and one address can carry several names (`??_E`/`??_G`),
    so the candidate's aliases must first be canonicalised onto the retail
    primary or an identical referent would tokenise differently on the two
    sides.
    """
    plan = {}
    ts = sect(R.pe, ".text")
    afx_lo, afx_hi, xlo = _retail_text_split(R, C)
    plan["text.plain"] = (True, (R.ilt_hi, afx_lo),
                          (C.ilt_hi, C.groups[".text$AFX_AUX"][0]))
    plan["text.afx"] = (True, (afx_lo, afx_hi),
                        (C.groups[".text$AFX_AUX"][0], C.groups[".text$AFX_TERM"][1]))
    rg = _rdata_groups(R.pe, _first_rtti_retail())
    cg = _rdata_groups(C.pe, min(rva for nm, rva, _ in C.pubs
                                 if nm.startswith("??_R")))
    for g in (".rdata", ".rdata$r", ".xdata$x"):
        plan["rdata" + g] = (False, rg[g], cg[g])
    rd, cd = sect(R.pe, ".data"), sect(C.pe, ".data")
    plan["data.init"] = (False, (rd["rva"], rd["rva"] + rd["raw_size"]),
                         (cd["rva"], cd["rva"] + cd["raw_size"]))
    out = {}
    for k, (code, rspan, cspan) in plan.items():
        out[k] = (code,) + pair_by_name(R, C, *rspan, *cspan) + (rspan, cspan)
    for k, v in out.items():
        for (ra, re_), (ca, ce), nm in v[1]:
            # every candidate alias of a paired region takes the retail name
            for alias in C.aka.get(ca, ()):
                C.canon[alias] = nm
            # both sides now know this name, so a disagreement about it MEANS
            # something; anything unpaired can only be judged by content
            R.pairable.add(nm)
            C.pairable.add(nm)
            # and the paired symbol's extent is the SHORTER of the two: the
            # candidate's "up to the next public" would otherwise swallow the
            # unnamed pool constants that follow it
            n = min(re_ - ra, ce - ca)
            R.label_end[ra] = ra + n
            C.label_end[ca] = ca + n
    # Every IAT slot is named identically on both sides BY CONSTRUCTION - from
    # the import table, not from a symbol table.  The .map also publishes the
    # same slot as `__imp__PtInRect@8`, and `seal()` would pick that as the
    # primary on our side while retail (no .map) keeps `__imp_PtInRect`; the
    # asymmetry then reads as a wrong referent.  Canonicalise every alias.
    for side in (R, C):
        for slot, name in side.iat.items():
            side.pairable.add("__imp_" + name)
            for alias in side.aka.get(slot, ()):
                side.canon[alias] = "__imp_" + name
    return out, (afx_lo, afx_hi, xlo), rg, cg


# ---------------------------------------------------------------------- .text
def do_text(R, C, plan, split):
    rs, cs = sect(R.pe, ".text"), sect(C.pe, ".text")
    rep = SecReport(".text", rs["virtual_size"], cs["virtual_size"])
    rep.method = "symbol-paired, operands masked"

    # --- the incremental-link thunk band, its own animal ---------------------
    rband, cband = (rs["rva"], R.ilt_hi), (cs["rva"], C.ilt_hi)
    rt = _band_targets(R, rband)
    ct = _band_targets(C, cband)
    rnames = Counter(R.label(t + R.base)[1] for t in rt)
    cnames = Counter(C.label(t + C.base)[1] for t in ct)
    shared = sum((rnames & cnames).values())
    rep.row("ILT thunk band", rband[1] - rband[0], cband[1] - cband[0])
    rep.notes["ILT thunks: retail %d, candidate %d, %d reach the same function; "
              "each costs 10 B, so the +%s is a /INCREMENTAL link-line fact (only "
              "objects NAMED on the link line get a thunk), not code"
              % (len(rt), len(ct), shared,
                 "{:,}".format((cband[1] - cband[0]) - (rband[1] - rband[0])))] += 1

    afx_lo, afx_hi, xlo = split
    rx = (xlo, rs["rva"] + rs["virtual_size"])
    cx = C.groups[".text$x"]

    # --- plain .text ---------------------------------------------------------
    _c, pairs, rleft, cleft, notes, _rs, _cs = plan["text.plain"]
    rep.notes.update(notes)
    for rspan, cspan, nm in pairs:
        region_diff(R, C, rspan, cspan, rep.cmp, True, nm)
    pr = sum(e - a for (a, e), _, _ in pairs)
    pc = sum(e - a for _, (a, e), _ in pairs)
    rf, rz, rc = classify_leftover(R, rleft, True)
    cf, cz, cc = classify_leftover(C, cleft, True)
    # split retail's unpaired CONTENT: a body somebody carved but we could not
    # pair, versus code no carve, FID label or helper row ever claimed.  They
    # are different work: the first is a naming/attribution defect, the second
    # is a function nobody has reconstructed.
    ru = _overlap_bytes(R, rleft, [(a, e) for a, e, _n in R.syms])
    rep.row("plain .text - paired bodies", pr, pc)
    rep.row("plain .text - unpaired filler (0xCC/0x90)", rf, cf)
    rep.row("plain .text - unpaired zero", rz, cz)
    rep.row("plain .text - unpaired carved bodies", ru, 0)
    rep.row("plain .text - NEVER-CARVED code", rc - ru, cc)
    rep.padding_r = rf
    rep.detail.append(("plain .text never-carved content",
                       _top_spans(R, rleft, 10), _top_spans(C, cleft, 10)))
    rep.notes["%s B of retail's unpaired plain .text is inside a CARVED function "
              "we could not pair by name (%d regions); the other %s B is code no "
              "carve, FID label or helper row has ever claimed - that is the "
              "reconstruction worklist, and per-object objdiff cannot see it "
              "because an unclaimed hole has no object to score"
              % ("{:,}".format(ru), notes["retail region with no candidate symbol"],
                 "{:,}".format(rc - ru))] += 1

    # --- .text$AFX_*: MFC bodies, paired through the FID names ---------------
    _c, ap, arl, acl, an, _rs, _cs = plan["text.afx"]
    for rspan, cspan, nm in ap:
        region_diff(R, C, rspan, cspan, rep.cmp, True, nm)
    apr = sum(e - a for (a, e), _, _ in ap)
    apc = sum(e - a for _, (a, e), _ in ap)
    arf, arz, arc = classify_leftover(R, arl, True)
    acf, acz, acc = classify_leftover(C, acl, True)
    rep.row(".text$AFX_* - paired MFC bodies", apr, apc)
    rep.row(".text$AFX_* - unpaired filler", arf, acf)
    rep.row(".text$AFX_* - unpaired zero", arz, acz)
    rep.row(".text$AFX_* - unpaired CONTENT", arc, acc)
    rep.row("pad AFX -> $x", xlo - afx_hi, cx[0] - C.groups[".text$AFX_TERM"][1])
    rep.row(".text$x (UNMEASURED - funclets are not public)",
            rx[1] - rx[0], cx[1] - cx[0])
    rep.notes[".text$AFX_*: %d MFC bodies paired by FID name (%s B of %s); the "
              "unwind funclets in .text$x have no public symbol in EITHER image, "
              "so %s B of retail is UNMEASURED, not scored 0"
              % (len(ap), "{:,}".format(apr), "{:,}".format(afx_hi - afx_lo),
                 "{:,}".format(rx[1] - rx[0]))] += 1
    rep.paired_r, rep.paired_c = pr + apr, pc + apc
    rep.unmeasured_r = rx[1] - rx[0]
    rep.close()
    return rep


def _band_targets(side, span):
    lo, hi = span
    o = side.pe.off(lo)
    d = side.pe.data[o:o + (hi - lo)]
    out, i = [], 0
    while i + 5 <= len(d):
        if d[i] == 0xE9:
            out.append(lo + i + 5 + struct.unpack_from("<i", d, i + 1)[0])
            i += 5
        elif d[i] in FILLER:
            i += 1
        else:
            break
    return out


def _retail_text_split(R, C):
    """Retail's (.text$AFX_* lo, hi, .text$x lo) - link_sections' derivation."""
    lib = defaultdict(list)
    p = REPO / "config/retail/library_labels.csv"
    with p.open() as f:
        for r in csv.DictReader(f):
            lib[r["name"]].append(int(r["rva"], 16))
    alo, ahi = C.groups[".text$AFX_AUX"]
    hits = []
    for nm, rva, _ in C.pubs:
        if alo <= rva < ahi:
            hits += lib.get(nm) or lib.get(nm.lstrip("_")) or []
    lo = min(hits)
    size = sum(C.groups[g][1] - C.groups[g][0] for g in C.groups
               if g.startswith(".text$AFX_"))
    return lo, lo + size, (lo + size + 15) & ~15


def _overlap_bytes(side, spans, other):
    """Bytes of `spans` that also lie inside one of `other` (both unsorted)."""
    import bisect
    ot = sorted(other)
    starts = [a for a, _e in ot]
    tot = 0
    for a, e in spans:
        i = max(0, bisect.bisect_right(starts, a) - 1)
        while i < len(ot) and ot[i][0] < e:
            tot += max(0, min(e, ot[i][1]) - max(a, ot[i][0]))
            i += 1
    return tot


def _top_spans(side, spans, k):
    out = []
    for a, e in spans:
        o = side.pe.off(a)
        if o is None:
            continue
        b = side.pe.data[o:o + (e - a)]
        nz = sum(1 for x in b if x not in FILLER and x != 0)
        if nz:
            out.append((nz, a, e - a))
    out.sort(reverse=True)
    return out[:k]


# --------------------------------------------------------------------- .rdata
def do_rdata(R, C, plan, rgrp, cgrp):
    rs, cs = sect(R.pe, ".rdata"), sect(C.pe, ".rdata")
    rep = SecReport(".rdata", rs["virtual_size"], cs["virtual_size"])
    rep.method = "symbol-paired (manifest x .map)"
    for g in (".rdata", ".rdata$r", ".xdata$x"):
        _c, pairs, rleft, cleft, notes, _r, _cc2 = plan["rdata" + g]
        rep.notes.update({("%s: %s" % (g, k)): v for k, v in notes.items()})
        for rspan, cspan, nm in pairs:
            region_diff(R, C, rspan, cspan, rep.cmp, False, nm, clamp=True)
        pr = sum(e - a for (a, e), _, _ in pairs)
        pc = sum(e - a for _, (a, e), _ in pairs)
        rep.paired_r += pr
        rep.paired_c += pc
        _, rz, rc = classify_leftover(R, rleft, False)
        _, cz, cc = classify_leftover(C, cleft, False)
        rep.row("%s paired" % g, pr, pc)
        rep.row("%s unpaired zero/pad" % g, rz, cz)
        rep.row("%s unpaired CONTENT" % g, rc, cc)
        if g == ".rdata":
            rep.detail.append((".rdata unpaired content",
                               _top_spans(R, rleft, 8), _top_spans(C, cleft, 8)))
    rep.unmeasured_r = rgrp[".xdata$x"][1] - rgrp[".xdata$x"][0]
    rep.notes[".xdata$x (%s B of retail) holds the compiler's __ehfuncinfo blobs; "
              "NEITHER image gives them a symbol, so they are UNMEASURED rather "
              "than scored 0 - the EH record and try-block census in "
              "link_sections is what covers them"
              % "{:,}".format(rep.unmeasured_r)] += 1
    rep.notes["boundaries: retail .rdata$r@%#08x .xdata$x@%#08x; candidate "
              ".rdata$r@%#08x .xdata$x@%#08x (== the .map)"
              % (rgrp[".rdata$r"][0], rgrp[".xdata$x"][0],
                 cgrp[".rdata$r"][0], cgrp[".xdata$x"][0])] += 1
    rep.close()
    return rep


def _rdata_groups(pe, rtti_lo):
    s = sect(pe, ".rdata")
    d = pe.data[s["raw_offset"]: s["raw_offset"] + s["virtual_size"]]
    base, end = s["rva"], s["rva"] + s["virtual_size"]
    xs = end
    for i in range(((rtti_lo - base) + 3) & ~3, len(d) - 4, 4):
        if struct.unpack_from("<I", d, i)[0] == 0x19930520:
            xs = base + i
            break
    return {".rdata": (base, rtti_lo), ".rdata$r": (rtti_lo, xs), ".xdata$x": (xs, end)}


def _first_rtti_retail():
    lo = None
    with DATAMAN.open() as f:
        for r in csv.DictReader(f, delimiter="\t"):
            if r["storage"] == "rdata" and r["name"].startswith("??_R"):
                a = int(r["rva"], 16)
                lo = a if lo is None else min(lo, a)
    return lo


# ---------------------------------------------------------------------- .data
def do_data(R, C, plan):
    rs, cs = sect(R.pe, ".data"), sect(C.pe, ".data")
    rep = SecReport(".data", rs["virtual_size"], cs["virtual_size"])
    rep.method = "symbol-paired over initialized"
    _c, pairs, rleft, cleft, notes, rinit, cinit = plan["data.init"]
    rep.notes.update(notes)
    for rspan, cspan, nm in pairs:
        region_diff(R, C, rspan, cspan, rep.cmp, False, nm, clamp=True)
    pr = sum(e - a for (a, e), _, _ in pairs)
    pc = sum(e - a for _, (a, e), _ in pairs)
    rep.paired_r, rep.paired_c = pr, pc
    _, rz, rc = classify_leftover(R, rleft, False)
    _, cz, cc = classify_leftover(C, cleft, False)
    rep.row("initialized - paired", pr, pc)
    rep.row("initialized - unpaired zero/pad", rz, cz)
    rep.row("initialized - unpaired CONTENT", rc, cc)
    rep.row(".bss zero fill (UNMEASURABLE - no bytes)",
            rs["virtual_size"] - rs["raw_size"], cs["virtual_size"] - cs["raw_size"])
    rep.unmeasured_r = rs["virtual_size"] - rs["raw_size"]
    bss = C.groups.get(".bss")
    if bss:
        rep.notes["candidate .bss starts %#08x (.map); raw_size split puts it at "
                  "%#08x" % (bss[0], cinit[1])] += 1
    rep.close()
    return rep


# --------------------------------------------------------------------- .idata
def do_idata(R, C):
    rs, cs = sect(R.pe, ".idata"), sect(C.pe, ".idata")
    rep = SecReport(".idata", rs["virtual_size"], cs["virtual_size"])
    rep.method = "structural: paired by (dll, import name)"
    ri = {d["dll"]: d for d in R.imports}
    ci = {d["dll"]: d for d in C.imports}
    both = [k for k in ri if k in ci]
    nimp = nsame = 0
    reorder = 0
    for k in both:
        re_ = [e[0] for e in ri[k]["entries"]]
        ce = [e[0] for e in ci[k]["entries"]]
        nimp += len(re_)
        nsame += len(set(re_) & set(ce))
        if re_ != ce and set(re_) == set(ce):
            reorder += 1
        # the hint/name blobs, paired by name
        rmap = {e[0]: e[2] for e in ri[k]["entries"]}
        cmap = {e[0]: e[2] for e in ci[k]["entries"]}
        for n in set(rmap) & set(cmap):
            if not rmap[n] or not cmap[n]:
                rep.cmp.compared += 4          # by-ordinal: the slot IS the datum
                rep.cmp.slot_sym += 4
                continue
            rl = 2 + len(n) + 1
            ro, co = R.pe.off(rmap[n]), C.pe.off(cmap[n])
            rb = R.pe.data[ro:ro + rl]
            cb = C.pe.data[co:co + rl]
            rep.cmp.compared += rl
            eq = sum(1 for a, b in zip(rb, cb) if a == b)
            rep.cmp.equal += eq
            rep.cmp.differ += rl - eq
            # the two 4-byte pointers that reach this name (ILT + IAT slots)
            rep.cmp.compared += 8
            rep.cmp.slot_sym += 8
            rep.cmp.classes["import thunk slot -> same import"] += 2
        # the descriptor: 5 dwords, 3 of them RVAs into this section
        rep.cmp.compared += 20
        rep.cmp.slot_sym += 12
        rd = R.pe.data[ri[k]["desc"]: ri[k]["desc"] + 20]
        cd = C.pe.data[ci[k]["desc"]: ci[k]["desc"] + 20]
        for off in (4, 8):
            rep.cmp.equal += sum(1 for a, b in zip(rd[off:off + 4], cd[off:off + 4])
                                 if a == b)
            rep.cmp.differ += sum(1 for a, b in zip(rd[off:off + 4], cd[off:off + 4])
                                  if a != b)
        # the DLL name string
        rn = R.pe.cstr(ri[k]["name_rva"])
        rl = len(rn) + 1
        rep.cmp.compared += rl
        same = rn == C.pe.cstr(ci[k]["name_rva"])
        rep.cmp.equal += rl if same else 0
        rep.cmp.differ += 0 if same else rl
    rep.cmp.classes["import descriptor RVA field -> same target"] += 3 * len(both)
    # the null thunk terminators and the null descriptor are structure, not
    # content: both images have them and they are zero on both sides
    term = 8 * len(both) + 20
    rep.cmp.compared += term
    rep.cmp.equal += term
    rep.paired_r = rep.paired_c = rep.cmp.compared
    rep.unmeasured_r = rep.rsize - rep.paired_r
    rep.row("import descriptors + terminator",
            20 * (len(ri) + 1), 20 * (len(ci) + 1))
    rslots = sum(len(d["entries"]) + 1 for d in R.imports)
    cslots = sum(len(d["entries"]) + 1 for d in C.imports)
    rep.row("ILT + IAT thunk arrays", 8 * rslots, 8 * cslots)
    rstr = rs["virtual_size"] - 20 * (len(ri) + 1) - 8 * rslots
    cstr = cs["virtual_size"] - 20 * (len(ci) + 1) - 8 * cslots
    rep.row("hint/name + dll-name pool (incl. alignment)", rstr, cstr)
    rep.notes["%d DLLs, %d imports; %d paired by name, %d DLLs whose thunk array "
              "retail orders differently" % (len(both), nimp, nsame, reorder)] += 1
    rep.notes["order inside a thunk array is an import-library member-order "
              "artifact, not content: the (dll, name) SETS are identical"] += 1
    rep.notes["the %s B not paired is hint/name pool alignment padding, whose "
              "position follows that ordering - UNMEASURED, not a defect"
              % "{:,}".format(rep.unmeasured_r)] += 1
    rep.close()
    return rep


# ---------------------------------------------------------------------- .rsrc
def do_rsrc(R, C):
    rs, cs = sect(R.pe, ".rsrc"), sect(C.pe, ".rsrc")
    if rs is None or cs is None:
        return None
    rep = SecReport(".rsrc", rs["virtual_size"], cs["virtual_size"])
    rep.method = "resource tree, RVA fields shifted"
    rres = _walk_rsrc(R.pe, rs)
    cres = _walk_rsrc(C.pe, cs)
    shift = cs["rva"] - rs["rva"]
    both = set(rres) & set(cres)
    rbytes = sum(sz for _rva, sz in rres.values())
    cbytes = sum(sz for _rva, sz in cres.values())

    # Is the section POSITIONALLY aligned?  It is if every paired resource sits
    # at the same offset inside its own section and is the same length; then a
    # straight byte compare is legitimate and the only thing that can differ is
    # a stored RVA.  If it is not, say so instead of inventing an alignment.
    aligned = (len(both) == len(rres) == len(cres)
               and rs["virtual_size"] == cs["virtual_size"]
               and all(rres[k][0] - rs["rva"] == cres[k][0] - cs["rva"]
                       and rres[k][1] == cres[k][1] for k in both))
    rep.row("resource payloads (%d paired)" % len(both), rbytes, cbytes)
    rep.row("directory tree + data entries + name strings",
            rs["virtual_size"] - rbytes, cs["virtual_size"] - cbytes)
    if not aligned:
        rep.why = ("the two resource trees are not positionally aligned "
                   "(%d retail-only, %d candidate-only resources)"
                   % (len(set(rres) - both), len(set(cres) - both)))
        rep.unmeasured_r = rep.rsize
        rep.close()
        return rep

    # every IMAGE_RESOURCE_DATA_ENTRY.OffsetToData is an RVA: a field differing
    # by exactly the placement delta is not a difference, it is where the
    # section landed.  That is the whole of .rsrc's byte delta, and it is proved
    # here rather than asserted.
    ents = {off - rs["raw_offset"] for off, _ in _rsrc_entries(R.pe, rs)}
    rb = raw(R.pe, rs)[:rs["virtual_size"]]
    cb = raw(C.pe, cs)[:cs["virtual_size"]]
    n = min(len(rb), len(cb))
    eq = shifted = bad = 0
    for i in range(n):
        if rb[i] == cb[i]:
            eq += 1
            continue
        base = (i // 4) * 4
        if base in ents and base + 4 <= n and \
                (struct.unpack_from("<I", cb, base)[0]
                 - struct.unpack_from("<I", rb, base)[0]) == shift:
            shifted += 1
        else:
            bad += 1
            if len(rep.cmp.worst) < 40:
                rep.cmp.worst.append((1, "byte %#x in .rsrc" % (rs["rva"] + i), 1))
    rep.cmp.compared = n
    rep.cmp.equal = eq
    rep.cmp.slot_content = shifted
    rep.cmp.differ = bad
    rep.paired_r = rep.paired_c = n
    rep.cmp.classes["OffsetToData dword shifted by the placement delta (%+#x)"
                    % shift] += shifted // 4
    rep.notes["the two trees are positionally aligned: %d resources, identical "
              "offsets and lengths, so a straight byte compare is legitimate"
              % len(both)] += 1
    rep.notes["%d resources retail-only, %d candidate-only"
              % (len(set(rres) - both), len(set(cres) - both))] += 1
    rep.close()
    return rep


def _rsrc_entries(pe, s):
    """[(file_offset_of_OffsetToData, rva)] for every IMAGE_RESOURCE_DATA_ENTRY."""
    return [(s["raw_offset"] + off, rva)
            for _k, (rva, _sz), off in _walk_rsrc(pe, s, want_off=True)]


def _walk_rsrc(pe, s, want_off=False):
    """{(type, name, lang): (data_rva, size)} - the whole resource tree."""
    base = s["raw_offset"]
    res = {}
    flat = []

    def rec(off, path):
        n_named, n_id = struct.unpack_from("<HH", pe.data, base + off + 12)
        for i in range(n_named + n_id):
            e = base + off + 16 + 8 * i
            nid, sub = struct.unpack_from("<II", pe.data, e)
            key = ("#%d" % (nid & 0x7FFFFFFF) if not (nid & 0x80000000)
                   else _rsrc_name(pe, base, nid & 0x7FFFFFFF))
            if sub & 0x80000000:
                rec(sub & 0x7FFFFFFF, path + (key,))
            else:
                rva, size = struct.unpack_from("<II", pe.data, base + sub)
                res[path + (key,)] = (rva, size)
                flat.append((path + (key,), (rva, size), sub))

    rec(0, ())
    return flat if want_off else res


def _rsrc_name(pe, base, off):
    ln = struct.unpack_from("<H", pe.data, base + off)[0]
    return pe.data[base + off + 2: base + off + 2 + 2 * ln].decode("utf-16-le")


# --------------------------------------------------------------------- .reloc
def do_reloc(R, C, reps):
    """`.reloc` has no content of its own - it is a FUNCTION of where the
    relocated words are.  So it is scored through the other sections' pairings:
    a retail relocation is reproduced when the aligned candidate region carries
    one at the same place naming the same thing, which `region_diff` has already
    decided (its `reloc` slots).  Sites outside every paired region cannot be
    judged and are reported UNMEASURED, not scored zero."""
    rs, cs = sect(R.pe, ".reloc"), sect(C.pe, ".reloc")
    rep = SecReport(".reloc", rs["virtual_size"], cs["virtual_size"])
    rep.method = "sites via the region pairing"
    rb = _reloc_blocks(R.pe)
    cb = _reloc_blocks(C.pe)
    rn = sum(len(v) for v in rb.values())
    cn = sum(len(v) for v in cb.values())
    rep.row("HIGHLOW entries (2 B each)", 2 * rn, 2 * cn)
    rep.row("page-block headers (8 B each)", 8 * len(rb), 8 * len(cb))
    rep.row("intra-block padding / ABSOLUTE entries",
            rs["virtual_size"] - 2 * rn - 8 * len(rb),
            cs["virtual_size"] - 2 * cn - 8 * len(cb))
    ok = sum(r.cmp.reloc_ok for r in reps)
    bad = sum(r.cmp.reloc_bad for r in reps)
    rep.cmp.compared = 2 * (ok + bad)
    rep.cmp.equal = 2 * ok
    rep.cmp.differ = 2 * bad
    rep.paired_r = rep.paired_c = 2 * (ok + bad)
    rep.unmeasured_r = rep.rsize - rep.paired_r
    rep.notes["%s of retail's %s relocation sites sit inside a paired region; %s "
              "of those are reproduced at the aligned position naming the same "
              "referent" % ("{:,}".format(ok + bad), "{:,}".format(rn),
                            "{:,}".format(ok))] += 1
    rep.notes["the other %s sites are in unclaimed .text or unpaired data: "
              "UNMEASURED, not scored 0.  Retail relocates %s words, we relocate "
              "%s (%s), which IS the section delta"
              % ("{:,}".format(rn - ok - bad), "{:,}".format(rn),
                 "{:,}".format(cn), "{:+,}".format(cn - rn))] += 1
    rep.close()
    return rep


def _reloc_blocks(pe):
    d = pe.data
    rva = struct.unpack_from("<I", d, pe._opt + 96 + 5 * 8)[0]
    sz = struct.unpack_from("<I", d, pe._opt + 96 + 5 * 8 + 4)[0]
    out = {}
    p = pe.off(rva)
    end = p + sz
    while p < end:
        page, blk = struct.unpack_from("<II", d, p)
        if blk == 0:
            break
        ents = [struct.unpack_from("<H", d, p + 8 + 2 * i)[0]
                for i in range((blk - 8) // 2)]
        out[page] = [page + (e & 0xFFF) for e in ents if e >> 12 == 3]
        p += blk
    return out


# ---------------------------------------------------------------------- driver
def analyse():
    R, C = build_retail(), build_candidate()
    plan, split, rgrp, cgrp = make_plan(R, C)
    reps = [do_text(R, C, plan, split),
            do_rdata(R, C, plan, rgrp, cgrp),
            do_data(R, C, plan),
            do_idata(R, C)]
    rr = do_rsrc(R, C)
    if rr:
        reps.append(rr)
    reps.append(do_reloc(R, C, reps))
    return R, C, reps


def fmt(rep, detail=0):
    L = []
    L.append("")
    L.append("=" * 78)
    L.append("%-8s  retail %10s   candidate %10s   delta %+9s"
             % (rep.name, "{:,}".format(rep.rsize), "{:,}".format(rep.csize),
                "{:,}".format(rep.delta)))
    L.append("=" * 78)
    L.append("  SIZE, attributed              %12s %12s %11s"
             % ("retail", "candidate", "delta"))
    for n, r, c in rep.rows:
        mark = "  <-- unexplained" if n == "unattributed" and (c - r) else ""
        L.append("    %-28s %12s %12s %+11s%s"
                 % (n[:28], "{:,}".format(r), "{:,}".format(c),
                    "{:,}".format(c - r), mark))
    cm = rep.cmp
    L.append("")
    L.append("  BYTES  (%s)" % rep.method)
    if not cm.compared:
        L.append("    UNMEASURED - %s" % (rep.why or "nothing to align on"))
    else:
        unm = rep.rsize - rep.paired_r
        L.append("    every RETAIL byte of this section, once:")
        L.append("      aligned + identical              %11s"
                 % "{:,}".format(cm.equal))
        L.append("      masked operand -> same symbol    %11s"
                 % "{:,}".format(cm.slot_sym))
        L.append("      masked operand -> same target    %11s"
                 % "{:,}".format(cm.slot_content))
        L.append("      masked operand, not aligned      %11s"
                 % "{:,}".format(cm.slot_bad + cm.slot_unres))
        L.append("      masked operand, referent UNKNOWN %11s"
                 % "{:,}".format(cm.slot_undec))
        L.append("      no aligned counterpart           %11s"
                 % "{:,}".format(cm.differ))
        L.append("      never paired (nothing to pair on)%11s%s"
                 % ("{:,}".format(unm),
                    "  of which UNMEASURABLE %s" % "{:,}".format(rep.unmeasured_r)
                    if rep.unmeasured_r else ""))
        L.append("      %s= %s = retail section size"
                 % (" " * 33, "{:,}".format(cm.compared + unm)))
        L.append("    ------------------------------------------------")
        L.append("    RETAIL REPRODUCED                  %9.2f%%   (%s of %s)"
                 % (100.0 * rep.reproduced, "{:,}".format(cm.matching),
                    "{:,}".format(rep.rsize)))
        L.append("    within paired regions              %9.2f%%"
                 % (100.0 * cm.matching / cm.compared))
        if rep.unmeasured_r:
            L.append("    of the MEASURABLE part             %9.2f%%"
                     % (100.0 * cm.matching / (rep.rsize - rep.unmeasured_r)))
        if rep.padding_r:
            L.append("    of MEASURABLE, NON-FILLER retail   %9.2f%%   (filler is "
                     "%s B of incremental-link 0xCC)"
                     % (100.0 * cm.matching
                        / (rep.rsize - rep.unmeasured_r - rep.padding_r),
                        "{:,}".format(rep.padding_r)))
        L.append("    candidate bytes with no retail twin  %9s"
                 % "{:,}".format(rep.csize - rep.paired_c + cm.cand_extra))
    if cm.ref_total:
        L.append("")
        L.append("  REFERENTS  (independent of where the bytes landed: does the "
                 "paired code")
        L.append("              reach the same things, in the same order?)")
        L.append("    address operands in paired retail regions %8s"
                 % "{:,}".format(cm.ref_total))
        L.append("    reaching the same referent, in order      %8s  (%.2f%%)"
                 % ("{:,}".format(cm.ref_ok), 100.0 * cm.ref_ok / cm.ref_total))
        L.append("    regions whose referent sequence is exact  %8s"
                 % "{:,}".format(cm.ref_pairs_clean))
        L.append("    ... same referents, different ORDER       %8s"
                 % "{:,}".format(cm.ref_pairs_reordered))
        L.append("    ... same identities, different COUNT      %8s"
                 % "{:,}".format(cm.ref_pairs_multiplicity))
        L.append("    ... genuinely different referents         %8s  <-- the "
                 "wrong-referent worklist" % "{:,}".format(cm.ref_pairs_bad))
    if cm.classes:
        L.append("")
        L.append("  masked/suppressed classes (counted, never silent)")
        for k, v in cm.classes.most_common():
            L.append("    %-52s %8s" % (k[:52], "{:,}".format(v)))
    for k, v in rep.notes.items():
        L.append("  note: %s%s" % (k, "" if v == 1 else "  (x%d)" % v))
    if detail and cm.worst:
        L.append("")
        L.append("  worst paired regions (retail bytes not reproduced / region)")
        for bad, nm, n in sorted(cm.worst, reverse=True)[:detail]:
            L.append("    %7s / %-7s  %s" % ("{:,}".format(bad), "{:,}".format(n),
                                             nm[:52]))
    if detail and cm.ref_bad:
        L.append("")
        L.append("  regions calling something ELSE (retail -> candidate)")
        seen = Counter(x[0] for x in cm.ref_bad)
        for nm, cnt in seen.most_common(detail):
            rows = [x for x in cm.ref_bad if x[0] == nm][:2]
            L.append("    %s   (%d divergence%s)"
                     % (nm[:60], cnt, "" if cnt == 1 else "s"))
            for _n, rr, cc in rows:
                L.append("        retail    %s" % (", ".join(rr[:3])[:64] or "-"))
                L.append("        candidate %s" % (", ".join(cc[:3])[:64] or "-"))
    if detail:
        for title, rtop, ctop in rep.detail:
            L.append("")
            L.append("  %s - largest retail spans" % title)
            for nz, a, ln in rtop:
                L.append("    %#08x  %7s B span, %7s B content"
                         % (a, "{:,}".format(ln), "{:,}".format(nz)))
            if ctop:
                L.append("  %s - largest candidate spans" % title)
                for nz, a, ln in ctop:
                    L.append("    %#08x  %7s B span, %7s B content"
                             % (a, "{:,}".format(ln), "{:,}".format(nz)))
    return "\n".join(L)


def summary(R, C, reps):
    rt, ct = RETAIL.stat().st_size, CAND.stat().st_size
    L = ["", "=" * 78,
         "IMAGE  retail %s B   candidate %s B   (%.2f%% of retail's size, %s B)"
         % ("{:,}".format(rt), "{:,}".format(ct), 100.0 * ct / rt,
            "{:+,}".format(ct - rt)),
         "=" * 78,
         "  %-8s %11s %11s %10s %10s %9s %9s  %s"
         % ("section", "retail", "candidate", "delta", "reproduced", "unmeasrd",
            "unattrib", "method")]
    tot_m = tot_r = tot_u = 0
    for r in reps:
        un = next((c - x for n, x, c in r.rows if n == "unattributed"), 0)
        L.append("  %-8s %11s %11s %+10s %9.2f%% %9s %9s  %s"
                 % (r.name, "{:,}".format(r.rsize), "{:,}".format(r.csize), r.delta,
                    100.0 * r.reproduced, "{:,}".format(r.unmeasured_r),
                    "{:,}".format(un), r.method[:30]))
        tot_m += r.cmp.matching
        tot_r += r.rsize
        tot_u += r.unmeasured_r
    L.append("  %-8s %11s %11s %+10s %9.2f%% %9s %9s"
             % ("TOTAL", "{:,}".format(sum(r.rsize for r in reps)),
                "{:,}".format(sum(r.csize for r in reps)),
                sum(r.delta for r in reps), 100.0 * tot_m / tot_r,
                "{:,}".format(tot_u),
                "{:,}".format(sum(next((c - x for n, x, c in r.rows
                                        if n == "unattributed"), 0) for r in reps))))
    rt_ = sum(r.cmp.ref_total for r in reps)
    ro_ = sum(r.cmp.ref_ok for r in reps)
    L.append("")
    L.append("  `reproduced` = matching bytes / retail's section size.  Matching means")
    L.append("  aligned+identical, or a masked address operand reaching the same")
    L.append("  referent.  Retail bytes we never paired count AGAINST us; bytes we")
    L.append("  emit that retail lacks never count FOR us; `unmeasrd` is retail bytes")
    L.append("  no honest alignment exists for, and they are inside `reproduced`'s")
    L.append("  denominator, so the number is a floor, not a flatterer.")
    if rt_:
        L.append("  Referents: %s of %s address operands in paired regions reach the "
                 "same" % ("{:,}".format(ro_), "{:,}".format(rt_)))
        L.append("  thing in the same order (%.2f%%) - the measure that does NOT care"
                 % (100.0 * ro_ / rt_))
        L.append("  where the bytes landed.")
    return "\n".join(L)


# -------------------------------------------------------------------- selftest
def selftest():
    """Plant known defects and require the differ to find, attribute and
    classify each one.  A differ nobody has tried to fool is a differ nobody
    should believe: every check below states what a WRONG implementation would
    have reported instead."""
    import shutil
    import tempfile
    global CAND
    ok = True

    def check(label, cond, got=""):
        nonlocal ok
        ok &= bool(cond)
        print("  [%s] %s%s" % ("PASS" if cond else "FAIL", label,
                               "  (%s)" % got if got else ""))

    print("=== image_diff selftest ===")

    # --- 0. the aligner, on synthetic input ---------------------------------
    # The property everything rests on: ONE inserted byte must not make the
    # whole tail "different".  A positional differ scores this 0.
    a = bytes(range(256)) * 8
    b = a[:1000] + b"\xde\xad\xbe\xef" + a[1000:]
    m = sum(l for _i, _j, l in align(a, b))
    check("aligner: a 4-byte INSERTION still matches the whole body",
          m == len(a), "matched %d of %d" % (m, len(a)))
    c = bytearray(a)
    c[500:507] = b"\x00" * 7
    m2 = sum(l for _i, _j, l in align(a, bytes(c)))
    check("aligner: a 7-byte overwrite costs exactly 7",
          m2 == len(a) - 7, "matched %d of %d" % (m2, len(a)))
    term = b"\0Software\0NEXT"
    check("a string terminator is isolated from its next neighbour",
          _is_string_terminator(term, 9)
          and not _is_string_terminator(b"\0\0NEXT", 1),
          "terminator classifier")

    orig = CAND
    R, C, base = analyse()
    tb = base[0]
    open_name = "?Open@RegistryHelper@Utils@@QAEHPAD000PAUHKEY__@@0@Z"
    open_bad = [x for r in base for x in r.cmp.ref_bad if x[0] == open_name]
    check("RegistryHelper::Open compares the Software terminator, not its neighbour",
          not open_bad, "%d false row(s)" % len(open_bad))
    rect_name = "?g_levelMsgRectsB@@3PAUtagRECT@@A"
    rect_bad = []
    for side in (R, C):
        for rva in side.names.get(rect_name, ()):
            desc, how = resolve(side, rva + side.base + 4)
            if (desc, how) != (rect_name + "+0x4", "symbol"):
                rect_bad.append((side.tag, desc, how))
    check("a paired RECT field outranks coincidental one-byte string content",
          len(rect_bad) == 0, "%r" % rect_bad)

    # --- 1. the size arithmetic closes, per section -------------------------
    for r in base:
        un = next((c - x for n, x, c in r.rows if n == "unattributed"), 0)
        check("%-7s size buckets sum to the observed delta" % r.name,
              sum(cc - rr for _n, rr, cc in r.rows) == r.delta
              and un == 0, "unattributed %+d" % un)

    # --- 2. a planted body defect is found and attributed -------------------
    tmp = Path(tempfile.mkdtemp(prefix="imgdiff-"))
    victim = _pick_clean_pair(R, C)
    if victim is None:
        check("found a byte-clean paired body to perturb", False)
        return 1
    (ra, re_), (ca, ce), nm = victim
    buf = bytearray(orig.read_bytes())
    off = C.pe.off(ca)
    at = _safe_offset(R, C, victim)
    for t in range(7):
        buf[off + at + t] ^= 0x5A
    f1 = tmp / "flip.EXE"
    f1.write_bytes(bytes(buf))
    CAND = f1
    _, _, mut = analyse()
    CAND = orig
    d = mut[0].cmp.differ - tb.cmp.differ
    check("7 flipped body bytes -> exactly +7 unreproduced", d == 7,
          "got %+d in %s" % (d, nm[:40]))
    hit = [w for w in mut[0].cmp.worst if w[1] == nm]
    was = [w for w in tb.cmp.worst if w[1] == nm]
    check("...and it is attributed to the right region",
          len(hit) == 1 and hit[0][0] == 7 and not was,
          "%s" % (hit[0][:1] if hit else "region not listed"))

    # --- 3. a repointed operand is a REFERENT defect, not a silent match ----
    site = _pick_reloc_pair(R, C)
    if site is None:
        check("found a masked operand to repoint", False)
        return 1
    foff, cur, alt, vnm = site
    buf = bytearray(orig.read_bytes())
    struct.pack_into("<I", buf, foff, alt)
    f2 = tmp / "repoint.EXE"
    f2.write_bytes(bytes(buf))
    CAND = f2
    _, _, mut2 = analyse()
    CAND = orig
    db = mut2[0].cmp.ref_pairs_bad - tb.cmp.ref_pairs_bad
    check("a repointed relocated dword -> a REFERENT divergence", db == 1,
          "%+d region(s), in %s" % (db, vnm[:40]))
    check("...and the masking did NOT swallow it "
          "(a name-blind differ reports 0 here)",
          mut2[0].cmp.ref_ok < tb.cmp.ref_ok,
          "referents ok %d -> %d" % (tb.cmp.ref_ok, mut2[0].cmp.ref_ok))

    # UNKNOWN carries no identity and therefore cannot split a sequence of
    # otherwise identical referents.  This is the exact false-positive shape
    # that made LoadPickupSprites look as if HEALTH1 reached REDBRICK.
    a = ["A", UNDECIDABLE, "B"]
    b = ["A", "B"]
    check("an undecidable operand cannot manufacture a wrong referent",
          _align_undecidable(a, b)[:2] == (["A", "B"], ["A", "B"]),
          "sequence split")
    a2 = ["A", "B", "C"]
    b2 = ["A", UNDECIDABLE, "C"]
    ra2, ca2, ru2 = _align_undecidable(a2, b2)
    check("a one-sided symbol opposite UNKNOWN is not called wrong",
          (ra2, ca2, ru2) == (["A", "C"], ["A", "C"], 1),
          "%r / %r / %d" % (ra2, ca2, ru2))
    a3 = ["A", "B", "C"]
    b3 = ["A", "X", "C"]
    check("a known-vs-known referent difference survives normalization",
          _align_undecidable(a3, b3)[:2] == (a3, b3),
          "known difference suppressed")
    # A wildcard alignment must not destroy a multiset identity already proven
    # by every decidable operand.  This is the minimal form of the reordered
    # LoadPickupSprites switch that used to delete different known items.
    a4 = [UNDECIDABLE, "A", "B", "C"]
    b4 = ["A", UNDECIDABLE, "C", "B"]
    ra4, ca4, _ru4 = _align_undecidable(a4, b4)
    check("unknowns cannot turn a proven referent permutation into an identity defect",
          Counter(ra4) == Counter(ca4) == Counter(("A", "B", "C")),
          "%r / %r" % (ra4, ca4))
    check("a repeated use-count difference is multiplicity, not wrong identity",
          _referent_relation(["A", "A", "B"], ["A", "B"]) == "multiplicity",
          "multiplicity misclassified")
    check("a genuinely absent identity remains wrong",
          _referent_relation(["A", "B"], ["A", "C"]) == "wrong",
          "identity difference suppressed")

    # Region pairing itself must be symmetric.  FID can assign one byte-shape
    # name to many retail library bodies; choosing the first one is fitting,
    # not evidence, even when the candidate has exactly one such symbol.
    class _PairSide:
        pass
    pr, pc = _PairSide(), _PairSide()
    pr.syms = [(1, 2, "dup"), (3, 4, "dup")]
    pr.aka = defaultdict(set)
    pc.syms = [(11, 12, "dup")]
    pc.aka = defaultdict(set)
    pp = pair_by_name(pr, pc, 0, 10, 10, 20)[0]
    check("a duplicate retail name cannot claim one candidate region",
          not pp, "%d fabricated pair(s)" % len(pp))

    # --- 3b. the resolver's ASYMMETRY traps --------------------------------
    # Each of these four made a correct operand read as a wrong referent, and
    # each is one-sided by construction: whatever retail happens to NAME, our
    # .map does not, so the two images resolved the same target differently.
    # A wrong implementation reports the FIRST count here in the hundreds.
    bad = [(t, va) for side in (R, C)
           for va, t in _resolver_traps(side)]
    for label, want in (
        ("an IAT slot resolves to its IMPORT, not to the hint RVA read as text",
         "iat"),
        ("a relocated pool window is UNDECIDABLE, not a byte mismatch", "reloc"),
        ("a literal shorter than 4 chars resolves to its TEXT", "short"),
        (r"a literal carrying \n resolves to its TEXT", "escape"),
    ):
        n = sum(1 for t, _ in bad if t == want)
        check(label, n == 0, "%d target(s) still mis-resolved" % n)

    check("an x87 DWORD operand fingerprints only the four bytes it reads",
          _target_window_width(b"\xd8\x0d\x00\x00\x00\x00", 2, True) == 4,
          "DWORD width not recovered")
    check("an x87 QWORD operand retains an eight-byte fingerprint",
          _target_window_width(b"\xdc\x0d\x00\x00\x00\x00", 2, True) == 8,
          "QWORD width narrowed")
    check("unknown operand forms retain the conservative eight-byte window",
          _target_window_width(b"\x8b\x05\x00\x00\x00\x00", 2, True) == 8,
          "unknown form guessed")

    # --- 3c. an interior self-reference never reaches the worklist ---------
    # A switch jump table names its OWN function at a codegen-dependent offset,
    # and retail's carve routinely ends before its table while ours does not.
    self_rows = sum(1 for r in base for nm, rr, cc in r.cmp.ref_bad
                    for d in rr + cc if d.startswith(nm + "+"))
    check("no wrong-referent row is an interior SELF-reference (a jump table)",
          self_rows == 0, "%d row(s)" % self_rows)

    # --- 3d. two swapped operands are ORDERING-ONLY, not wrong referents ----
    # The multiset is intact by construction, so a differ that pushes this
    # into the wrong-referent worklist is double-counting order as identity.
    site2 = _pick_swap_site(R, C)
    if site2 is None:
        check("found two distinct named operands to swap", False)
        return 1
    fo1, fo2, snm = site2
    buf = bytearray(orig.read_bytes())
    buf[fo1:fo1 + 4], buf[fo2:fo2 + 4] = buf[fo2:fo2 + 4], buf[fo1:fo1 + 4]
    f3 = tmp / "swap.EXE"
    f3.write_bytes(bytes(buf))
    CAND = f3
    _, _, mut3 = analyse()
    CAND = orig
    dr = mut3[0].cmp.ref_pairs_reordered - tb.cmp.ref_pairs_reordered
    db2 = mut3[0].cmp.ref_pairs_bad - tb.cmp.ref_pairs_bad
    check("two swapped relocated dwords -> ONE ordering-only region",
          dr == 1, "%+d region(s), in %s" % (dr, snm[:40]))
    check("...and NOT a wrong-referent region (the multiset is intact)",
          db2 == 0, "%+d wrong-referent region(s)" % db2)
    named3 = [x for x in mut3[0].cmp.ref_reordered if x[0] == snm]
    was3 = [x for x in tb.cmp.ref_reordered if x[0] == snm]
    check("...and the ordering worklist attributes it to the right region",
          bool(named3) and not was3,
          "%d row(s) for the victim, baseline %d" % (len(named3), len(was3)))

    # --- 4. placement shift is classified, not counted ----------------------
    rr = next((r for r in base if r.name == ".rsrc"), None)
    if rr:
        sh = [k for k in rr.cmp.classes if k.startswith("OffsetToData")]
        check(".rsrc: every differing byte is a classified placement shift",
              bool(sh) and rr.cmp.differ == 0 and rr.cmp.slot_content > 0,
              "%s shifted, %d unexplained" % (rr.cmp.slot_content, rr.cmp.differ))

    # --- 5. nothing is scored on evidence we do not have --------------------
    for r in base:
        if r.unmeasured_r:
            check("%-7s counts its %s UNMEASURABLE bytes in the DENOMINATOR"
                  % (r.name, "{:,}".format(r.unmeasured_r)),
                  r.paired_r <= r.rsize - r.unmeasured_r,
                  "paired %s of %s" % ("{:,}".format(r.paired_r),
                                       "{:,}".format(r.rsize)))
    shutil.rmtree(tmp, ignore_errors=True)
    print("  -> %s" % ("all checks passed" if ok else "SELFTEST FAILED"))
    return 0 if ok else 1


def _resolver_traps(side):
    """[(va, trap)] - targets this side still resolves the WRONG way.

    Four one-sided readings, each of which used to manufacture a wrong-referent
    row out of a correct operand.  Every one is checkable from a single image:
    the question is not "do the two agree" but "is this side's answer the kind
    of answer that can possibly agree".
    """
    out = []
    for slot, name in side.iat.items():
        d, how = resolve(side, slot + side.base)
        if how != "symbol" or d != "__imp_" + name:
            out.append((slot + side.base, "iat"))
    for rva in side.reloc:
        s = side.pe.sec_of(rva)
        if s is None or side.pe.is_exec(rva):
            continue
        d, how = resolve(side, rva + side.base)
        if how in ("weak", "string"):
            out.append((rva + side.base, "reloc"))
        if len(out) > 64:
            break
    # A `??_C@` symbol IS a string literal by definition, so its bytes decide
    # what a correct resolver must say - asking `content()` would just ask the
    # code under test.
    for rva, _e, nm in side.syms:
        if not nm.startswith("??_C@"):
            continue
        o = side.pe.off(rva)
        if o is None:
            continue
        b = side.pe.data[o:o + 256]
        z = b.find(b"\0")
        # this predicate is deliberately LOCAL: asking TEXT_BYTES would ask the
        # constant under test, and the check would pass by agreeing with itself
        if z <= 0 or any(not (0x20 <= x < 0x7F or x in (9, 10, 13)) for x in b[:z]):
            continue
        if resolve(side, rva + side.base)[1] != "string":
            out.append((rva + side.base,
                        "short" if z < 4 else "escape"))
    return out


def _plain_text_pairs(R, C):
    return pair_by_name(R, C, R.ilt_hi, _retail_text_split(R, C)[0],
                        C.ilt_hi, C.groups[".text$AFX_AUX"][0])[0]


def _pick_clean_pair(R, C):
    """A paired body of equal length that currently reproduces perfectly."""
    for rspan, cspan, nm in _plain_text_pairs(R, C):
        if rspan[1] - rspan[0] < 160 or (rspan[1] - rspan[0]) != (cspan[1] - cspan[0]):
            continue
        c = Cmp()
        region_diff(R, C, rspan, cspan, c, True, nm)
        if c.differ == 0 and c.slot_bad == 0 and c.slot_unres == 0 \
                and c.ref_pairs_bad == 0 and c.ref_pairs_multiplicity == 0:
            return rspan, cspan, nm
    return None


def _safe_offset(R, C, pair):
    """An offset inside the pair that is not part of any masked slot, so the
    flip lands in plain content and must be counted as plain content."""
    (ra, re_), (ca, ce), _nm = pair
    n = min(re_ - ra, ce - ca)
    ro, co = R.pe.off(ra), C.pe.off(ca)
    rsl = slots_of(R, ra, R.pe.data[ro:ro + n], True)
    csl = slots_of(C, ca, C.pe.data[co:co + n], True)
    busy = set()
    for off, _k, _t, _r, _w in rsl + csl:
        busy.update(range(off - 1, off + 5))
    for i in range(16, n - 16):
        if all((i + t) not in busy for t in range(7)):
            return i
    raise RuntimeError("no slot-free window in the victim body")


def _pick_reloc_pair(R, C):
    """A relocated dword inside a currently-perfect body, plus a DIFFERENT but
    equally valid symbol address to repoint it at."""
    alt = altn = None
    for a, _e, n in C.syms:
        s = C.pe.sec_of(a)
        if s and s[0] == ".data" and n in C.pairable:
            alt, altn = a + C.base, n
            break
    for rspan, cspan, nm in _plain_text_pairs(R, C):
        c = Cmp()
        region_diff(R, C, rspan, cspan, c, True, nm)
        if c.differ or c.slot_bad or c.slot_unres or c.ref_pairs_bad \
                or c.ref_pairs_multiplicity:
            continue
        n = min(rspan[1] - rspan[0], cspan[1] - cspan[0])
        for off, kind, _t, res, _width in slots_of(C, cspan[0],
                                           C.pe.data[C.pe.off(cspan[0]):
                                                     C.pe.off(cspan[0]) + n], True):
            if kind != "reloc" or res != "symbol":
                continue
            fo = C.pe.off(cspan[0] + off)
            cur = struct.unpack_from("<I", C.pe.data, fo)[0]
            if alt and alt != cur:
                return fo, cur, alt, nm
    return None


def _pick_swap_site(R, C):
    """TWO relocated dwords inside a currently-perfect body that name DIFFERENT
    referents - swapping them keeps the multiset intact, so the differ must
    read it as ORDERING, never as a wrong referent."""
    for rspan, cspan, nm in _plain_text_pairs(R, C):
        c = Cmp()
        region_diff(R, C, rspan, cspan, c, True, nm)
        if c.differ or c.slot_bad or c.slot_unres or c.ref_pairs_bad \
                or c.ref_pairs_reordered or c.ref_pairs_multiplicity:
            continue
        n = min(rspan[1] - rspan[0], cspan[1] - cspan[0])
        co = C.pe.off(cspan[0])
        sites = []
        for off, kind, _t, res, width in slots_of(C, cspan[0],
                                           C.pe.data[co:co + n], True):
            if kind != "reloc" or res != "symbol":
                continue
            va = struct.unpack_from("<I", C.pe.data, co + off)[0]
            d = resolve(C, va, width)[0]
            if d.startswith(nm + "+"):
                continue
            sites.append((off, d))
        for i, (o1, d1) in enumerate(sites):
            for o2, d2 in sites[i + 1:]:
                if d1 != d2:
                    return co + o1, co + o2, nm
    return None


def _referent_evidence(divs):
    """Strongest evidence carried by any descriptor in a region's divergences.

    Evidence is monotone: seeing a later string must not downgrade an already
    symbol-proven region.  The old `min()` spelling did exactly that because
    "string literal" sorts before "symbol" lexically, misclassifying three mixed
    structural regions as one-line literal fixes.
    """
    rank = 0
    for rr, cc in divs:
        for desc in rr + cc:
            if desc.startswith('"'):
                rank = max(rank, 1)
            elif not (desc.startswith("<") or desc.startswith("0x")):
                rank = 2
    return ("weak / content only", "string literal", "symbol")[rank]


def _referent_worklist(reps, top):
    """The actionable output: paired regions that reach something ELSE.

    This is the linked-image identity check beyond per-object objdiff. Strict
    relocation scoring catches named target differences in objects; this pass
    additionally resolves aliases, indirect/final-image targets, ordering, and
    multiplicity after the real link.
    """
    rows = defaultdict(list)
    for r in reps:
        for nm, rr, cc in r.cmp.ref_bad:
            rows[(r.name, nm)].append((rr, cc))
    print("=== wrong-referent worklist ===")
    print("  %d paired region(s) reach a different referent than retail does."
          % len(rows))
    print("  Each line: what RETAIL reaches at that point, then what WE reach.")

    # Triage by the STRONGEST evidence behind each region's divergence, because
    # the three classes cost very different amounts to act on.  A `weak` row is
    # an 8-byte window over an unnamed target: real, but not proof, and the
    # reader must not spend a symbol-class effort on it.
    cls = Counter()
    for (sec, nm), divs in rows.items():
        cls[_referent_evidence(divs)] += 1
    print("  by strongest evidence:")
    for k, v in cls.most_common():
        print("    %-22s %4d region(s)" % (k, v))
    reordered = sum(r.cmp.ref_pairs_reordered for r in reps)
    print("  ordering-only (same decidable referent multiset): %d region(s)"
          " - `--ordering N` lists them" % reordered)
    multiplicity = sum(r.cmp.ref_pairs_multiplicity for r in reps)
    print("  multiplicity-only (same identities, different counts): %d region(s)"
          " - `--multiplicity N` lists them" % multiplicity)

    for (sec, nm), divs in sorted(rows.items(), key=lambda kv: -len(kv[1]))[:top]:
        print("\n  %s  %s   (%d divergence%s)"
              % (sec, nm[:64], len(divs), "" if len(divs) == 1 else "s"))
        for rr, cc in divs[:4]:
            print("      retail    %s" % (", ".join(rr[:4])[:96] or "(nothing)"))
            print("      us        %s" % (", ".join(cc[:4])[:96] or "(nothing)"))
    return 0


def _ordering_worklist(reps, top):
    """The ordering-only worklist: paired regions that reach retail's exact
    decidable referent multiset in a DIFFERENT ORDER.

    Nothing here points at the wrong thing - the multiset test has already
    proven every referent correct - so the defect is SEQUENCE: statement order,
    evaluation order or emitted-branch layout in the source.  Ranked
    easiest-first by displaced operand slots: 2 is ONE referent out of place,
    usually a two-statement swap in the source.
    """
    rows = defaultdict(list)
    for r in reps:
        for nm, rr, cc in r.cmp.ref_reordered:
            rows[(r.name, nm)].append((rr, cc))
    total = sum(r.cmp.ref_pairs_reordered for r in reps)
    # pair_by_name refuses ambiguous names, so (section, region) is unique and
    # the grouped listing must be COMPLETE - a truncated one is not a worklist.
    assert len(rows) == total, (len(rows), total)
    print("=== ordering-only worklist ===")
    print("  %d paired region(s) reach the same decidable referents in another "
          "order." % total)
    print("  Each segment: where RETAIL's sequence has it, then where OURS does;")
    print("  a one-sided segment is a referent that only MOVED.")
    displaced = {k: sum(len(rr) + len(cc) for rr, cc in v)
                 for k, v in rows.items()}
    hist = Counter(min(d, 5) for d in displaced.values())
    print("  by displaced operand slots (2 = one referent out of place):")
    for d in sorted(hist):
        print("    %-4s %4d region(s)" % ("5+" if d == 5 else d, hist[d]))
    for key in sorted(rows, key=lambda k: (displaced[k], k))[:top]:
        sec, nm = key
        print("\n  %s  %s   (%d displaced slot%s)"
              % (sec, nm[:64], displaced[key],
                 "" if displaced[key] == 1 else "s"))
        for rr, cc in rows[key][:6]:
            print("      retail    %s" % (", ".join(rr[:4])[:96] or "(in place)"))
            print("      us        %s" % (", ".join(cc[:4])[:96] or "(in place)"))
    return 0


def _multiplicity_worklist(reps, top):
    """Bodies reaching the same identities with different occurrence counts."""
    rows = defaultdict(list)
    for r in reps:
        for nm, rr, cc in r.cmp.ref_multiplicity:
            rows[(r.name, nm)].append((rr, cc))
    total = sum(r.cmp.ref_pairs_multiplicity for r in reps)
    assert len(rows) == total, (len(rows), total)
    print("=== multiplicity-only referent worklist ===")
    print("  %d paired region(s) reach the same referent identities a different "
          "number of times." % total)
    print("  This is code-shape/call-count evidence, not a wrong-target claim.")
    for (sec, nm), divs in sorted(rows.items(), key=lambda kv: -len(kv[1]))[:top]:
        print("\n  %s  %s   (%d divergence%s)"
              % (sec, nm[:64], len(divs), "" if len(divs) == 1 else "s"))
        for rr, cc in divs[:4]:
            print("      retail    %s" % (", ".join(rr[:4])[:96] or "(nothing)"))
            print("      us        %s" % (", ".join(cc[:4])[:96] or "(nothing)"))
    return 0


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--section", action="append", default=None,
                    help="restrict the report to these sections")
    ap.add_argument("--detail", type=int, default=0, metavar="N",
                    help="also list the N worst paired regions / unpaired spans")
    ap.add_argument("--tsv", type=Path, help="write the per-section table as TSV")
    ap.add_argument("--json", type=Path, help="write the whole report as JSON")
    ap.add_argument("--referents", type=int, default=0, metavar="N",
                    help="print only the wrong-referent worklist (N regions): the "
                         "paired bodies that reach something ELSE than retail does")
    ap.add_argument("--ordering", type=int, default=0, metavar="N",
                    help="print only the ordering-only worklist (N regions): the "
                         "paired bodies that reach retail's exact referents in a "
                         "DIFFERENT ORDER")
    ap.add_argument("--multiplicity", type=int, default=0, metavar="N",
                    help="print only the multiplicity worklist (N regions): the "
                         "paired bodies that reach the same identities a different "
                         "number of times")
    ap.add_argument("--selftest", action="store_true",
                    help="plant known defects and require the differ to find them")
    a = ap.parse_args(argv)

    for p in (RETAIL, CAND, CMAP):
        if not p.is_file():
            print("[image-diff] missing %s\n  run `ninja candidate` first" % p,
                  file=sys.stderr)
            return 2
    if a.selftest:
        return selftest()

    try:
        R, C, reps = analyse()
    except MissingInput as e:
        print("[image-diff] %s" % e, file=sys.stderr)
        return 2
    keep = [r for r in reps if not a.section or r.name in a.section]
    if a.referents:
        return _referent_worklist(keep, a.referents)
    if a.ordering:
        return _ordering_worklist(keep, a.ordering)
    if a.multiplicity:
        return _multiplicity_worklist(keep, a.multiplicity)
    print(summary(R, C, reps))
    for r in keep:
        print(fmt(r, a.detail))
    print("")
    print("suppression classes in force (all counted above, none silent):")
    for k, v in SUPPRESSIONS.items():
        print("  %-28s %s" % (k, v))

    if a.tsv:
        with a.tsv.open("w", newline="") as f:
            w = csv.writer(f, delimiter="\t")
            w.writerow(["section", "retail", "candidate", "delta", "paired_retail",
                        "paired_cand", "compared", "matching", "unaligned",
                        "operand_unaligned", "operand_unknown", "unmeasurable",
                        "reproduced_pct", "referents", "referents_ok",
                        "unattributed", "method"])
            for r in reps:
                un = next((c - x for n, x, c in r.rows if n == "unattributed"), 0)
                w.writerow([r.name, r.rsize, r.csize, r.delta, r.paired_r, r.paired_c,
                            r.cmp.compared, r.cmp.matching, r.cmp.differ,
                            r.cmp.slot_bad + r.cmp.slot_unres, r.cmp.slot_undec,
                            r.unmeasured_r, "%.4f" % (100.0 * r.reproduced),
                            r.cmp.ref_total, r.cmp.ref_ok, un, r.method])
        print("\n[image-diff] wrote %s" % a.tsv)
    if a.json:
        blob = dict(retail=str(RETAIL), candidate=str(CAND),
                    sections=[dict(name=r.name, retail=r.rsize, candidate=r.csize,
                                   delta=r.delta, method=r.method,
                                   buckets=[dict(name=n, retail=x, candidate=c)
                                            for n, x, c in r.rows],
                                   paired_retail=r.paired_r, paired_cand=r.paired_c,
                                   compared=r.cmp.compared, matching=r.cmp.matching,
                                   equal=r.cmp.equal, slot_symbol=r.cmp.slot_sym,
                                   slot_content=r.cmp.slot_content,
                                   operand_unaligned=r.cmp.slot_bad + r.cmp.slot_unres,
                                   operand_unknown=r.cmp.slot_undec,
                                   referents=r.cmp.ref_total,
                                   referents_ok=r.cmp.ref_ok,
                                   referent_divergent_regions=r.cmp.ref_pairs_bad,
                                   referent_reordered_regions=r.cmp.ref_pairs_reordered,
                                   referent_multiplicity_regions=
                                       r.cmp.ref_pairs_multiplicity,
                                   differing=r.cmp.differ,
                                   cand_extra=r.cmp.cand_extra,
                                   unmeasured=r.unmeasured_r,
                                   reproduced=r.reproduced,
                                   classes=dict(r.cmp.classes),
                                   notes=list(r.notes))
                              for r in reps])
        a.json.write_text(json.dumps(blob, indent=1))
        print("[image-diff] wrote %s" % a.json)
    return 0


if __name__ == "__main__":
    sys.exit(main())
