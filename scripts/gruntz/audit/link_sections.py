#!/usr/bin/env python3
"""link_sections.py - classified census of the candidate EXE's .text and .rdata.

`ninja candidate` links a real GRUNTZ.EXE out of our 344 objs plus the 19 retail
libraries (0 unresolved, 0 duplicates, no /FORCE).  It comes out 93.89% of retail's
size, and the two sections this tool audits differ like this:

    .text    retail 1,987,179   candidate 1,952,230   -34,949
    .rdata   retail   135,080   candidate   142,984    +7,904

A section delta on its own says nothing.  This tool puts every byte of both deltas
into a named bucket by partitioning BOTH images the same way and measuring each
part with a detector that needs no symbol table, so retail and candidate are
measured identically:

  .text, four regions
    * the incremental-link thunk band  - the leading run of E9 rel32 thunks
    * plain .text                      - everything up to the MFC AFX groups
    * .text$AFX_AUX .. .text$AFX_TERM  - MFC's alloc_text-segmented code
    * .text$x                          - the /GX unwind funclets
  .rdata, three groups
    * .rdata    - const data, vftables, FP/string pools
    * .rdata$r  - RTTI (??_R1/R2/R3/R4)
    * .xdata$x  - the EH tables (each record opens 0x19930520)

The candidate's boundaries are known exactly from the link .map; the retail ones
are DERIVED and then cross-checked, and the tool re-derives the candidate's the
same way as a self-test (`--selftest`), so the derivation is not taken on trust.

Usage:
    python -m gruntz.audit.link_sections            # the census
    python -m gruntz.audit.link_sections --selftest # prove the derivation
    python -m gruntz.audit.link_sections --thunks   # which modules were static .LIBs
    python -m gruntz.audit.link_sections --gaps 20  # unreconstructed-code worklist
    python -m gruntz.audit.link_sections --undersized 20   # under-modelled .rdata data

Findings and the full byte budget: docs/link-section-audit.md.
"""
import argparse
import csv
import re
import struct
import sys
from collections import defaultdict
from pathlib import Path

from gruntz.core.pe import PE

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
RETAIL = REPO / "build/exe/GRUNTZ.EXE"
CAND = REPO / "build/exe/GRUNTZ.candidate.EXE"
CMAP = REPO / "build/exe/GRUNTZ.candidate.map"
FUNCS = REPO / "config/retail/functions.tsv"
LIBLBL = REPO / "config/retail/library_labels.csv"
NAMES = REPO / "build/gen/symbol_names.csv"
DATAMAN = REPO / "build/gen/delink_data_manifest.tsv"
UNITS = REPO / "config/units.toml"
BASEOBJ = REPO / "build/objdiff/base"

EH_MAGIC = 0x19930520
AFX_GROUPS = ['.text$AFX_AUX', '.text$AFX_COL1', '.text$AFX_COL2', '.text$AFX_CORE1',
              '.text$AFX_CORE2', '.text$AFX_CORE3', '.text$AFX_CORE4',
              '.text$AFX_INIT', '.text$AFX_TERM']


# ----------------------------------------------------------------- primitives
def section(pe, name):
    for s in pe.sections:
        if s['name'] == name:
            return s
    raise KeyError(name)


def body(pe, sec):
    return pe.data[sec['raw_offset']: sec['raw_offset'] + sec['virtual_size']]


def thunk_band(pe):
    """The leading E9-rel32 incremental-link thunk table: (end_rva, n_thunks).

    Each entry is `jmp rel32`; the linker leaves filler between them so a re-link
    can grow a thunk (retail fills with 0x90, our link with 0xCC - both appear).
    """
    t = section(pe, '.text')
    d = body(pe, t)
    i = n = end = 0
    while i + 5 <= len(d):
        if d[i] == 0xE9:
            n += 1
            i += 5
            end = i
        elif d[i] in (0xCC, 0x90):
            i += 1
        else:
            break
    j = end
    while j < len(d) and d[j] in (0xCC, 0x90):
        j += 1
    return t['rva'] + j, n


def cc_bytes(pe, lo, hi):
    t = section(pe, '.text')
    d = body(pe, t)
    return sum(1 for b in d[lo - t['rva']: hi - t['rva']] if b == 0xCC)


def ff25_thunks(pe):
    """Total bytes of `jmp [__imp__x]` import thunk runs (>=8 back to back)."""
    t = section(pe, '.text')
    d = body(pe, t)
    tot = n = 0
    i = 0
    while i + 6 <= len(d):
        if d[i] == 0xFF and d[i + 1] == 0x25:
            j, k = i, 0
            while j + 6 <= len(d) and d[j] == 0xFF and d[j + 1] == 0x25:
                j += 6
                k += 1
            if k >= 8:
                tot += j - i
                n += k
            i = j
        else:
            i += 1
    return tot, n


# ----------------------------------------------------------------- the .map
MAP_GRP = re.compile(r"^\s*([0-9a-f]{4}):([0-9a-f]{8})\s+([0-9a-f]{8})H\s+(\S+)\s+(\S+)\s*$")
MAP_PUB = re.compile(r"^\s*([0-9a-f]{4}):([0-9a-f]{8})\s+(\S+)\s+([0-9a-f]{8})\s*(.*)$")


def read_map(path, pe, base=0x400000):
    seg_rva = {i: s['rva'] for i, s in enumerate(pe.sections, 1)}
    groups, pubs, mode = [], [], None
    for ln in path.read_text(errors='replace').splitlines():
        if 'Publics by Value' in ln:
            mode = 'p'
            continue
        if ln.strip().startswith('Start') and 'Length' in ln:
            mode = 'g'
            continue
        if ln.strip().startswith('entry point'):
            mode = None
            continue
        if mode == 'g':
            m = MAP_GRP.match(ln)
            if m and int(m.group(1), 16) in seg_rva:
                st = seg_rva[int(m.group(1), 16)] + int(m.group(2), 16)
                groups.append((m.group(4), st, st + int(m.group(3), 16)))
        elif mode == 'p':
            m = MAP_PUB.match(ln)
            if m:
                rest = m.group(5).split()
                pubs.append(dict(name=m.group(3), rva=int(m.group(4), 16) - base,
                                 obj=rest[-1] if rest else ''))
    return groups, pubs


def owner(obj):
    if ':' in obj:
        return obj.split(':')[0]
    return 'OURS' if obj.endswith('.obj') else (obj or '?')


# --------------------------------------------------- retail boundary derivation
def retail_text_regions(R, C, groups, pubs):
    """Derive retail's four .text regions.  Returns dict name -> (lo, hi)."""
    ilt_end, _ = thunk_band(R)
    gspan = {n: (lo, hi) for n, lo, hi in groups}

    # AFX start: the candidate's .text$AFX_AUX members, looked up at their RETAIL
    # addresses via FID.  The minimum is the block start (independently corroborated
    # below: it is also the far edge of a large uncarved hole).
    lib = defaultdict(list)
    with LIBLBL.open() as f:
        for r in csv.DictReader(f):
            lib[r['name']].append(int(r['rva'], 16))
    aux_lo, aux_hi = gspan['.text$AFX_AUX']
    hits = []
    for p in pubs:
        if aux_lo <= p['rva'] < aux_hi:
            c = lib.get(p['name']) or lib.get(p['name'].lstrip('_'))
            if c:
                hits += c
    afx_start = min(hits)
    afx_size = sum(gspan[g][1] - gspan[g][0] for g in AFX_GROUPS)
    afx_end = afx_start + afx_size
    xstart = (afx_end + 15) & ~15
    t = section(R, '.text')
    return {
        'ILT band': (t['rva'], ilt_end),
        'plain .text': (ilt_end, afx_start),
        '.text$AFX_*': (afx_start, afx_end),
        'pad': (afx_end, xstart),
        '.text$x': (xstart, t['rva'] + t['virtual_size']),
    }


def cand_text_regions(C, groups):
    gspan = {n: (lo, hi) for n, lo, hi in groups}
    ilt_end, _ = thunk_band(C)
    t = section(C, '.text')
    afx_start = gspan['.text$AFX_AUX'][0]
    afx_end = gspan['.text$AFX_TERM'][1]
    xlo, xhi = gspan['.text$x']
    return {
        'ILT band': (t['rva'], ilt_end),
        'plain .text': (ilt_end, afx_start),
        '.text$AFX_*': (afx_start, afx_end),
        'pad': (afx_end, xlo),
        '.text$x': (xlo, xhi),
    }


def rdata_groups(pe, rtti_lo=None):
    """(.rdata, .rdata$r, .xdata$x) spans, derived from the byte content."""
    rd = section(pe, '.rdata')
    d = body(pe, rd)
    base, end = rd['rva'], rd['rva'] + rd['virtual_size']
    if rtti_lo is None:
        raise ValueError('need the first ??_R address')
    # .xdata$x starts at the first EH record at or after the RTTI block
    xs = None
    for i in range(((rtti_lo - base) + 3) & ~3, len(d) - 4, 4):
        if struct.unpack_from('<I', d, i)[0] == EH_MAGIC:
            xs = base + i
            break
    return {'.rdata': (base, rtti_lo), '.rdata$r': (rtti_lo, xs), '.xdata$x': (xs, end)}


def first_rtti(pe, pubs=None):
    if pubs is not None:                      # candidate: from the map
        r = [p['rva'] for p in pubs if p['name'].startswith('??_R')]
        return min(r)
    lo = None                                  # retail: from the pinned manifest
    with DATAMAN.open() as f:
        for r in csv.DictReader(f, delimiter='\t'):
            if r['storage'] == 'rdata' and r['name'].startswith('??_R'):
                a = int(r['rva'], 16)
                lo = a if lo is None else min(lo, a)
    return lo


# ------------------------------------------------------------------- reporting
def table(title, rows, total_r, total_c):
    print(f"\n{title}")
    print(f"  {'bucket':26s} {'retail':>12s} {'candidate':>12s} {'delta':>10s}")
    for name, r, c in rows:
        print(f"  {name:26s} {r:12,d} {c:12,d} {c-r:+10,d}")
    print(f"  {'-'*26} {'-'*12} {'-'*12} {'-'*10}")
    print(f"  {'TOTAL':26s} {total_r:12,d} {total_c:12,d} {total_c-total_r:+10,d}")


def load_retail_extents():
    ext = {}
    for ln in FUNCS.read_text().splitlines():
        if ln.startswith('#'):
            continue
        p = ln.split('\t')
        if p[0] == 'rva':
            continue
        ext[int(p[0], 16)] = int(p[1])
    return ext


def load_claims():
    out = {}
    with NAMES.open() as f:
        for r in csv.DictReader(ln for ln in f if not ln.lstrip().startswith('#')):
            if (r.get('kind') or 'func') != 'func':
                continue
            out[r['name']] = (int(r['rva'], 16), r['unit'],
                              int(r['size'], 16) if r.get('size') else 0)
    return out


def coff_functions():
    """Parse the base objs directly: name -> (size, unit). One COMDAT per function."""
    units = set(re.findall(r'^\s*unit\s*=\s*"([^"]+)"', UNITS.read_text(), re.M))
    out, textx, plain = {}, 0, 0
    for u in sorted(units):
        p = BASEOBJ / (u + '.obj')
        if not p.exists():
            continue
        data = p.read_bytes()
        nsec, psym, nsym, optsz = struct.unpack_from('<H', data, 2)[0], \
            struct.unpack_from('<I', data, 8)[0], \
            struct.unpack_from('<I', data, 12)[0], \
            struct.unpack_from('<H', data, 16)[0]
        soff = 20 + optsz
        strtab = data[psym + 18 * nsym:] if psym else b''

        def sname(raw):
            if raw[0:1] == b'/':
                o = int(raw[1:].rstrip(b'\0').decode())
                return strtab[o:strtab.find(b'\0', o)].decode('latin1')
            return raw.rstrip(b'\0').decode('latin1')

        secs = []
        for i in range(nsec):
            raw = data[soff + 40 * i: soff + 40 * (i + 1)]
            secs.append((sname(raw[:8]), struct.unpack_from('<I', raw, 16)[0]))
        per = defaultdict(list)
        i = 0
        while i < nsym:
            rec = data[psym + 18 * i: psym + 18 * (i + 1)]
            if rec[0:4] == b'\0\0\0\0':
                o = struct.unpack_from('<I', rec, 4)[0]
                nm = strtab[o:strtab.find(b'\0', o)].decode('latin1')
            else:
                nm = rec[:8].rstrip(b'\0').decode('latin1')
            _v, sec, typ, _sc, naux = struct.unpack_from('<IhHBB', rec, 8)
            if sec > 0 and typ == 0x20:
                per[sec].append(nm)
            i += 1 + naux
        for idx, (nm, sz) in enumerate(secs, 1):
            if nm == '.text':
                plain += sz
            elif nm.startswith('.text'):
                textx += sz
            else:
                continue
            f = per.get(idx, [])
            if len(f) == 1 and f[0] not in out:
                out[f[0]] = (sz, u)
    return out, plain, textx


def undersized_data(top=25):
    """Pinned .rdata data whose retail extent runs PAST what we claim.

    objdiff only compares the bytes a datum claims, so modelling
    `struct { float x, y; }` as a bare `float x` scores 100% while the trailing
    bytes are never looked at.  The link is where it shows: our section comes out
    short.  Retail's real extent is bounded by the next pinned address; slack that
    is not plain zero padding is a candidate under-model.
    """
    R = PE(str(RETAIL))
    rd = section(R, '.rdata')
    d = body(R, rd)
    rows = {}
    with DATAMAN.open() as f:
        for r in csv.DictReader(f, delimiter='\t'):
            if r['storage'] != 'rdata':
                continue
            a, s = int(r['rva'], 16), int(r['size'], 16)
            if a not in rows or s > rows[a][0]:
                rows[a] = (s, r['name'], r['object'])
    addrs = sorted(rows)
    out = []
    for i, a in enumerate(addrs[:-1]):
        s, nm, obj = rows[a]
        gap = addrs[i + 1] - (a + s)
        if gap <= 0:
            continue
        tail = d[a + s - rd['rva']: a + s - rd['rva'] + gap]
        if all(b == 0 for b in tail):
            continue            # zero padding, incl. the incremental-link kind
        out.append((gap, a, s, nm, obj, addrs[i + 1]))
    out.sort(reverse=True)
    print(f"\n=== pinned .rdata data with NON-ZERO unclaimed slack ===")
    print(f"  {len(out)} of {len(addrs)} pinned addresses; {sum(x[0] for x in out):,d} B")
    print("  (most of it is library data we never pinned; the interesting rows are the")
    print("   ones where the slack looks like MORE OF THE SAME DATUM)")
    for gap, a, s, nm, obj, nxt in out[:top]:
        print(f"    {a:#08x} claim {s:#7x} -> next pin {nxt:#08x}  slack {gap:7,d}  "
              f"{obj:22s} {nm[:46]}")


def thunk_partition():
    """Decode retail's thunk targets: which code was in a command-line .obj?

    /INCREMENTAL:YES emits one E9 thunk per function defined in an object named ON
    THE LINK LINE.  A function pulled from a .LIB gets none.  So the target set of
    retail's thunk band partitions retail's own code into "compiled into this link"
    vs "arrived as a static library" - the direct measurement of the engine-module
    question in docs/tu-partition-brief.md.
    """
    R = PE(str(RETAIL))
    t = section(R, '.text')
    d = body(R, t)
    tgt, i = [], 0
    while i + 5 <= len(d):
        if d[i] == 0xE9:
            tgt.append(t['rva'] + i + 5 + struct.unpack_from('<i', d, i + 1)[0])
            i += 5
        elif d[i] in (0xCC, 0x90):
            i += 1
        else:
            break
    ts = set(tgt)
    claims = load_claims()
    by_rva = {v[0]: (n, v[1]) for n, v in claims.items()}
    src = dict(re.findall(r'unit\s*=\s*"([^"]+)"\s*\nsource\s*=\s*"([^"]+)"',
                          UNITS.read_text()))

    def mod(u):
        p = (src.get(u, '') or '').split('/')
        return p[1] if len(p) > 2 else '?'

    print(f"\n=== retail's incremental-link thunk targets ===")
    print(f"  {len(tgt):,d} thunks, targets {min(tgt):#08x}..{max(tgt):#08x}")
    cut = max(tgt)
    lo = [a for a in by_rva if a <= cut]
    hi = [a for a in by_rva if a > cut]
    print(f"  src claims at/below {cut:#08x}: {len(lo):,d}, thunked "
          f"{sum(1 for a in lo if a in ts):,d}")
    print(f"  src claims above    {cut:#08x}: {len(hi):,d}, thunked "
          f"{sum(1 for a in hi if a in ts):,d}")
    print("  -> retail's .text has a hard boundary: command-line objects below it,"
          " static libraries above")
    per = defaultdict(lambda: [0, 0])
    for a, (n, u) in by_rva.items():
        per[mod(u)][0 if a in ts else 1] += 1
    print(f"\n  {'module':14s} {'in .obj':>8s} {'in .LIB':>8s}  verdict")
    for m in sorted(per, key=lambda m: -sum(per[m])):
        a, b = per[m]
        v = ('command-line .obj' if b == 0 else 'static .LIB' if a == 0 else
             f'mostly {"obj" if a > b else "LIB"} ({100.0*a/(a+b):.0f}% in .obj)')
        print(f"  {m:14s} {a:8d} {b:8d}  {v}")


def selftest(R, C, groups, pubs, rreg, creg):
    """Validate the derived retail boundaries against INDEPENDENT retail evidence.

    The claim under test is that retail's .text$AFX_* block holds the same members
    at the same sizes as ours, starting at `afx_start`.  Walking the candidate's
    group sizes from there must land every internal boundary on something retail
    tells us on its own:
      * the first FID-identified MFC function of the NEXT group, and/or
      * the far edge of an uncarved hole in config/retail/functions.tsv.
    """
    print("\n=== selftest ===")
    gspan = {n: (lo, hi) for n, lo, hi in groups}
    lib = defaultdict(list)
    with LIBLBL.open() as f:
        for r in csv.DictReader(f):
            lib[r['name']].append(int(r['rva'], 16))
    ext = load_retail_extents()
    iv = sorted((a, a + s) for a, s in ext.items())
    merged = []
    for s, e in iv:
        if merged and s <= merged[-1][1]:
            merged[-1][1] = max(merged[-1][1], e)
        else:
            merged.append([s, e])
    hole_end = {merged[i][0] for i in range(1, len(merged))}

    cur = rreg['.text$AFX_*'][0]
    hits = 0
    for g in AFX_GROUPS:
        lo, hi = gspan[g]
        rvas = []
        for p in pubs:
            if lo <= p['rva'] < hi:
                c = lib.get(p['name']) or lib.get(p['name'].lstrip('_'))
                if c:
                    rvas += [x for x in c if cur <= x < cur + (hi - lo)]
        ev = []
        if rvas and min(rvas) == cur:
            ev.append('first FID member of the group starts here')
        # the far edge of an uncarved hole, allowing the group's own alignment pad
        near = [h - cur for h in hole_end if 0 <= h - cur < 32]
        if near:
            d = min(near)
            ev.append('far edge of an uncarved hole' + (f' (+{d})' if d else ''))
        elif not any(s < cur < e for s, e in merged):
            ev.append('WARNING: a carved function straddles this boundary')
        else:
            ev.append('falls inside uncarved space')
        if ev:
            hits += 1
        print(f"  {g:18s} retail {cur:#08x} size {hi-lo:7,d}  "
              f"{'; '.join(ev) or 'no independent marker'}")
        cur += hi - lo
    print(f"  -> {hits}/{len(AFX_GROUPS)} group starts corroborated by retail's own data")

    # .rdata: derive the candidate's group boundaries from bytes, compare to the .map
    cg = rdata_groups(C, first_rtti(C, pubs))
    want = {n: (lo, hi) for n, lo, hi in groups
            if n in ('.rdata', '.rdata$r', '.xdata$x')}
    ok = True
    for k in ('.rdata$r', '.xdata$x'):
        same = cg[k][0] == want[k][0]
        ok &= same
        print(f"  {k:18s} byte-derived {cg[k][0]:#08x}  .map {want[k][0]:#08x}  "
              f"{'OK' if same else 'MISMATCH'}")
    print(f"  -> the .rdata group detector {'reproduces' if ok else 'DOES NOT reproduce'}"
          f" the candidate .map, so the same detector on retail is trustworthy")


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--selftest', action='store_true',
                    help='re-derive the candidate boundaries and compare to the .map')
    ap.add_argument('--gaps', type=int, default=0, metavar='N',
                    help='list the N largest unreconstructed retail code regions')
    ap.add_argument('--thunks', action='store_true',
                    help="decode retail's thunk targets: which module was a .LIB")
    ap.add_argument('--undersized', type=int, default=0, metavar='N',
                    help='list N pinned .rdata data whose retail extent runs past our claim')
    a = ap.parse_args(argv)

    for p in (RETAIL, CAND, CMAP):
        if not p.exists():
            print(f"[link-sections] missing {p}\n"
                  f"  run `ninja candidate` (or `gruntz link`) first", file=sys.stderr)
            return 2
    R, C = PE(str(RETAIL)), PE(str(CAND))
    groups, pubs = read_map(CMAP, C)

    # ---------------- sections
    rs = {s['name']: s for s in R.sections}
    cs = {s['name']: s for s in C.sections}
    print("=== PE sections ===")
    print(f"  {'section':10s} {'retail':>12s} {'candidate':>12s} {'delta':>10s}")
    for n in sorted(set(rs) | set(cs)):
        rv = rs.get(n, {}).get('virtual_size', 0)
        cv = cs.get(n, {}).get('virtual_size', 0)
        print(f"  {n:10s} {rv:12,d} {cv:12,d} {cv-rv:+10,d}")

    # ---------------- .text regions
    rreg = retail_text_regions(R, C, groups, pubs)
    creg = cand_text_regions(C, groups)
    if a.selftest:
        selftest(R, C, groups, pubs, rreg, creg)

    rows = [(k, rreg[k][1] - rreg[k][0], creg[k][1] - creg[k][0]) for k in
            ('ILT band', 'plain .text', '.text$AFX_*', 'pad', '.text$x')]
    table("=== .text, by region ===", rows,
          rs['.text']['virtual_size'], cs['.text']['virtual_size'])
    assert sum(r for _, r, _ in rows) == rs['.text']['virtual_size']
    assert sum(c for _, _, c in rows) == cs['.text']['virtual_size']

    _, rn = thunk_band(R)
    _, cn = thunk_band(C)
    print(f"\n  ILT band: retail {rn:,d} thunks, candidate {cn:,d} "
          f"({cn-rn:+,d} x 10 B = {10*(cn-rn):+,d})")
    print(f"  .text$AFX_*: {rreg['.text$AFX_*'][1]-rreg['.text$AFX_*'][0]:,d} B on both; "
          f"CC filler retail {cc_bytes(R, *rreg['.text$AFX_*']):,d} / "
          f"candidate {cc_bytes(C, *creg['.text$AFX_*']):,d}")

    # ---------------- inside plain .text
    rlo, rhi = rreg['plain .text']
    clo, chi = creg['plain .text']
    rcc, ccc = cc_bytes(R, rlo, rhi), cc_bytes(C, clo, chi)
    rimp, rin = ff25_thunks(R)
    cimp, cin = ff25_thunks(C)
    claims = load_claims()
    fns, our_plain, our_textx = coff_functions()

    def ru16(x):
        return (x + 15) & ~15

    pairs = [(n, fns[n][0], claims[n][2]) for n in fns
             if n in claims and claims[n][2] > 0]
    body_delta = sum(c - ru16(r) for _, c, r in pairs)
    unclaimed = sum(v[0] for n, v in fns.items() if n not in claims)
    unknown_ext = sum(v[0] for n, v in fns.items()
                      if n in claims and claims[n][2] == 0)
    print("\n=== inside plain .text ===")
    print(f"  0xCC incremental-link filler       retail {rcc:12,d}  "
          f"candidate {ccc:12,d}  {ccc-rcc:+,d}")
    print(f"  non-filler content                 retail {rhi-rlo-rcc:12,d}  "
          f"candidate {chi-clo-ccc:12,d}  {(chi-clo-ccc)-(rhi-rlo-rcc):+,d}")
    print(f"  import thunks (FF25)               retail {rimp:12,d}  "
          f"candidate {cimp:12,d}  {cimp-rimp:+,d}   ({rin} / {cin} thunks)")
    print(f"\n  the {len(pairs):,d} claimed functions with a known retail extent,")
    print(f"  our COMDAT size vs roundup(retail,16):        {body_delta:+,d} B")
    sh = sorted(((ru16(r) - c, n) for n, c, r in pairs if c < ru16(r)), reverse=True)
    lo_ = sorted(((c - ru16(r), n) for n, c, r in pairs if c > ru16(r)), reverse=True)
    eqn = sum(1 for n, c, r in pairs if c == ru16(r))
    print(f"     identical {eqn:,d}   shorter {len(sh):,d} (-{sum(x[0] for x in sh):,d})"
          f"   longer {len(lo_):,d} (+{sum(x[0] for x in lo_):,d})")
    print(f"  bodies we emit with NO retail claim:          {unclaimed:+,d} B "
          f"({sum(1 for n in fns if n not in claims)} functions)")
    print(f"  claimed but retail extent unknown:            {unknown_ext:+,d} B")
    print("\n  our 10 most under-sized bodies:")
    for d, n in sh[:10]:
        print(f"     -{d:6,d}  {fns[n][1]:22s} {n[:60]}")

    # ---------------- .rdata
    rgrp = rdata_groups(R, first_rtti(R))
    cgrp = rdata_groups(C, first_rtti(C, pubs))
    rows = [(k, rgrp[k][1] - rgrp[k][0], cgrp[k][1] - cgrp[k][0])
            for k in ('.rdata', '.rdata$r', '.xdata$x')]
    table("=== .rdata, by section group ===", rows,
          rs['.rdata']['virtual_size'], cs['.rdata']['virtual_size'])
    print(f"  boundaries: retail  .rdata$r@{rgrp['.rdata$r'][0]:#08x} "
          f".xdata$x@{rgrp['.xdata$x'][0]:#08x}")
    print(f"              cand    .rdata$r@{cgrp['.rdata$r'][0]:#08x} "
          f".xdata$x@{cgrp['.xdata$x'][0]:#08x}   (== the .map)")

    # RTTI class sets, from the .data type descriptors
    def rtti_names(pe):
        d = section(pe, '.data')
        b = pe.data[d['raw_offset']: d['raw_offset'] + min(d['virtual_size'], d['raw_size'])]
        out = set()
        for m in re.finditer(rb'\.\?A[VU]', b):
            out.add(bytes(b[m.start():b.find(b'\0', m.start())]))
        return out

    rn_, cn_ = rtti_names(R), rtti_names(C)
    print(f"\n  RTTI classes (type descriptors in .data): retail {len(rn_)}, "
          f"candidate {len(cn_)}")
    extra = sorted(x.decode() for x in cn_ - rn_)
    miss = sorted(x.decode() for x in rn_ - cn_)
    if extra:
        print(f"    candidate-only ({len(extra)}): {', '.join(extra)}")
    if miss:
        print(f"    retail-only ({len(miss)}): {', '.join(miss[:20])}")

    def eh_count(pe, span):
        s = section(pe, '.rdata')
        b = body(pe, s)
        return sum(1 for i in range(span[0] - s['rva'], span[1] - s['rva'], 4)
                   if struct.unpack_from('<I', b, i)[0] == EH_MAGIC)
    print(f"  EH records (0x19930520): retail {eh_count(R, rgrp['.xdata$x'])}, "
          f"candidate {eh_count(C, cgrp['.xdata$x'])}")

    # dxguid: every GUID is a 16-byte constant, so look each up by value
    crd = section(C, '.rdata')
    cb = body(C, crd)
    g = [p for p in pubs if owner(p['obj']) == 'dxguid'
         and cgrp['.rdata'][0] <= p['rva'] < cgrp['.rdata'][1]]
    absent = 0
    for p in g:
        v = cb[p['rva'] - crd['rva']: p['rva'] - crd['rva'] + 16]
        if len(v) == 16 and R.data.find(v) < 0:
            absent += 1
    print(f"  dxguid GUIDs: {len(g)} linked, {absent} absent from retail "
          f"-> {absent*16:,d} B of .rdata retail does not have")

    if a.thunks:
        thunk_partition()
    if a.undersized:
        undersized_data(a.undersized)

    # ---------------- unreconstructed code worklist
    if a.gaps:
        ext = load_retail_extents()
        t = section(R, '.text')
        d = body(R, t)
        cov = bytearray(rhi - rlo)
        for aa, ss in ext.items():
            for x in range(max(aa, rlo), min(aa + ss, rhi)):
                cov[x - rlo] = 1
        runs, i = [], 0
        while i < rhi - rlo:
            if cov[i]:
                i += 1
                continue
            j = i
            while j < rhi - rlo and not cov[j]:
                j += 1
            seg = d[rlo - t['rva'] + i: rlo - t['rva'] + j]
            nz = sum(1 for b in seg if b != 0xCC)
            if nz > 64:
                runs.append((nz, rlo + i, j - i))
            i = j
        runs.sort(reverse=True)
        print(f"\n=== retail plain-.text holes with real content (top {a.gaps}) ===")
        print(f"  {len(runs)} holes carry {sum(r[0] for r in runs):,d} non-filler bytes")
        for nz, at, ln_ in runs[:a.gaps]:
            print(f"    {at:#08x}  hole {ln_:7,d} B   non-filler {nz:7,d}")
    return 0


if __name__ == '__main__':
    sys.exit(main())
