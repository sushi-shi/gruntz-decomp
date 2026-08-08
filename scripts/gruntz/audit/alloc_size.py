#!/usr/bin/env python3
"""alloc_size.py - the retail ALLOCATION-SIZE oracle for class layout.

`push <n>; call ??2@YAPAXI@Z` is ground truth for `sizeof(C)`: the immediate the
retail compiler baked into every `new C` site. Nothing else in the tree reads it.

`gruntz.cleanliness.class_sizes` checks two INTERNAL things - that every class
carries a `SIZE()`/`SIZE_UNKNOWN()` annotation, and that a declared `SIZE(C, N)`
is the N our own headers compute. Both can be self-consistently WRONG: a class
whose members are all invented agrees with itself perfectly. This tool supplies
the EXTERNAL yardstick, so the three numbers can finally be triangulated:

    retail   the `push <n>` immediate at a `new C` site      (ground truth)
    declared the `SIZE(C, N)` annotation in our headers      (our claim)
    computed clang's sizeof over our real member list        (what we emit)

ATTRIBUTION - which class is being allocated. Scan forward from the site to the
end of the enclosing function, collecting (a) every `mov [reg], <??_7X@@6B@>`
primary-vptr stamp at displacement ZERO (a nonzero disp is a member sub-object or
a secondary MI vtable - not the object's own identity) and (b) every
`call ??0X@@...`, while tracking which registers provably hold the pointer
`operator new` just returned (from the first `mov reg,eax` before any call,
through register copies; eax/ecx/edx die at each call). Four tiers, strongest
first, and the ORDER is the whole design - a factory allocates several objects
and the nearest stamp often belongs to a different one:

  vtbl    a stamp through an OWNED register. Among all owned stamps (and owned
          ctors) take the class every other is an ancestor of - an inlined ctor
          chain stamps each base in turn and the most-derived one is the object.
          This proves `sizeof(CProjectile) == 0x228` even though the only CALL at
          that site is `??0CMovingLogic`.
  ctor    no owned stamp, but exactly one ctor called with the fresh pointer in
          ecx (the object's ctor is out of line and stamps its vptr internally).
          A MEMBER's ctor takes `lea ecx,[esi+n]` and so never qualifies.
  vtbl?   no owned evidence at all (the pointer was spilled to the stack and came
          back through an untracked register). Only stamps BEFORE the next
          allocation count.
  ctor?   likewise for a ctor. Weakest; `--ctor-tier` to include these.

    python -m gruntz.audit.alloc_size              # the mismatch report
    python -m gruntz.audit.alloc_size --all        # every attributed class
    python -m gruntz.audit.alloc_size --sites      # raw per-site table
    python -m gruntz.audit.alloc_size --class CFoo # one class, with its sites
    python -m gruntz.audit.alloc_size --ctor-tier  # include the weak ctor tier

NOT A GATE. Three innocent causes of a "mismatch":
  * a deliberately PARTIAL model (we declare the retail size, model few members)
    - that is `declared == retail, computed < retail`, which is the intended
      state for a class nothing `new`s;
  * `new C[n]` with a constant count (size = n*sizeof + 4 cookie) - flagged when
    a `??_H`/`??_E` vector iterator follows;
  * a class we simply have not modelled (reported as UNMODELLED, free knowledge).
The row that matters is `retail != declared` - our claim contradicts the binary.
"""
from __future__ import annotations

import argparse
import json
import struct
from collections import defaultdict
from pathlib import Path

from gruntz.audit._textdisasm import preceding, text_insns
from gruntz.core import get_context
from gruntz.core.pe import ILT_HI, ILT_LO, IMAGEBASE

REPO = Path(__file__).resolve().parents[3]
_STRUCTS = REPO / "build/gen/structs.json"

# `operator new(unsigned int)`. Pinned in config/retail/library_labels.csv; the
# EXE has exactly one (and one `??3` delete) - MSVC 5 / NAFXCW statically linked.
OPERATOR_NEW = 0x1b9b46

# `mov dword ptr [reg], imm32` (C7 /0) at displacement ZERO. mod=00 covers most
# registers, but `[ebp]` has NO mod=00 encoding (rm=101 there means disp32-absolute),
# so cl spells it `C7 45 00 <imm32>` - mod=01 with an explicit zero disp8. Missing
# that form hid every vptr stamp through ebp, which is exactly how three
# `new CSBI_ImageSet` sites were mis-attributed to their base CStatusBarItem.
# rm=100 (SIB) is excluded in both modes: not a plain `[reg]`.
def _disp0_store(tb, o):
    """(True, length) if tb[o:] is `mov [reg+0], imm32`; the register is rm."""
    m = tb[o + 1]
    mod, rm = m >> 6, m & 7
    if mod == 0 and rm not in (4, 5):
        return True, 6
    if mod == 1 and rm != 4 and tb[o + 2] == 0:
        return True, 7
    return False, 0

# The forward scan is bounded by the NEXT new-site (in a factory switch each arm
# is `push n; call new; <ctor>; jmp end`, so the next arm's allocation is the exact
# fence) and by the enclosing function's end. The cap only matters when neither is
# known. 400 was too short: CGrunt's inlined ctor chain runs ~0x237 bytes and the
# scan stopped on the intermediate `??_7CMovingLogic` stamp, reporting 0x8d8 as
# CMovingLogic's size.
_WINDOW = 4096

# `mov r32, r/m32` (8B /r) with mod=11: modrm = 0xC0 | dst<<3 | src.
_REG_NAMES = ("eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi")


class Sweep:
    def __init__(self, ctx=None):
        self.ctx = ctx or get_context()
        self.pe = self.ctx.pe
        self.sym = self.ctx.symbols
        _n, self.tva, self.tvsz, trp, trsz = self.pe.text
        self.tb = self.pe.data[trp:trp + trsz]
        tg = {OPERATOR_NEW} | set(self.pe.thunks_to(OPERATOR_NEW))
        self.sites = sorted(
            s for t in tg for s, op in self.pe.call_index.get(t, [])
            if op == 0xE8 and not (ILT_LO <= s < ILT_HI))
        self._siteset = set(self.sites)

    # --- byte helpers -------------------------------------------------------
    def _b(self, rva, n):
        o = rva - self.tva
        return self.tb[o:o + n] if o >= 0 else b""

    def _pushed_size(self, site):
        """`operator new`'s argument: the LAST `push` before the call, when it is
        an immediate. None when the size is computed (array new, a CString grow).

        Requiring the push to be byte-adjacent to the call missed 369 of 798
        sites - cl routinely slots unrelated work between them
        (`push 0x260; mov [esi+0x1c],0x3e8; call ??2` in CreateBoomerang;
        `push 0x414; mov eax,[edi]; mov ds:g_x,eax; call ??2` in
        CImagePool::AddImageFile). Walk the DECODED stream back instead, take the
        first `push`, and stop at a `call` - anything the intervening call pushed
        it also popped, so a push on the far side of one is not this argument.
        """
        insn = text_insns()
        for p in preceding(insn, site, 12):
            mn, ops = insn[p]
            if mn in ("call", "ret", "leave"):
                return None
            if mn == "push":
                ops = ops.strip()
                if ops.startswith("0x"):
                    try:
                        return int(ops, 16)
                    except ValueError:
                        return None
                return None                # push reg / push [mem]: computed size
        return None

    def _callee(self, p):
        rel = struct.unpack_from("<i", self._b(p, 5), 1)[0]
        tgt = p + 5 + rel
        it = self.pe.ilt_target(tgt)
        return it if it is not None else tgt

    def _next_site(self, site):
        """The next `operator new` call in the image (or +inf)."""
        import bisect
        i = bisect.bisect_right(self.sites, site)
        return self.sites[i] if i < len(self.sites) else 1 << 62

    def _fence(self, site):
        """Where the forward scan stops: the enclosing function's end, or the cap.

        NOT the next new-site. A factory routinely allocates the object and then
        allocates its sub-parts before finishing construction
        (`CDDrawChildGroup::CreateDotObject`: `new CWwdGameObjectC` 0x190, then
        `new AnimWorkerObj` 0x17c, and only afterwards the CWwdGameObjectC vptr
        stamp), so fencing at the inner allocation cut the scan short and named
        the outer object after whichever BASE vptr happened to land first -
        reporting CWwdGameObjectC as 0x17c. Register ownership, not distance, is
        what separates the two objects.
        """
        lim = site + _WINDOW
        own = self.sym.owner(site)
        if own is not None:
            sz = self.sym.fsize.get(own)
            if sz:
                lim = min(lim, own + sz)
        return lim

    # --- attribution --------------------------------------------------------
    def attribute(self, site):
        """(stamps, ctors, vector) after a new-site: [(rva, ??_7 name, own)],
        [(rva, ??0 name)], and whether a vector ctor/dtor iterator follows.

        `own` is True when the vptr store goes through a register that provably
        holds the pointer `operator new` just returned (tracked from the initial
        `mov reg, eax` through register-to-register copies). A stamp through some
        OTHER register belongs to some OTHER object and must not name this one.
        """
        stamps, ctors, vector = [], [], False
        live, seen_call = set(), False
        p, lim = site + 5, self._fence(site)
        while p < lim:
            o = p - self.tva
            if o + 6 >= len(self.tb):
                break
            by = self.tb[o]
            if by == 0xE8:
                tgt = self._callee(p)
                nm = self.sym.name_of(tgt)[0]
                if nm.startswith(("??_E", "??_H", "??_I")):
                    vector = True
                if nm.startswith("??0"):
                    # `mov ecx, <fresh>` immediately before => this ctor runs ON the
                    # object we just allocated (a MEMBER's ctor takes `lea ecx,[esi+n]`
                    # and so never shows ecx holding the raw pointer).
                    ctors.append((p, nm, 1 in live))
                live -= {0, 1, 2}          # eax/ecx/edx are call-clobbered
                seen_call = True
                p += 5
                continue
            if by == 0x8B and (self.tb[o + 1] & 0xC0) == 0xC0:
                dst, src = (self.tb[o + 1] >> 3) & 7, self.tb[o + 1] & 7
                if src == 0 and not live and not seen_call:
                    # the FIRST `mov reg,eax` before any call is the fresh pointer.
                    # A byte-distance cap missed it whenever cl slotted the object's
                    # field stores first (30 bytes in CreateDotObject).
                    live.add(dst)
                elif src in live:
                    live.add(dst)
                elif dst in live:
                    live.discard(dst)        # clobbered
                p += 2
                continue
            if by == 0xC7:
                hit, ln = _disp0_store(self.tb, o)
                if hit:
                    imm = struct.unpack_from("<I", self._b(p, ln), ln - 4)[0]
                    g = self.sym.gsyms.get(imm - IMAGEBASE)
                    if g and g.startswith("??_7"):
                        stamps.append((p, g, (self.tb[o + 1] & 7) in live))
                    p += ln
                    continue
            p += 1
        return stamps, ctors, vector

    def _most_derived(self, names):
        """The one class in `names` that every other is an ANCESTOR of.

        An inlined ctor chain stamps each base's vptr in turn, so the stamps at a
        `new CDerived` site all lie on ONE inheritance path and the object's true
        class is its bottom. Textual order alone is not enough (the pointer is
        often spilled and the derived stamp comes back through a different
        register); RTTI's primary-base spine settles it. `None` when the names do
        not lie on one path - that is two objects, and the site is ambiguous.
        """
        if len(names) == 1:
            return next(iter(names))
        reg = _registry()
        for cand in names:
            ci = reg.get(cand)
            if ci is None:
                continue
            from gruntz.core.vtable_hierarchy import spine_names
            anc = set(spine_names(ci))
            if names - {cand} <= anc:
                return cand
        return None

    def rows(self):
        """[(site, size, cls, tier, owner_name, vector)] for every sized site."""
        out = []
        for site in self.sites:
            size = self._pushed_size(site)
            if size is None:
                continue
            stamps, ctors, vector = self.attribute(site)
            own_stamps = {_cls_of(n) for _, n, o in stamps if o}
            own_ctors = {_cls_of(n) for _, n, o in ctors if o}
            # The weak (not-register-owned) tiers may only look as far as the
            # next allocation. Register ownership survives an intervening `new`;
            # mere proximity does not - without this bound `CGruntzMgr::Run`, which
            # allocates eleven unrelated objects, attributed all eleven sizes to
            # whichever class happened to stamp a vptr later in the body.
            nxt = self._next_site(site)
            near_stamps = [s for s in stamps if s[0] < nxt]
            near_ctors = [c for c in ctors if c[0] < nxt]
            # STRICT PRECEDENCE, strongest first. Evidence that provably runs on
            # the pointer `operator new` just returned outranks evidence that
            # merely appears nearby: in a factory that allocates two objects, the
            # nearby stamp belongs to the OTHER one.
            if own_stamps:
                cls = self._most_derived(own_stamps | own_ctors)
                tier = "vtbl"
                if cls is None:                     # not one inheritance path
                    cls, tier = _cls_of([s for s in stamps if s[2]][-1][1]), "vtbl?"
            elif own_ctors:
                if len(own_ctors) != 1:
                    continue
                cls, tier = next(iter(own_ctors)), "ctor"
            elif near_stamps:
                cls = self._most_derived({_cls_of(n) for _, n, _o in near_stamps})
                if cls is None:
                    continue
                tier = "vtbl?"
            elif near_ctors:
                names = {_cls_of(n) for _, n, _o in near_ctors}
                if len(names) != 1:
                    continue
                cls, tier = next(iter(names)), "ctor?"
            else:
                continue
            own = self.sym.owner(site)
            out.append((site, size, cls, tier,
                        self.sym.name_of(own)[0] if own else "?", vector))
        return out


_REG = None


def _registry():
    """The RTTI/vtable class registry, built once."""
    global _REG
    if _REG is None:
        from gruntz.core.vtable_hierarchy import build_registry
        _REG = build_registry()[0]
    return _REG


def _cls_of(mangled):
    """`??_7CFoo@@6B@` / `??0CFoo@@QAE@XZ` -> `CFoo` (nested names keep the tail)."""
    body = mangled[4:] if mangled.startswith("??_7") else mangled[3:]
    return body.split("@@", 1)[0]


# --- our side ---------------------------------------------------------------
def declared_sizes():
    from gruntz.core.class_meta import positional_size_annotations
    out = {}
    for name, entries in positional_size_annotations().items():
        if name is None:
            continue
        for e in entries:
            val = e[0]
            if val is not None:
                out.setdefault(name, val)
    return out


def computed_sizes():
    if not _STRUCTS.is_file():
        return {}
    out = {}
    for e in json.load(_STRUCTS.open()):
        out.setdefault(e["name"], e.get("size"))
    return out


def def_counts():
    from gruntz.core.class_meta import iter_class_defs
    c = defaultdict(int)
    for name, _p, _l, _b in iter_class_defs():
        c[name] += 1
    return c


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--all", action="store_true",
                    help="every attributed class, matching or not")
    ap.add_argument("--sites", action="store_true", help="raw per-site table")
    ap.add_argument("--class", dest="klass", help="restrict to one class")
    ap.add_argument("--ctor-tier", action="store_true",
                    help="include the weaker ctor-only attribution tier")
    a = ap.parse_args()

    sw = Sweep()
    rows = sw.rows()
    if a.klass:
        rows = [r for r in rows if r[2] == a.klass]

    if a.sites:
        print(f"{'site':>10}  {'size':>7}  {'tier':4}  class / enclosing fn")
        for site, size, cls, tier, own, vec in rows:
            v = " [vector?]" if vec else ""
            print(f"0x{site:08x}  0x{size:<5x}  {tier:4}  {cls}{v}   <- {own}")
        return 0

    # per class: the set of sizes proven, strong tier first
    strong, weak = defaultdict(set), defaultdict(set)
    where = defaultdict(list)
    for site, size, cls, tier, own, vec in rows:
        (strong if tier.startswith("vtbl") else weak)[cls].add(size)
        where[cls].append((site, size, tier, own, vec))

    decl, comp, ndefs = declared_sizes(), computed_sizes(), def_counts()
    if not comp:
        print("[alloc_size] build/gen/structs.json missing - run `gruntz structs` "
              "for the computed-size column")

    bad_decl, bad_comp, unknown, unmodelled, split, ok = [], [], [], [], [], []
    names = set(strong) | (set(weak) if a.ctor_tier else set())
    for cls in sorted(names):
        sizes = strong.get(cls) or weak.get(cls)
        tier = "vtbl" if cls in strong else "ctor"
        if len(sizes) > 1:
            split.append((cls, sorted(sizes), tier))
            continue
        n = next(iter(sizes))
        d, c = decl.get(cls), comp.get(cls)
        if cls not in ndefs:
            unmodelled.append((cls, n, tier))
        elif d is None:
            unknown.append((cls, n, c, tier))
        elif d != n:
            bad_decl.append((cls, n, d, c, tier, ndefs[cls]))
        elif c is not None and c != n and ndefs[cls] == 1:
            bad_comp.append((cls, n, d, c, tier))
        else:
            ok.append((cls, n, tier))

    def hx(v):
        return "-" if v is None else f"0x{v:x}"

    if bad_decl:
        print(f"\n=== SIZE() CONTRADICTS RETAIL  ({len(bad_decl)}) "
              f"- our declared size is not the one retail allocates")
        for cls, n, d, c, tier, k in sorted(bad_decl, key=lambda r: -abs(r[1] - r[2])):
            dd = n - d
            print(f"  {cls:<34} retail {hx(n):>7}  declared {hx(d):>7} "
                  f"({dd:+#x})  computed {hx(c):>7}  [{tier}, {k} def]")
            for site, size, t, own, vec in where[cls][:3]:
                print(f"        0x{site:08x} push 0x{size:x}"
                      f"{' [vector?]' if vec else ''}   <- {own}")
    if bad_comp:
        print(f"\n=== WE EMIT THE WRONG sizeof  ({len(bad_comp)}) "
              f"- SIZE() agrees with retail but our members do not compute it")
        for cls, n, d, c, tier in sorted(bad_comp, key=lambda r: -abs(r[1] - r[3])):
            print(f"  {cls:<34} retail {hx(n):>7}  computed {hx(c):>7} "
                  f"({c - n:+#x})  [{tier}]")
    if split:
        print(f"\n=== AMBIGUOUS  ({len(split)}) - several sizes attributed to one "
              f"class (array new, a misattributed inline chain, or two classes)")
        for cls, sizes, tier in split:
            print(f"  {cls:<34} {', '.join(hx(s) for s in sizes)}  [{tier}]")
            for site, size, t, own, vec in where[cls]:
                print(f"        0x{site:08x} push 0x{size:x}"
                      f"{' [vector?]' if vec else ''}   <- {own}")
    if unknown:
        print(f"\n=== SIZE_UNKNOWN, RETAIL KNOWS  ({len(unknown)}) - free to bank")
        for cls, n, c, tier in unknown:
            flag = "" if c is None or c <= n else "   !! computed EXCEEDS retail"
            print(f"  {cls:<34} retail {hx(n):>7}  computed {hx(c):>7} "
                  f"[{tier}]{flag}")
    if unmodelled:
        print(f"\n=== UNMODELLED  ({len(unmodelled)}) - retail news a class we "
              f"have no definition for")
        for cls, n, tier in unmodelled:
            print(f"  {cls:<34} retail {hx(n):>7}  [{tier}]")
    if a.all and ok:
        print(f"\n=== AGREES  ({len(ok)})")
        for cls, n, tier in ok:
            print(f"  {cls:<34} 0x{n:x}  [{tier}]")

    print(f"\n{len(sw.sites)} operator-new sites, {len(rows)} with a constant size "
          f"and an attributed class; {len(strong)} classes by vptr stamp, "
          f"{len(weak)} more by ctor only.")
    print(f"contradicts SIZE(): {len(bad_decl)}   wrong computed sizeof: "
          f"{len(bad_comp)}   ambiguous: {len(split)}   "
          f"unknown-but-provable: {len(unknown)}   unmodelled: {len(unmodelled)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
