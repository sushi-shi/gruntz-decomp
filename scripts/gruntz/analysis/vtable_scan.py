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
    python3 -m gruntz.analysis.vtable_scan --emit-unknown-header include/UnknownVTables.h
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
INCLUDE = _first(REPO / "include", "/home/sheep/Projects/gruntz/include")
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
for (nm, va, vsz, rsz, rp, ch) in SECS:
    if nm != '.rdata': continue
    for a in range(va, va+rsz-20, 4):
        if u32(a) != 0: continue
        ptd = u32(a+12)
        if ptd in TD: COL[a+IMAGEBASE] = (TD[ptd], u32(a+4))

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
SRC_DATA_NAMES = {}
if SRC:
    pat = re.compile(r'DATA\(0x([0-9a-fA-F]+)\)')
    extern_pat = re.compile(
        r'DATA\(0x([0-9a-fA-F]+)\)\s*(?://[^\n]*)?\s*'
        r'(?P<decl>[^;{}=]+);',
        re.MULTILINE,
    )
    for root in [x for x in (SRC, INCLUDE) if x]:
        for p in list(root.rglob("*.cpp")) + list(root.rglob("*.h")):
            text = p.read_text(errors='ignore')
            for m in pat.finditer(text):
                a = int(m.group(1), 16); SRC_DATA.add(a-IMAGEBASE if a >= IMAGEBASE else a)
            for m in extern_pat.finditer(text):
                a = int(m.group(1), 16); SRC_DATA.add(a-IMAGEBASE if a >= IMAGEBASE else a)
                names = re.findall(r'\b[A-Za-z_]\w*\b', m.group('decl').split('//', 1)[0])
                if names:
                    SRC_DATA_NAMES.setdefault(a-IMAGEBASE if a >= IMAGEBASE else a, names[-1])

def confidence(v):
    if v['rtti']: return "rtti"
    if v['code_refs'] and v['size'] >= 2 and v['sec'] == '.rdata': return "code-ref"
    if v['code_refs']: return "code-ref-weak"   # size1 or in .data: maybe a fn-ptr global
    return "unref"                              # run head, no COL, no code ref: likely EH/jump table

def ident(s, fallback):
    if s == "__purecall":
        return "__purecall"
    if re.fullmatch(r"`?scalar[_ -]?deleting[_ -]?destructor`?", s, flags=re.I):
        return "_scalar_deleting_destructor_"
    s = s.replace("`", "").replace("'", "")
    s = s.replace("~", "Dtor_")
    s = re.sub(r"scalar[_ -]?deleting[_ -]?destructor", "_scalar_deleting_destructor_", s, flags=re.I)
    s = re.sub(r"vector[_ -]?deleting[_ -]?destructor", "vector_deleting_destructor", s, flags=re.I)
    s = re.sub(r"[^0-9A-Za-z_]+", "_", s).strip("_")
    if not s:
        s = fallback
    if s[0].isdigit():
        s = "_" + s
    return s

def vtable_struct_name(v):
    src = SRC_DATA_NAMES.get(v['start'])
    if src:
        s = src
        for prefix in ("g_", "s_"):
            if s.startswith(prefix):
                s = s[len(prefix):]
        if "Vtbl" in s or "Vftbl" in s or "Vtable" in s or "Vftable" in s:
            return ident(s[:1].upper() + s[1:], f"Vtbl_{v['start']:06x}")
        for suffix in ("_vftable", "_vtable", "_Vtbl", "Vtbl", "_vftbl", "_vtbl"):
            if s.endswith(suffix):
                s = s[:-len(suffix)]
                break
        if s:
            return ident(s[:1].upper() + s[1:] + "Vtbl", f"Vtbl_{v['start']:06x}")
    return f"Vtbl_{v['start']:06x}"

def slot_name(v, i):
    rva = u32(v['start'] + i * 4) - IMAGEBASE
    label = fn_label(rva)
    if label.startswith("sub_"):
        return f"slot{i}_{rva:06x}", label, rva
    return ident(label, f"slot{i}_{rva:06x}"), label, rva

def emit_unknown_header(path, real):
    unknown = [
        v for v in sorted(real, key=lambda x: x['start'])
        if not v['rtti'] and v['sec'] == '.rdata' and v['code_refs']
    ]
    out = []
    out.append("// UnknownVTables.h - AUTO-GENERATED catalog of the discovered non-RTTI vtables\n")
    out.append("// (no Complete Object Locator => no recoverable class name). We DO know every\n")
    out.append("// address: each vtable is named by its src g_*Vtbl name when one exists, else by\n")
    out.append("// its RVA (Vtbl_<rva>); each slot is a named member carrying its target RVA + the\n")
    out.append("// function name where known (scalar/vector-deleting dtor, __purecall, or a matched\n")
    out.append("// fn) or sub_<rva> for the still-un-reconstructed engine virtuals. Slots are typed\n")
    out.append("// UnkVfn (void(void)) - TRACKING ONLY; this header emits no code, is not #included,\n")
    out.append("// and is matching-neutral. Generated by gruntz.analysis.vtable_scan from retail\n")
    out.append("// GRUNTZ.EXE. Sizes exact (reloc-run cut at every RTTI/code-referenced start).\n")
    out.append("#ifndef GRUNTZ_UNKNOWN_VTABLES_H\n")
    out.append("#define GRUNTZ_UNKNOWN_VTABLES_H\n\n")
    out.append("typedef void (*UnkVfn)(void);\n\n")
    used_structs = set()
    for idx, v in enumerate(unknown, 1):
        src = SRC_DATA_NAMES.get(v['start'])
        src_suffix = f"  src:{src}" if src else ""
        out.append(
            f"// ClassWithUnknownVTable{idx}  @ 0x{v['start'] + IMAGEBASE:08x}  "
            f"({v['size']} slots)  refs x{v['code_refs']}{src_suffix}\n"
        )
        struct_name = vtable_struct_name(v)
        if struct_name in used_structs:
            struct_name = f"{struct_name}_{v['start']:06x}"
        used_structs.add(struct_name)
        out.append(f"struct {struct_name} {{\n")
        used_slots = set()
        rows = []
        for i in range(v['size']):
            nm, label, rva = slot_name(v, i)
            if nm in used_slots:
                nm = f"{nm}_{i}"
            used_slots.add(nm)
            rows.append((nm, i, rva, label))
        width = max((len(nm) for nm, _, _, _ in rows), default=0)
        for nm, i, rva, label in rows:
            out.append(f"    UnkVfn {nm};{' ' * (width - len(nm))} // [{i}] -> 0x{rva:06x} {label}\n")
        out.append("};\n\n")
    out.append("#endif // GRUNTZ_UNKNOWN_VTABLES_H\n")
    Path(path).write_text("".join(out))
    return len(unknown)

def main():
    a = sys.argv[1:]
    csv_path = None
    if "--csv" in a: k = a.index("--csv"); csv_path = a[k+1]; del a[k:k+2]
    names_path = None
    if "--emit-names" in a: k = a.index("--emit-names"); names_path = a[k+1]; del a[k:k+2]
    unknown_header_path = None
    if "--emit-unknown-header" in a:
        k = a.index("--emit-unknown-header"); unknown_header_path = a[k+1]; del a[k:k+2]
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

    if unknown_header_path:
        n = emit_unknown_header(unknown_header_path, real)
        print(f"\n# wrote {unknown_header_path} ({n} non-RTTI vtables)")

if __name__ == "__main__":
    main()
