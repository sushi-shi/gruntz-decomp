#!/usr/bin/env python3
"""gen_class_skeletons.py - generate the class inheritance + vtable CARCASS headers
for the RTTI classes in GRUNTZ.EXE: one `<Class>.h` each, with the inheritance
edge, the vtable slots this class introduces/overrides (as `virtual void slot_NN()`
placeholders), `#include`s of its base headers, and an include guard. NO fields,
NO bodies - just the scaffold the matching work fills in later.

MSVC 32-bit RTTI chain (absolute VAs, VC5/6):
  vtable[-1] -> CompleteObjectLocator { sig,off,cd, pTypeDescriptor, pCHD }
  CHD { sig, attr, numBaseClasses, pBaseClassArray -> [pBCD,...] }
  BCD { pTypeDescriptor, numContainedBases, PMD{mdisp,pdisp,vdisp}, attr }
Direct bases: DFS pre-order over the BCD array (i=1.., skip numContainedBases+1).
Vtable slots are followed through incremental-link jmp thunks to the real body.

    nix develop --command python3 scripts/analysis/gen_class_skeletons.py --sample
    nix develop --command python3 scripts/analysis/gen_class_skeletons.py --emit /tmp/incs
"""
from __future__ import annotations
import struct, os, re, csv, sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
EXE = os.environ["GRUNTZ_EXE"]
data = open(EXE, "rb").read()
N = len(data)

pe = struct.unpack_from("<I", data, 0x3c)[0]
nsec = struct.unpack_from("<H", data, pe + 6)[0]
opt = pe + 24
IMG = struct.unpack_from("<I", data, opt + 28)[0]
sec0 = opt + struct.unpack_from("<H", data, pe + 20)[0]
SECS = []
for i in range(nsec):
    o = sec0 + i * 40
    name = data[o:o + 8].rstrip(b"\0").decode("latin1")
    vsz = struct.unpack_from("<I", data, o + 8)[0]
    va = struct.unpack_from("<I", data, o + 12)[0]
    rsz = struct.unpack_from("<I", data, o + 16)[0]
    SECS.append((name, va, max(vsz, rsz), struct.unpack_from("<I", data, o + 20)[0]))
TEXT = [(va, vs) for n, va, vs, raw in SECS if n == ".text"][0]
LO, HI = TEXT[0], TEXT[0] + TEXT[1]


def off(rva):
    for n, va, vs, raw in SECS:
        if va <= rva < va + vs:
            return raw + (rva - va)
    return None


def dw(rva):
    o = off(rva)
    if o is None or o < 0 or o + 4 > N:
        return None
    return struct.unpack_from("<I", data, o)[0]


def is_code(rva):
    return rva is not None and LO <= rva < HI


def resolve_thunk(rva, hops=4):
    """Follow incremental-link `jmp rel32` (E9) thunks to the real body."""
    for _ in range(hops):
        o = off(rva)
        if o is None or o >= N or data[o] != 0xE9:
            break
        rel = struct.unpack_from("<i", data, o + 1)[0]
        rva = rva + 5 + rel
    return rva


NAMED = {}
sn = REPO / "build" / "gen" / "symbol_names.csv"
if sn.is_file():
    for r in csv.DictReader(open(sn)):
        if r.get("kind") == "func":
            NAMED[int(r["rva"], 16)] = r["name"]


def clean(td):
    m = re.match(r"\.\?A[UV](.+)@@$", td)
    return m.group(1) if m else td


# C++ identifiers can't be templates/operators; sanitise for a filename/typename.
def ident(cn):
    return re.sub(r"[^A-Za-z0-9_]", "_", cn)


# --- type descriptors ---
td_by_va = {}
for n, va, vs, raw in SECS:
    if n not in (".data", ".rdata"):
        continue
    blob = data[raw:raw + min(vs, N - raw)]
    for m in re.finditer(rb"\.\?A[UV][\x21-\x7e]*?@@", blob):
        td_by_va[IMG + (va + m.start() - 8)] = m.group().decode("latin1")

# --- COLs: col_va -> (name, pCHD) ---
col_info = {}
for n, va, vs, raw in SECS:
    if n not in (".data", ".rdata"):
        continue
    blob = data[raw:raw + min(vs, N - raw)]
    for o2 in range(0, len(blob) - 20, 4):
        if struct.unpack_from("<I", blob, o2)[0] != 0:
            continue
        ptd = struct.unpack_from("<I", blob, o2 + 12)[0]
        if ptd in td_by_va:
            col_info[va + o2] = (clean(td_by_va[ptd]),
                                 struct.unpack_from("<I", blob, o2 + 16)[0],   # pCHD
                                 struct.unpack_from("<I", blob, o2 + 4)[0])    # offset (subobject)

# --- vtables: vt_va -> (name, pCHD) ---
vt_info = {}
for n, va, vs, raw in SECS:
    if n not in (".data", ".rdata"):
        continue
    blob = data[raw:raw + min(vs, N - raw)]
    for o2 in range(0, len(blob) - 8, 4):
        colptr = struct.unpack_from("<I", blob, o2)[0] - IMG
        if colptr in col_info and is_code(struct.unpack_from("<I", blob, o2 + 4)[0] - IMG):
            vt_info[va + o2 + 4] = col_info[colptr]


def bcds(pchd):
    num = dw(pchd - IMG + 8)
    parr = dw(pchd - IMG + 0xc)
    if not num or not parr or num > 200:
        return []
    out = []
    for i in range(num):
        pbcd = dw(parr - IMG + i * 4)
        if not pbcd:
            break
        ptd = dw(pbcd - IMG)
        nm = td_by_va.get(ptd)
        out.append((clean(nm) if nm else None,
                    dw(pbcd - IMG + 8) or 0,      # mdisp
                    dw(pbcd - IMG + 4) or 0))     # numContainedBases
    return out


def direct_bases(bb):
    if not bb:
        return []
    out, i = [], 1
    while i < len(bb):
        nm, mdisp, ncont = bb[i]
        out.append((nm, mdisp))
        i += ncont + 1
    return out


def slots(vt, limit=120):
    out, p = [], vt
    while len(out) < limit:
        v = dw(p)
        if v is None or not is_code(v - IMG):
            break
        out.append(resolve_thunk(v - IMG))
        p += 4
    return out


# raw (unresolved) slot walk - needed to spot adjustor thunks on secondary vtables
def raw_slots(vt, limit=120):
    out, p = [], vt
    while len(out) < limit:
        v = dw(p)
        if v is None or not is_code(v - IMG):
            break
        out.append(v - IMG)
        p += 4
    return out


def adjustor(rva, hops=6):
    """If rva (through incremental-link jmps) is a `sub ecx, M; jmp tgt` this-
    adjustor thunk, return (M, resolved tgt); else None. M is the secondary-base
    sub-object offset, so an adjustor by M in the +M vtable == a real override."""
    cur = rva
    for _ in range(hops):
        o = off(cur)
        if o is None or o + 2 > N:
            return None
        if data[o] == 0xE9:                               # incr-link jmp rel32
            cur = cur + 5 + struct.unpack_from("<i", data, o + 1)[0]
            continue
        if data[o] == 0x83 and data[o + 1] == 0xE9:       # sub ecx, imm8
            m, jrva = data[o + 2], cur + 3
        elif data[o] == 0x81 and data[o + 1] == 0xE9:     # sub ecx, imm32
            m, jrva = struct.unpack_from("<I", data, o + 2)[0], cur + 6
        else:
            return None
        jo = off(jrva)
        if jo is None or jo >= N or data[jo] != 0xE9:
            return None
        return (m, resolve_thunk(jrva + 5 + struct.unpack_from("<i", data, jo + 1)[0]))
    return None


# every vtable per class keyed by sub-object offset (offset 0 == primary vtable)
vtabs = {}                                  # name -> {offset: (vt_rva, pCHD)}
for vt, (nm, pchd, offs) in vt_info.items():
    vtabs.setdefault(nm, {}).setdefault(offs, (vt, pchd))

# class name -> (primary vt_rva, [(base,mdisp)], [resolved primary slot rvas])
cls = {}
for nm, byoff in vtabs.items():
    vt, pchd = byoff.get(0, next(iter(byoff.values())))   # the primary (offset 0) vtable
    cls[nm] = (vt, direct_bases(bcds(pchd)), slots(vt))


def primary_base(cn):
    for b, mdisp in cls[cn][1]:
        if b and b != cn and mdisp == 0 and b in cls:
            return b
    return None


# --- library/runtime classes we never reconstruct: drop them from the carcass.
# A game class that derives from one keeps it only as a 1-line external base-stub.
_MFC = {
    "CObject", "CCmdTarget", "CWnd", "CDC", "CClientDC", "CWindowDC", "CPaintDC",
    "CGdiObject", "CBrush", "CPen", "CFont", "CBitmap", "CPalette", "CRgn", "CMenu",
    "CDialog", "CFrameWnd", "CMDIFrameWnd", "CMDIChildWnd", "CView", "CScrollView",
    "CCtrlView", "CEditView", "CFormView", "CDocument", "CWinApp", "CWinThread",
    "CButton", "CEdit", "CComboBox", "CListBox", "CStatic", "CScrollBar",
    "CSliderCtrl", "CSpinButtonCtrl", "CProgressCtrl", "CAnimateCtrl", "CHeaderCtrl",
    "CDragListBox", "CListCtrl", "CTreeCtrl", "CRichEditCtrl", "CToolTipCtrl",
    "CImageList", "CFile", "CStdioFile", "CMemFile", "CException", "CFileException",
    "CArchiveException", "CMemoryException", "CNotSupportedException",
    "CResourceException", "CUserException", "COleException", "CSimpleException",
    "CArchive", "CByteArray", "CWordArray", "CDWordArray", "CPtrArray", "CObArray",
    "CStringArray", "CPtrList", "CObList", "CStringList", "CMapWordToPtr",
    "CMapPtrToWord", "CMapPtrToPtr", "CMapWordToOb", "CMapStringToPtr",
    "CMapStringToOb", "CMapStringToString", "CCommandLineInfo", "CNoTrackObject",
    "CWaitCursor", "CDataExchange", "CControlBar", "CStatusBar", "CToolBar",
    "CDialogBar", "CStatusBarCtrl", "CToolBarCtrl",
}
_CRT = {
    "ios", "istream", "ostream", "iostream", "ifstream", "ofstream", "fstream",
    "istrstream", "ostrstream", "strstream", "streambuf", "filebuf",
    "strstreambuf", "stdiobuf", "stdiostream", "istream_withassign",
    "ostream_withassign", "iostream_withassign", "type_info", "exception",
    "bad_cast", "bad_typeid", "bad_alloc", "bad_exception", "logic_error",
    "runtime_error", "__non_rtti_object",
}


def is_lib(cn):
    return ("?$" in cn or cn.startswith("AFX") or cn.startswith("_")
            or cn in _MFC or cn in _CRT)


KEEP = {cn for cn in cls if not is_lib(cn)}


def ext_bases():
    """Bases a kept class derives from that are not themselves kept (lib/missing)."""
    refs = set()
    for cn in KEEP:
        for b, _ in cls[cn][1]:
            if b and b != cn and b not in KEEP:
                refs.add(b)
    return refs


def own_slots(cn):
    """(slot, rva, is_override) for slots this class introduces/overrides vs its
    primary base. Diff against the base's real vtable even when the base is a
    DROPPED library class (so inherited library methods aren't re-emitted in every
    subclass) - OVERRIDE is marked only when the base is itself emitted (kept), as
    a dropped base is a fieldless stub with no virtual to override."""
    _, _, s = cls[cn]
    pb = primary_base(cn)              # primary_base() already guarantees pb in cls
    if pb:
        bs = cls[pb][2]
        kept = pb in KEEP
        out = []
        for i, r in enumerate(s):
            if i >= len(bs):
                out.append((i, r, False))     # new virtual this class introduces
            elif bs[i] != r:
                out.append((i, r, kept))      # override (OVERRIDE only if base emitted)
            # else bs[i] == r: inherited -> not emitted
        return out
    return [(i, r, False) for i, r in enumerate(s)]


def slot_comment(r):
    return NAMED.get(r, "")


def emit(cn):
    vt, dbases, _ = cls[cn]
    bases = [b for b, _ in dbases if b and b != cn]
    g = "GRUNTZ_STUB_" + ident(cn) + "_H"   # case-sensitive: IStream != istream
    L = [f"#ifndef {g}", f"#define {g}",
         f"// {cn} - RTTI inheritance + vtable carcass (no fields/bodies yet).",
         f"// vtable @0x{vt:x}; generated by scripts/analysis/gen_class_skeletons.py.",
         '#include "../rva.h"   // OVERRIDE (no-op under MSVC 5.0)']
    for b in bases:
        L.append(f'#include "{ident(b)}.h"')
    inh = (" : " + ", ".join("public " + ident(b) for b in bases)) if bases else ""
    L.append("")
    L.append(f"class {ident(cn)}{inh} {{")
    L.append("public:")
    for i, r, ovr in own_slots(cn):
        nm = slot_comment(r)
        tail = f"  {nm}" if nm else ""
        o = " OVERRIDE" if ovr else ""
        L.append(f"    virtual void slot_{i:02x}(){o};   // 0x{r:06x}{tail}")
    L.append("};")
    L.append(f"#endif  // {g}")
    return "\n".join(L)


def ext_stub(b):
    g = "GRUNTZ_STUB_" + ident(b) + "_H"
    return (f"#ifndef {g}\n#define {g}\n"
            f"// {b} - external/library base (not reconstructed; minimal stub).\n"
            f"class {ident(b)} {{}};\n#endif  // {g}\n")


if "--class" in sys.argv:
    cn = sys.argv[sys.argv.index("--class") + 1]
    print(emit(cn) if cn in cls else f"// {cn}: not among {len(cls)} RTTI classes")
    sys.exit()

if "--compare" in sys.argv:
    i = sys.argv.index("--compare")
    a = sys.argv[i + 1]
    va, dba, sa = cls[a]
    print(f"{a}  vtable@0x{va:x}  {len(sa)} slots; direct bases: "
          f"{[(b, hex(m)) for b, m in dba]}")
    pb = primary_base(a)
    sb = cls[pb][2] if pb and pb in cls else []
    print(f"primary base = {pb} ({len(sb)} slots)\n")
    print(f"  {'idx':>4}  {a:>10}  {pb or '-':>12}  raw@vt   verdict")
    p = va
    for j in range(max(len(sa), len(sb))):
        ra = f"0x{sa[j]:06x}" if j < len(sa) else "----"
        rb = f"0x{sb[j]:06x}" if j < len(sb) else "----"
        raw = dw(p) if j < len(sa) else None
        rawr = (raw - IMG) if raw else 0
        if j >= len(sb):
            v = "new"
        elif j >= len(sa):
            v = "(base only)"
        elif sa[j] == sb[j]:
            v = "inherited"
        else:
            v = "OVERRIDE"
        rawtxt = f"raw=0x{rawr:06x}{'(thunk)' if rawr != (sa[j] if j<len(sa) else -1) else ''}"
        print(f"  {j:>4x}  {ra:>10}  {rb:>12}  {rawtxt:>16}  {v}")
        p += 4
    sys.exit()

if "--secondary" in sys.argv:
    cn = sys.argv[sys.argv.index("--secondary") + 1]
    print(f"{cn} sub-object vtables (offset -> vtable):")
    for offs in sorted(vtabs[cn]):
        vt, _ = vtabs[cn][offs]
        rs = raw_slots(vt)
        print(f"\n  offset +0x{offs:x}  vtable@0x{vt:x}  ({len(rs)} slots)")
        for i, r in enumerate(rs):
            a = adjustor(r) if offs else None
            tag = (f"OVERRIDE via adjustor(0x{a[0]:x}) -> 0x{a[1]:06x}"
                   if (a and a[0] == offs) else ("inherited" if offs else ""))
            print(f"    slot {i:02x}: raw 0x{r:06x}  {tag}")
    sys.exit()

if "--emit" in sys.argv:
    outdir = Path(sys.argv[sys.argv.index("--emit") + 1])
    outdir.mkdir(parents=True, exist_ok=True)
    for cn in KEEP:
        (outdir / f"{ident(cn)}.h").write_text(emit(cn) + "\n")
    eb = ext_bases()
    for b in eb:
        (outdir / f"{ident(b)}.h").write_text(ext_stub(b))
    print(f"emitted {len(KEEP)} game-class carcasses + {len(eb)} external-base stubs "
          f"to {outdir} (dropped {len(cls) - len(KEEP)} library/runtime classes)")
    sys.exit()

# default: summary
dropped = sorted(cn for cn in cls if is_lib(cn))
print(f"// {len(cls)} RTTI classes -> KEEP {len(KEEP)} game classes, "
      f"drop {len(dropped)} library/runtime, +{len(ext_bases())} external-base stubs")
print("\ndropped (library/runtime):\n  " + ", ".join(dropped))
print("\n=== sample (own-slot diff + OVERRIDE + thunk-resolved) ===")
for cn in ("CBoomerang", "CGruntStaminaSprite", "CActionArea", "CBattlezDlg"):
    if cn in cls:
        print(emit(cn), "\n")
