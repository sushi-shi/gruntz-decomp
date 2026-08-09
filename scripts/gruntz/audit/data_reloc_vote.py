#!/usr/bin/env python3
"""Address an UNPINNED data symbol out of retail's own relocation directory.

The strict pairing `data_manifest.fp_pool_rows` uses needs our DIR32 count inside
a function to EQUAL retail's, and discards the function otherwise - which is most
functions, because most are not byte-exact yet. This is the local form of the same
oracle, and it keeps the property that matters: THE PAIRING PROVES ITSELF.

  * retail's `.reloc` directory lists every site the linker wrote an absolute
    address into, so retail's DIR32 sites inside `[fn_rva, fn_rva+size)` are known
    exactly - and so are our base obj's, from its COFF relocations;
  * for each of our sites whose symbol already has an rva, the EXPECTED value is
    `rva + the addend in our own instruction bytes`. Aligning that expected
    sequence against retail's written sequence with an LCS anchors the two streams
    on every site we can already predict;
  * only a symbol we CANNOT predict, landing in a 1<->1 replace block between two
    anchored runs, has its address read off. Anchors on both sides are the proof;
  * then it is re-proven against the shipped bytes: retail's payload at the voted
    address must EQUAL our candidate's, and the address's PE storage class must be
    the one the datum's section has. A wrong reconstruction withholds the row
    instead of hiding behind it.

This is strictly stronger than content matching, which is SELF-CONFIRMING: matching
a datum by its bytes means copying retail's bytes from the address you found *by*
those bytes, so a wrong constant still scores 100. It is what named `s_HELP`
(0x2111b0, where "HELP\\0" occurs at four rvas) and what showed that the short game
strings several TUs share are one /Gf-pooled literal, not a per-TU array.

Usage:
    python -m gruntz.audit.data_reloc_vote                 # every unit
    python -m gruntz.audit.data_reloc_vote grunt warlord   # named units
"""
import bisect
import csv
import difflib
import struct
import sys
from collections import defaultdict
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(REPO / "scripts"))
sys.path.insert(0, str(REPO / "scripts/gruntz/build"))
from coff_oracle import _Coff                                    # noqa: E402
from gruntz.core.pe import PE, IMAGEBASE                         # noqa: E402
from gruntz.core.data_audit import read_pe, classify_pe_storage  # noqa: E402

EXE = REPO / "build/exe/GRUNTZ.EXE"
SYMBOLS = REPO / "build/gen/symbol_names.csv"
BASE = REPO / "build/objdiff/base"
STORAGE = {"rdata": "rdata", "data-initialized": "data",
           "data-loader-zero-tail": "bss"}
ORDINARY = {".data": "data", ".rdata": "rdata", ".bss": "bss"}
LNK_COMDAT = 0x00001000
MEM_EXECUTE = 0x20000000
COFF_DIR32 = 6


def _symbols():
    known, extent = {}, {}
    with SYMBOLS.open(newline="") as f:
        for r in csv.DictReader(l for l in f if not l.lstrip().startswith("#")):
            rva = int(r["rva"], 16)
            known[r["name"]] = rva
            if (r.get("kind") or "") == "func" and (r.get("size") or "").strip():
                extent[r["name"]] = (rva, int(r["size"], 16))
    return known, extent


def _unpinned_members(c, known):
    """{symbol: (storage, payload, size)} for this object's unpinned ordinary data."""
    out = {}
    for sec in c.section_table:
        storage = ORDINARY.get(sec["name"])
        if storage is None or sec["characteristics"] & LNK_COMDAT:
            continue
        members = sorted(c.section_members(sec["index"]))
        payload = c.section_payload(sec["index"])[:sec["size"]]
        for i, (off, name, _scl) in enumerate(members):
            if name in known:
                continue
            end = members[i + 1][0] if i + 1 < len(members) else sec["size"]
            out[name] = (storage, payload[off:end], end - off)
    return out


def _dir32(c, sec):
    ptr, count, first, rel = sec["reloc_offset"], sec["reloc_count"], 0, {}
    if not ptr:
        return rel
    if sec["characteristics"] & 0x01000000 and count == 0xFFFF:
        count = struct.unpack_from("<I", c.buf, ptr)[0]
        first = 1
    for i in range(first, count):
        site, idx, typ = struct.unpack_from("<IIH", c.buf, ptr + i * 10)
        if typ == COFF_DIR32:
            rel[site] = c.sym_name(idx)
    return rel


def votes_for(obj, pe, sites, known, extent, want):
    """{symbol: {rva: [(fn, anchors, sites)]}} from every function in one object."""
    c = _Coff(obj)
    out = defaultdict(lambda: defaultdict(list))
    for sec in c.section_table:
        if not sec["characteristics"] & MEM_EXECUTE:
            continue
        rel = _dir32(c, sec)
        if not rel:
            continue
        text = c.section_payload(sec["index"])
        for off, name in c.defined_symbols(sec["index"]):
            hit = extent.get(name)
            if hit is None:
                continue
            rva, size = hit
            mine = sorted((s, n) for s, n in rel.items() if off <= s < off + size)
            lo = bisect.bisect_left(sites, rva)
            hi = bisect.bisect_left(sites, rva + size)
            theirs = sites[lo:hi]
            if not mine or not theirs:
                continue
            expected, addends, syms = [], [], []
            for i, (site, sym) in enumerate(mine):
                addend = struct.unpack("<I", text[site:site + 4])[0]
                anchor = known.get(sym)
                # an unpredictable site gets a value nothing can match, so the LCS
                # can never anchor ON it - only around it
                expected.append(anchor + addend if anchor is not None else -(i + 1))
                addends.append(addend)
                syms.append(sym)
            written = []
            for target in theirs:
                at = pe.off(target)
                written.append(struct.unpack("<I", pe.data[at:at + 4])[0] - IMAGEBASE
                               if at is not None else -10 ** 9)
            sm = difflib.SequenceMatcher(None, expected, written, autojunk=False)
            anchors = sum(b.size for b in sm.get_matching_blocks())
            for tag, i1, i2, j1, j2 in sm.get_opcodes():
                if tag != "replace" or (i2 - i1) != 1 or (j2 - j1) != 1:
                    continue
                if syms[i1] not in want:
                    continue
                out[syms[i1]][written[j1] - addends[i1]].append(
                    (name, anchors, len(expected)))
    return c, out


def main(argv=None):
    units = set(argv if argv is not None else sys.argv[1:])
    pe, spe = PE(str(EXE)), read_pe(str(EXE))
    sites = pe.reloc_sites
    known, extent = _symbols()
    for obj in sorted(BASE.glob("*.obj")):
        if units and obj.stem not in units:
            continue
        try:
            c = _Coff(obj)
        except Exception:
            continue
        want = _unpinned_members(c, known)
        if not want:
            continue
        _c, voted = votes_for(obj, pe, sites, known, extent, want)
        for sym in sorted(voted):
            storage, payload, size = want[sym]
            candidates = sorted(voted[sym])
            if len(candidates) != 1:
                print("%-20s %-44s AMBIGUOUS %s" %
                      (obj.stem, sym[:44], [hex(x) for x in candidates]))
                continue
            rva = candidates[0]
            at = pe.off(rva)
            got = pe.data[at:at + size] if at is not None else b""
            # compare the datum, not cl's trailing inter-symbol padding
            n = payload.find(b"\0") + 1 if b"\0" in payload else size
            storage_at = classify_pe_storage(spe, rva)["class"]
            fn, anchors, total = voted[sym][rva][0]
            print("%-20s %-44s 0x%06x %-5s bytes=%-8s storage=%-18s %s %d/%d" %
                  (obj.stem, sym[:44], rva, storage,
                   "OK" if got[:n] == payload[:n] else "MISMATCH",
                   "OK" if STORAGE.get(storage_at) == storage else storage_at,
                   fn[:34], anchors, total))
    return 0


if __name__ == "__main__":
    sys.exit(main())
