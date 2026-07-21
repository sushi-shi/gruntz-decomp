#!/usr/bin/env python3
"""interleavers.py - find lone methods COMDAT-placed INSIDE another unit's block.

A method whose RVA neighbours on BOTH sides belong to the SAME other unit is not a
separate obj - it is a COMDAT the linker placed next to the code that references it
(an inline method from a header the caller includes, or an out-of-line method defined
in the caller's .cpp). Proof of ownership: scan .text for the CALL site (direct or via
the function's /INCREMENTAL ILT jmp-thunk) - the caller's unit is where it was emitted,
so that is where the SOURCE belongs. Modelling it as a standalone .cpp (e.g.
GruntzMgr2.cpp) is a byte-correct workaround but the WRONG structure: home it to the
caller (inline-in-header or the caller's source) instead.

Usage: python -m gruntz.audit.interleavers
"""
import bisect
import csv
import os
import struct

from gruntz.core.symbols import SYMCSV

ILT_HI = 0x7c20


def _load_exe():
    d = open(os.environ["GRUNTZ_EXE"], "rb").read()
    pe = struct.unpack_from("<I", d, 0x3c)[0]
    nsec = struct.unpack_from("<H", d, pe + 6)[0]
    opt = struct.unpack_from("<H", d, pe + 20)[0]
    base = pe + 24 + opt
    text = None
    for i in range(nsec):
        q = base + i * 40
        nm = d[q:q + 8].rstrip(b"\0").decode()
        vs, va, rs, rp = struct.unpack_from("<IIII", d, q + 8)
        if nm == ".text":
            text = (va, rp, rs)
    return d, text


def _fns():
    def sz(s):
        s = (s or "").strip()
        return int(s, 16) if s.startswith("0x") else (int(s) if s else 0)
    rows = [(int(r["rva"], 16), sz(r["size"]), r["unit"], r["name"])
            for r in csv.DictReader(open(SYMCSV)) if r["kind"] == "func" and r["rva"].strip()]
    return sorted(rows)


def main():
    d, (tva, trp, trs) = _load_exe()
    body = d[trp:trp + trs]
    fns = _fns()
    starts = [f[0] for f in fns]

    def owner(rva):
        k = bisect.bisect_right(starts, rva) - 1
        return fns[k] if 0 <= k < len(fns) else (0, 0, "?", "?")

    # call-site index + ILT thunk map (E9 jmp at <0x7c20 -> real fn)
    callers, thunk = {}, {}
    for i in range(len(body) - 5):
        op = body[i]
        if op == 0xE8:
            tgt = tva + i + 5 + struct.unpack_from("<i", body, i + 1)[0]
            callers.setdefault(tgt, []).append(tva + i)
        elif op == 0xE9 and tva + i < ILT_HI:
            tgt = tva + i + 5 + struct.unpack_from("<i", body, i + 1)[0]
            thunk[tgt] = tva + i

    out = []
    for k in range(1, len(fns) - 1):
        rva, sz, unit, name = fns[k]
        pu, nu = fns[k - 1][2], fns[k + 1][2]
        if unit != pu or pu != nu or "?" in (unit, pu):
            continue
        sites = callers.get(rva, []) + callers.get(thunk.get(rva, -1), [])
        cu = sorted({owner(s)[2] for s in sites})
        out.append((rva, name, unit, pu, cu))

    print(f"{len(out)} interleaved lone methods (unit sandwiched inside another unit)\n")
    print("  rva       method                                    modeled-as        sits-in           called-by")
    for rva, name, unit, host, cu in out:
        cs = ",".join(cu) if cu else "(no direct caller)"
        # the strongest signal: caller unit == host unit -> home there
        star = " *" if cu == [host] else ""
        print(f"  0x{rva:06x} {name[:40]:<40} {unit:<17} {host:<17} {cs}{star}")
    homed = sum(1 for _, _, _, h, cu in out if cu == [h])
    print(f"\n  {homed} have caller==host (highest-confidence: home into that unit / a header it includes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
