#!/usr/bin/env python3
"""vtable_owner.py - "which class OWNS this function?", answered from the binary.

WHY THIS EXISTS.

A destructor is the identity fingerprint of a class, and the binary states that identity
exactly once: in the class's own vtable. But it states it INDIRECTLY, through two hops that
every by-hand attribution gets wrong:

    ??_7Class  slot 0/1  ->  ILT jmp-thunk  ->  ??_GClass (scalar-deleting dtor)
                                                    |  call
                                                    v
                                                ??1Class   <- the body you are looking at

`vtable_scan.find_holding()` walks the first hop (slot -> thunk -> fn). It does NOT walk the
second, so a base destructor - which is what almost every unattributed leaf body IS - looks
like it is in NO vtable at all. That gap is why a whole family of placeholders was
mis-explained for months as "N COMDAT copies of one base dtor".

That explanation is IMPOSSIBLE and this tool exists to make the refutation cheap:

    MSVC5 keeps exactly ONE COMDAT per mangled name. The linker discards the duplicates.
    So N byte-identical empty destructors in the image can NEVER be N copies of one
    ~Base - they can only be N DIFFERENT classes, each with its own vtable.

Run the probe on any one of them and you get its distinct vtable back, with the RTTI name
when the COL survived. Eight placeholders were dissolved this way in one pass, and three
`VTBL()` bindings turned out to point at the WRONG class (a silent wrong-dispatch bug that a
green gate cannot see, because uniqueness and coverage are both satisfied by a lie).

    python -m gruntz.cleanliness.vtable_owner 0x0000b940     # who owns this body?
    python -m gruntz.cleanliness.vtable_owner --audit        # re-derive EVERY src VTBL() binding
    python -m gruntz.cleanliness.vtable_owner --audit --all  # ... including the ones that agree
"""
import argparse
import bisect
import re
import struct
import sys
from pathlib import Path

from gruntz.core import vtable_scan as vs
from gruntz.core.class_meta import _blank_comments as blank_comments


def _find_repo():
    for base in (Path.cwd(), Path(__file__).resolve().parent):
        for p in (base, *base.parents):
            if (p / "flake.nix").exists():
                return p
    return Path(__file__).resolve().parents[3]


REPO = _find_repo()
IMAGEBASE = vs.IMAGEBASE


# ---------------------------------------------------------------------------
# the second hop: ??1 <- (call) -- ??_G
# ---------------------------------------------------------------------------
def containing_fn(rva):
    """The Ghidra-carved function that CONTAINS `rva` (vtable_scan already loaded the
    carve as FN/FN_STARTS)."""
    i = bisect.bisect_right(vs.FN_STARTS, rva) - 1
    if i < 0:
        return None
    s = vs.FN_STARTS[i]
    _nm, sz = vs.FN[s]
    return s if rva < s + max(sz, 1) else None


_CALLERS = None


def callers_of(fn_rva):
    """Every function whose body contains a `call rel32` (E8) to fn_rva, thunks chased."""
    global _CALLERS
    if _CALLERS is None:
        _CALLERS = {}
        for (nm, va, vsz, rsz, rp, ch) in vs.SECS:
            if nm != ".text":
                continue
            blob = vs.d[rp:rp + rsz]
            for i in range(len(blob) - 5):
                if blob[i] != 0xE8:
                    continue
                site = va + i
                rel = struct.unpack_from("<i", blob, i + 1)[0]
                tgt = (site + 5 + rel) & 0xFFFFFFFF
                tgt = vs.chase_thunk(tgt) or tgt
                _CALLERS.setdefault(tgt, []).append(site)
    out = []
    for site in _CALLERS.get(fn_rva, []):
        f = containing_fn(site)
        if f is not None:
            out.append((f, site))
    return out


# ---------------------------------------------------------------------------
# the owner query
# ---------------------------------------------------------------------------
def is_scalar_deleting_dtor(rva):
    """The MSVC `??_G` shape, recognised from the bytes - NOT from the name:

        push esi ; mov esi,ecx ; call ??1Class ; test BYTE PTR [esp+8],1 ; je .. ;
        push esi ; call ??3@YAXPAX@Z ; add esp,4 ; mov eax,esi ; pop esi ; ret 4

    The load-bearing tell is the deleting-flag test `F6 44 24 xx 01` inside a SMALL body
    that returns `this` (`ret 4`). Without this filter the ??1 <- caller hop would accept
    ANY vtable method that merely CALLS the destructor (a Close(), a Reset(), a manager
    teardown) and would hand back that method's class as the "owner" - which is exactly
    the false attribution this tool exists to kill."""
    ent = vs.FN.get(rva)
    if not ent:
        return False
    _nm, size = ent
    if not (4 <= size <= 0x50):
        return False
    o = vs.off(rva)
    if o is None:
        return False
    body = vs.d[o:o + size]
    if b"\xf6\x44\x24" not in body:      # test BYTE PTR [esp+imm8], 1
        return False
    return body.endswith(b"\xc2\x04\x00") or b"\xc2\x04\x00" in body[-8:]


def owner_of(fn_rva):
    """[(vtable, slot, via_thunk, sdd_rva_or_None)] - every vtable that dispatches to
    fn_rva, either DIRECTLY in a slot or INDIRECTLY through its scalar-deleting dtor."""
    hits = [(v, k, t, None) for (v, k, t) in vs.find_holding(fn_rva)]
    seen = {(v["start"], k) for (v, k, _t, _s) in hits}
    for sdd, _site in callers_of(fn_rva):
        if not is_scalar_deleting_dtor(sdd):
            continue
        for v, k, t in vs.find_holding(sdd):
            if (v["start"], k) not in seen:
                seen.add((v["start"], k))
                hits.append((v, k, t, sdd))
    return hits


# ---------------------------------------------------------------------------
# what src claims
# ---------------------------------------------------------------------------
_VTBL_RE = re.compile(r"\bVTBL\(\s*([A-Za-z_]\w*)\s*,\s*(0x[0-9a-fA-F]+)\s*\)", re.S)
# an RVA()-annotated definition of a destructor:  RVA(0xa, 0xb)\n Class::~Class(
_DTOR_RE = re.compile(
    r"RVA\(\s*(0x[0-9a-fA-F]+)\s*,[^)]*\)\s*(?://[^\n]*\n\s*)*([A-Za-z_]\w*)\s*::\s*~\s*\2\s*\(", re.S
)


def _src_files():
    for d in ("src", "include"):
        for p in (REPO / d).rglob("*"):
            if p.suffix in (".cpp", ".h"):
                yield p


def src_claims():
    """({class: (rva, file:line)}, {class: (dtor_rva, file:line)})

    Comments are BLANKED first: a `VTBL(...)` quoted in prose (e.g. "the old
    VTBL(CButeStore, ..) bound them to a class that no longer exists") is not a
    claim, and labels.py's own scan blanks comments the same way."""
    vtbl, dtor = {}, {}
    for p in _src_files():
        txt = blank_comments(p.read_text(errors="ignore"))
        for m in _VTBL_RE.finditer(txt):
            line = txt.count("\n", 0, m.start()) + 1
            vtbl[m.group(1)] = (int(m.group(2), 16), f"{p.relative_to(REPO)}:{line}")
        for m in _DTOR_RE.finditer(txt):
            line = txt.count("\n", 0, m.start()) + 1
            dtor[m.group(2)] = (int(m.group(1), 16), f"{p.relative_to(REPO)}:{line}")
    return vtbl, dtor


# ---------------------------------------------------------------------------
# reporting
# ---------------------------------------------------------------------------
def _vt_label(v):
    return v["rtti"] or "(no RTTI COL)"


def _slot_digest(v, n=4):
    parts = []
    for k, _sr, _raw, body in vs.iter_slots(v):
        if k >= n:
            break
        parts.append("[%d] 0x%06x" % (k, body))
    return "  ".join(parts)


def explain(rva, vtbl_by_class):
    by_rva = {r: (c, w) for c, (r, w) in vtbl_by_class.items()}
    print("RVA 0x%06x  %s" % (rva, vs.fn_label(rva) or ""))
    hits = owner_of(rva)
    if not hits:
        print("  no vtable dispatches to it (a non-virtual method, a free function, or a")
        print("  dead COMDAT the linker kept - MSVC5 has no /OPT:REF).")
        return
    for v, k, via, sdd in hits:
        chain = "??_7 @0x%06x slot %d" % (v["start"], k)
        if via:
            chain += " -> ILT thunk"
        if sdd is not None:
            chain += " -> sdd 0x%06x -> call" % sdd
        print("  %s -> 0x%06x" % (chain, rva))
        print("      vtable 0x%06x  %d slots  conf=%s  RTTI: %s"
              % (v["start"], v["size"], vs.confidence(v), _vt_label(v)))
        print("      slots: %s" % _slot_digest(v))
        claim = by_rva.get(v["start"])
        if claim:
            print("      src binds this rva: VTBL(%s)  [%s]" % (claim[0], claim[1]))
        else:
            print("      src binds this rva: (nothing)")


def rtti_leaf(decorated):
    """Leaf class identifier of a decorated TypeDescriptor name: ".?AVCFoo@@" ->
    "CFoo", ".?AVCGameMgr@WAP32@@" -> "CGameMgr" (namespaces follow the leaf),
    ".?AV?$CArray@..." -> "?$CArray" (a template - matchable by NO plain
    identifier, so any VTBL(ident, ..) on it is wrong by construction)."""
    s = decorated
    for pre in (".?AV", ".?AU", ".?AW", ".?AT"):
        if s.startswith(pre):
            s = s[len(pre):]
            break
    return s.split("@", 1)[0]


def rtti_check(vtbl):
    """RTTI cross-check: for every VTBL claim whose rva carries an RTTI Complete
    Object Locator, the COL's class name is GROUND TRUTH. A VTBL(cls, rva) whose
    RTTI leaf differs from `cls` - or whose COL says base_off != 0 (an MI secondary
    vtable, whose mangled name embeds the base and can never be the plain
    ??_7cls@@6B@ that VTBL emits) - is a MISBINDING the dtor-based audit cannot see
    when the class has no RVA()-bound destructor (the CWorker39f20/CMovieScratch
    conflation at 0x1e971c == CArray<PLAYLISTINFOSTRUCT*> was exactly this)."""
    bad = 0
    for cls, (rva, where) in sorted(vtbl.items(), key=lambda kv: kv[1][0]):
        v = vs.vtable_at(rva)
        dec = v["decorated"] if v else None
        if not dec:
            continue
        leaf = rtti_leaf(dec)
        boff = v["base_off"]
        if leaf == cls and boff == 0:
            continue
        why = ("RTTI names %r (leaf %s)" % (dec, leaf)) if leaf != cls else \
              ("RTTI base_off=%d: an MI secondary vtable (its mangled name embeds "
               "the base; a plain ??_7%s@@6B@ row is the wrong symbol)" % (boff, cls))
        bad += 1
        print("  RTTI-MISBOUND %s" % cls)
        print("      src : VTBL(0x%06x)   %s" % (rva, where))
        print("      rtti: %s" % why)
    return bad


def audit(show_all=False):
    """Re-derive every src VTBL() binding from the binary.

    For a class that owns an RVA()-bound destructor, the binary KNOWS which vtable
    dispatches to it. If src binds that class to a different rva, the binding is a
    MISBINDING - a wrong-dispatch bug that VTBL-uniqueness + vtable-coverage both pass,
    because both are satisfied by any self-consistent lie.

    Second gate (RTTI): where the bound rva carries an RTTI COL, the COL's class
    name must agree with the VTBL class name (see rtti_check)."""
    vtbl, dtor = src_claims()
    bad = agree = unprovable = 0
    for cls, (drva, dwhere) in sorted(dtor.items()):
        if cls not in vtbl:
            continue
        claimed, where = vtbl[cls]
        hits = owner_of(drva)
        starts = {v["start"] for (v, _k, _t, _s) in hits}
        if not starts:
            unprovable += 1
            if show_all:
                print("  ?  %-28s dtor 0x%06x -> no vtable dispatches to it   %s"
                      % (cls, drva, where))
            continue
        if claimed in starts:
            agree += 1
            if show_all:
                print("  OK %-28s VTBL(0x%06x)  <- dtor 0x%06x" % (cls, claimed, drva))
            continue
        bad += 1
        print("  MISBOUND %s" % cls)
        print("      src : VTBL(0x%06x)   %s" % (claimed, where))
        print("      dtor: 0x%06x   %s" % (drva, dwhere))
        for v, k, _t, sdd in hits:
            via = " (via sdd 0x%06x)" % sdd if sdd is not None else ""
            print("      binary: ??_7 @0x%06x slot %d%s  RTTI: %s"
                  % (v["start"], k, via, _vt_label(v)))
        owner = next(( (c, r) for c, (r, _w) in vtbl.items() if r in starts and c != cls), None)
        if owner:
            print("      (0x%06x is currently bound to %s - one of the two is wrong)"
                  % (owner[1], owner[0]))
    rtti_bad = rtti_check(vtbl)
    print()
    print("vtable-owner audit: %d MISBOUND, %d RTTI-MISBOUND, %d agree, "
          "%d unprovable (dtor in no vtable)" % (bad, rtti_bad, agree, unprovable))
    return bad + rtti_bad


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("rva", nargs="?", help="function RVA (e.g. 0xb940) - who owns it?")
    ap.add_argument("--audit", action="store_true", help="re-derive every src VTBL() binding")
    ap.add_argument("--all", action="store_true", help="with --audit: also print the agreeing ones")
    a = ap.parse_args()
    if a.audit:
        sys.exit(1 if audit(a.all) else 0)
    if not a.rva:
        ap.error("give an RVA, or --audit")
    r = int(a.rva, 16)
    if r >= IMAGEBASE:
        r -= IMAGEBASE
    vtbl, _d = src_claims()
    explain(r, vtbl)


if __name__ == "__main__":
    main()
