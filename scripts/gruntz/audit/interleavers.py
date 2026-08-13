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
import glob
import os
import re
import struct
import subprocess

from gruntz.audit.tu_layout import pooled  # the two shared special-member bands
from gruntz.core.pe import ILT_HI, REPO    # one band definition, never a local copy
from gruntz.core.symbols import SYMCSV

EXILES = REPO / "config/retail/kept-comdat-exiles.tsv"
BASE_OBJS = REPO / "build/objdiff/base"


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
        if unit == pu or pu != nu or "?" in (unit, pu):
            continue
        sites = callers.get(rva, []) + callers.get(thunk.get(rva, -1), [])
        cu = sorted({owner(s)[2] for s in sites})
        out.append((rva, name, unit, pu, cu))

    # --- classification inputs -------------------------------------------------
    # exile ledger: rows already adjudicated + re-proven per build by tu_order_check
    exiled = set()
    if EXILES.exists():
        for ln in EXILES.read_text().splitlines():
            if ln and not ln.startswith("#"):
                exiled.add(int(ln.split("\t")[0], 16))
    # base-obj definers per symbol: >1 obj == COMDAT, so the modeled-as unit is
    # labels.py's keep-last cosmetic pick, not a partition claim
    ndef = {}
    if BASE_OBJS.is_dir():
        want = {name for _, name, _u, _h, _c in out}
        objs = sorted(glob.glob(str(BASE_OBJS / "*.obj")))
        r = subprocess.run(["bash", "-c",
                            "for f in %s/*.obj; do llvm-nm --defined-only \"$f\" "
                            "2>/dev/null; done" % BASE_OBJS],
                           capture_output=True, text=True)
        for ln in r.stdout.splitlines():
            parts = ln.split(None, 2)
            if len(parts) == 3 and parts[2] in want:
                ndef[parts[2]] = ndef.get(parts[2], 0) + 1

    # RVA_COMPGEN pin addresses: compiler-materialized copies. The pin's TU is the
    # (an) emitting TU by gate proof; the retail address is the linker's KEPT copy,
    # so a foreign host records materialization order, never a partition claim.
    pins = set()
    for path in glob.glob(str(REPO / "src/**/*.cpp"), recursive=True):
        for m in re.finditer(r"RVA_COMPGEN\((0x[0-9a-fA-F]+),", open(path).read()):
            pins.add(int(m.group(1), 16))

    def klass(rva, name, cu, host):
        if rva in exiled:
            return "EXILE"      # ledgered kept-COMDAT, host-verified every build
        if pooled(rva):
            return "POOL"       # special-member band: attribution granularity only
        if rva in pins:
            return "PIN"        # RVA_COMPGEN copy: kept where the host materialized it
        if ndef.get(name, 0) > 1:
            return "COMDAT"     # multi-emitter inline: modeled-as is keep-last cosmetic
        return "DEFECT?"        # single emitter modeled elsewhere: partition signal

    print(f"{len(out)} interleaved lone methods (unit sandwiched inside another unit)\n")
    print("  rva       class    method                                    modeled-as        sits-in           called-by")
    for rva, name, unit, host, cu in out:
        cs = ",".join(cu) if cu else "(no direct caller)"
        # the strongest signal: caller unit == host unit -> home there
        star = " *" if cu == [host] else ""
        kl = klass(rva, name, cu, host)
        print(f"  0x{rva:06x} {kl:<8} {name[:40]:<40} {unit:<17} {host:<17} {cs}{star}")
    homed = sum(1 for _, _, _, h, cu in out if cu == [h])
    kc = {}
    for rva, name, unit, host, cu in out:
        kc[klass(rva, name, cu, host)] = kc.get(klass(rva, name, cu, host), 0) + 1
    print(f"\n  {homed} have caller==host (highest-confidence: home into that unit / a header it includes)")
    print("  " + "  ".join(f"{k}={v}" for k, v in sorted(kc.items())))
    print("  EXILE = kept-comdat-exiles.tsv row (re-proven per build); POOL = special-member band;")
    print("  COMDAT = >1 base-obj definer, modeled-as is labels keep-last, not a partition claim;")
    print("  DEFECT? = single definer sandwiched in a foreign run - a real re-home/partition lead.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
