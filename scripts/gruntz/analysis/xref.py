#!/usr/bin/env python3
"""gruntz.analysis.xref - who calls this function? (retail call/jmp-graph xrefs).

The delink/objdiff pipeline has no cross-reference DB, but for SEMANTIC re-homing a
matcher needs the *callers* of a function (grind/boundary/malloc stubs land on
placeholder names; the class that `new`s a ctor, or the method that calls a leaf,
is what attributes it to its real owning class/TU). This scans GRUNTZ.EXE's .text
for direct `call`/`jmp rel32` (E8/E9) sites whose target is the queried RVA and
reports each caller resolved to its containing function + unit. It is the caller-
side complement of `dump_target` (which shows a function's callees/relocs).

Names are resolved best-first: build/gen/symbol_names.csv (matched src/ names +
their units) -> ghidra functions.csv -> a FUN_<rva> fallback.

Usage (inside nix develop .#build, or plain - only reads files + $GRUNTZ_EXE):
    python3 -m gruntz.analysis.xref 0x136180 0x139bf0        # callers of each
    python3 -m gruntz.analysis.xref CGameApp::CloseResources # by name (symbol_names)
    python3 -m gruntz.analysis.xref --callees 0x136180       # forward: its call targets
    python3 -m gruntz.analysis.xref --raw 0x136180           # list every call site (no dedup)
"""
import os, sys, struct, csv, bisect
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parent)
EXE = Path(os.environ.get("GRUNTZ_EXE") or REPO / "build/exe/GRUNTZ.EXE")
SYMCSV = REPO / "build/gen/symbol_names.csv"
FUNCS = REPO / "build/ghidra-enrich/exports/functions.csv"
IMAGEBASE = 0x400000


def _load():
    d = EXE.read_bytes()
    e = struct.unpack_from("<I", d, 0x3c)[0]
    nsec = struct.unpack_from("<H", d, e + 6)[0]
    optsz = struct.unpack_from("<H", d, e + 20)[0]
    opt = e + 24
    secs = []
    for i in range(nsec):
        o = opt + optsz + i * 40
        name = d[o:o + 8].rstrip(b"\0").decode("latin1")
        vsz, va, rsz, rp = struct.unpack_from("<IIII", d, o + 8)
        secs.append((name, va, vsz, rp, rsz))
    return d, secs


def _text(secs):
    return next(s for s in secs if s[0] == ".text")


# name maps: rva -> (name, unit). symbol_names first (matched src/), then ghidra.
def _names():
    names = {}
    if SYMCSV.exists():
        with open(SYMCSV) as f:
            for r in csv.DictReader(f):
                try:
                    names[int(r["rva"], 16)] = (r["name"], r.get("unit", ""))
                except Exception:
                    pass
    fstarts = []
    byname = {}
    if FUNCS.exists():
        with open(FUNCS) as f:
            for r in csv.DictReader(f):
                try:
                    rva = int(r["entry_rva"], 16)
                except Exception:
                    continue
                fstarts.append(rva)
                names.setdefault(rva, (r["name"], "ghidra"))
                byname.setdefault(r["name"], rva)
    for rva, (nm, _u) in names.items():
        byname.setdefault(nm, rva)
    fstarts.sort()
    return names, byname, fstarts


def _resolve(arg, byname):
    try:
        return int(arg, 16)
    except ValueError:
        if arg in byname:
            return byname[arg]
        sys.exit(f"[xref] '{arg}' not an RVA and not found in symbol_names/functions.csv")


def callers_of(targets, d, secs, names, fstarts, raw=False):
    tname, tva, tvsz, trp, trsz = _text(secs)
    tb = d[trp:trp + trsz]
    tset = set(targets)
    found = {t: [] for t in targets}
    n = len(tb) - 5
    i = 0
    while i < n:
        op = tb[i]
        if op == 0xE8 or op == 0xE9:
            rel = struct.unpack_from("<i", tb, i + 1)[0]
            src = tva + i
            tgt = src + 5 + rel
            if tgt in tset:
                found[tgt].append((src, op))
        i += 1

    def owner(rva):
        k = bisect.bisect_right(fstarts, rva) - 1
        return fstarts[k] if k >= 0 else None

    for t in targets:
        tn = names.get(t, (f"FUN_{t:x}", "?"))[0]
        print(f"\n==== callers of 0x{t:08x}  {tn} ====")
        if not found[t]:
            print("  (no direct call/jmp rel32 caller in .text)")
            continue
        seen = set()
        for src, op in found[t]:
            o = owner(src)
            nm, unit = names.get(o, (f"FUN_{o:x}" if o else "?", "?"))
            kind = "call" if op == 0xE8 else "jmp "
            if raw:
                print(f"  {kind} @0x{src:08x}  in 0x{o:08x} {nm} [{unit}]")
            else:
                if o in seen:
                    continue
                seen.add(o)
                print(f"  {kind} in 0x{o:08x} {nm} [{unit}]")


def callees_of(targets, d, secs, names, fstarts):
    tname, tva, tvsz, trp, trsz = _text(secs)
    # size of a function = next-start - start (from fstarts)
    for t in targets:
        k = bisect.bisect_right(fstarts, t) - 1
        start = fstarts[k] if k >= 0 and fstarts[k] == t else t
        end = fstarts[bisect.bisect_right(fstarts, t)] if bisect.bisect_right(fstarts, t) < len(fstarts) else t + 0x400
        tn = names.get(t, (f"FUN_{t:x}", "?"))[0]
        print(f"\n==== callees of 0x{t:08x}  {tn}  (span 0x{start:x}..0x{end:x}) ====")
        b = d[trp + (start - tva):trp + (end - tva)]
        seen = set()
        for i in range(len(b) - 5):
            if b[i] in (0xE8, 0xE9):
                rel = struct.unpack_from("<i", b, i + 1)[0]
                tgt = start + i + 5 + rel
                if tva <= tgt < tva + tvsz and tgt not in seen:
                    seen.add(tgt)
                    nm, unit = names.get(tgt, (f"FUN_{tgt:x}", "?"))
                    print(f"  -> 0x{tgt:08x} {nm} [{unit}]")


def main():
    args = sys.argv[1:]
    mode = "callers"
    raw = False
    rest = []
    for a in args:
        if a == "--callees":
            mode = "callees"
        elif a == "--raw":
            raw = True
        else:
            rest.append(a)
    if not rest:
        sys.exit(__doc__)
    d, secs = _load()
    names, byname, fstarts = _names()
    targets = [_resolve(a, byname) for a in rest]
    if mode == "callees":
        callees_of(targets, d, secs, names, fstarts)
    else:
        callers_of(targets, d, secs, names, fstarts, raw=raw)


if __name__ == "__main__":
    main()
