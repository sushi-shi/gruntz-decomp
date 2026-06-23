#!/usr/bin/env python3
"""gruntz.analysis.grind_ctors - exhaustively find game/engine constructors.

Unions three ctor signals, then recurses to a fixpoint:
  1. vtable-stamp     - a function with `mov [this], <vftable>` (ctor or dtor).
  2. alloc-adjacency  - the call right after an allocation (operator new / malloc).
  3. sub-object       - from any known ctor, `mov ecx,<ptr>/lea ecx,[this+d]; call X`
                        constructs a base/member sub-object -> X is a ctor too.

A candidate is kept if it is CTOR-SHAPED (stamps a vtable OR returns `this`), a real
body, GAME (its stamped vtable's RTTI class is not a library class; non-RTTI is
assumed game), and not already labeled/matched/FID-library. Dtors are dropped
(call operator delete / don't return this and aren't alloc-reached).

Emits ctor-survey/grind_ctors.csv (rva,size,class,source) - the full game-ctor set,
unlabeled ones flagged for labeling.

Run after `gruntz build`: python -m gruntz.analysis.grind_ctors
"""
import os, csv, re, struct, bisect, sys
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
EXE = Path(os.environ.get("GRUNTZ_EXE") or REPO / "build/exe/GRUNTZ.EXE")
FUNCS = REPO / "build/ghidra-enrich/exports/functions.csv"
MATCHED = REPO / "build/gen/symbol_names.csv"
LIBCSV = REPO / "config/library_labels.csv"
VTABS = REPO / "ctor-survey/vtables.csv"
SRC = REPO / "src"
OPNEW = 0x1b9b46
ODEL = 0x1b9b82                                   # operator delete (frees `this`)
SANE = 0x40000
LIB = set("""CWinApp CWinThread CCmdTarget CCmdUI CTestCmdUI CCommandLineInfo
CRecentFileList CNoTrackObject CObject CWnd CFrameWnd CDialog CView CCtrlView
CScrollView CDocument CDC CClientDC CPaintDC CWindowDC CGdiObject CBrush CPen CRgn
CMenu CImageList CImage CButton CComboBox CEdit CStatic CListBox CDragListBox
CScrollBar CSliderCtrl CSpinButtonCtrl CProgressCtrl CHeaderCtrl CHotKeyCtrl
CListCtrl CTabCtrl CTreeCtrl CRichEditCtrl CAnimateCtrl CStatusBarCtrl CToolBarCtrl
CFile CMemFile CMirrorFile CArchiveStream CByteArray CDWordArray CStringArray
CPtrArray CObArray CStringList CPtrList CObList CMapStringToOb CMapStringToPtr
CMapPtrToPtr CException CArchiveException CFileException CMemoryException
CNotSupportedException CResourceException CSimpleException CUserException CThreadData
type_info ios iostream istream ostream istream_withassign ostream_withassign
istrstream ostrstream strstream filebuf fstream ifstream ofstream streambuf
strstreambuf IUnknown IStream ISequentialStream""".split())


def load():
    data = EXE.read_bytes()
    pe = struct.unpack_from("<I", data, 0x3c)[0]
    nsec = struct.unpack_from("<H", data, pe + 6)[0]
    opt = struct.unpack_from("<H", data, pe + 20)[0]
    text = opdel = None
    for i in range(nsec):
        o = pe + 24 + opt + i * 40
        nm = data[o:o + 8].rstrip(b"\0").decode("latin1")
        _vs, va, _rs, rp = struct.unpack_from("<IIII", data, o + 8)
        if nm == ".text":
            text = (va, _rs, rp)
    return data, text


def main():
    data, text = load()
    trva = text[0]
    tb = data[text[2]:text[2] + text[1]]
    n = len(tb)

    starts, fsize, fname = [], {}, {}
    with open(FUNCS) as f:
        for r in csv.DictReader(f):
            rva = int(r["entry_rva"], 16)
            starts.append(rva); fsize[rva] = int(r["byte_size"]); fname[rva] = r["name"]
    starts.sort()

    def func_of(rva):
        i = bisect.bisect_right(starts, rva) - 1
        return starts[i] if i >= 0 and rva < starts[i] + fsize[starts[i]] else None

    thunk = {}
    for rva in starts:
        if fsize.get(rva) == 5 and 0 <= rva - trva < n and tb[rva - trva] == 0xE9:
            thunk[rva] = rva + 5 + struct.unpack_from("<i", tb, rva - trva + 1)[0]
    resolve = lambda t: thunk.get(t, t)

    # vtable VA -> class (RTTI)
    vtab_class = {}
    if VTABS.exists():
        for r in csv.DictReader(open(VTABS)):
            vtab_class[int(r["vftable_va"], 16)] = r["class"]
    vtab_set = set(vtab_class)

    # labeled set
    labeled = set()
    if MATCHED.exists():
        for line in MATCHED.read_text().splitlines()[1:]:
            try: labeled.add(int(line.split(",", 1)[0], 16))
            except ValueError: pass
    if LIBCSV.exists():
        for r in csv.DictReader(open(LIBCSV)):
            try: labeled.add(int(r["rva"], 16))
            except (ValueError, KeyError): pass
    for p in list(SRC.rglob("*.cpp")) + list(SRC.rglob("*.h")):
        if p.name == "MallocConstructors.cpp":       # this unit's own labels - keep idempotent
            continue
        for m in re.finditer(r"RVAU?\(\s*0x([0-9a-fA-F]+)", p.read_text(errors="ignore")):
            labeled.add(int(m.group(1), 16))

    def named(rva):
        nm = fname.get(rva, "")
        return nm and not nm.startswith(("FUN_", "thunk_FUN_"))

    def stamped_vtable(rva):
        """last offset-0 vtable VA the function stamps (most-derived), or None."""
        b, last = rva - trva, None
        for p in range(b, b + fsize.get(rva, 0) - 3):
            if 0 <= p < n - 3:
                v = tb[p] | (tb[p+1] << 8) | (tb[p+2] << 16) | (tb[p+3] << 24)
                if v in vtab_set and p >= 2 and tb[p-2] == 0xC7 and (tb[p-1] >> 3) & 7 == 0:
                    last = v
        return last

    def returns_this(rva):
        """`mov eax,<this-reg>` followed only by epilogue then ret - the MSVC
        complete-object-ctor tell (dtors return void)."""
        b = rva - trva; e = b + fsize.get(rva, 0)
        for p in range(e - 2, max(b, e - 24) - 1, -1):
            if not (0 <= p < n - 1 and tb[p] == 0x8B and tb[p+1] in (0xC1, 0xC3, 0xC6, 0xC7)):
                continue
            q = p + 2                            # everything to ret must be epilogue
            while q < e and q < n:
                c = tb[q]
                if c in (0xC3, 0xC2): return True            # ret / ret n
                if 0x58 <= c <= 0x5F: q += 1; continue       # pop reg
                if c == 0xC9: q += 1; continue               # leave
                if c == 0x8B and q+1 < n and tb[q+1] == 0xE5: q += 2; continue  # mov esp,ebp
                if c == 0x83 and q+1 < n and tb[q+1] == 0xC4: q += 3; continue  # add esp,imm8
                if c == 0x64: q += 7; continue               # fs: SEH unlink (approx)
                if c == 0x89 and q+1 < n and tb[q+1] == 0x4C: q += 4; continue  # mov [esp+x],ecx
                break
        return False

    def calls_opdelete(rva):
        # a scalar deleting destructor frees `this` (it ALSO returns this, so the
        # returns-this test alone can't tell it from a ctor - operator delete can).
        b, e = rva - trva, rva - trva + fsize.get(rva, 0)
        for p in range(b, e - 4):
            if 0 <= p < n - 4 and tb[p] == 0xE8 and \
                    resolve((trva + p + 5) + struct.unpack_from("<i", tb, p + 1)[0]) == ODEL:
                return True
        return False

    def is_game_ctor(rva):
        if rva not in fsize or func_of(rva) != rva or fsize[rva] < 8:
            return None
        vt = stamped_vtable(rva)
        cls = vtab_class.get(vt) if vt else None
        if cls in LIB:
            return None                          # library ctor -> drop
        if not returns_this(rva):
            return None                          # base/complete dtor (returns void)
        if calls_opdelete(rva):
            return None                          # SCALAR DELETING DTOR (returns this but frees)
        return cls or ""                         # game ctor (RTTI class name or non-RTTI "")

    # ---- seed 1: alloc-adjacency call-afters ----
    def adj_size(o):
        if tb[o-5] == 0x68:
            v = struct.unpack_from("<I", tb, o-4)[0]; return v if 1 <= v <= SANE else None
        if tb[o-2] == 0x6A and tb[o-1] >= 1: return tb[o-1]
        return None
    def ptr_after(o):
        for q in range(o+5, min(n-1, o+12)):
            if tb[q] == 0x85 and tb[q+1] in (0xC0, 0xF6, 0xFF, 0xDB, 0xC9): return True
            if tb[q] == 0x8B and tb[q+1] in (0xF0, 0xF8, 0xD8, 0xC8, 0xD0): return True
        return False
    call_tgt = lambda o: resolve((trva + o + 5) + struct.unpack_from("<i", tb, o + 1)[0])
    allochits = {}
    for o in range(1, n - 5):
        if tb[o] == 0xE8 and adj_size(o) is not None and ptr_after(o):
            allochits[call_tgt(o)] = allochits.get(call_tgt(o), 0) + 1
    allocators = {OPNEW} | {t for t, c in allochits.items() if c >= 6 and func_of(t) == t}
    seed = set()
    for o in range(1, n - 5):
        if tb[o] == 0xE8 and call_tgt(o) in allocators:
            s, e = o + 5, min(n - 5, o + 53)
            while s < e:
                if tb[s] == 0xE8:
                    seed.add(call_tgt(s)); break
                s += 1

    # ---- seed 2: every vtable-stamping function (ctor or dtor) ----
    lo, hi = min(vtab_set), max(vtab_set)
    for o in range(2, n - 3):
        v = tb[o] | (tb[o+1] << 8) | (tb[o+2] << 16) | (tb[o+3] << 24)
        if lo <= v <= hi and v in vtab_set and tb[o-2] == 0xC7 and (tb[o-1] >> 3) & 7 == 0:
            fn = func_of(trva + o)
            if fn is not None: seed.add(fn)

    # ---- fixpoint: recurse sub-object ctor calls from known ctors ----
    ctors = {}                                   # rva -> class
    work = list(seed)
    seen = set()
    while work:
        rva = work.pop()
        if rva in seen: continue
        seen.add(rva)
        cls = is_game_ctor(rva)
        if cls is None: continue
        ctors[rva] = cls
        # sub-object ctor calls: `call X` with this set just before (mov ecx,* / lea ecx,[*])
        b, e = rva - trva, rva - trva + fsize.get(rva, 0)
        for p in range(b, e - 5):
            if 0 <= p < n - 5 and tb[p] == 0xE8:
                # this-setup in the preceding ~7 bytes?
                pre = tb[max(0, p - 7):p]
                if (b"\x8b\xc8" in pre or b"\x8b\xce" in pre or b"\x8b\xcf" in pre or
                        b"\x8b\xcb" in pre or b"\x8d\x4e" in pre or b"\x8d\x4f" in pre or b"\x8d\x48" in pre):
                    t = call_tgt(p)
                    if t not in seen:
                        work.append(t)

    rows = sorted(ctors.items(), key=lambda kv: (kv[1] == "", -fsize.get(kv[0], 0)))
    newlbl = [(r, c) for r, c in rows if r not in labeled]
    named_ct = sum(1 for _, c in rows if c)
    print(f"game/engine ctors found      : {len(ctors)}")
    print(f"  RTTI-named                  : {named_ct}   non-RTTI: {len(ctors)-named_ct}")
    print(f"  already labeled/matched     : {len(ctors)-len(newlbl)}")
    print(f"  NEW (to label)              : {len(newlbl)}")
    for r, c in sorted(newlbl, key=lambda kv: (kv[1] == "", -fsize.get(kv[0], 0))):
        print(f"    0x{r:06x}  {fsize.get(r,0):4d} B  {c or '(non-RTTI)'}")
    out = REPO / "ctor-survey"; out.mkdir(exist_ok=True)
    with open(out / "grind_ctors.csv", "w", newline="") as f:
        w = csv.writer(f); w.writerow(["rva", "size", "class", "labeled"])
        for r, c in rows:
            w.writerow([f"0x{r:06x}", fsize.get(r, 0), c, int(r in labeled)])
    print(f"wrote {out / 'grind_ctors.csv'}")

    if "--label" in sys.argv:
        # Each discovered ctor is labeled as a ctor OF ITS CLASS with the class
        # SIZE()-set: the real RTTI name where that class isn't defined yet, else a
        # placeholder (non-RTTI has no name) - sized from its operator-new site.
        szmap = {}
        nf = REPO / "ctor-survey/new_functions.csv"
        if nf.exists():
            for row in csv.DictReader(open(nf)):
                try: szmap[int(row["rva"], 16)] = int(row["top_sizeof"], 16)
                except (ValueError, KeyError): pass
        defined = set()
        HDR, CPP = REPO / "include/Stub/MallocConstructors.h", REPO / "src/Stub/MallocConstructors.cpp"
        for p in list(SRC.rglob("*.h")) + list(SRC.rglob("*.cpp")) + list((REPO / "include").rglob("*.h")):
            if p in (HDR, CPP):                          # our own emitted classes don't count
                continue
            for m in re.finditer(r"\b(?:class|struct)\s+(\w+)", p.read_text(errors="ignore")):
                defined.add(m.group(1))
        # idempotent: treat THIS unit's own labels as not-yet-labeled so re-running
        # --label regenerates the full set (symbol_names.csv carries them post-build).
        own = set()
        if CPP.exists():
            for m in re.finditer(r"RVAU?\(\s*0x([0-9a-fA-F]+)", CPP.read_text(errors="ignore")):
                own.add(int(m.group(1), 16))
        emit_rows = [(r, c) for r, c in rows if r not in (labeled - own)]
        # Classes go in a HEADER (so they can be reused/inherited); ctor bodies in the .cpp.
        H = ["#ifndef GRUNTZ_STUB_MALLOCCONSTRUCTORS_H", "#define GRUNTZ_STUB_MALLOCCONSTRUCTORS_H",
             "#include <rva.h>",
             "// MallocConstructors.h - classes whose constructors gruntz.analysis.grind_ctors",
             "// discovered at operator-new/malloc sites; class SIZE()-set from the allocation",
             "// site. Real RTTI name where the class wasn't defined elsewhere, else a",
             "// MallocCtor_<rva> placeholder (no recoverable name).", ""]
        C = ["#include <rva.h>", "#include <Stub/MallocConstructors.h>",
             "// MallocConstructors.cpp - constructor bodies for MallocConstructors.h.", ""]
        emitted, skipped = 0, []
        for r, c in sorted(emit_rows, key=lambda kv: (kv[1] == "", -fsize.get(kv[0], 0))):
            if c and c in defined:
                skipped.append((r, c)); continue          # class defined elsewhere -> label there
            name = c if c else f"MallocCtor_{r:06x}"       # placeholder for a non-RTTI class
            s = szmap.get(r)
            H.append(f"// {c or 'non-RTTI'} ctor @ 0x{r:08x}" + (f", sizeof 0x{s:x}" if s else " (size TBD)"))
            H.append(f"class {name} {{")
            H.append("public:")
            H.append(f"    {name}();")
            if s: H.append(f"    char m_data[0x{s:x}]; // operator-new size; real fields TBD")
            H.append("};")
            if s: H.append(f"SIZE({name}, 0x{s:x});")
            H.append("")
            C.append(f"RVA(0x{r:08x}, 0x{fsize.get(r,0):x})")
            C.append(f"{name}::{name}() {{}}")
            C.append("")
            emitted += 1
        H.append("#endif")
        if skipped:
            C.append("// ctors whose class is already defined elsewhere - label them in that TU:")
            for r, c in skipped:
                C.append(f"//   {c}::ctor @ 0x{r:08x}")
        HDR.write_text("\n".join(H) + "\n")
        CPP.write_text("\n".join(C) + "\n")
        allc = REPO / "src/Stub/All.cpp"; at = allc.read_text()
        if "MallocConstructors.cpp" not in at:
            allc.write_text(at.rstrip() + '\n#include "MallocConstructors.cpp"\n')
        print(f"labeled {emitted} ctors -> MallocConstructors.h/.cpp; {len(skipped)} skipped (class already defined)")


if __name__ == "__main__":
    main()
