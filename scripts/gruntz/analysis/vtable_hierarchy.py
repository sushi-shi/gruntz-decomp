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
"""
import argparse
import csv
import re

from gruntz.analysis import vtable_scan as vs

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


SYM = load_symbol_names()


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
def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--csv", metavar="PATH", help="write the machine-readable per-slot CSV")
    ap.add_argument("--class", dest="klass", metavar="NAME",
                    help="restrict to one class (case-insensitive substring)")
    ap.add_argument("--summary", action="store_true", help="one line per class (no slot tables)")
    args = ap.parse_args()

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
