#!/usr/bin/env python3
"""gruntz.sema.xref - `gruntz sema xref`: who calls this function? (retail
call/jmp-graph xrefs).

Scans GRUNTZ.EXE's .text for direct `call`/`jmp rel32` (E8/E9) sites whose
target is the queried RVA and reports each caller resolved to its containing
function + unit (size-bounded attribution: a site past a fn's body rolls up to
the unrecovered gap, never a phantom edge).

--tree is the DEFAULT (caller ancestry): it chases THROUGH ILT/thunk forwarders,
expands through matched callers, stops on unmatched frontier fns, AND surfaces
data/.text references - fn-ptr tables, vtable slots, and `push offset fn` /
`mov [slot], offset fn` ADDRESS-TAKINGS in code (the registration idiom, e.g.
CreateActionArea pushed into RegisterGameObjectTypes' worker table). A fn with
"no rel32 caller" is usually reached this way, so the flat scan misses it -
prefer the default. --flat opts back into the direct-caller-only scan; --callees
is the forward view. Targets: RVAs, exact mangled names, `CClass::Member`, or
bare `Member`. Runs in-process over gruntz.core (one EXE/symbol load).

    python3 -m gruntz.sema.xref 0x136180 0x139bf0     # tree (default)
    python3 -m gruntz.sema.xref CGameApp::CloseResources
    python3 -m gruntz.sema.xref --callees 0x136180
    python3 -m gruntz.sema.xref --flat 0x136180       # direct callers only
    python3 -m gruntz.sema.xref [--depth N] 0x0e35f0
"""
import bisect
import struct
import sys

from gruntz.core import get_context
from gruntz.core.pe import ILT_HI, ILT_LO, IMAGEBASE


def data_refs(ctx, target):
    """EVERY 4-byte occurrence of the VA of `target` (or of an ILT thunk to it),
    byte-granular, across ALL sections - the fn-ptr tables / vtable slots / command
    tables in data AND the address-takings in .text (`push offset fn` / `mov [slot],
    offset fn` that register a fn into a runtime table; the reason a fn with no
    rel32 caller is still reachable). Returns [(addr, via_thunk_rva_or_None, section)].
    A 4-aligned-only or non-.text-only scan misses exactly the registration idiom
    (e.g. CreateActionArea, pushed into RegisterGameObjectTypes' worker table)."""
    pe = ctx.pe
    wanted = {struct.pack("<I", target + IMAGEBASE): None}
    for th in pe.thunks_to(target):
        wanted[struct.pack("<I", th + IMAGEBASE)] = th
    hits = []
    for (name, va, vsz, rp, rsz) in pe.secs:
        blob = pe.data[rp:rp + rsz]
        for pat, via in wanted.items():
            s = 0
            while True:
                i = blob.find(pat, s)
                if i < 0:
                    break
                hits.append((va + i, via, name))
                s = i + 1
    return hits


def _print_data_refs(ctx, target, indent="  "):
    drefs = sorted(data_refs(ctx, target))
    if not drefs:
        return
    db = ctx.symbols
    names = db.names
    print(f"{indent}-- referenced as data / address-taken (fn-ptr table / vtable slot "
          f"/ command table / registration):")
    for draddr, via, sec in drefs[:16]:
        vs = f"  (via thunk 0x{via:x})" if via is not None else ""
        if sec == ".text":
            # an address-taking inside code: attribute to its containing fn - THAT
            # fn is the effective caller (it registers/stores this fn's address).
            o = db.owner(draddr)
            if o is not None:
                nm, unit = db.name_of(o)
                owner = f"  address-taken in {nm} [{unit}] (+0x{draddr - o:x})"
            else:
                owner = f"  address-taken in (unrecovered fn ~0x{db.gap_start(draddr):08x})"
        else:
            owner = ""                 # a nearby named symbol (the table start)
            for a in sorted((a for a in names if a <= draddr), reverse=True)[:1]:
                if draddr - a < 0x400:
                    owner = f"  ~{names[a][0]}+0x{draddr - a:x}"
        print(f"{indent}   @0x{draddr:08x} [{sec}]{vs}{owner}")
    if len(drefs) > 16:
        print(f"{indent}   ... (+{len(drefs) - 16} more)")


def callers_of(ctx, targets, raw=False):
    db = ctx.symbols
    for t in targets:
        print(f"\n==== callers of 0x{t:08x}  {db.name_of(t)[0]} ====")
        sites = ctx.pe.call_index.get(t, [])
        if not sites:
            print("  (no direct call/jmp rel32 caller in .text)")
            _print_data_refs(ctx, t)
            continue
        seen = set()
        for src, op in sites:
            o = db.owner(src)
            kind = "call" if op == 0xE8 else "jmp "
            if o is None:      # site sits in an unrecovered gap - roll it up
                gap = db.gap_start(src)
                if raw:
                    print(f"  {kind} @0x{src:08x}  in (unrecovered fn ~0x{gap:08x})")
                elif ("gap", gap) not in seen:
                    seen.add(("gap", gap))
                    print(f"  {kind} in (unrecovered fn @ ~0x{gap:08x})")
                continue
            nm, unit = db.name_of(o)
            if raw:
                print(f"  {kind} @0x{src:08x}  in 0x{o:08x} {nm} [{unit}]")
            elif o not in seen:
                seen.add(o)
                print(f"  {kind} in 0x{o:08x} {nm} [{unit}]")


def caller_tree(ctx, targets, depth_cap=4):
    """Recursive caller ancestry: THROUGH thunks, stops on unmatched fns.
    depth_cap=0 means unlimited."""
    db, idx = ctx.symbols, ctx.pe.call_index

    def effective_callers(rva, via, guard):
        if rva in guard:
            return
        guard.add(rva)
        for site, op in idx.get(rva, []):
            o = db.owner(site)
            if o == rva:
                continue               # intra-fn / self
            if o is None and ILT_LO <= site < ILT_HI:
                yield from effective_callers(site, via + (site,), guard)
            elif o is not None and db.is_thunk(o):
                yield from effective_callers(o, via + (o,), guard)
            else:
                yield (o, op, site, via)

    for t in targets:
        print(f"\n==== caller tree of {db.label(t)} ====")
        seen = set()

        def walk(rva, depth):
            if depth_cap and depth > depth_cap:
                print("  " * (depth + 1) + "... (--depth cap)")
                return
            owners, uniq = [], set()
            for o, op, site, via in effective_callers(rva, (), set()):
                key = (o, op) if o is not None else ("gap", db.gap_start(site))
                if key in uniq:
                    continue
                uniq.add(key)
                owners.append((o, op, site, via))
            if not owners:
                if depth == 0:
                    print("  (no direct call/jmp rel32 caller in .text)")
                return
            for o, op, site, via in owners:
                kind = "call" if op == 0xE8 else "jmp "
                pad = "  " * (depth + 1)
                via_s = f"  (via {len(via)}t)" if via else ""
                if o is None:          # a genuine unrecovered gap - a real leaf
                    print(pad + f"<- {kind} (unrecovered fn @ ~0x{db.gap_start(site):08x}){via_s}")
                    continue
                if o in seen:
                    print(pad + f"<- {kind} {db.label(o)}{via_s} (*seen)")
                    continue
                seen.add(o)
                if db.is_matched(o):
                    print(pad + f"<- {kind} {db.label(o)}{via_s}")
                    walk(o, depth + 1)  # go THROUGH matched callers
                else:
                    print(pad + f"<- {kind} {db.label(o)}{via_s}  [UNMATCHED - frontier]")
        walk(t, 0)
        _print_data_refs(ctx, t)


def callees_of(ctx, targets):
    db, pe = ctx.symbols, ctx.pe
    _n, tva, tvsz, trp, trsz = pe.text
    for t in targets:
        k = bisect.bisect_right(db.fstarts, t) - 1
        start = db.fstarts[k] if k >= 0 and db.fstarts[k] == t else t
        # span the fn's OWN body (known size beats next-start: no phantom callees
        # from the unrecovered gap)
        sz = db.fsize.get(start)
        if sz:
            end = start + sz
        else:
            j = bisect.bisect_right(db.fstarts, start)
            end = db.fstarts[j] if j < len(db.fstarts) else start + 0x400
        print(f"\n==== callees of 0x{t:08x}  {db.name_of(t)[0]}  "
              f"(span 0x{start:x}..0x{end:x}) ====")
        b = pe.data[trp + (start - tva):trp + (end - tva)]
        seen = set()
        indirect = 0
        for i in range(max(0, len(b) - 4)):
            if b[i] in (0xE8, 0xE9):
                rel = struct.unpack_from("<i", b, i + 1)[0]
                tgt = start + i + 5 + rel
                if tva <= tgt < tva + tvsz and tgt not in seen:
                    seen.add(tgt)
                    nm, unit = db.name_of(tgt)
                    print(f"  -> 0x{tgt:08x} {nm} [{unit}]")
            elif b[i] == 0xFF and i + 1 < len(b) and ((b[i + 1] >> 3) & 7) in (2, 4):
                indirect += 1          # call/jmp r/m32 - vtable/IAT/fn-ptr dispatch
        if not seen:
            print("  (no direct call/jmp rel32 callee)")
        if indirect:
            print(f"  (~{indirect} indirect call/jmp site(s) - vtable/IAT/fn-ptr; "
                  "invisible to rel32 xref, use the Ghidra decomp for those)")


def query(ctx, targets, mode="callers", raw=False, depth=4):
    rvas = [ctx.symbols.resolve(a) for a in targets]
    if mode == "callees":
        callees_of(ctx, rvas)
    elif mode == "tree":
        caller_tree(ctx, rvas, depth_cap=depth)
    else:
        callers_of(ctx, rvas, raw=raw)


def run(args) -> None:
    # --tree (recursive, thunk-chasing, data/.text-ref-surfacing) is the DEFAULT:
    # the flat direct-caller scan repeatedly missed fns reachable only through a
    # thunk or an address-taking registration. --flat opts back into it.
    mode = ("callees" if args.callees
            else "callers" if getattr(args, "flat", False)
            else "tree")
    query(get_context(), args.target, mode=mode, raw=args.raw, depth=args.depth)
    sys.exit(0)


def main():
    args = sys.argv[1:]
    if not args or "-h" in args or "--help" in args:
        print(__doc__)
        return
    mode, raw, depth, rest = "tree", False, 4, []   # --tree is the DEFAULT
    it = iter(args)
    for a in it:
        if a == "--callees":
            mode = "callees"
        elif a == "--raw":
            raw = True
        elif a == "--tree":
            mode = "tree"               # accepted for back-compat (now the default)
        elif a == "--flat":
            mode = "callers"            # opt back into the flat direct-caller scan
        elif a == "--depth":
            depth = int(next(it, "4"))
        else:
            rest.append(a)
    if not rest:
        print(__doc__)
        return
    query(get_context(), rest, mode=mode, raw=raw, depth=depth)


if __name__ == "__main__":
    main()
