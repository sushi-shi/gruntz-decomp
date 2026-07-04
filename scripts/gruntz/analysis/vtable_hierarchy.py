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
        self.bodies, self.where, self.src_base = {}, {}, {}
        _txt = {}  # per-file line cache, so the base can be read from the DECLARATION
        for name, path, lineno, body in class_meta.iter_class_defs():
            self.where.setdefault(name, (class_meta.rel(path), lineno))
            self.bodies.setdefault(name, []).append(body)
            if name not in self.src_base:
                # iter_class_defs' `body` is text AFTER the `{`, so the `: public Base`
                # lives on the declaration line(s) - read them from the file directly.
                if path not in _txt:
                    try:
                        _txt[path] = path.read_text(errors="ignore").splitlines()
                    except OSError:
                        _txt[path] = []
                lines = _txt[path]
                decl = " ".join(lines[lineno - 1:lineno + 2]).split("{", 1)[0]
                m = re.search(r"\b" + re.escape(name) + r"\b\s*:\s*"
                              r"(?:public\s+|protected\s+|private\s+|virtual\s+)*([\w:]+)", decl)
                self.src_base[name] = m.group(1).split("::")[-1] if m else None
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


def known_base_vtables(aud):
    """rva -> (rtti_name, slots) for every RTTI base vtable, so a derived class that
    declares NO base can still be matched to the base it silently inherits from by
    its vtable slot-PREFIX (the CObject-slots-but-standalone gap)."""
    out = {}
    for rva, name in aud.col.items():
        sl = aud.slots.get(rva)
        if sl:
            out[rva] = (name, sl)
    return out


def reconstruct_intermediates(aud):
    """{name: slots} for ABSTRACT intermediate bases with NO own vtable (so
    col_by_rva/known_base_vtables miss them) but which ARE the RTTI mdisp-0 base of
    some class - e.g. CObject -> CWapObj -> CImage, where CWapObj emits no standalone
    ??_7. Slots are recovered from a derived RTTI class's vtable PREFIX, extended past
    CObject's 5 while each slot rva is SHARED across >=2 classes at that index (the
    hallmark of an INHERITED base virtual vs a class-OWN new virtual). Without this,
    the slot-prefix match would wrongly infer the grand-base CObject and skip CWapObj."""
    idxcount = {}
    for sl in aud.slots.values():
        for i, r in enumerate(sl):
            idxcount[(i, r)] = idxcount.get((i, r), 0) + 1
    out = {}
    for name in list(aud.reg):
        ci = aud.reg.get(name)
        if not (ci and ci.primary() and ci.is_rtti):
            continue
        _, meta = diff_primary(aud.reg, ci)
        inter = meta["declared_base"]
        if not (meta["base_inferred"] and inter and inter not in out):
            continue
        b = aud.reg.get(inter)
        if b and b.primary():
            continue  # it HAS a vtable -> not an uncataloged abstract intermediate
        slots = ci.primary()[2]
        k = 5
        while k < len(slots) and idxcount.get((k, slots[k]), 0) >= 2:
            k += 1
        out[inter] = slots[:k]
    return out


def _deepest_base(slots, self_rva, known, inter):
    """The DEEPEST base whose vtable slots are a PREFIX of `slots` - prefers an
    abstract intermediate (CWapObj, 7 slots) over the grand-base it derives (CObject,
    5). Cataloged bases need an EXACT prefix; a reconstructed intermediate tolerates
    OVERRIDDEN middle slots (a derived class may override an intermediate virtual) -
    it matches on the grand-base prefix (slots 0-4) PLUS the intermediate's distinctive
    last slot at its index (0x1c08 for CWapObj, which the family doesn't override).
    slot 1 (the scalar-deleting dtor) is always class-specific, allowed to differ.
    Returns (name, nslots) or None."""
    best = None
    for rva, (nm, bslots) in known.items():
        if rva == self_rva:
            continue
        n = len(bslots)
        if 5 <= n <= len(slots) and all(i == 1 or slots[i] == bslots[i] for i in range(n)):
            if best is None or n > best[1]:
                best = (nm, n)
    for nm, bslots in inter.items():
        n = len(bslots)
        if n < 6 or n > len(slots):
            continue
        head_ok = all(i == 1 or slots[i] == bslots[i] for i in range(5))  # CObject prefix
        tail_ok = any(slots[i] == bslots[i] for i in range(5, n))         # >=1 intermediate slot (rest may be overridden)
        if head_ok and tail_ok and (best is None or n > best[1]):
            best = (nm, n)
    return best


_SRCBASE_RE = re.compile(r"\bclass\s+\w+\s*:\s*(?:public\s+|protected\s+|private\s+|virtual\s+)*([\w:]+)")


def _source_base(bodies):
    """The base our SOURCE actually writes (`class X : public Y`), or None."""
    for b in bodies:
        m = _SRCBASE_RE.search(b)
        if m:
            return m.group(1).split("::")[-1]
    return None


def _body_counts(bodies):
    """(virtual-decl count, OVERRIDE-macro count) from the class's MOST-complete
    shim body (max virtuals), so multiple per-TU views don't double-count."""
    best = (0, 0)
    for b in bodies:
        nv = len(re.findall(r"\bvirtual\b", b))
        if nv >= best[0]:
            best = (nv, len(re.findall(r"\bOVERRIDE\b", b)))
    return best


def cmd_audit(aud):
    """SINGLE inheritance/override audit - diff every vtable-bearing class's SOURCE
    against the binary-proven vtable and report, in one pass:
      INHERIT   incorrect/absent inheritance (declared base != the base the vtable
                proves, via the RTTI spine OR a shared slot-prefix)
      RENAME    src class name != authoritative RTTI type-descriptor name
      REDECLARE parent (inherited) virtuals redeclared in the derived class (drop them)
      OVERRIDE  override slots lacking the OVERRIDE macro (make overrides explicit)
      MISSING   fewer virtual decls than the class's own (override+new) slots
    INHERIT/RENAME are exact; REDECLARE/OVERRIDE/MISSING are slot-count checks (see
    clang -Wsuggest-override for the precise per-method missing-override list)."""
    known = known_base_vtables(aud)
    inter = reconstruct_intermediates(aud)
    F = {k: [] for k in ("INHERIT", "RENAME", "REDECLARE", "OVERRIDE", "MISSING")}
    for name in aud.vtable_bearing():
        rva, src, col = aud.resolve(name)
        if rva is None:
            continue
        ci = aud.reg.get(name)
        if not (ci and ci.primary()):
            continue
        rows, meta = diff_primary(aud.reg, ci)
        slots = ci.primary()[2]
        # TRUTH = the RTTI mdisp-0 direct base (the real base, incl. an uncataloged
        # abstract intermediate like CWapObj); for src-only classes, the DEEPEST prefix
        # base incl. reconstructed intermediates. NOT `meta["base"]` (beff) - that skips
        # uncataloged intermediates and wrongly points at the grand-base (the old bug).
        truth = meta["declared_base"] if ci.is_rtti else None
        via = "rtti" if truth else ""
        if not truth:
            db = _deepest_base(slots, rva, known, inter)
            if db:
                truth, via = db[0], ("intermediate" if db[0] in inter else "slot-prefix")
        source_base = aud.src_base.get(name)
        bslots = inter.get(truth) or (aud.reg[truth].primary()[2]
                                      if aud.reg.get(truth) and aud.reg[truth].primary() else [])
        nB = len(bslots)
        if nB:
            n_inh = sum(1 for i in range(min(nB, len(slots))) if i != 1 and slots[i] == bslots[i])
            n_ovr = sum(1 for i in range(min(nB, len(slots))) if i == 1 or slots[i] != bslots[i])
            n_new = max(0, len(slots) - nB)
        else:
            n_inh = sum(1 for r in rows if r[4] == "inherited")
            n_ovr = sum(1 for r in rows if r[4] == "override")
            n_new = sum(1 for r in rows if r[4] == "new")
        own = n_ovr + n_new
        n_virt, n_macro = _body_counts(aud.bodies.get(name, []))
        loc = "%s:%d" % aud.where[name]
        # (1) INCORRECT INHERITANCE: our source's WRITTEN base != the binary-true base
        if truth and source_base != truth:
            note = " [uncataloged intermediate - model it]" if (
                via == "intermediate" or (ci.is_rtti and meta["base_inferred"])) else ""
            F["INHERIT"].append((name, "source=%s -> derive %s (%s)%s"
                                 % (source_base or "(none)", truth, via, note), loc))
        if src != "manual-stamp" and col and col != name:
            F["RENAME"].append((name, "-> RTTI name %s" % col, loc))
        if n_inh and n_virt > own:
            F["REDECLARE"].append((name, "%d virtual decls, only %d own (ovr+new): drop ~%d inherited parent virtuals" % (n_virt, own, n_virt - own), loc))
        if n_ovr and n_macro < n_ovr:
            F["OVERRIDE"].append((name, "%d override slots, %d OVERRIDE macros: %d unmarked" % (n_ovr, n_macro, n_ovr - n_macro), loc))
        if own and n_virt < own:
            F["MISSING"].append((name, "%d own virtuals (ovr+new), source declares %d" % (own, n_virt), loc))
    print("# INHERITANCE / OVERRIDE AUDIT - our source vs the binary-proven vtable (one report)")
    for k in ("INHERIT", "RENAME", "REDECLARE", "OVERRIDE", "MISSING"):
        print("# %-9s : %d" % (k, len(F[k])))
    print("# (INHERIT/RENAME exact; REDECLARE/OVERRIDE/MISSING = slot-count; -Wsuggest-override = precise per-method)")
    for k, hdr in (("INHERIT", "declare the real base / fix wrong-absent inheritance"),
                   ("RENAME", "rename to the authoritative RTTI name"),
                   ("REDECLARE", "parent virtuals redeclared - remove (compiler re-emits inherited slots)"),
                   ("OVERRIDE", "overrides not explicit - add the OVERRIDE macro"),
                   ("MISSING", "fewer virtual decls than own slots - a virtual is unmodeled")):
        if not F[k]:
            continue
        print("\n## %s - %s" % (k, hdr))
        for name, detail, loc in sorted(F[k]):
            print("  %-9s %-30s %s  %s" % (k.lower() + ":", name, detail, loc))


def cmd_tree(aud):
    """Build the full inheritance FOREST from the binary and emit a topological work
    QUEUE. Edges come from the RTTI Class-Hierarchy-Descriptor spine for RTTI classes
    (authoritative - includes abstract intermediates like CWapObj that emit no vtable);
    for src-only classes, from slot-prefix CONTAINMENT oriented by vtable size (a base
    is the SHORTER prefix - so a base is never derived from its own child, fixing the
    same-prefix direction ambiguity). The QUEUE lists a base before every class that
    derives it, so modeling/re-basing can be dispatched in dependency order."""
    known = known_base_vtables(aud)
    inter = reconstruct_intermediates(aud)
    parent, nodes = {}, set()
    for name in aud.vtable_bearing():
        rva, src, col = aud.resolve(name)
        if rva is None:
            continue
        ci = aud.reg.get(name)
        if not (ci and ci.primary()):
            continue
        nodes.add(name)
        p = primary_base_name(ci) if ci.is_rtti else None
        if not p:
            db = _deepest_base(ci.primary()[2], rva, known, inter)
            p = db[0] if db else None
        if p and p != name:
            parent[name] = p
            nodes.add(p)
    children = {}
    for c, p in parent.items():
        children.setdefault(p, []).append(c)
    roots = sorted(n for n in nodes if n not in parent)

    def modeled(n):
        src_base = aud.src_base.get(n)
        return "" if src_base == parent.get(n, src_base) else "  <- source says %s" % (src_base or "no base")

    def kind(n):
        if n in inter:
            return "  [ABSTRACT INTERMEDIATE - no own vtable - MODEL as : CObject]"
        if n not in aud.reg or not aud.reg[n].primary():
            return "  [uncataloged]"
        return modeled(n)

    print("# INHERITANCE FOREST (binary-proven: RTTI spine + size-oriented slot-prefix)")
    seen = set()

    def walk(n, depth):
        if n in seen:
            print("  " + "  " * depth + n + " (*seen)")
            return
        seen.add(n)
        print("  " + "  " * depth + n + kind(n))
        for c in sorted(children.get(n, [])):
            walk(c, depth + 1)
    for r in roots:
        walk(r, 0)

    print("\n# TOPOLOGICAL QUEUE (model/re-base a base BEFORE its derived classes)")
    order, done = [], set()

    def emit(n, stack):
        if n in done or n in stack:
            return
        stack = stack | {n}
        if n in parent:
            emit(parent[n], stack)
        done.add(n)
        order.append(n)
    for n in sorted(nodes):
        emit(n, set())
    for i, n in enumerate(order):
        needs = kind(n)
        if needs.strip():
            print("  %3d  %-32s parent=%-16s%s" % (i, n, parent.get(n, "(root)"), needs))


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
    ap.add_argument("--audit", action="store_true",
                    help="SINGLE inheritance/override audit: incorrect inheritance + redeclared "
                         "parent virtuals + non-explicit overrides + missing virtuals, in one report")
    ap.add_argument("--tree", action="store_true",
                    help="build the inheritance FOREST (RTTI spine + size-oriented slot-prefix, "
                         "incl. abstract intermediates) + a topological work QUEUE")
    args = ap.parse_args()

    if args.coverage or args.name_audit or args.audit or args.tree:
        aud = Audit()
        if args.coverage:
            cmd_coverage(aud)
        if args.name_audit:
            if args.coverage:
                print()
            cmd_name_audit(aud)
        if args.audit:
            if args.coverage or args.name_audit:
                print()
            cmd_audit(aud)
        if args.tree:
            if args.coverage or args.name_audit or args.audit:
                print()
            cmd_tree(aud)
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
