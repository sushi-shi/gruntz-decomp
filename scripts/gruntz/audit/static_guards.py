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


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--claimed", action="store_true",
                    help="only guards inside a function src/ already claims")
    ap.add_argument("--deltas", action="store_true",
                    help="print the guard->datum offset histogram and exit")
    args = ap.parse_args(argv)

    if args.deltas:
        hist = datum_deltas()
        print(f"guard -> first-stored-datum offset ({sum(hist.values())} samples):")
        for d, c in sorted(hist.items(), key=lambda t: -t[1]):
            print(f"  {d:+#8x}  x{c}")
        print("\nNo fixed relationship: the guard and its datum are independent bss objects.")
        return 0

    ent = src_claims()
    starts = [e[0] for e in ent]
    rows = guards()
    claimed, unclaimed = [], []
    for g, sites, bits in rows:
        hit = next((o for o in (owner(ent, starts, s) for s in sites) if o), None)
        (claimed if hit else unclaimed).append((g, sites, bits, hit))

    print(f"{len(rows)} function-local-static guard(s) in retail .text: "
          f"{len(claimed)} in a reconstructed function, {len(unclaimed)} not yet reconstructed")
    print("\n-- inside a function src/ claims (spell these `static T x = <init>;`) --")
    for g, sites, bits, hit in claimed:
        b = " bits=" + ",".join(hex(x) for x in bits) if bits else ""
        print(f"  guard 0x{g:06x}  n={len(sites)}{b}  -> 0x{hit[0]:06x}  {hit[2]}:{hit[3]}")
    if not args.claimed:
        print("\n-- not yet reconstructed (the body will need a local static) --")
        for g, sites, bits, _ in unclaimed:
            print(f"  guard 0x{g:06x}  n={len(sites)}  sites "
                  + " ".join(f"0x{s:06x}" for s in sites))
    return 0


if __name__ == "__main__":
    sys.path.insert(0, str(REPO / "scripts"))
    sys.exit(main())
