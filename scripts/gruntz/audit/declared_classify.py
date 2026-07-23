#!/usr/bin/env python3
"""classify_declared_only.py - rebuild of the two-resolver drain classifier.

For every symbol in config/declared-only-baseline.tsv: find each base-obj reloc
site that references it, map the site to its retail address via the enclosing
labeled function/global ($L jump-table labels skipped), read the retail bytes to
vote the TRUE target RVA, then classify:

  ALIAS_DEF   voted RVA carries a name some base obj DEFINES  -> rename the decl/callsite
  ALIAS_CLAIM voted RVA carries a name in symbol_names.csv (no base def) -> rename
  UNRECON     voted RVA has no claim -> a real body must be reconstructed there
  DATA        voted RVA lands outside .text -> data global, define in owner TU
  UNTRUSTED   only non-fuzzy-100 code callers -> site may have shifted, needs LCS/v3
  NOSITE      no site found at all (reference exists only per llvm-nm)
  CONFLICT    trusted sites disagree

Usage: python3 classify_declared_only.py [--only SYM] > classified.tsv
"""
import glob
import json
import re
import struct
import subprocess
import sys
from collections import Counter, defaultdict
from pathlib import Path


from gruntz.audit.assert_relocs import (REPO, IMAGE_BASE, _D, _fo, target_at,
                                        load_symbols, resolve_thunk)
from gruntz.cleanliness.view_debt import _current_objs

TEXT = (0x1000, 0x1E626B)
BASELINE = Path(REPO) / "config/declared-only-baseline.tsv"

REL_DIR32 = 6
REL_DIR32NB = 7
REL_REL32 = 20


def coff(obj):
    """Parse one COFF obj: sections, symbols, relocs.
    Returns (sections, defined, relocs) where
      sections: idx -> name
      defined:  list of (name, sec_idx(1-based), value)
      relocs:   list of (sec_idx, off_in_section, type, sym_name)"""
    b = open(obj, "rb").read()
    nsec = struct.unpack_from("<H", b, 2)[0]
    symptr = struct.unpack_from("<I", b, 8)[0]
    nsym = struct.unpack_from("<I", b, 12)[0]
    opt = struct.unpack_from("<H", b, 16)[0]
    strtab = symptr + nsym * 18

    def symname(base):
        if struct.unpack_from("<I", b, base)[0] == 0:
            off = struct.unpack_from("<I", b, base + 4)[0]
            e = b.index(b"\0", strtab + off)
            return b[strtab + off:e].decode("latin1")
        return b[base:base + 8].split(b"\0")[0].decode("latin1")

    # symbol table: index -> (name, sec, value); skip aux by stepping
    syms = {}
    defined = []
    i = 0
    while i < nsym:
        base = symptr + i * 18
        nm = symname(base)
        val = struct.unpack_from("<I", b, base + 8)[0]
        sec = struct.unpack_from("<h", b, base + 12)[0]
        scls = b[base + 16]
        syms[i] = (nm, sec, val)
        if sec > 0 and scls in (2, 3, 6):  # external, static, label
            defined.append((nm, sec, val, i))
        i += 1 + b[base + 17]

    sections = {}
    relocs = []
    shdr = 20 + opt
    for s in range(nsec):
        base = shdr + s * 40
        name = b[base:base + 8].split(b"\0")[0].decode("latin1")
        sections[s + 1] = name
        preloc = struct.unpack_from("<I", b, base + 24)[0]
        nrel = struct.unpack_from("<H", b, base + 32)[0]
        for r in range(nrel):
            rb = preloc + r * 10
            va = struct.unpack_from("<I", b, rb)[0]
            si = struct.unpack_from("<I", b, rb + 4)[0]
            ty = struct.unpack_from("<H", b, rb + 8)[0]
            nm = syms.get(si, ("?", 0, 0))[0]
            relocs.append((s + 1, va, ty, nm))
    return sections, defined, relocs


def main():
    only = None
    if "--only" in sys.argv:
        only = sys.argv[sys.argv.index("--only") + 1]

    targets = {ln.split("\t")[0].strip()
               for ln in BASELINE.read_text().splitlines()
               if ln.strip() and not ln.startswith("#")}
    if only:
        targets = {only}

    sym, data, dups = load_symbols()
    # reverse map rva -> best name (prefer mangled cl names over ghidra)
    rva2name = {}
    import csv as _csv
    for r in _csv.reader(open(Path(REPO) / "build/gen/symbol_names.csv", encoding="latin-1")):
        if len(r) >= 2:
            try:
                v = int(r[0], 16)
            except ValueError:
                continue
            if v not in rva2name or (r[1].startswith("?") and not rva2name[v].startswith("?")):
                rva2name[v] = r[1]

    # fuzzy% per mangled fn name
    fuzzy = {}
    rep = json.load(open(Path(REPO) / "build/objdiff/report.json"))
    for u in rep["units"]:
        for f in u.get("functions", []):
            fuzzy[f["name"]] = f.get("fuzzy_match_percent", 0.0)

    # defined set across base objs (for ALIAS_DEF vs ALIAS_CLAIM)
    base_defined = set()
    objs = _current_objs()
    for o in objs:
        out = subprocess.run(["llvm-nm", str(o)], capture_output=True, text=True).stdout
        for ln in out.splitlines():
            p = ln.split()
            if len(p) == 3 and p[1] != "U":
                base_defined.add(p[2])

    # walk every obj once, collect sites per target symbol
    sites = defaultdict(list)  # tgt -> [(obj, encl_name, encl_kind, site_rva, type, trusted)]
    for o in objs:
        try:
            sections, defined, relocs = coff(o)
        except Exception as e:
            print(f"# parse fail {o}: {e}", file=sys.stderr)
            continue
        want = [r for r in relocs if r[3] in targets]
        if not want:
            continue
        # section -> sorted defined syms (skip $L jump-table labels + section names)
        per_sec = defaultdict(list)
        for nm, sec, val, idx in defined:
            if nm.startswith("$L") or nm.startswith(".") or nm.startswith("__ehhandler") \
               or nm.startswith("__unwindfunclet") or nm.startswith("$SG"):
                continue
            per_sec[sec].append((val, nm))
        for sec in per_sec:
            per_sec[sec].sort()
        for sec_idx, off, ty, nm in want:
            secname = sections.get(sec_idx, "?")
            cands = [x for x in per_sec.get(sec_idx, []) if x[0] <= off]
            if not cands:
                sites[nm].append((o, "?", secname, None, ty, False))
                continue
            eval_, ename = cands[-1]
            # enclosing symbol's retail rva
            frva = sym.get(ename) or sym.get(ename.replace("\xef\xbf\xbd", "_"))
            if frva is None:
                sites[nm].append((o, ename, secname, None, ty, False))
                continue
            site_rva = frva + (off - eval_)
            if secname.startswith(".text"):
                trusted = fuzzy.get(ename, 0.0) == 100.0
                kind = "text"
            else:
                trusted = True   # data-section slot (vtable etc.) - layout exact
                kind = secname
            sites[nm].append((o, ename, kind, site_rva, ty, trusted))

    # classify each target
    print("symbol\tclass\tvoted_rva\treal_name\tsites\tdetail")
    tally = Counter()
    for t in sorted(targets):
        ss = sites.get(t, [])
        if not ss:
            tally["NOSITE"] += 1
            print(f"{t}\tNOSITE\t-\t-\t0\t-")
            continue
        votes = Counter()
        for (o, ename, kind, site_rva, ty, trusted) in ss:
            if site_rva is None or not trusted:
                continue
            typ = "REL32" if ty == REL_REL32 else "DIR32"
            v = target_at(site_rva, typ)
            if v is not None:
                votes[v] += 1
        detail = ";".join(
            f"{Path(o).stem}:{ename}:{kind}:{'T' if tr else 'u'}"
            for (o, ename, kind, sr, ty, tr) in ss[:6])
        if not votes:
            tally["UNTRUSTED"] += 1
            print(f"{t}\tUNTRUSTED\t-\t-\t{len(ss)}\t{detail}")
            continue
        if len(votes) > 1:
            # tolerate off-by-small (symbol+offset data refs): if all within 16 bytes, take min
            vs = sorted(votes)
            if vs[-1] - vs[0] > 0x40:
                tally["CONFLICT"] += 1
                pretty = ",".join(f"0x{v:06x}x{c}" for v, c in votes.most_common())
                print(f"{t}\tCONFLICT\t{pretty}\t-\t{len(ss)}\t{detail}")
                continue
        rva = votes.most_common(1)[0][0]
        name = rva2name.get(rva, "")
        bytes12 = _D[_fo(rva):_fo(rva) + 12].hex() if _D is not None else ""
        if not (TEXT[0] <= rva < TEXT[1]):
            cls = "DATA"
        elif name and name in base_defined:
            cls = "ALIAS_DEF"
        elif name:
            cls = "ALIAS_CLAIM"
        else:
            cls = "UNRECON"
        tally[cls] += 1
        print(f"{t}\t{cls}\t0x{rva:06x}\t{name or bytes12}\t{len(ss)}\t{detail}")
    print("# " + "  ".join(f"{k}:{v}" for k, v in tally.most_common()), file=sys.stderr)


if __name__ == "__main__":
    main()
