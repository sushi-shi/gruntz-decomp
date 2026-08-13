#!/usr/bin/env python3
"""static_guards.py - find every function-local static in retail GRUNTZ.EXE.

MSVC 5.0 compiles `static T x = <dynamic initializer>;` inside a function into a
one-time GUARD: one flag byte per function, one BIT per static in it, tested and
set around the initializer.

    mov  al,ds:0xNNNN        ; the guard byte
    test al,0x1              ; this static's bit
    jne  done
    mov  dl,al
    or   dl,0x1
    mov  BYTE PTR ds:0xNNNN,dl
    <initializer>
    mov  ds:0xMMMM,eax       ; the static - a SEPARATE bss object
  done:

Transcribing that expansion as two file-scope globals plus an `if` is a
recurring mis-model (`u8 g_xxxSeeded` / `char g_xxxRolled` / `u8 g_xxxLoadFlags`
whose only two uses are `& bit` and `|= bit`). This scans the retail .text for
the shape and maps each hit to the enclosing `RVA()` claim in src/, so a matcher
can tell BEFORE writing a body that the function holds a local static - and a
sweep can tell whether an already-written body spells it as one.

The load-bearing discriminator is the CONDITIONAL BRANCH between the byte read
and the byte OR-store: an ordinary flags-byte update (`hdr.flags |= 0x80` in
CMulti::SendChannelStat422 0xbb0b0) reads, ORs and stores the same byte with no
branch and is NOT a guard.

Do NOT use the guard->datum address delta as a fingerprint: the two are
independent bss objects and the delta ranges -0x88..+0x258 across the image
(`--deltas` prints the histogram). See
docs/patterns/function-local-static-dynamic-init-guard.md.

  python -m gruntz.audit.static_guards            # all guards, claimed first
  python -m gruntz.audit.static_guards --claimed  # only ones in a reconstructed fn
  python -m gruntz.audit.static_guards --deltas   # guard->datum offset histogram
  python -m gruntz.audit.static_guards --verify  # are cl's own `$E` bodies byte-exact?

Three of the buckets need NO work.  A guard inside a compiler-private `$E<n>` helper
is already reconstructed - cl emits that helper from the object definition, and the
ordinal is too volatile to pin, so it can never carry an RVA().  A guard inside CRT/MFC
code is carved out.  Only the last bucket is a worklist; `--verify` turns the `$E`
bucket from an assumption into a byte proof (and that proof is what finds the real
defects: a wrong ctor argument, an undefined global, or the /GX inlining mismatch in
docs/patterns/gx-blocks-ctor-inlining-into-e-helper.md).
"""
import argparse
import bisect
import pathlib
import re
import struct
import subprocess
import sys
from collections import Counter, defaultdict

REPO = pathlib.Path(__file__).resolve().parents[3]

IMAGEBASE = 0x400000
# .data + its uninitialized (bss) tail, where every module-scope static lives.
DATA_LO, DATA_HI = 0x00629000, 0x00700000
MODRM_DISP32 = (0x05, 0x0D, 0x15, 0x1D, 0x25, 0x2D, 0x35, 0x3D)

_RVA_LINE = re.compile(
    r"([^:]+):(\d+):RVA(?:_COMPGEN)?\(0x([0-9a-f]{8}), *(0x[0-9a-f]+)"
)


def _scan(blob, base):
    """(reads, writes): absolute-addressed BYTE loads and stores, by address."""
    reads, writes = defaultdict(list), defaultdict(list)
    i, n = 0, len(blob)

    def at(o):
        return struct.unpack_from("<I", blob, o)[0]

    while i < n - 8:
        b, b1 = blob[i], blob[i + 1]
        if b == 0xF6 and b1 == 0x05:  # test byte ptr [a], imm8
            reads[at(i + 2)].append((i, blob[i + 6]))
            i += 7
        elif b == 0x80 and b1 == 0x0D:  # or byte ptr [a], imm8
            writes[at(i + 2)].append((i, blob[i + 6]))
            i += 7
        elif b == 0xA0:  # mov al, [moffs32]
            reads[at(i + 1)].append((i, None))
            i += 5
        elif b == 0xA2:  # mov [moffs32], al
            writes[at(i + 1)].append((i, None))
            i += 5
        elif b == 0x8A and b1 in MODRM_DISP32:  # mov r8, [disp32]
            reads[at(i + 2)].append((i, None))
            i += 6
        elif b == 0x88 and b1 in MODRM_DISP32:  # mov [disp32], r8
            writes[at(i + 2)].append((i, None))
            i += 6
        else:
            i += 1
    return reads, writes


def _has_cond_branch(blob, lo, hi):
    j = lo
    while j < hi:
        if blob[j] in (0x74, 0x75):
            return True
        if blob[j] == 0x0F and j + 1 < hi and blob[j + 1] in (0x84, 0x85):
            return True
        j += 1
    return False


def guards():
    """[(guard_rva, [site_rva, ...], [bit, ...])] - every local-static guard."""
    from gruntz.core import pe

    data, secs = pe.load()
    _n, base, _v, rp, rsz = pe.text(secs)
    blob = data[rp : rp + rsz]
    reads, writes = _scan(blob, base)
    out = []
    for a in sorted(set(reads) & set(writes)):
        if not (DATA_LO <= a < DATA_HI):
            continue
        sites, bits = set(), set()
        for ro, ri in reads[a]:
            for wo, wi in writes[a]:
                if 0 < wo - ro <= 0x40 and _has_cond_branch(blob, ro, wo):
                    sites.add(base + ro)
                    bits |= {b for b in (ri, wi) if b is not None}
                    break
        if sites:
            out.append((a - IMAGEBASE, sorted(sites), sorted(bits)))
    return out


def datum_deltas():
    """Counter of guard->first-stored-datum offsets (the anti-fingerprint)."""
    from gruntz.core import pe

    data, secs = pe.load()
    _n, base, _v, rp, rsz = pe.text(secs)
    blob = data[rp : rp + rsz]
    hist = Counter()
    for g, sites, _bits in guards():
        for s in sites:
            j = s - base
            while j < s - base + 0x160 and j < len(blob) - 10:
                addr = None
                if blob[j] == 0xA3:
                    addr, step = struct.unpack_from("<I", blob, j + 1)[0], 5
                elif blob[j] == 0x89 and blob[j + 1] in MODRM_DISP32:
                    addr, step = struct.unpack_from("<I", blob, j + 2)[0], 6
                elif blob[j] == 0xC7 and blob[j + 1] == 0x05:
                    addr, step = struct.unpack_from("<I", blob, j + 2)[0], 10
                else:
                    j += 1
                    continue
                if DATA_LO <= addr < DATA_HI and addr - IMAGEBASE != g:
                    hist[addr - IMAGEBASE - g] += 1
                    break
                j += step
            break
    return hist


def src_claims():
    """sorted [(rva, size, file, line)] for every RVA()/RVA_COMPGEN() in src/."""
    out = subprocess.run(
        ["rg", "-n", "--no-heading", r"^RVA(_COMPGEN)?\(0x([0-9a-f]{8}), *(0x[0-9a-f]+)", "src/"],
        capture_output=True, text=True, cwd=REPO,
    ).stdout
    ent = []
    for ln in out.splitlines():
        m = _RVA_LINE.match(ln)
        if m:
            ent.append((int(m.group(3), 16), int(m.group(4), 16), m.group(1), int(m.group(2))))
    ent.sort()
    return ent


def owner(ent, starts, rva):
    k = bisect.bisect_right(starts, rva) - 1
    if k < 0:
        return None
    e = ent[k]
    return e if (e[1] == 0 or rva < e[0] + e[1]) else None


def compgen_spans():
    """[(lo, hi, name, unit)] for every compiler-private `$E<n>` helper.

    cl EMITS these itself from a file-scope/template-static object definition, so a
    guard inside one is already reconstructed - it just cannot carry an RVA() (the
    ordinal is volatile; see labels.VOLATILE_ORDINAL_FN_RE). Reporting them as
    unwritten functions manufactures a worklist of bodies nobody can write.
    """
    from gruntz.core.dyninit import rows as dyninit_rows
    return [(r["rva"], r["rva"] + r["size"], r["owner"], r["unit"])
            for r in dyninit_rows(REPO)]


def library_spans():
    """[(rva, name, lib)] for every FID/anchored library label (CRT/MFC carve-out)."""
    out = []
    p = REPO / "config/retail/library_labels.csv"
    for ln in p.read_text().splitlines()[1:]:
        f = ln.split(",")
        if len(f) >= 3 and f[0].startswith("0x"):
            out.append((int(f[0], 16), f[1], f[2]))
    out.sort()
    return out


def _span_hit(spans, rva):
    k = bisect.bisect_right([s[0] for s in spans], rva) - 1
    return spans[k] if k >= 0 and rva < spans[k][1] else None


def _lib_hit(libs, rva, window=0x80):
    """Nearest library label at or just below `rva` (library sizes are not recorded)."""
    k = bisect.bisect_right([l[0] for l in libs], rva) - 1
    return libs[k] if k >= 0 and rva - libs[k][0] <= window else None


def verify_compgen_emitted():
    """Every retail `$E` body must appear byte-identically in some base obj.

    Masks the 4-byte fields at the BASE object's own relocation offsets on both
    sides (COFF relocates DIR32 and REL32 alike, so that offset list covers every
    address operand objdiff would mask). A row with no byte-equal counterpart is a
    real source defect - a wrong ctor argument, a global nobody defines, or a /GX
    mismatch that stopped cl inlining the ctor
    (docs/patterns/gx-blocks-ctor-inlining-into-e-helper.md).

    Returns (hits, misses); misses are the (rva, size, name, unit) rows.
    """
    from gruntz.core import pe

    data, secs = pe.load()
    _n, base, _v, rp, _rsz = pe.text(secs)

    def coff_e_bodies(path):
        d = path.read_bytes()
        nsec = struct.unpack_from("<H", d, 2)[0]
        symoff = struct.unpack_from("<I", d, 8)[0]
        nsym = struct.unpack_from("<I", d, 12)[0]
        secs_ = []
        for i in range(nsec):
            o = 20 + 40 * i
            sz = struct.unpack_from("<I", d, o + 16)[0]
            ptr = struct.unpack_from("<I", d, o + 20)[0]
            rptr = struct.unpack_from("<I", d, o + 24)[0]
            nrel = struct.unpack_from("<H", d, o + 32)[0]
            secs_.append((sz, ptr, [struct.unpack_from("<I", d, rptr + 10 * k)[0]
                                    for k in range(nrel)]))
        strtab = symoff + 18 * nsym
        out, i = [], 0
        while i < nsym:
            o = symoff + 18 * i
            raw = d[o:o + 8]
            if raw[:4] == b"\0\0\0\0":
                off = struct.unpack_from("<I", raw, 4)[0]
                nm = d[strtab + off:d.index(b"\0", strtab + off)].decode("latin1")
            else:
                nm = raw.rstrip(b"\0").decode("latin1")
            val = struct.unpack_from("<I", d, o + 8)[0]
            sec = struct.unpack_from("<h", d, o + 12)[0]
            if sec > 0 and re.match(r"^_\$E\d+$", nm):
                sz, ptr, rel = secs_[sec - 1]
                out.append((d[ptr + val:ptr + sz], [r - val for r in rel if val <= r < sz]))
            i += 1 + d[o + 17]
        return out

    def masked(b, rl):
        a = bytearray(b)
        for r in rl:
            if 0 <= r and r + 4 <= len(a):
                a[r:r + 4] = b"\0\0\0\0"
        return bytes(a)

    bodies = []
    for p in sorted((REPO / "build/objdiff/base").glob("*.obj")):
        bodies += [(p.stem, body, rl) for body, rl in coff_e_bodies(p)]

    hits, misses = [], []
    for rva, hi, nm, unit in compgen_spans():
        size = hi - rva
        rb = data[rp + (rva - base):rp + (rva - base) + size]
        for bunit, body, rl in bodies:
            if len(body) >= size and masked(body[:size], rl) == masked(rb, rl):
                hits.append((rva, size, nm, bunit))
                break
        else:
            misses.append((rva, size, nm, unit))
    return hits, misses


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--claimed", action="store_true",
                    help="only guards inside a function src/ already claims")
    ap.add_argument("--deltas", action="store_true",
                    help="print the guard->datum offset histogram and exit")
    ap.add_argument("--verify", action="store_true",
                    help="prove every compiler-private `$E` body reproduces byte-identically "
                         "in build/objdiff/base (needs a build); a miss is a real defect")
    args = ap.parse_args(argv)

    if args.deltas:
        hist = datum_deltas()
        print(f"guard -> first-stored-datum offset ({sum(hist.values())} samples):")
        for d, c in sorted(hist.items(), key=lambda t: -t[1]):
            print(f"  {d:+#8x}  x{c}")
        print("\nNo fixed relationship: the guard and its datum are independent bss objects.")
        return 0

    if args.verify:
        hits, misses = verify_compgen_emitted()
        print(f"{len(hits) + len(misses)} compiler-private `$E` bodies: "
              f"{len(hits)} reproduce byte-identically, {len(misses)} do NOT")
        for rva, size, nm, unit in misses:
            print(f"  MISSING 0x{rva:06x} size=0x{size:x}  {nm}  (last seen in {unit})")
        if misses:
            print("\nA missing body is a real source defect: a wrong ctor argument, a global\n"
                  "nobody defines, or a /GX mismatch that stopped cl inlining the ctor\n"
                  "(docs/patterns/gx-blocks-ctor-inlining-into-e-helper.md).")
        return 0

    ent = src_claims()
    starts = [e[0] for e in ent]
    cgen = compgen_spans()
    libs = library_spans()
    rows = guards()
    claimed, compgen, library, unclaimed = [], [], [], []
    for g, sites, bits in rows:
        hit = next((o for o in (owner(ent, starts, s) for s in sites) if o), None)
        if hit:
            claimed.append((g, sites, bits, hit))
            continue
        cg = next((c for c in (_span_hit(cgen, s) for s in sites) if c), None)
        if cg:
            compgen.append((g, sites, bits, cg))
            continue
        lb = next((l for l in (_lib_hit(libs, s) for s in sites) if l), None)
        if lb:
            library.append((g, sites, bits, lb))
            continue
        unclaimed.append((g, sites, bits, None))

    print(f"{len(rows)} function-local-static guard(s) in retail .text: "
          f"{len(claimed)} in a reconstructed function, "
          f"{len(compgen)} in a compiler-emitted `$E` helper, "
          f"{len(library)} in library code, "
          f"{len(unclaimed)} not yet reconstructed")
    print("\n-- inside a function src/ claims (spell these `static T x = <init>;`) --")
    for g, sites, bits, hit in claimed:
        b = " bits=" + ",".join(hex(x) for x in bits) if bits else ""
        print(f"  guard 0x{g:06x}  n={len(sites)}{b}  -> 0x{hit[0]:06x}  {hit[2]}:{hit[3]}")
    if args.claimed:
        return 0

    print("\n-- inside a compiler-emitted `$E` helper: ALREADY reconstructed, nothing to write --")
    print("   cl emits these from the object definition itself; the ordinal is volatile so they")
    print("   carry no RVA(). `--verify` proves each one reproduces byte-identically.")
    for g, sites, bits, cg in compgen:
        print(f"  guard 0x{g:06x}  n={len(sites)}  -> 0x{cg[0]:06x} {cg[2]}  [{cg[3]}]")

    print("\n-- inside library code (CRT/MFC carve-out: never reconstructed) --")
    for g, sites, bits, lb in library:
        print(f"  guard 0x{g:06x}  n={len(sites)}  -> 0x{lb[0]:06x} {lb[1]}  [{lb[2]}]")

    print("\n-- not yet reconstructed (the body will need a local static) --")
    for g, sites, bits, _ in unclaimed:
        print(f"  guard 0x{g:06x}  n={len(sites)}  sites "
              + " ".join(f"0x{s:06x}" for s in sites))
    if not unclaimed:
        print("  (none)")
    return 0


if __name__ == "__main__":
    sys.path.insert(0, str(REPO / "scripts"))
    sys.exit(main())
