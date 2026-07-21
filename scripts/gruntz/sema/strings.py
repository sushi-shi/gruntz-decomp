#!/usr/bin/env python3
"""gruntz.sema.strings - `gruntz sema strings`: per-fn string set / reverse lookup.

Recovers, for every recovered function, the .rdata/.data string literals it
directly references (an immediate 4-byte LE VA equal to a string start, i.e. a
`push offset`/`mov reg,offset` operand in .text). Runs in-process over
gruntz.core; the function->strings table is computed once per process.

Usage:
    python3 -m gruntz.sema.strings --rva 0x141400 [0x...]
    python3 -m gruntz.sema.strings --find WORLDZ
    python3 -m gruntz.sema.strings          # ranked report of bare FUN_ fns with
                                            # distinctive strings (labeling aid)
"""
import re
import sys

from gruntz.core import get_context

_FUNC_STRS = None


def func_strings(ctx):
    """{fn_start_rva: set(text)} - every string a recovered fn references by VA
    immediate. Cached per process."""
    global _FUNC_STRS
    if _FUNC_STRS is None:
        str_at = ctx.pe.strings_at
        db = ctx.symbols
        _n, tva, tvsz, trp, trsz = ctx.pe.text
        tb = ctx.pe.data[trp:trp + trsz]
        out = {}
        lo, hi = min(str_at), max(str_at)
        for o in range(len(tb) - 3):
            v = tb[o] | (tb[o + 1] << 8) | (tb[o + 2] << 16) | (tb[o + 3] << 24)
            if v < lo or v > hi:
                continue
            s = str_at.get(v)
            if s is None:
                continue
            fn = db.owner(tva + o)
            if fn is not None:
                out.setdefault(fn, set()).add(s)
        _FUNC_STRS = out
    return _FUNC_STRS


def print_rvas(ctx, rvas) -> int:
    fs = func_strings(ctx)
    db = ctx.symbols
    hit = 0
    for rva in rvas:
        strs = fs.get(rva, [])
        print(f"### 0x{rva:06x} {db.name_of(rva)[0]} sz={db.fsize.get(rva, 0)}")
        for s in sorted(strs):
            print("   " + repr(s))
        hit += bool(strs)
    return 0 if hit else 1                 # rc=1: answered, no strings found


def print_find(ctx, needle) -> int:
    fs = func_strings(ctx)
    db = ctx.symbols
    needle = needle.lower()
    hit = False
    for rva in sorted(fs):
        hits = [s for s in fs[rva] if needle in s.lower()]
        if hits:
            hit = True
            print(f"### 0x{rva:06x} {db.name_of(rva)[0]}")
            for s in sorted(hits):
                print("   " + repr(s))
    return 0 if hit else 1                 # rc=1: answered, no referencing fn


def print_ranked(ctx):
    """Ranked FUN_ candidates with distinctive strings (the labeling aid)."""
    dist = re.compile(r"GRUNTZ|AREA|STAGE|WORLDZ?|QUESTZ|TOOLZ|TOYZ|WARLORDZ|POWERUPZ|"
                      r"\.wwd|\.rez|\.vob|\.SF2|BOOTY|MULTI|STATEZ|SECRET|TELEPORT|CHEAT|"
                      r"DDERR_|DIERR_|DSERR_|CURSORZ|DEATHZ", re.I)
    fs = func_strings(ctx)
    db = ctx.symbols
    rows = []
    for rva, strs in fs.items():
        nm = db.name_of(rva)[0]
        if not (nm.startswith("FUN_") or nm.startswith("thunk_FUN_")):
            continue
        good = [s for s in strs if len(s) >= 4]
        score = sum(3 if dist.search(s) else 1 for s in good)
        if score:
            rows.append((score, rva, db.fsize.get(rva, 0), sorted(good)))
    rows.sort(key=lambda r: (-r[0], r[1]))
    for score, rva, sz, good in rows[:80]:
        head = " | ".join(good[:6]) + (f" (+{len(good) - 6})" if len(good) > 6 else "")
        print(f"[{score:4d}] 0x{rva:06x} sz={sz:<5d} {head}")


def run(args):
    """`gruntz sema strings` entry: <rva> or --find <text>."""
    from gruntz.sema._common import die
    ctx = get_context()
    if args.find:
        sys.exit(print_find(ctx, args.find))
    if not args.rva:
        die("sema strings: give an <rva> or --find <text>")
    sys.exit(print_rvas(ctx, [int(args.rva, 16)]))


def main():
    ctx = get_context()
    args = sys.argv[1:]
    if "--rva" in args:
        sys.exit(print_rvas(ctx, [int(t, 16) for t in args[args.index("--rva") + 1:]]))
    if "--find" in args:
        sys.exit(print_find(ctx, args[args.index("--find") + 1]))
    print_ranked(ctx)


if __name__ == "__main__":
    main()
