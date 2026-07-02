#!/usr/bin/env python3
"""gruntz.analysis.vtable_hierarchy - per-class virtual table with every slot
tagged new / override / inherited, to drive the manual-vtable drain.

For each class this emits its primary vtable (and, for multiple-inheritance
classes, its secondary vtables), aligns it slot-by-slot against the class's
primary base, and tags each slot:

  * ``inherited`` - D.slot[i] points at the SAME function as B.slot[i]
  * ``override``  - D.slot[i] differs and i < len(B.vtable)  (D re-defines it)
  * ``new``       - i >= len(B.vtable)  (the slot is introduced by D)

and attributes the slot to its ORIGIN class: the highest ancestor whose vtable
first grew to include that slot index. A vtable-conversion worker transcribes
the CSV directly - declare each ``new`` slot as a plain ``virtual`` and each
``override`` slot with the OVERRIDE macro, in slot order, and DON'T redeclare
``inherited`` slots (the compiler re-emits them).

Sources of truth (in priority order)
-------------------------------------
1. **RTTI** (authoritative). Every vtable's Complete Object Locator (``vtable-4``)
   points at a Class Hierarchy Descriptor -> Base Class Array; walking the
   MSVC ``_RTTIBaseClassDescriptor`` pre-order (with ``numContainedBases``)
   recovers each class's DIRECT bases and their sub-object offsets (``mdisp``).
   This yields the exact class graph for all ~221 RTTI classes.
2. **Reconstructed src** (fallback, for NON-RTTI vtables). ``VTBL(Class, rva)``
   macros give class->vtable and ``class X : public Y`` gives the parent, so
   already-converted non-RTTI classes (e.g. CFader) are covered too.

The per-slot function addresses are read straight out of the vtable (each slot
is an absolute DIR32 reloc). The vtable enumeration + binary reader are reused
from ``gruntz.analysis.vtable_scan``.

Correctness over coverage: when a class's DIRECT primary base has no standalone
vtable in the binary (abstract intermediate base - ~17 classes), the diff walks
up to the nearest ancestor that DOES, and the affected class is flagged
``base-inferred`` (slots beyond the known ancestor are ``new`` *since that
ancestor*, which may include an intermediate base's slots). Multiple-inheritance
classes are flagged ``MI`` so a human reviews the secondary-vtable overrides
(which appear as adjustor thunks).

Usage:
    python3 -m gruntz.analysis.vtable_hierarchy                 # full per-class slot tables
    python3 -m gruntz.analysis.vtable_hierarchy --summary       # one line per class
    python3 -m gruntz.analysis.vtable_hierarchy --class CImage  # one class, full detail
    python3 -m gruntz.analysis.vtable_hierarchy --csv out.csv   # machine-readable, all classes
    python3 -m gruntz.analysis.vtable_hierarchy --coverage      # completeness: anchored vs UNANCHORED (omitted) src vtables
    python3 -m gruntz.analysis.vtable_hierarchy --name-audit    # src-name vs RTTI-COL-name mismatches (RTTI authoritative)
"""
import argparse
import csv
import re

from gruntz.analysis import vtable_scan as vs
from gruntz.match import class_meta, class_vtables

IB = vs.IMAGEBASE
REPO = vs.REPO


# ---------------------------------------------------------------------------
# names
# ---------------------------------------------------------------------------
def load_symbol_names():
    """rva -> emitted MSVC symbol (build/gen/symbol_names.csv)."""
    m = {}
    p = REPO / "build/gen/symbol_names.csv"
    if p.exists():
        with open(p) as f:
            for r in csv.DictReader(f):
                try:
                    m[int(r["rva"], 16)] = r["name"]
                except (ValueError, KeyError):
                    pass
    return m


def load_symbol_name_to_rva():
    """emitted MSVC symbol -> rva (inverse of load_symbol_names). Lets us anchor a
    class through its OWN cl-emitted ``??_7<Name>@@6B@`` vtable datum."""
    m = {}
    p = REPO / "build/gen/symbol_names.csv"
    if p.exists():
        with open(p) as f:
            for r in csv.DictReader(f):
                try:
                    m[r["name"]] = int(r["rva"], 16)
                except (ValueError, KeyError):
                    pass
    return m


SYM = load_symbol_names()
SYM_BY_NAME = load_symbol_name_to_rva()


def slot_name(idx, fn_rva):
    """Best readable name for a slot's function; unknown -> Slot<NN>_<rva>."""
    fn = vs.FN.get(fn_rva)  # functions.csv exact-start name (readable: FreeAll, ~CFader, __purecall)
    if fn:
        return fn[0]
    if fn_rva in SYM:
        return SYM[fn_rva]
    return "Slot%02d_%06x" % (idx, fn_rva)


# ---------------------------------------------------------------------------
# RTTI: Complete Object Locator -> Class Hierarchy Descriptor -> Base Class Array
# (all RTTI pointers are VAs, so read them via rd()).
# ---------------------------------------------------------------------------
def rd(va):
    return vs.u32(va - IB)


def parse_chd(col_va):
    """From a COL VA recover (attrib, direct_bases[(decorated, mdisp)], spine).

    The base-class array is the pre-order (DFS) flattening of the base tree;
    a node at index i owning ``numContainedBases`` bases occupies [i, i+n], so
    the DIRECT bases of the root are found by hopping over each child's subtree.

    ``spine`` is the full PRIMARY-base spine (most-derived first): every base
    sub-object that shares the offset-0 vptr, i.e. ``mdisp == 0``. It runs past
    intermediate bases that have no standalone vtable of their own (e.g.
    CImage -> CWapObj -> CObject), which is what lets the slot diff walk up to
    the nearest ancestor that DOES have a vtable.
    """
    pchd = rd(col_va + 16)
    attrib = rd(pchd + 4)
    numbase = rd(pchd + 8)
    pbca = rd(pchd + 12)
    bcds = []  # (decorated, numContained, mdisp)
    for i in range(numbase):
        pbcd = rd(pbca + 4 * i)
        bcds.append((vs.TD.get(rd(pbcd)), rd(pbcd + 4), rd(pbcd + 8)))
    direct = []
    if bcds:
        n_self = bcds[0][1]
        idx = 1
        while idx <= n_self and idx < len(bcds):
            direct.append((bcds[idx][0], bcds[idx][2]))  # (decorated, mdisp)
            idx += bcds[idx][1] + 1
    # primary spine: the offset-0 (mdisp==0) bases, in pre-order (most-derived first)
    spine = [vs.demangle(bd) for (bd, _numc, md) in bcds[1:] if bd and md == 0]
    return attrib, direct, spine


# ---------------------------------------------------------------------------
# reconstructed-src hierarchy: VTBL(Class, rva) + `class X : public Y`
# ---------------------------------------------------------------------------
CLASS_RE = re.compile(r"\b(?:class|struct)\s+([A-Za-z_]\w*)\s*:\s*public\s+([A-Za-z_]\w*)")
VTBL_RE = re.compile(r"\bVTBL\s*\(\s*([A-Za-z_]\w*)\s*,\s*(0x[0-9a-fA-F]+)")


def load_src_hierarchy():
    """Scan src/+include/ -> (parent{child:parent}, vtbl{rva:class})."""
    parent, vtbl = {}, {}
    for sub in ("include", "src"):
        base = REPO / sub
        if not base.exists():
            continue
        for f in base.rglob("*"):
            if f.suffix not in (".h", ".cpp"):
                continue
            try:
                t = f.read_text(errors="ignore")
            except OSError:
                continue
            for m in CLASS_RE.finditer(t):
                parent.setdefault(m.group(1), m.group(2))
            for m in VTBL_RE.finditer(t):
                rva = int(m.group(2), 16)
                if rva >= IB:
                    rva -= IB
                vtbl[rva] = m.group(1)
    return parent, vtbl


# ---------------------------------------------------------------------------
# unified class registry (RTTI first, src VTBL classes fill the gaps)
# ---------------------------------------------------------------------------
class ClassInfo:
    __slots__ = ("name", "decorated", "is_rtti", "attrib", "direct_bases", "spine", "vtables")

    def __init__(self, name):
        self.name = name
        self.decorated = None
        self.is_rtti = False
        self.attrib = 0
        self.direct_bases = []  # [(base_name, mdisp)]
        self.spine = None       # RTTI primary-base spine (most-derived first), or None
        self.vtables = {}       # base_off -> (start_rva, size, [fn_rva,...])

    def primary(self):
        return self.vtables.get(0)


def slots_of(start, size):
    return [(vs.u32(start + 4 * i) or IB) - IB for i in range(size)]


def build_registry():
    reg = {}
    start_size = {v["start"]: v["size"] for v in vs.VTABLES}

    # 1) RTTI classes (authoritative).
    for v in vs.VTABLES:
        dec = v["decorated"]
        if not dec:
            continue
        col_va = vs.u32(v["start"] - 4)
        if col_va is None:
            continue
        name = vs.demangle(dec)
        ci = reg.get(name)
        if ci is None:
            ci = reg[name] = ClassInfo(name)
            ci.decorated = dec
            ci.is_rtti = True
            attrib, direct, spine = parse_chd(col_va)
            ci.attrib = attrib
            ci.direct_bases = [(vs.demangle(bd), md) for (bd, md) in direct if bd]
            ci.spine = spine
        ci.vtables[v["base_off"] or 0] = (v["start"], v["size"], slots_of(v["start"], v["size"]))

    # 2) reconstructed-src classes for NON-RTTI vtables (fallback; don't clobber RTTI).
    src_parent, src_vtbl = load_src_hierarchy()
    for rva, cname in src_vtbl.items():
        if cname in reg and reg[cname].is_rtti:
            continue
        size = start_size.get(rva)
        if size is None:
            continue  # VTBL points at an rva vtable_scan didn't enumerate; skip
        ci = reg.get(cname)
        if ci is None:
            ci = reg[cname] = ClassInfo(cname)
            p = src_parent.get(cname)
            ci.direct_bases = [(p, 0)] if p else []
        ci.vtables.setdefault(0, (rva, size, slots_of(rva, size)))

    # 3) classes whose OWN cl-emitted ``??_7<Name>@@6B@`` already landed in
    #    build/gen/symbol_names.csv (a real-polymorphic class whose vtable datum the
    #    build names) but that carry NO VTBL and no RTTI COL under their own name.
    #    That emitted symbol IS the class's primary vtable -> anchor it. This closes
    #    part of the src-only omission gap (see --coverage).
    sym7 = re.compile(r"^\?\?_7([A-Za-z_]\w*)@@6B@$")
    for sym, rva in SYM_BY_NAME.items():
        m = sym7.match(sym)
        if not m:
            continue
        cname = m.group(1)
        size = start_size.get(rva)
        if size is None:
            continue
        ci = reg.get(cname)
        if ci is not None and ci.primary():
            continue
        if ci is None:
            ci = reg[cname] = ClassInfo(cname)
            p = src_parent.get(cname)
            ci.direct_bases = [(p, 0)] if p else []
        ci.vtables.setdefault(0, (rva, size, slots_of(rva, size)))

    global SRC_PARENT
    SRC_PARENT = src_parent
    return reg, src_parent


SRC_PARENT = {}


# ---------------------------------------------------------------------------
# slot diff
# ---------------------------------------------------------------------------
def primary_base_name(ci):
    """The DIRECT primary base (offset-0 sub-object): RTTI mdisp-0 base, else
    the reconstructed-src parent."""
    for (b, md) in ci.direct_bases:
        if md == 0:
            return b
    if ci.spine is None:  # non-RTTI: fall back to the src `class X : public Y`
        return SRC_PARENT.get(ci.name)
    return ci.spine[0] if ci.spine else None


def spine_names(ci):
    """Ordered ancestor names up the primary spine (most-derived first),
    EXCLUDING ci. From RTTI for RTTI classes, else the src parent chain."""
    if ci.spine is not None:
        return list(ci.spine)
    names, seen, cur = [], {ci.name}, ci.name  # walk src parents
    while True:
        p = SRC_PARENT.get(cur)
        if not p or p in seen:
            break
        names.append(p)
        seen.add(p)
        cur = p
    return names


def diff_primary(reg, ci):
    """Return (rows, meta). rows: [(idx, fn_rva, name, origin, disposition)].
    meta: dict(base, declared_base, base_inferred, mi, no_base_vtable)."""
    start, size, slots = ci.primary()
    # spine members that HAVE a standalone vtable, most-derived first: (name, size)
    known = [(ci.name, size)]
    for n in spine_names(ci):
        c = reg.get(n)
        if c and c.primary():
            known.append((n, c.primary()[1]))

    declared_base = primary_base_name(ci)           # what RTTI/src say the direct base is
    beff = known[1][0] if len(known) > 1 else None   # nearest ancestor WITH a vtable
    nB = known[1][1] if len(known) > 1 else 0
    base_slots = reg[beff].primary()[2] if beff else []
    base_inferred = beff is not None and beff != declared_base
    # multiple inheritance = >1 DIRECT base (RTTI mdisp list). A secondary vtable
    # (base_off != 0) is the case the primary diff can't capture on its own -
    # overrides of a polymorphic non-primary base live there (as adjustor thunks).
    # An MI class whose extra base is EMPTY (e.g. CWapX) has no secondary vtable,
    # so its primary diff IS complete.
    mi = len([b for b in ci.direct_bases if b[0]]) > 1
    has_sec = len(ci.vtables) > 1

    def origin_of(i):
        # highest ancestor (with a known vtable) whose size > i; sizes are
        # non-increasing up the spine, so the last such going up is the origin.
        org = None
        for name, sz in known:
            if sz > i:
                org = name
            else:
                break
        return org or ci.name

    rows = []
    for i in range(size):
        fn = slots[i]
        if beff is None:
            # root (no declared base) -> every slot is introduced here; a declared
            # base whose vtable we can't find anywhere -> genuinely undiffable.
            disp = "new" if not declared_base else "unknown"
        elif i >= nB:
            disp = "new"
        elif fn == base_slots[i]:
            disp = "inherited"
        else:
            disp = "override"
        rows.append((i, fn, slot_name(i, fn), origin_of(i), disp))
    meta = dict(base=beff, declared_base=declared_base, base_inferred=base_inferred,
                mi=mi, has_sec=has_sec, no_base_vtable=(beff is None and bool(declared_base)))
    return rows, meta


def diff_secondary(reg, ci, base_off):
    """Diff an MI secondary vtable (base_off != 0) against the corresponding
    base sub-object's primary vtable. Overrides may be adjustor thunks."""
    start, size, slots = ci.vtables[base_off]
    # the base sub-object at this offset: a direct/indirect base with mdisp==base_off
    bname = None
    for (b, md) in ci.direct_bases:
        if md == base_off and b in reg:
            bname = b
            break
    base = reg.get(bname) if bname else None
    base_slots = base.primary()[2] if (base and base.primary()) else []
    nB = len(base_slots)
    rows = []
    for i in range(size):
        fn = slots[i]
        if not base or not base.primary():
            disp = "unknown"
        elif i >= nB:
            disp = "new"
        elif fn == base_slots[i]:
            disp = "inherited"
        else:
            disp = "override"  # (typically an adjustor thunk to the derived override)
        origin = bname or ci.name
        rows.append((i, fn, slot_name(i, fn), origin, disp))
    return rows, bname


# ---------------------------------------------------------------------------
# output
# ---------------------------------------------------------------------------
def class_flags(meta):
    fl = []
    if meta["mi"]:
        fl.append("MI")
    if meta["has_sec"]:
        fl.append("sec-vtbl")  # has secondary vtable(s): primary diff is INCOMPLETE, review
    if meta["base_inferred"]:
        fl.append("base-inferred")
    if meta["no_base_vtable"]:
        fl.append("no-base-vtable")
    return fl


def print_class(reg, ci, verbose=True):
    prim = ci.primary()
    rows, meta = diff_primary(reg, ci)
    base = meta["declared_base"] or ""
    flags = class_flags(meta)
    kind = "rtti" if ci.is_rtti else "src"
    n_new = sum(1 for r in rows if r[4] == "new")
    n_ovr = sum(1 for r in rows if r[4] == "override")
    n_inh = sum(1 for r in rows if r[4] == "inherited")
    hdr = f"{ci.name}" + (f" : {base}" if base else "")
    tail = f"[{kind}] vtbl@0x{prim[0]:06x} {prim[1]} slots  ({n_new} new, {n_ovr} override, {n_inh} inherited)"
    if flags:
        tail += "  <" + ",".join(flags) + ">"
    if meta["base_inferred"]:
        tail += f"  diffed-vs {meta['base']}"
    print(f"{hdr}  {tail}")
    if not verbose:
        return
    for (i, fn, name, origin, disp) in rows:
        oc = "" if origin == ci.name else f"  (origin {origin})"
        print(f"    [{i:2}] {disp:<9} {name:<40} // 0x{fn:06x}{oc}")
    # MI secondary vtables
    for boff in sorted(k for k in ci.vtables if k):
        srows, bname = diff_secondary(reg, ci, boff)
        print(f"    -- secondary vtable @+{boff} (base {bname or '?'}); overrides may be adjustor thunks")
        for (i, fn, name, origin, disp) in srows:
            print(f"    [{i:2}] {disp:<9} {name:<40} // 0x{fn:06x}")


def write_csv(reg, path):
    with open(path, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["class", "base", "slot_index", "fn_rva", "fn_name",
                    "origin_class", "disposition", "base_off", "multi_base", "note"])
        for name in sorted(reg):
            ci = reg[name]
            if not ci.primary():
                continue
            rows, meta = diff_primary(reg, ci)
            note = ";".join(class_flags(meta))
            base = meta["declared_base"] or ""
            for (i, fn, sname, origin, disp) in rows:
                w.writerow([name, base, i, "0x%06x" % fn, sname, origin, disp,
                            0, int(meta["mi"]), note])
            for boff in sorted(k for k in ci.vtables if k):
                srows, bname = diff_secondary(reg, ci, boff)
                for (i, fn, sname, origin, disp) in srows:
                    w.writerow([name, bname or "", i, "0x%06x" % fn, sname, origin, disp,
                                boff, 1, "MI-secondary;thunks-possible"])


# ---------------------------------------------------------------------------
# coverage + RTTI-name-authority audit
#
# The per-class slot tables above only cover a class the registry can ANCHOR
# (RTTI COL / VTBL / emitted ??_7). A class that declares cl-emitted ``virtual``s
# but has none of those is silently ABSENT from that output - the "missed src-only
# vtables" gap. These two modes turn that silent omission into an explicit report,
# and make the RTTI type-descriptor name (the original dev name) authoritative.
# ---------------------------------------------------------------------------
_STAMP_RE = re.compile(r"&\s*(g_\w*(?:[Vv]tbl|vftable))\b")
_GVTBL_RE = re.compile(r"\b(g_\w*(?:[Vv]tbl|vftable))\b")
_HEX_RE = re.compile(r"0x([0-9a-fA-F]{5,8})\b")
_FUN_RE = re.compile(r"\bFUN_([0-9a-fA-F]{6,8})\b")
_VTBLFIELD_RE = re.compile(r"\*\s*vtbl\s*;")   # hand-rolled COM `SomeVtbl* vtbl;` idiom


def col_by_rva():
    """rva -> authoritative RTTI type-descriptor (COL) name, PRIMARY vtables only.
    This is the original developers' class name for that vtable."""
    return {v["start"]: v["rtti"] for v in vs.VTABLES if v["rtti"] and v["base_off"] == 0}


def stamp_global_rvas():
    """{g_*Vtbl global: rva} harvested from any src/ + include/ line that carries
    EXACTLY ONE ``g_*Vtbl`` token and ONE ``0x<addr>`` (the ``extern ... // 0x<VA>``
    binding or the stamp-site comment) - the unambiguous address of a manual-stamp
    vtable datum, so a class that only carries ``*(void**)this = &g_XVtbl`` can be
    anchored to its base vtable. The one-token/one-hex rule keeps it collision-free."""
    out = {}
    for path in class_meta.source_files():
        try:
            t = path.read_text(errors="ignore")
        except OSError:
            continue
        for line in t.splitlines():
            gs = _GVTBL_RE.findall(line)
            hs = _HEX_RE.findall(line)
            if len(gs) == 1 and len(hs) == 1:
                va = int(hs[0], 16)
                out.setdefault(gs[0], va - IB if va >= IB else va)
    return out


def _struct_slot_rvas(body):
    """Ordered per-virtual-slot fn rva (or None for a dtor/named/pure slot) parsed
    from a class body's ``virtual ... FUN_<VA> ...;`` declarations (the FUN_<VA>
    transcription convention: VA = RVA + 0x400000)."""
    out = []
    for m in re.finditer(r"\bvirtual\b", body):
        seg = body[m.start():]
        semi = seg.find(";")
        if semi >= 0:
            seg = seg[:semi]
        fm = _FUN_RE.search(seg)
        out.append(int(fm.group(1), 16) - IB if fm else None)
    return out


def structural_anchor(body, size_by_start, slots_cache):
    """Locate a class's vtable purely from its transcribed slot fn addresses: the
    vtable whose size == the declared virtual count AND every concrete FUN_ slot
    matches at its index. When the concrete slots are all shared base thunks (so
    several vtables match), the tie is broken toward the one whose vptr-stamp is
    code-referenced FAR more often - a shared grand-base dominates - and only when
    that dominance is unambiguous (>= 2x the runner-up). Returns an rva or None."""
    ss = _struct_slot_rvas(body)
    if not ss or not any(r is not None for r in ss):
        return None
    n = len(ss)
    cands = [st for st, sz in size_by_start.items() if sz == n
             and all(r is None or (i < len(slots_cache[st]) and slots_cache[st][i] == r)
                     for i, r in enumerate(ss))]
    if not cands:
        return None
    if len(cands) == 1:
        return cands[0]
    cands.sort(key=lambda s: vs.CODE_REF.get(s, 0), reverse=True)
    top, second = cands[0], cands[1]
    if vs.CODE_REF.get(top, 0) >= 2 * max(1, vs.CODE_REF.get(second, 0)):
        return top
    return None


class Audit:
    """Everything the coverage / name-audit walks need, computed once. Enumerates
    every vtable-bearing src class (class_meta/class_vtables scoping) and resolves
    each to a vtable rva through a layered anchor cascade."""

    def __init__(self):
        self.reg, _ = build_registry()
        self.col = col_by_rva()
        self.size_by_start = {v["start"]: v["size"] for v in vs.VTABLES}
        self.slots = {v["start"]: slots_of(v["start"], v["size"]) for v in vs.VTABLES}
        self.stamp = stamp_global_rvas()
        self.vtbl_ann = class_meta.vtbl_annotated_names()
        self.rtti_cfg = class_vtables.rtti_vtables()
        # per-class NAME signals, keeping EVERY per-TU body (the FUN_ transcription
        # may live in only one of a class's several shim definitions).
        self.virtual, self.manual, self.vtbl_field = set(), set(), set()
        self.bodies, self.where = {}, {}
        for name, path, lineno, body in class_meta.iter_class_defs():
            self.where.setdefault(name, (class_meta.rel(path), lineno))
            self.bodies.setdefault(name, []).append(body)
            if re.search(r"\bvirtual\b", body):
                self.virtual.add(name)
            if class_vtables._MANUAL_RE.search(body):
                self.manual.add(name)
            if _VTBLFIELD_RE.search(body):
                self.vtbl_field.add(name)

    def vtable_bearing(self):
        """Sorted src class NAMES carrying a vtable signal: a real virtual, a manual
        &g_*Vtbl / m_vtbl / m_vptr stamp, a hand-rolled ``* vtbl;`` field, a VTBL()
        annotation, or an RTTI ??_7 in config/vtable_names.csv."""
        return sorted(n for n in self.where
                      if n in self.virtual or n in self.manual or n in self.vtbl_field
                      or n in self.vtbl_ann or n in self.rtti_cfg)

    def resolve(self, name):
        """(rva, source, rtti_col) or (None, None, None). source is one of
        rtti / vtbl / sym-vtbl / manual-stamp / structural. rtti_col is the COL
        (dev) name at that rva, or None for a non-RTTI vtable."""
        ci = self.reg.get(name)
        if ci and ci.primary():
            rva = ci.primary()[0]
            if ci.is_rtti:
                src = "rtti"
            elif name in self.vtbl_ann:
                src = "vtbl"
            else:
                src = "sym-vtbl"        # anchored via the emitted ??_7 fallback
            return rva, src, self.col.get(rva)
        # manual stamp of a (base) vtable, resolvable to an rva via its extern comment
        for body in self.bodies.get(name, []):
            for g in _STAMP_RE.findall(body):
                if g in self.stamp:
                    r = self.stamp[g]
                    return r, "manual-stamp", self.col.get(r)
        # structural: a unique / dominant slot-sequence match (try each shim body)
        hits = {structural_anchor(b, self.size_by_start, self.slots)
                for b in self.bodies.get(name, [])}
        hits.discard(None)
        if len(hits) == 1:
            r = next(iter(hits))
            return r, "structural", self.col.get(r)
        return None, None, None

    def reason(self, name):
        """Why an unanchored class is still omitted: the signal(s) it carries."""
        sig = []
        if name in self.virtual:
            sig.append("virtual")
        if name in self.manual:
            sig.append("manual-stamp")
        if name in self.vtbl_field:
            sig.append("vtbl-field")
        if name in self.vtbl_ann:
            sig.append("VTBL")
        if name in self.rtti_cfg:
            sig.append("rtti-cfg")
        return "+".join(sig) or "?"


def cmd_coverage(aud):
    """Report the COMPLETENESS of vtable coverage: total vtable-bearing classes,
    how many the analyzer anchors (by source), and the UNANCHORED worklist."""
    by_src, anchored, unanchored, newly = {}, [], [], []
    for name in aud.vtable_bearing():
        rva, src, col = aud.resolve(name)
        if rva is None:
            unanchored.append((name, aud.reason(name)))
            continue
        by_src[src] = by_src.get(src, 0) + 1
        anchored.append((name, src, rva, col))
        if src in ("sym-vtbl", "manual-stamp", "structural"):
            newly.append((name, src, rva, col))
    total = len(anchored) + len(unanchored)
    print("# vtable-coverage audit - every src/+include/ class carrying a vtable")
    print("# signal (declares a real virtual / a manual &g_*Vtbl|m_vtbl|m_vptr stamp /")
    print("# named in config/vtable_names.csv), and whether the analyzer can ANCHOR")
    print("# its vtable rva (RTTI COL / VTBL / emitted ??_7 / resolvable manual stamp /")
    print("# unique structural slot match).")
    print(f"# total vtable-bearing classes : {total}")
    print(f"# anchored                     : {len(anchored)}  "
          + "(" + ", ".join(f"{k}={v}" for k, v in sorted(by_src.items())) + ")")
    print(f"# UNANCHORED (silently omitted): {len(unanchored)}  <- add a VTBL() (or the")
    print("#   class is an abstract intermediate base with no standalone vtable of its own)")
    print(f"# newly anchored by try-harder : {len(newly)}  (sym-vtbl / manual-stamp / structural)")
    print()
    if newly:
        print("## newly anchored (previously omitted from the hierarchy)")
        for name, src, rva, col in sorted(newly):
            tail = ""
            if col:
                tail = f"  rtti={col}"
                if col != name and src != "manual-stamp":
                    tail += "  [NAME-MISMATCH]"
            print(f"  {name:<36} {src:<12} vtbl@0x{rva:06x}{tail}")
        print()
    print("## UNANCHORED - the src-only omission worklist (add VTBL / locate the vtable)")
    for name, reason in unanchored:
        path, ln = aud.where[name]
        print(f"  {name:<40} [{reason}]  {path}:{ln}")


def cmd_name_audit(aud):
    """Report every src class whose name disagrees with the RTTI type-descriptor
    (COL) name at the vtable it OWNS. The RTTI name is the original developers'
    and is authoritative -> these are renames to apply. Manual stamps of a SHARED
    base vtable are excluded (reusing a base's vtable is not a rename)."""
    mism = []
    for name in aud.vtable_bearing():
        rva, src, col = aud.resolve(name)
        if rva is None or src == "manual-stamp" or not col or col == name:
            continue
        mism.append((name, col, rva, src))
    print("# RTTI-name authority audit - the RTTI type-descriptor (COL) name is the")
    print("# original developers' class name and is AUTHORITATIVE. Each line is a src")
    print("# class whose name disagrees with the RTTI name at the vtable it owns")
    print("# (via VTBL / its emitted ??_7 / a unique structural slot match).")
    print(f"# src-vs-RTTI name mismatches : {len(mism)}")
    print()
    for name, col, rva, src in sorted(mism):
        path, ln = aud.where[name]
        print(f"  name-mismatch: src={name:<22} rtti={col:<16} vtbl@0x{rva:06x}  "
              f"via={src:<10} {path}:{ln}")


# ---------------------------------------------------------------------------
def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--csv", metavar="PATH", help="write the machine-readable per-slot CSV")
    ap.add_argument("--class", dest="klass", metavar="NAME",
                    help="restrict to one class (case-insensitive substring)")
    ap.add_argument("--summary", action="store_true", help="one line per class (no slot tables)")
    ap.add_argument("--coverage", action="store_true",
                    help="COMPLETENESS audit: total vtable-bearing classes, # anchored, "
                         "# UNANCHORED (silently-omitted worklist) with the reason per class")
    ap.add_argument("--name-audit", dest="name_audit", action="store_true",
                    help="list every src-class-name vs RTTI-COL-name mismatch (RTTI is authoritative)")
    args = ap.parse_args()

    if args.coverage or args.name_audit:
        aud = Audit()
        if args.coverage:
            cmd_coverage(aud)
        if args.name_audit:
            if args.coverage:
                print()
            cmd_name_audit(aud)
        return

    reg, _ = build_registry()
    classes = [reg[n] for n in sorted(reg) if reg[n].primary()]

    n_rtti = sum(1 for c in classes if c.is_rtti)
    n_mi = n_sec = n_inferred = n_root = 0
    for c in classes:
        _, meta = diff_primary(reg, c)
        if meta["mi"]:
            n_mi += 1
        if meta["has_sec"]:
            n_sec += 1
        if meta["base_inferred"]:
            n_inferred += 1
        if not meta["declared_base"]:
            n_root += 1

    print(f"# classes with vtables: {len(classes)} ({n_rtti} RTTI, {len(classes)-n_rtti} src-only)")
    print(f"# roots/no-declared-base: {n_root} | multi-base (MI): {n_mi} | "
          f"has-secondary-vtable (primary diff incomplete): {n_sec} | "
          f"abstract-intermediate-base (inferred): {n_inferred}")
    print("# disposition: inherited=redeclare-nothing  override=OVERRIDE macro  new=plain virtual")
    print()

    if args.klass:
        q = args.klass.lower()
        classes = [c for c in classes if q in c.name.lower()]
        if not classes:
            print(f"# no class matching {args.klass!r}")
            return

    for c in classes:
        print_class(reg, c, verbose=not args.summary)
        if not args.summary:
            print()

    if args.csv:
        write_csv(reg, args.csv)
        print(f"# wrote {args.csv}")


if __name__ == "__main__":
    main()
