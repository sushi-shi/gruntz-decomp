#!/usr/bin/env python3
"""gruntz.analysis.vtable_scan - recover *every* vtable in GRUNTZ.EXE and its
exact size, by combining three independent signals from the retail binary.

Why this works
--------------
1. Every vtable slot is an absolute DIR32 function address => a PE base-reloc
   site. A maximal stride-4 run of data-section reloc sites whose values all
   point into an executable section is a band of one-or-more adjacent vtables.
2. For MSVC `/GR` code each *RTTI* vtable has, at `vtable-4`, a pointer to a
   Complete Object Locator (.rdata); following it gives the class name and the
   base-subobject offset -> CONFIRMS the start and names it.
3. A vtable's start address is referenced by code (the ctor/dtor that stamps the
   vptr: `mov dword[this], offset vtable`). So any code DIR32 whose target is a
   run member marks a real vtable start -- including NON-RTTI vtables that have
   no COL (library / interface / WAP `z`-container classes).

Cutting each reloc-run at the union of {COL starts} u {code-referenced starts}
de-merges adjacent vtables (which a raw run scan glues together, since nothing
separates them in memory) and yields the EXACT per-vtable entry count.

Usage:
    python3 -m gruntz.analysis.vtable_scan                # summary + table
    python3 -m gruntz.analysis.vtable_scan --csv out.csv  # machine-readable
    python3 -m gruntz.analysis.vtable_scan --new          # only starts not yet in src/ DATA()
"""
import os, sys, struct, csv, bisect, re
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parent)
def _first(*c):
    for x in c:
        if x and Path(x).exists(): return Path(x)
    return None
EXE = _first(os.environ.get("GRUNTZ_EXE"), REPO / "build/exe/GRUNTZ.EXE",
             "/home/sheep/Projects/gruntz/build/exe/GRUNTZ.EXE")
FUNCS = _first(REPO / "build/ghidra-enrich/exports/functions.csv",
               "/home/sheep/Projects/gruntz/build/ghidra-enrich/exports/functions.csv")
SYMS = _first(REPO / "build/ghidra-enrich/exports/symbols.csv",
              "/home/sheep/Projects/gruntz/build/ghidra-enrich/exports/symbols.csv")
SRC = _first(REPO / "src", "/home/sheep/Projects/gruntz/src")
IMAGEBASE = 0x400000

d = EXE.read_bytes()
e = struct.unpack_from('<I', d, 0x3c)[0]
nsec = struct.unpack_from('<H', d, e+6)[0]; optsz = struct.unpack_from('<H', d, e+20)[0]; opt = e+24
SECS = []
for i in range(nsec):
    b = opt+optsz+i*40
    nm = d[b:b+8].rstrip(b'\0').decode('latin1')
    vsz, va, rsz, rp = struct.unpack_from('<IIII', d, b+8); ch = struct.unpack_from('<I', d, b+36)[0]
    SECS.append((nm, va, vsz, rsz, rp, ch))
EXEC = [(va, va+max(vsz, rsz)) for (nm, va, vsz, rsz, rp, ch) in SECS if ch & 0x20000000]

def off(rva):
    for (nm, va, vsz, rsz, rp, ch) in SECS:
        if va <= rva < va+max(vsz, rsz):
            o = rva-va+rp; return o if o < rp+rsz else None
    return None
def u32(rva):
    o = off(rva); return struct.unpack_from('<I', d, o)[0] if o is not None else None
def sec(rva):
    for (nm, va, vsz, rsz, rp, ch) in SECS:
        if va <= rva < va+max(vsz, rsz): return nm
    return None
def is_exec(rva): return any(lo <= rva < hi for lo, hi in EXEC)
def cstr(rva, n=512):
    o = off(rva)
    if o is None: return None
    end = d.find(b'\0', o, o+n); return d[o:end].decode('latin1') if end != -1 else None

# --- base relocations ---
rr = struct.unpack_from('<I', d, opt+96+5*8)[0]; rs = struct.unpack_from('<I', d, opt+96+5*8+4)[0]
REL = []
if rr:
    base = off(rr); end = base+rs; p = base
    while p < end:
        pg, blk = struct.unpack_from('<II', d, p)
        if blk == 0: break
        for i in range((blk-8)//2):
            ent = struct.unpack_from('<H', d, p+8+i*2)[0]
            if ent >> 12 == 3: REL.append(pg + (ent & 0xfff))
        p += blk
REL.sort(); RELSET = set(REL)

# --- functions for labeling ---
FN = {}
if FUNCS:
    with open(FUNCS) as f:
        for r in csv.DictReader(f):
            try: FN[int(r['entry_rva'], 16)] = (r['name'], int(r['byte_size']))
            except Exception: pass
FN_STARTS = sorted(FN)
def fn_label(rva):
    if rva in FN: return FN[rva][0]
    i = bisect.bisect_right(FN_STARTS, rva)-1
    if i >= 0:
        s = FN_STARTS[i]; nm, sz = FN[s]
        if rva < s+sz: return f"{nm}+0x{rva-s:x}"
    return f"sub_{rva:06x}"

# --- RTTI: TypeDescriptors + Complete Object Locators ---
def demangle(s):
    for pre in (".?AV", ".?AU", ".?AW", ".?AT"):
        if s.startswith(pre): s = s[len(pre):]; break
    if s.endswith("@@"): s = s[:-2]
    parts = [x for x in s.split("@") if x]
    return "::".join(reversed(parts)) if parts else s

def vftable_name(decorated):
    """Decorated TypeDescriptor name -> the deterministic primary-vtable mangled
    symbol. ".?AVCFoo@@" -> "??_7CFoo@@6B@" (drop the .?A[VUWT] tag, wrap 6B@).
    Only valid for the base_off==0 primary vtable; MI secondaries embed the base."""
    s = decorated
    for pre in (".?AV", ".?AU", ".?AW", ".?AT"):
        if s.startswith(pre): s = s[len(pre):]; break
    return "??_7" + s + "6B@"

TD = {}                                   # td_va -> DECORATED name (".?AVClass@@")
for m in re.finditer(rb'\.\?A[VUWT][\w@?$]+@@\x00', d):
    so = m.start(); rva = None
    for (nm, va, vsz, rsz, rp, ch) in SECS:
        if rp <= so < rp+rsz: rva = so-rp+va; break
    if rva is None: continue
    TD[(rva-8)+IMAGEBASE] = m.group(0)[:-1].decode('latin1')
COL = {}                                   # col_va -> (decorated, base_off)
COL_CHD = {}                               # col_va -> ClassHierarchyDescriptor VA
for (nm, va, vsz, rsz, rp, ch) in SECS:
    if nm != '.rdata': continue
    for a in range(va, va+rsz-20, 4):
        if u32(a) != 0: continue
        ptd = u32(a+12)
        if ptd in TD:
            COL[a+IMAGEBASE] = (TD[ptd], u32(a+4))
            COL_CHD[a+IMAGEBASE] = u32(a+16)   # COL+16 -> ClassHierarchyDescriptor

# --- data-section code-pointer runs ---
sites = [s for s in REL if not is_exec(s)]
RUNS = []           # (start_rva, end_rva)
i = 0; n = len(sites)
while i < n:
    s0 = sites[i]; v0 = u32(s0)
    if v0 is None or not is_exec(v0-IMAGEBASE): i += 1; continue
    last = s0; j = i+1
    while j < n and sites[j] == last+4:
        vj = u32(sites[j])
        if vj is None or not is_exec(vj-IMAGEBASE): break
        last = sites[j]; j += 1
    RUNS.append((s0, last+4)); i = j if j > i else i+1
MEMBERS = set()
for lo, hi in RUNS:
    MEMBERS.update(range(lo, hi, 4))

# --- vtable starts referenced by code (the ctor/dtor vptr stamps) ---
CODE_REF = {}       # start_rva -> #code-sites that point here
for s in REL:
    if not is_exec(s): continue
    v = u32(s)
    if v is None: continue
    t = v-IMAGEBASE
    if t in MEMBERS: CODE_REF[t] = CODE_REF.get(t, 0)+1

# COL-confirmed vtable starts: a run member whose slot[-1] is a COL pointer
COL_START = {}      # start_rva -> (name, base_off)
for lo, hi in RUNS:
    pv = u32(lo-4) if (lo-4) in RELSET else None
    if pv in COL: COL_START[lo] = COL[pv]
# also: any interior member preceded by a COL pointer (de-merged RTTI vtables)
for a in MEMBERS:
    if (a-4) in RELSET:
        pv = u32(a-4)
        if pv in COL: COL_START.setdefault(a, COL[pv])

KNOWN = set(COL_START) | set(CODE_REF)

# --- cut runs at KNOWN starts -> individual vtables with exact sizes ---
VTABLES = []        # dict per vtable
for lo, hi in RUNS:
    cuts = sorted(a for a in KNOWN if lo <= a < hi)
    if not cuts or cuts[0] != lo: cuts = [lo]+cuts
    cuts = sorted(set(cuts))
    for k, st in enumerate(cuts):
        en = cuts[k+1] if k+1 < len(cuts) else hi
        size = (en-st)//4
        col = COL_START.get(st)
        VTABLES.append(dict(
            start=st, size=size, sec=sec(st),
            rtti=demangle(col[0]) if col else None, decorated=col[0] if col else None,
            base_off=col[1] if col else None,
            code_refs=CODE_REF.get(st, 0),
            head_of_run=(st == lo),
            first=u32(st)-IMAGEBASE,
        ))

# --- project cross-reference: addresses already annotated DATA() in src/ ---
SRC_DATA = set()
if SRC:
    pat = re.compile(r'DATA\(0x([0-9a-fA-F]+)\)')
    for p in SRC.rglob("*.cpp"):
        for m in pat.finditer(p.read_text(errors='ignore')):
            a = int(m.group(1), 16); SRC_DATA.add(a-IMAGEBASE if a >= IMAGEBASE else a)

# ===========================================================================
# RTTI virtual-function recovery (--emit-vfuncs)
# ---------------------------------------------------------------------------
# A vtable slot stores the VA of an ILT *jmp thunk* (`e9 rel32`) in the low
# .text band, not the real body; the body is the jmp target.  resolve_slot
# follows that one hop.  A class's *new* and *overriding* virtuals are found by
# slot-diffing its resolved bodies against its direct RTTI parent's: slot i is
# inherited (parent owns it) when body[i]==parent_body[i]; an OVERRIDE when it
# differs at i<parent_count; a NEW virtual when i>=parent_count.
# ===========================================================================

def resolve_slot(rva):
    """Follow a vtable slot to its real body: an `e9 rel32` jmp thunk -> target,
    otherwise the slot value itself (already a body)."""
    o = off(rva)
    if o is not None and d[o] == 0xe9:
        rel = struct.unpack_from('<i', d, o+1)[0]
        return rva + 5 + rel
    return rva

# decorated TD name -> primary (base_off==0) RTTI vtable RVA.
PRIMARY_VT = {}
for v in VTABLES:
    if v['decorated'] and v['base_off'] == 0:
        PRIMARY_VT.setdefault(v['decorated'], v['start'])

# decorated TD name -> the vtable dict (so we can read its size + start).
VT_BY_DECOR = {}
for v in VTABLES:
    if v['decorated'] and v['base_off'] == 0:
        VT_BY_DECOR.setdefault(v['decorated'], v)

def vt_bodies(decorated):
    """Resolved body RVAs for a class's primary vtable slots, or [] if unknown."""
    v = VT_BY_DECOR.get(decorated)
    if not v: return []
    start = v['start']
    return [resolve_slot(u32(start + i*4) - IMAGEBASE) for i in range(v['size'])]

def class_bases(decorated):
    """Direct-parent decorated name for a class via its RTTI hierarchy, or None.
    Walks the BaseClassArray (COL+16 -> CHD; CHD+8 numBaseClasses, CHD+12 ->
    array of BaseClassDescriptor VAs; BCD+0 -> TD, BCD+4 numContainedBases).
    base[0] is always the class itself; the direct parent is the next base with
    the largest numContainedBases (the closest ancestor in single inheritance)."""
    # find a COL naming this class with base_off 0
    col_va = None
    for cv, (dn, bo) in COL.items():
        if dn == decorated and bo == 0:
            col_va = cv; break
    if col_va is None: return None
    chd = COL_CHD.get(col_va)
    if not chd: return None
    numbase = u32(chd-IMAGEBASE+8)
    bca = u32(chd-IMAGEBASE+12)
    if not numbase or not bca: return None
    bases = []   # (decorated, numContainedBases) excluding self
    for i in range(numbase):
        bcd = u32(bca-IMAGEBASE+i*4)
        if bcd is None: continue
        btd = u32(bcd-IMAGEBASE+0)
        ncb = u32(bcd-IMAGEBASE+4)
        bdn = TD.get(btd)
        if bdn is None: continue
        if bdn == decorated:  # base[0] is self
            continue
        bases.append((bdn, ncb))
    if not bases: return None
    # closest ancestor = most contained bases (deepest non-self base subtree)
    bases.sort(key=lambda x: -x[1])
    return bases[0][0]

def emit_vfuncs(decorated):
    """Per-class virtual-function plan.  Returns a list of dicts, one per slot
    that THIS class introduces or overrides (inherited slots are omitted)."""
    bodies = vt_bodies(decorated)
    if not bodies: return None
    parent = class_bases(decorated)
    pbodies = vt_bodies(parent) if parent else []
    pcount = len(pbodies)
    out = []
    for i, body in enumerate(bodies):
        if i < pcount and body == pbodies[i]:
            kind = "inherited"          # parent owns it -> do not emit
        elif i < pcount:
            kind = "override"
        else:
            kind = "new"
        if kind == "inherited":
            continue
        # name: reuse a real (non-FUN_/sub_) functions.csv name, else Vf<i>
        nm, sz = FN.get(body, (None, None))
        if not nm or nm.startswith("FUN_") or nm.startswith("sub_"):
            method = f"Vf{i}"
        else:
            method = nm
        # reconcile: is this body already labeled elsewhere in symbol_names.csv?
        reconcile = SYM_BY_RVA.get(body)
        out.append(dict(slot=i, kind=kind, body=body, size=sz,
                        method=method, reconcile=reconcile,
                        thunk=u32(VT_BY_DECOR[decorated]['start'] + i*4) - IMAGEBASE))
    return dict(decorated=decorated, rtti=demangle(decorated),
                parent=demangle(parent) if parent else None,
                parent_count=pcount, slots=len(bodies), virtuals=out)

# generated symbol_names.csv: body RVA -> (name, unit) for the reconcile flag.
SYM_BY_RVA = {}
_sym = _first(REPO / "build/gen/symbol_names.csv",
              "/home/sheep/Projects/gruntz/build/gen/symbol_names.csv")
if _sym:
    with open(_sym) as f:
        for r in csv.DictReader(f):
            try:
                if r.get('kind') == 'func':
                    SYM_BY_RVA[int(r['rva'], 16)] = (r['name'], r['unit'])
            except Exception: pass

def confidence(v):
    if v['rtti']: return "rtti"
    if v['code_refs'] and v['size'] >= 2 and v['sec'] == '.rdata': return "code-ref"
    if v['code_refs']: return "code-ref-weak"   # size1 or in .data: maybe a fn-ptr global
    return "unref"                              # run head, no COL, no code ref: likely EH/jump table

def find_decorated(name):
    """Resolve a user-supplied class name to a decorated TD name.  Accepts the
    decorated form, the bare class name (CFoo), or the demangled RTTI form."""
    for v in VTABLES:
        if not v['decorated']: continue
        if name in (v['decorated'], v['rtti'], demangle(v['decorated'])):
            return v['decorated']
    return None


def run_vfuncs(target):
    """--emit-vfuncs <ClassName|--all>: print the per-class virtual-function plan
    (human summary + machine-readable JSON-lines block)."""
    if target == "--all":
        decs = sorted({v['decorated'] for v in VTABLES
                       if v['decorated'] and v['base_off'] == 0})
    else:
        dec = find_decorated(target)
        if not dec:
            print(f"# class not found: {target!r} (try the bare CFoo name or .?AVCFoo@@)")
            return
        decs = [dec]

    plans = []
    for dec in decs:
        p = emit_vfuncs(dec)
        if p: plans.append(p)

    # human summary
    for p in plans:
        print(f"# class {p['rtti']}  ({p['slots']} slots) "
              f"parent={p['parent'] or '<root>'} (parent_count={p['parent_count']})")
        if not p['virtuals']:
            print("#   (all slots inherited; nothing to emit)")
        for vf in p['virtuals']:
            szs = f"0x{vf['size']:x}" if vf['size'] else "?(uncarved->RVAU)"
            rc = ""
            if vf['reconcile']:
                rc = f"  RECONCILE: already '{vf['reconcile'][0]}' in unit '{vf['reconcile'][1]}'"
            print(f"#   slot {vf['slot']:>2}  {vf['kind']:<8}  thunk 0x{vf['thunk']:06x}"
                  f" -> body 0x{vf['body']:06x}  size {szs:<18}  {vf['method']}{rc}")
        print()

    # machine-readable: one JSON object per (class, virtual) row.
    import json
    print("# --- machine-readable (JSON lines) ---")
    for p in plans:
        for vf in p['virtuals']:
            print(json.dumps(dict(
                rtti=p['rtti'], decorated=p['decorated'],
                parent=p['parent'], parent_count=p['parent_count'],
                slot=vf['slot'], kind=vf['kind'],
                thunk_rva=f"0x{vf['thunk']:06x}", body_rva=f"0x{vf['body']:06x}",
                size=(f"0x{vf['size']:x}" if vf['size'] else None),
                method=vf['method'],
                reconcile=(dict(name=vf['reconcile'][0], unit=vf['reconcile'][1])
                           if vf['reconcile'] else None))))


def main():
    a = sys.argv[1:]
    csv_path = None
    if "--csv" in a: k = a.index("--csv"); csv_path = a[k+1]; del a[k:k+2]
    names_path = None
    if "--emit-names" in a: k = a.index("--emit-names"); names_path = a[k+1]; del a[k:k+2]
    if "--emit-vfuncs" in a:
        k = a.index("--emit-vfuncs"); target = a[k+1]; del a[k:k+2]
        run_vfuncs(target); return
    only_new = "--new" in a

    for v in VTABLES: v['conf'] = confidence(v)
    rtti = [v for v in VTABLES if v['conf'] == "rtti"]
    cref = [v for v in VTABLES if v['conf'] == "code-ref"]
    weak = [v for v in VTABLES if v['conf'] == "code-ref-weak"]
    unref = [v for v in VTABLES if v['conf'] == "unref"]
    real = rtti + cref + weak                       # the actual vtables

    print("# sections: " + ", ".join(f"{nm}[{va:#x}+{max(vsz,rsz):#x}]{'X' if ch&0x20000000 else ''}"
          for (nm, va, vsz, rsz, rp, ch) in SECS))
    print(f"# reloc sites total={len(REL)} in-data={len(sites)} | data code-ptr runs={len(RUNS)}")
    print(f"# RTTI: typedescriptors={len(TD)} COLs={len(COL)}")
    print(f"# VTABLES: rtti={len(rtti)}  non-rtti(code-ref,sz>=2,.rdata)={len(cref)}  "
          f"non-rtti weak(sz1/.data)={len(weak)}  | total real={len(real)}")
    print(f"# (excluded: {len(unref)} unreferenced run-heads = C++ EH / switch tables, not vtables)")
    new = [v for v in real if v['start'] not in SRC_DATA]
    print(f"# already annotated DATA() in src/: {len(real)-len(new)}   NOT yet in src/: {len(new)}")
    print()

    show = sorted(new if only_new else real, key=lambda v: v['start'])
    print(f"{'start':>9} {'VA':>10} {'sec':<6} {'sz':>3} {'conf':<13} {'refs':>4} {'insrc':<5} class / first-entry")
    for v in show:
        cls = v['rtti'] or fn_label(v['first'])
        bo = f" [base+{v['base_off']}]" if v['base_off'] else ""
        insrc = "yes" if v['start'] in SRC_DATA else "NEW"
        print(f"  0x{v['start']:06x} {v['start']+IMAGEBASE:#010x} {v['sec']:<6} {v['size']:>3} "
              f"{v['conf']:<13} {v['code_refs']:>4} {insrc:<5} {cls}{bo}")

    if csv_path:
        with open(csv_path, "w", newline="") as f:
            w = csv.writer(f)
            w.writerow(["start_rva", "va", "section", "size", "confidence", "rtti_class",
                        "base_off", "code_refs", "in_src_data", "first_entry_rva", "first_entry"])
            for v in sorted(VTABLES, key=lambda v: v['start']):
                w.writerow([f"0x{v['start']:06x}", f"0x{v['start']+IMAGEBASE:08x}", v['sec'], v['size'],
                            v['conf'], v['rtti'] or "", v['base_off'] if v['base_off'] is not None else "",
                            v['code_refs'], int(v['start'] in SRC_DATA),
                            f"0x{v['first']:06x}", fn_label(v['first'])])
        print(f"\n# wrote {csv_path}")

    if names_path:
        # The deterministic target-side vtable names: ??_7<Class>@@6B@ at each RTTI
        # primary vtable RVA. The build (labels.py) applies a row ONLY when the
        # base obj actually emits that ??_7 (i.e. the class was made real-poly in
        # src), so this map is inert until a class is converted - zero noise.
        n = 0
        with open(names_path, "w", newline="") as f:
            w = csv.writer(f)
            w.writerow(["name", "rva", "size"])
            for v in sorted(VTABLES, key=lambda v: v['start']):
                if v['conf'] == "rtti" and v['base_off'] == 0 and v['decorated']:
                    w.writerow([vftable_name(v['decorated']), f"0x{v['start']:06x}",
                                f"0x{v['size']*4:x}"])
                    n += 1
        print(f"\n# wrote {names_path} ({n} RTTI primary vtable names)")

if __name__ == "__main__":
    main()
