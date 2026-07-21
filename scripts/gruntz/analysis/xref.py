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

Attribution is SIZE-BOUNDED: a call site is credited to a function only if it lies
within that function's [start, start+size) body (sizes from symbol_names/functions.csv).
A site in the unrecovered GAP between two carved functions is reported as
`(unrecovered fn @ ~0x..)` rather than misattributed to the previous one - the old
nearest-start-only heuristic manufactured phantom edges (a call 0x479 B past a 46-byte
function got blamed on it). Likewise `--callees` scans only the fn's own body, not up
to the next start, so it no longer reports phantom callees from the gap.

Usage (inside nix develop, or plain - only reads files + $GRUNTZ_EXE):
    python3 -m gruntz.analysis.xref 0x136180 0x139bf0        # callers of each
    python3 -m gruntz.analysis.xref CGameApp::CloseResources # by name (symbol_names)
    python3 -m gruntz.analysis.xref --callees 0x136180       # forward: its call targets
    python3 -m gruntz.analysis.xref --raw 0x136180           # list every call site (no dedup)
    python3 -m gruntz.analysis.xref --tree 0x0e35f0          # caller ancestry (depth 4):
                                                             # chases THROUGH ILT/thunk
                                                             # forwarders, expands through
                                                             # matched callers, stops on
                                                             # unmatched (frontier) fns
    python3 -m gruntz.analysis.xref --tree --depth 0 0x0e35f0  # unlimited (can be huge)
"""
import os, sys, struct, csv, bisect
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parent)
EXE = Path(os.environ.get("GRUNTZ_EXE") or REPO / "build/exe/GRUNTZ.EXE")
SYMCSV = REPO / "build/gen/symbol_names.csv"
FUNCS = REPO / "build/ghidra-enrich/exports/functions.csv"
IMAGEBASE = 0x400000

# The incremental-link (ILT) jmp-thunk band: the leading jump table the retail linker
# emits (a run of 5-byte `E9 rel32` forwarders, RVA ~0x1000..0x7c20). Real callers
# `call <ILT entry>`; the entry `jmp`s to the body. They are pass-through, not code -
# the caller tree chases THROUGH them so a target's real callers aren't hidden behind
# "jmp in (unrecovered fn @ ~0x3fda)" (that 0x3fda IS an ILT entry, not a lost body).
ILT_LO, ILT_HI = 0x1000, 0x7c20


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


def _psize(x):
    """Parse a size cell -> int bytes or None. symbol_names is hex (0x2e),
    functions.csv is decimal (46); int(_, 0) reads both. 0/blank -> None (unknown)."""
    x = str(x).strip()
    if not x:
        return None
    try:
        return int(x, 0) or None
    except ValueError:
        return None


# name maps: rva -> (name, unit). symbol_names first (matched src/), then ghidra.
# fstarts/fsize: the recovered function starts + their byte sizes. symbol_names sizes
# are authoritative for reconstructed fns (and their starts are boundaries Ghidra may
# have missed); functions.csv fills the rest. _owner() bounds attribution by size, so
# a call site in an unrecovered GAP after a fn's body is reported as unrecovered rather
# than misattributed to the previous fn (the old nearest-start-only bug).
def _names():
    names, fsize, starts, byname = {}, {}, set(), {}
    if SYMCSV.exists():
        with open(SYMCSV) as f:
            for r in csv.DictReader(f):
                try:
                    rva = int(r["rva"], 16)
                except Exception:
                    continue
                names[rva] = (r["name"], r.get("unit", ""))
                if (r.get("kind") or "func") == "func":  # data/vtable rows aren't fn starts
                    starts.add(rva)
                    sz = _psize(r.get("size", ""))
                    if sz:
                        fsize[rva] = sz
    if FUNCS.exists():
        with open(FUNCS) as f:
            for r in csv.DictReader(f):
                try:
                    rva = int(r["entry_rva"], 16)
                except Exception:
                    continue
                starts.add(rva)
                if rva not in fsize:  # symbol_names size wins (reconstructed authority)
                    sz = _psize(r.get("byte_size", ""))
                    if sz:
                        fsize[rva] = sz
                names.setdefault(rva, (r["name"], "ghidra"))
                byname.setdefault(r["name"], rva)
    for rva, (nm, _u) in names.items():
        byname.setdefault(nm, rva)
    # demangled aliases: `CClass::Member` and bare `Member` resolve too. An alias
    # shared by several fns maps to the SET of rvas; _resolve prints the candidates.
    from gruntz.analysis.caller_callee import parse_mangled
    for rva, (nm, _u) in names.items():
        if not nm.startswith("?"):
            continue
        pm = parse_mangled(nm)
        if not pm:
            continue
        cls, member, _access = pm
        if member == "ctor":
            member = cls
        elif member == "dtor":
            member = "~" + cls
        elif member in ("vec-dtor", "scalar-dtor", "op"):
            continue                       # no stable spelled name to alias
        for alias in ({f"{cls}::{member}", member} if cls else {member}):
            cur = byname.get(alias)
            if cur is None:
                byname[alias] = {rva}
            elif isinstance(cur, set):
                cur.add(rva)
            elif not alias.startswith("?"):
                byname[alias] = {cur, rva}  # ghidra flat name (often the ILT thunk)
                                            # + the body: surface both as candidates
    return names, byname, sorted(starts), fsize


def _gap_start(site, fstarts, fsize):
    """The likely START of the uncarved function containing `site` (which `_owner`
    couldn't place): the end of the nearest preceding carved function, or that
    function's start if its size is unknown. Uncarved caller sites are reported and
    de-duped by this gap start - so a stray mid-function jump label (LAB_004de400) rolls
    up to the one unrecovered function it lives in, not N phantom entries."""
    k = bisect.bisect_right(fstarts, site) - 1
    if k < 0:
        return site
    st = fstarts[k]
    sz = fsize.get(st)
    return st + sz if sz else st


def _owner(rva, fstarts, fsize):
    """The recovered function CONTAINING `rva`, or None if `rva` is in an unrecovered
    gap. Nearest start <= rva, bounded by that fn's known size: a call site past
    start+size sits between carved functions and must NOT be pinned on the previous
    one - that manufactured phantom caller edges (e.g. a boomerang-ctor call 0x479 B
    past Cancel's body was blamed on Cancel). Size unknown -> unbounded (old behavior)."""
    k = bisect.bisect_right(fstarts, rva) - 1
    if k < 0:
        return None
    start = fstarts[k]
    sz = fsize.get(start)
    if sz and rva >= start + sz:
        return None
    return start


def _resolve(arg, byname, names=None):
    try:
        return int(arg, 16)
    except ValueError:
        pass
    hit = byname.get(arg)
    if hit is None:
        sys.exit(f"[xref] '{arg}' not an RVA and not found in symbol_names/functions.csv "
                 f"(exact mangled, ghidra, `CClass::Member` and bare `Member` names resolve)")
    if isinstance(hit, int):
        return hit
    if len(hit) == 1:
        return next(iter(hit))
    lines = [f"[xref] '{arg}' is ambiguous ({len(hit)} functions) - pick one:"]
    for rva in sorted(hit):
        nm, unit = (names or {}).get(rva, ("?", "?"))
        lines.append(f"  0x{rva:08x}  [{unit}]  {nm}")
    sys.exit("\n".join(lines))


def _thunks_to(target, d, secs):
    """ILT-band thunk entry RVAs whose `E9 rel32` jumps to `target`. C++ vtables and
    the game's command tables hold these thunk addresses, not the body, so a data scan
    must look for them too."""
    tname, tva, tvsz, trp, trsz = _text(secs)
    tb = d[trp:trp + trsz]
    out = []
    i = 0
    while i < len(tb) - 4:
        if tb[i] == 0xE9 and ILT_LO <= tva + i < ILT_HI:
            rel = struct.unpack_from("<i", tb, i + 1)[0]
            if tva + i + 5 + rel == target:
                out.append(tva + i)
        i += 1
    return out


def data_refs(target, d, secs, names):
    """Data words (in non-.text sections) whose value is the VA of `target` OR of an
    ILT thunk to it: the fn-ptr tables / vtable slots / command tables that hold it
    indirectly - the references a rel32 .text scan can't see. Returns
    [(data_rva, via_thunk_rva_or_None, section)]."""
    wanted = {target + IMAGEBASE: None}
    for th in _thunks_to(target, d, secs):
        wanted[th + IMAGEBASE] = th
    hits = []
    for (name, va, vsz, rp, rsz) in secs:
        if name == ".text":
            continue
        blob = d[rp:rp + rsz]
        for i in range(0, len(blob) - 3, 4):
            w = struct.unpack_from("<I", blob, i)[0]
            if w in wanted:
                hits.append((va + i, wanted[w], name))
    return hits


def _print_data_refs(target, d, secs, names, indent="  "):
    """Print the data-side references of `target` (compact; capped)."""
    drefs = data_refs(target, d, secs, names)
    if not drefs:
        return
    print(f"{indent}-- referenced as data (fn-ptr table / vtable slot / command table):")
    for draddr, via, sec in drefs[:16]:
        vs = f"  (via thunk 0x{via:x})" if via is not None else ""
        # a nearby named symbol (the table/vtable start) helps identify it
        owner = ""
        for o in sorted((a for a in names if a <= draddr), reverse=True)[:1]:
            if draddr - o < 0x400:
                owner = f"  ~{names[o][0]}+0x{draddr - o:x}"
        print(f"{indent}   @0x{draddr:08x} [{sec}]{vs}{owner}")
    if len(drefs) > 16:
        print(f"{indent}   ... (+{len(drefs) - 16} more)")


def callers_of(targets, d, secs, names, fstarts, fsize, raw=False):
    tname, tva, tvsz, trp, trsz = _text(secs)
    tb = d[trp:trp + trsz]
    tset = set(targets)
    found = {t: [] for t in targets}
    n = len(tb) - 4  # -4: an E8/E9 in the last 5 bytes still counts
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

    for t in targets:
        tn = names.get(t, (f"FUN_{t:x}", "?"))[0]
        print(f"\n==== callers of 0x{t:08x}  {tn} ====")
        if not found[t]:
            print("  (no direct call/jmp rel32 caller in .text)")
            _print_data_refs(t, d, secs, names)
            continue
        seen = set()
        for src, op in found[t]:
            o = _owner(src, fstarts, fsize)
            kind = "call" if op == 0xE8 else "jmp "
            if o is None:  # site sits in an unrecovered gap - roll it up to the gap start
                gap = _gap_start(src, fstarts, fsize)
                if raw:
                    print(f"  {kind} @0x{src:08x}  in (unrecovered fn ~0x{gap:08x})")
                elif ("gap", gap) not in seen:
                    seen.add(("gap", gap))
                    print(f"  {kind} in (unrecovered fn @ ~0x{gap:08x})")
                continue
            nm, unit = names.get(o, (f"FUN_{o:x}", "?"))
            if raw:
                print(f"  {kind} @0x{src:08x}  in 0x{o:08x} {nm} [{unit}]")
            elif o not in seen:
                seen.add(o)
                print(f"  {kind} in 0x{o:08x} {nm} [{unit}]")


def _is_thunk(rva, names):
    """A pass-through forwarder, not real code: an ILT jump-table entry (0x1000..0x7c20)
    or a Ghidra thunk_* single-jmp forwarder. The tree chases THROUGH these."""
    if ILT_LO <= rva < ILT_HI:
        return True
    return names.get(rva, ("", ""))[0].startswith("thunk_")


def _is_matched(rva, names):
    """Reconstructed in src/ (a real symbol_names unit), vs a ghidra-only / FUN_ body
    we have not matched yet. Unmatched real fns are the attribution frontier: the tree
    expands THROUGH matched callers and STOPS on unmatched ones."""
    nm, unit = names.get(rva, (f"FUN_{rva:x}", "?"))
    return unit not in ("", "?", "ghidra") and not nm.startswith("FUN_")


def caller_tree(targets, d, secs, names, fstarts, fsize, depth_cap=0):
    """Recursive caller ancestry that goes THROUGH thunks and STOPS on unmatched fns.

    An ILT entry / thunk_* forwarder is transparently chased (its own callers are
    spliced in at the same depth, tagged `via thunk 0x..`) so a real caller is never
    hidden behind a jump-table entry. Expansion continues through MATCHED functions
    (reconstructed in src/) and stops at each UNMATCHED function - the frontier worth
    attributing - as well as at roots and unrecovered gaps. Dedup: an already-expanded
    function prints as (*seen). depth_cap=0 means unlimited; the default is 4."""
    tname, tva, tvsz, trp, trsz = _text(secs)
    tb = d[trp:trp + trsz]
    idx = {}  # callee entry-rva -> [(site, op)] over the WHOLE .text, one scan
    n = len(tb) - 4  # -4: an E8/E9 in the last 5 bytes still counts
    i = 0
    while i < n:
        op = tb[i]
        if op == 0xE8 or op == 0xE9:
            rel = struct.unpack_from("<i", tb, i + 1)[0]
            tgt = tva + i + 5 + rel
            if tva <= tgt < tva + tvsz:
                idx.setdefault(tgt, []).append((tva + i, op))
        i += 1

    def label(rva):
        nm, unit = names.get(rva, (f"FUN_{rva:x}", "?"))
        return f"0x{rva:08x} {nm} [{unit}]"

    def effective_callers(rva, via, guard):
        """Real callers of `rva`, transparently chasing through ILT/thunk forwarders.
        Yields (owner, op, site, via) where `via` is the tuple of thunk entries hopped."""
        if rva in guard:
            return
        guard.add(rva)
        for site, op in idx.get(rva, []):
            o = _owner(site, fstarts, fsize)
            if o == rva:
                continue  # intra-fn / self
            if o is None and ILT_LO <= site < ILT_HI:
                # the caller site IS an ILT entry (a lone `jmp` at the entry addr);
                # its own callers are the real ones - chase through it.
                yield from effective_callers(site, via + (site,), guard)
            elif o is not None and _is_thunk(o, names):
                yield from effective_callers(o, via + (o,), guard)
            else:
                yield (o, op, site, via)

    for t in targets:
        print(f"\n==== caller tree of {label(t)} ====")
        seen = set()

        def walk(rva, depth):
            if depth_cap and depth > depth_cap:
                print("  " * (depth + 1) + "... (--depth cap)")
                return
            owners, uniq = [], set()
            for o, op, site, via in effective_callers(rva, (), set()):
                key = (o, op) if o is not None else ("gap", _gap_start(site, fstarts, fsize))
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
                # compact: how many thunks were transparently skipped (not the full
                # address chain) - visible that the edge went through the jump table,
                # without eating the line.
                via_s = f"  (via {len(via)}t)" if via else ""
                if o is None:  # a genuine unrecovered gap (not a thunk) - a real leaf
                    gap = _gap_start(site, fstarts, fsize)
                    print(pad + f"<- {kind} (unrecovered fn @ ~0x{gap:08x}){via_s}")
                    continue
                if o in seen:
                    print(pad + f"<- {kind} {label(o)}{via_s} (*seen)")
                    continue
                seen.add(o)
                if _is_matched(o, names):
                    print(pad + f"<- {kind} {label(o)}{via_s}")
                    walk(o, depth + 1)  # go THROUGH matched callers
                else:
                    print(pad + f"<- {kind} {label(o)}{via_s}  [UNMATCHED - frontier]")
        walk(t, 0)
        _print_data_refs(t, d, secs, names)


def callees_of(targets, d, secs, names, fstarts, fsize):
    tname, tva, tvsz, trp, trsz = _text(secs)
    for t in targets:
        k = bisect.bisect_right(fstarts, t) - 1
        start = fstarts[k] if k >= 0 and fstarts[k] == t else t
        # span the fn's OWN body: prefer its known size (so we don't scan the
        # unrecovered gap up to the next start and report phantom callees); fall
        # back to next-start only when the size is unknown.
        sz = fsize.get(start)
        if sz:
            end = start + sz
        else:
            j = bisect.bisect_right(fstarts, start)
            end = fstarts[j] if j < len(fstarts) else start + 0x400
        tn = names.get(t, (f"FUN_{t:x}", "?"))[0]
        print(f"\n==== callees of 0x{t:08x}  {tn}  (span 0x{start:x}..0x{end:x}) ====")
        b = d[trp + (start - tva):trp + (end - tva)]
        seen = set()
        indirect = 0
        for i in range(max(0, len(b) - 4)):  # -4: an E8/E9 in the LAST 5 bytes counts
            if b[i] in (0xE8, 0xE9):
                rel = struct.unpack_from("<i", b, i + 1)[0]
                tgt = start + i + 5 + rel
                if tva <= tgt < tva + tvsz and tgt not in seen:
                    seen.add(tgt)
                    nm, unit = names.get(tgt, (f"FUN_{tgt:x}", "?"))
                    print(f"  -> 0x{tgt:08x} {nm} [{unit}]")
            elif b[i] == 0xFF and i + 1 < len(b) and ((b[i + 1] >> 3) & 7) in (2, 4):
                indirect += 1  # call/jmp r/m32 - vtable/IAT/fn-ptr dispatch
        if not seen:
            print("  (no direct call/jmp rel32 callee)")
        if indirect:
            print(f"  (~{indirect} indirect call/jmp site(s) - vtable/IAT/fn-ptr; "
                  "invisible to rel32 xref, use the Ghidra decomp for those)")


def main():
    args = sys.argv[1:]
    mode = "callers"
    raw = False
    depth = 4  # --depth 0 = unlimited
    rest = []
    it = iter(args)
    for a in it:
        if a == "--callees":
            mode = "callees"
        elif a == "--raw":
            raw = True
        elif a == "--tree":
            mode = "tree"
        elif a == "--depth":
            depth = int(next(it, "4"))
        else:
            rest.append(a)
    if not rest:
        sys.exit(__doc__)
    d, secs = _load()
    names, byname, fstarts, fsize = _names()
    targets = [_resolve(a, byname, names) for a in rest]
    if mode == "callees":
        callees_of(targets, d, secs, names, fstarts, fsize)
    elif mode == "tree":
        caller_tree(targets, d, secs, names, fstarts, fsize, depth_cap=depth)
    else:
        callers_of(targets, d, secs, names, fstarts, fsize, raw=raw)


if __name__ == "__main__":
    main()
