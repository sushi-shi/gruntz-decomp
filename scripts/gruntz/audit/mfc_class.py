#!/usr/bin/env python3
"""gruntz.audit.mfc_class - ask the BINARY which MFC class owns an address.

Why this exists
---------------
MFC's containers are byte-identical to each other.  `CObArray`/`CPtrArray`/
`CDWordArray`/`CUIntArray` all compile to the same code (a 4-byte element array);
`CObList`/`CPtrList` likewise; the map families likewise.  So a byte-signature
matcher (our FID, `config/library_labels.csv`) CANNOT tell them apart: every row
in those bands comes back `AMBIG`, and the tree has been *trusting* those rows -
which means it has been binding calls to the wrong NAFXCW symbol.  objdiff masks
relocations, so a wrong class costs ~0% and displays "100.00%" while being a link
defect and a silent wrong-symbol binding.

But the binary NAMES THE CLASSES ITSELF, and it does so unambiguously:

    IMPLEMENT_DYNAMIC/DYNCREATE/SERIAL emits, for class C,
      * a `CRuntimeClass` object  `classC`   whose FIRST field is `LPCSTR
        m_lpszClassName` -> a plain C string with the class's name, and
      * `CRuntimeClass* C::GetRuntimeClass() const { return &classC; }`, which
        /O2 lowers to exactly six bytes:   B8 <&classC> C3   (mov eax,imm32; ret)
    and `GetRuntimeClass()` is CObject's FIRST virtual, so it is **vtable slot 0**
    of every CObject-derived class.

So:  vtable -> slot0 -> `mov eax,imm32; ret` -> CRuntimeClass -> [0] -> "CObArray".
The class states its own name.  Nothing is inferred, nothing is fabricated.

Recovery chain (all four steps read GRUNTZ.EXE and nothing else)
---------------------------------------------------------------
  1. RUNTIME CLASSES  scan .text for the 6-byte `B8 imm32 C3` shape; keep the ones
                      whose imm32 points at a CRuntimeClass (field 0 -> a C
                      identifier string).                       -> 65 in GRUNTZ.EXE
  2. VTABLES          any data DWORD equal to a GetRuntimeClass body address is a
                      vtable's slot 0.                          -> vtable -> class
  3. ANCHORS          a .text function whose body contains a DIR32 base-reloc to a
                      class's vtable or CRuntimeClass IS that class's code (the
                      ctor/dtor stamp the vptr; Serialize/CreateObject take the
                      CRuntimeClass).  DIRECT, per-function evidence.
  4. BANDS            NAFXCW gives each container its own .obj (array_o.obj,
                      array_p.obj, list_o.obj, ...), and the linker lays an .obj's
                      code out contiguously - so a class's non-anchor methods
                      (SetSize/GetAt/InsertAt...) sit BETWEEN its anchors.  A band
                      runs from a class's first anchor to the next anchor belonging
                      to a DIFFERENT class.  Contiguity inference, one step weaker
                      than an anchor - always reported as BAND, never as DIRECT.

Self-validation: run `--map` and check it against everything independently known.
It reproduces the RTTI-less MFC vtable catalog in config/library_vtables.csv exactly
(CDWordArray@0x1ec29c, CPtrArray@0x1ec2dc, CByteArray@0x1ed28c, CObArray@0x1ed494,
CObList@0x1ed4b4, CPtrList@0x1eb054, CMapPtrToPtr@0x1ed264, ...), and it names, from
the binary alone, every band the FID could only call AMBIG.

CAVEAT (read before quoting a result): slot 0 gives the RUNTIME class, i.e. the
nearest DECLARE_DYNAMIC ancestor.  A game class derived from CDialog *without*
DECLARE_DYNAMIC inherits `CDialog::GetRuntimeClass`, so its vtable reports
"CDialog".  `--map` marks those (several vtables sharing one runtime class).  For
the MFC containers - all DECLARE_SERIAL - the mapping is exact and 1:1, which is
the case this tool exists for.

Usage (inside `nix develop`)
----------------------------
    python -m gruntz.audit.mfc_class 0x1b4b43        # who owns this RVA?
    python -m gruntz.audit.mfc_class 0x1ec29c        # ... a vtable
    python -m gruntz.audit.mfc_class 0x1b4b43 0x1b55e9 ...
    python -m gruntz.audit.mfc_class --map           # the whole recovered map
    python -m gruntz.audit.mfc_class --audit         # audit the TREE's MFC uses
    python -m gruntz.audit.mfc_class --labels        # AMBIG library_labels rows
                                                        # the technique disambiguates
    python -m gruntz.audit.mfc_class --relabel [--write]
                                                        # rewrite those rows (stage 5
                                                        # of gruntz.audit.fid_generate)

--audit is the point of the tool: for every function we have reconstructed, it pairs
our base COMDAT's relocation records against the retail bytes at the same offsets and
asks "the symbol we emit says CObList - does the address retail actually calls live in
the CObList band?".  A NO is a wrong-class binding that objdiff cannot see.
"""
import argparse
import bisect
import csv
import glob
import json
import os
import re
import struct
import sys
import tomllib
from collections import defaultdict
from pathlib import Path

IMAGEBASE = 0x400000


# Resolve REPO from the CWD first: in a worktree the shell's PYTHONPATH can point at
# MAIN's scripts/, so __file__ would mis-resolve to main ([[worktree-pythonpath-leaks-to-main]]).
def _find_repo():
    for base in (Path.cwd(), Path(__file__).resolve().parent):
        for p in (base, *base.parents):
            if (p / "flake.nix").exists():
                return p
    return Path(__file__).resolve().parents[3]


REPO = _find_repo()


def _exe_path():
    for c in (os.environ.get("GRUNTZ_EXE"), REPO / "build/exe/GRUNTZ.EXE"):
        if c and Path(c).is_file():
            return Path(c)
    sys.exit("mfc_class: no GRUNTZ.EXE ($GRUNTZ_EXE / build/exe/GRUNTZ.EXE)")


# ---------------------------------------------------------------- PE image


class Image:
    """The retail PE: section map, RVA reads, and the .reloc DIR32 site list."""

    def __init__(self, path):
        self.d = Path(path).read_bytes()
        d = self.d
        e = struct.unpack_from("<I", d, 0x3C)[0]
        nsec = struct.unpack_from("<H", d, e + 6)[0]
        optsz = struct.unpack_from("<H", d, e + 20)[0]
        opt = e + 24
        self.secs = []
        for i in range(nsec):
            b = opt + optsz + i * 40
            nm = d[b:b + 8].rstrip(b"\0").decode("latin1")
            vsz, va, rsz, rp = struct.unpack_from("<IIII", d, b + 8)
            ch = struct.unpack_from("<I", d, b + 36)[0]
            self.secs.append((nm, va, max(vsz, rsz), rsz, rp, ch))
        self.text = next(s for s in self.secs if s[0] == ".text")
        self.reloc_sites = self._reloc_sites()

    def off(self, rva):
        for (nm, va, vsz, rsz, rp, ch) in self.secs:
            if va <= rva < va + vsz:
                o = rva - va + rp
                return o if o < rp + rsz else None
        return None

    def u32(self, rva):
        o = self.off(rva)
        return struct.unpack_from("<I", self.d, o)[0] if o is not None else None

    def i32(self, rva):
        o = self.off(rva)
        return struct.unpack_from("<i", self.d, o)[0] if o is not None else None

    def cstr(self, rva, n=128):
        o = self.off(rva)
        if o is None:
            return None
        end = self.d.find(b"\0", o, o + n)
        return self.d[o:end].decode("latin1") if end >= 0 else None

    def section_of(self, rva):
        for (nm, va, vsz, rsz, rp, ch) in self.secs:
            if va <= rva < va + vsz:
                return nm
        return None

    def is_text(self, rva):
        _, va, vsz, _, _, _ = self.text
        return va <= rva < va + vsz

    def _reloc_sites(self):
        """Every DIR32 fixup site (RVA), sorted. An absolute address in code or data
        requires a base-reloc, so this is the complete, authoritative pointer list."""
        rel = next((s for s in self.secs if s[0] == ".reloc"), None)
        if not rel:
            return []
        out = []
        p, end = rel[4], rel[4] + rel[3]
        while p < end - 8:
            pg, sz = struct.unpack_from("<II", self.d, p)
            if sz < 8:
                break
            for k in range(p + 8, min(p + sz, end), 2):
                w = struct.unpack_from("<H", self.d, k)[0]
                if (w >> 12) == 3:
                    out.append(pg + (w & 0xFFF))
            p += sz
        out.sort()
        return out


# ---------------------------------------------------------- the recovery


class MfcMap:
    """The recovered MFC identity map (see the module docstring for the chain)."""

    def __init__(self, img):
        self.img = img
        self.grc = {}          # GetRuntimeClass body rva -> (CRuntimeClass rva, name)
        self.rtclass = {}      # CRuntimeClass rva        -> name
        self.crt_of = {}       # name                     -> CRuntimeClass rva
        self.base_of = {}      # name                     -> base class name (m_pBaseClass)
        self.vtables = {}      # vtable rva               -> runtime-class name
        self.vt_of = {}        # name                     -> [vtable rva]
        self.anchors = {}      # function rva             -> {(kind, name, target)}
        self.bands = []        # sorted [(lo, hi, name)]
        self._recover()

    # -- step 1: the CRuntimeClass objects, via their GetRuntimeClass bodies
    def _recover_runtime_classes(self):
        img = self.img
        _, tlo, tvsz, trsz, trp, _ = img.text
        raw = img.d[trp:trp + trsz]
        i = 0
        while True:
            i = raw.find(b"\xb8", i)
            if i < 0 or i + 6 > len(raw):
                break
            if raw[i + 5] == 0xC3:                     # mov eax,imm32 ; ret
                imm = struct.unpack_from("<I", raw, i + 1)[0]
                crt = imm - IMAGEBASE
                p = img.u32(crt)                       # CRuntimeClass.m_lpszClassName
                if p and IMAGEBASE <= p < IMAGEBASE + 0x400000:
                    nm = img.cstr(p - IMAGEBASE)
                    # An MFC class name: a C identifier, and MFC's are all C-prefixed.
                    if nm and nm[:1] == "C" and nm.isidentifier():
                        # corroborate: CRuntimeClass.m_nObjectSize is a sane size
                        size = img.u32(crt + 4)
                        if size is not None and 0 < size < 0x10000:
                            self.grc[tlo + i] = (crt, nm)
                            self.rtclass[crt] = nm
                            self.crt_of[nm] = crt
            i += 1
        # CRuntimeClass.m_pBaseClass (+0x10 in the static-MFC layout: name, objsize,
        # schema, pfnCreateObject, pBaseClass, pNextClass) -> the whole MFC hierarchy,
        # straight out of the binary.
        for crt, nm in self.rtclass.items():
            b = img.u32(crt + 0x10)
            if b:
                self.base_of[nm] = self.rtclass.get(b - IMAGEBASE)

    def ancestors(self, name):
        out, seen = set(), name
        while True:
            b = self.base_of.get(seen)
            if not b or b in out:
                return out
            out.add(b)
            seen = b

    # -- step 2: the vtables (slot 0 == the class's GetRuntimeClass)
    def _recover_vtables(self):
        img = self.img
        for (nm, va, vsz, rsz, rp, ch) in img.secs:
            if ch & 0x20000000 or nm not in (".rdata", ".data"):
                continue
            blob = img.d[rp:rp + rsz]
            for k in range(0, len(blob) - 4, 4):
                w = struct.unpack_from("<I", blob, k)[0]
                t = w - IMAGEBASE
                if t in self.grc:
                    self.vtables[va + k] = self.grc[t][1]
        for v, n in self.vtables.items():
            self.vt_of.setdefault(n, []).append(v)
        for n in self.vt_of:
            self.vt_of[n].sort()

    # -- step 3: the anchor functions (DIR32 to a vtable / CRuntimeClass of a class)
    def _recover_anchors(self, funcs):
        img = self.img
        sites = img.reloc_sites
        for (fs, fsz) in funcs:
            if not img.is_text(fs) or fsz <= 0:
                continue
            lo = bisect.bisect_left(sites, fs)
            hi = bisect.bisect_left(sites, fs + fsz)
            found = set()
            for s in sites[lo:hi]:
                t = img.u32(s)
                if t is None:
                    continue
                t -= IMAGEBASE
                if t in self.vtables:
                    found.add(("vtable", self.vtables[t], t))
                if t in self.rtclass:
                    found.add(("CRuntimeClass", self.rtclass[t], t))
            if found:
                self.anchors[fs] = found

    # -- step 4: the bands (one .obj per container => contiguous code)
    def _recover_bands(self):
        """A band is a class's own .obj's code.

        Two soundness rules, both needed - without them the bands are garbage:

        * ONLY a class with a UNIQUE vtable may anchor a band.  A derived class that
          skips DECLARE_DYNAMIC shares its base's GetRuntimeClass, so its vtable maps
          to the BASE's name (48 vtables report "CObject").  Any game ctor then looks
          like a "CObject anchor" and glues half the .text into one bogus CObject band.
          One-vtable-per-runtime-class == an identity we can actually stand behind.
        * The MOST-DERIVED reference wins.  A ctor inlines its base ctor, so it stamps
          the base's vptr too (CPen::CPen DIR32s both ??_7CGdiObject@@6B@ and
          ??_7CPen@@6B@).  m_pBaseClass gives us the real hierarchy, so we drop any
          referenced class that is an ANCESTOR of another referenced class.

        The band's END is the end of the CONTIGUOUS carved-function chain from the
        anchor - not blindly "the next class's anchor", which over-runs by kilobytes
        for the last class in an .obj run (CStringArray, CMapPtrToPtr).  A small hole
        (<= MAX_HOLE) between the chain's end and the next class's anchor is a start
        Ghidra failed to carve inside the same .obj run, so the band is extended to it.
        """
        MAX_HOLE = 0x200
        PAD = 0x10                       # alignment padding between COMDATs
        unique = {n for n, vs in self.vt_of.items() if len(vs) == 1}
        claim = {}
        for fs, refs in self.anchors.items():
            names = {n for (_k, n, _t) in refs if n in unique}
            names -= {a for n in names for a in self.ancestors(n)}
            if len(names) == 1:
                claim[fs] = next(iter(names))

        funcs = load_function_starts()
        fstarts = [f[0] for f in funcs]

        def chain_end(start, hard_cap):
            """End of the run of adjacent carved functions starting at `start`."""
            i = bisect.bisect_left(fstarts, start)
            end = start
            while i < len(funcs):
                s, sz = funcs[i]
                if s >= hard_cap or s > end + PAD:
                    break
                end = max(end, s + sz)
                i += 1
            return end

        pts = sorted(claim.items())
        bands = []
        for i, (lo, cls) in enumerate(pts):
            nxt = next((a for a, c in pts[i + 1:] if c != cls),
                       self.img.text[1] + self.img.text[2])
            ce = chain_end(lo, nxt)
            hi = nxt if (nxt - ce) <= MAX_HOLE else ce
            bands.append((lo, hi, cls))
        # merge the consecutive anchors of one class into a single band
        merged = []
        for lo, hi, cls in bands:
            if merged and merged[-1][2] == cls and lo <= merged[-1][1] + PAD:
                merged[-1] = (merged[-1][0], max(merged[-1][1], hi), cls)
            else:
                merged.append((lo, hi, cls))
        self.bands = merged
        self._band_lo = [b[0] for b in merged]

    def _recover(self):
        self._recover_runtime_classes()
        self._recover_vtables()
        self._recover_anchors(load_function_starts())
        self._recover_bands()

    # ---- queries

    def band_of(self, rva):
        i = bisect.bisect_right(self._band_lo, rva) - 1
        if i < 0:
            return None
        lo, hi, cls = self.bands[i]
        return (lo, hi, cls) if lo <= rva < hi else None

    def classify(self, rva):
        """(class_or_None, [evidence lines]) for an arbitrary RVA."""
        ev = []
        img = self.img

        # a vtable?
        if rva in self.vtables:
            cls = self.vtables[rva]
            s0 = (img.u32(rva) or 0) - IMAGEBASE
            crt, _ = self.grc[s0]
            ev.append("vtable 0x%06x slot0 -> 0x%06x  `mov eax,0x%08x; ret`"
                      % (rva, s0, crt + IMAGEBASE))
            ev.append("CRuntimeClass 0x%06x  .m_lpszClassName -> \"%s\"  .m_nObjectSize=0x%x"
                      % (crt, cls, img.u32(crt + 4)))
            return cls, ev

        # a CRuntimeClass object?
        if rva in self.rtclass:
            cls = self.rtclass[rva]
            ev.append("CRuntimeClass 0x%06x  .m_lpszClassName -> \"%s\"" % (rva, cls))
            return cls, ev

        # a GetRuntimeClass body?
        if rva in self.grc:
            crt, cls = self.grc[rva]
            ev.append("`mov eax,0x%08x; ret` == %s::GetRuntimeClass" % (crt + IMAGEBASE, cls))
            ev.append("CRuntimeClass 0x%06x -> \"%s\"" % (crt, cls))
            return cls, ev

        # an anchor function - DIRECT evidence
        fn = enclosing_function(rva)
        if fn and fn[0] in self.anchors:
            refs = self.anchors[fn[0]]
            names = {n for (_k, n, _t) in refs}
            if len(names) > 1:
                names.discard("CObject")
            for (k, n, t) in sorted(refs, key=lambda r: r[2]):
                ev.append("fn 0x%06x DIR32 -> 0x%06x  (%s of %s)" % (fn[0], t, k, n))
            if len(names) == 1:
                cls = next(iter(names))
                ev.append("=> DIRECT: the function's own body references only %s's "
                          "class objects" % cls)
                return cls, ev

        # band inference
        b = self.band_of(rva)
        if b:
            lo, hi, cls = b
            ev.append("band [0x%06x, 0x%06x) = %s  (its .obj's contiguous code; the "
                      "band runs from %s's first anchor to the next class's)"
                      % (lo, hi, cls, cls))
            if fn:
                ev.append("enclosing fn 0x%06x (+0x%x)" % (fn[0], rva - fn[0]))
            ev.append("=> BAND: contiguity inference, not a per-function anchor")
            return cls, ev

        ev.append("no MFC runtime-class anchor and no band contains 0x%06x" % rva)
        return None, ev


# ------------------------------------------------------- tree-side inputs


_FUNCS = None


def load_function_starts():
    """[(rva, size)] from the Ghidra export + our own symbol_names (which knows
    starts Ghidra missed)."""
    global _FUNCS
    if _FUNCS is not None:
        return _FUNCS
    out = {}
    p = REPO / "build/ghidra-enrich/exports/functions.csv"
    if p.is_file():
        for r in csv.DictReader(open(p)):
            try:
                out[int(r["entry_rva"], 16)] = int(r["byte_size"])
            except (KeyError, ValueError):
                pass
    p = REPO / "build/gen/symbol_names.csv"
    if p.is_file():
        for r in csv.DictReader(open(p)):
            if r.get("kind") == "func" and r.get("size"):
                out.setdefault(int(r["rva"], 16), int(r["size"], 16))
    _FUNCS = sorted(out.items())
    return _FUNCS


def enclosing_function(rva):
    fs = load_function_starts()
    keys = [f[0] for f in fs]
    i = bisect.bisect_right(keys, rva) - 1
    if i < 0:
        return None
    start, size = fs[i]
    return (start, size) if start <= rva < start + size else None


# The MFC container families whose bodies the linker folds - the reason this tool
# exists. FID can only ever say AMBIG inside these.
FOLD_PRONE = {
    "CObArray", "CPtrArray", "CDWordArray", "CByteArray", "CWordArray",
    "CUIntArray", "CStringArray",
    "CObList", "CPtrList", "CStringList",
    "CMapStringToOb", "CMapStringToPtr", "CMapPtrToPtr", "CMapStringToString",
    "CMapWordToOb", "CMapWordToPtr", "CMapPtrToWord",
}

_MANGLED_CLASS = [
    re.compile(r"^\?\?[0-9]([A-Za-z_]\w*)@@"),        # ??0C@@ ctor / ??1C@@ dtor
    re.compile(r"^\?\?_[A-Z]([A-Za-z_]\w*)@@"),       # ??_GC@@ scalar-deleting dtor
    re.compile(r"^\?\?_7([A-Za-z_]\w*)@@"),           # ??_7C@@6B@ vtable
    re.compile(r"^\?[A-Za-z_]\w*@([A-Za-z_]\w*)@@"),  # ?Meth@C@@ method
]


def mangled_class(sym):
    for rx in _MANGLED_CLASS:
        m = rx.match(sym)
        if m:
            return m.group(1)
    return None


# ------------------------------------------------------------ COFF (base objs)


class Coff:
    """Just enough COFF to pair our base COMDATs' relocs with the retail bytes."""

    def __init__(self, path):
        self.path = Path(path)
        self.d = self.path.read_bytes()
        d = self.d
        nsec = struct.unpack_from("<H", d, 2)[0]
        symp = struct.unpack_from("<I", d, 8)[0]
        nsym = struct.unpack_from("<I", d, 12)[0]
        strp = symp + nsym * 18
        self.sections = []
        for i in range(nsec):
            b = 20 + i * 40
            nm = d[b:b + 8].rstrip(b"\0").decode("latin1")
            _vs, _va, sz, ptr, rptr, _lp, nrel = struct.unpack_from("<IIIIIIH", d, b + 8)
            self.sections.append({"name": nm, "size": sz, "ptr": ptr,
                                  "rptr": rptr, "nrel": nrel})

        def strat(o):
            e = d.find(b"\0", strp + o)
            return d[strp + o:e].decode("latin1")

        self.symbols = []
        i = 0
        while i < nsym:
            b = symp + i * 18
            raw = d[b:b + 8]
            if raw[:4] == b"\0\0\0\0":
                nm = strat(struct.unpack_from("<I", raw, 4)[0])
            else:
                nm = raw.rstrip(b"\0").decode("latin1")
            val = struct.unpack_from("<I", d, b + 8)[0]
            snum = struct.unpack_from("<h", d, b + 12)[0]
            naux = d[b + 17]
            self.symbols.append({"name": nm, "value": val, "sec": snum})
            i += 1 + naux
            for _ in range(naux):
                self.symbols.append(None)

    def relocs(self, si):
        s = self.sections[si]
        out = []
        for k in range(s["nrel"]):
            b = s["rptr"] + k * 10
            va, sym, typ = struct.unpack_from("<IIH", self.d, b)
            out.append((va, sym, typ))
        return out

    def comdat_functions(self):
        """[(section_index, function_symbol_name)] for the code COMDATs."""
        out = []
        for i, s in enumerate(self.sections):
            if not s["name"].startswith(".text"):
                continue
            best = None
            for sym in self.symbols:
                if sym and sym["sec"] == i + 1 and sym["value"] == 0 and sym["name"].startswith("?"):
                    best = sym["name"]
                    break
            if best:
                out.append((i, best))
        return out


# ------------------------------------------------------------------ audit


def load_src_bindings():
    """mangled name -> (rva, size, unit) from build/gen/symbol_names.csv."""
    out = {}
    p = REPO / "build/gen/symbol_names.csv"
    if not p.is_file():
        sys.exit("mfc_class: no build/gen/symbol_names.csv - run `gruntz build --fast` first")
    for r in csv.DictReader(open(p)):
        out[r["name"]] = (int(r["rva"], 16),
                          int(r["size"], 16) if r.get("size") else 0,
                          r.get("unit", ""))
    return out


REL32 = 0x0014          # IMAGE_REL_I386_REL32
DIR32 = 0x0006          # IMAGE_REL_I386_DIR32


def load_exact_names():
    """The functions objdiff scores 100% fuzzy, from the FRESH report.json.

    The offset-paired test is only valid on these: when our bytes match retail's,
    a relocation record at COMDAT offset `o` sits over the SAME instruction as
    retail's byte at rva+o, so `rva + o + 4 + disp32` really is the call target.
    Below 100% the bytes have desynced and that arithmetic reads noise - so an
    inexact function gets the SET test instead, which needs no byte alignment."""
    p = REPO / "build/objdiff/report.json"
    if not p.is_file():
        sys.exit("mfc_class: no build/objdiff/report.json - run `gruntz build --fast`")
    rep = json.load(open(p))
    out = set()
    for u in rep.get("units", []):
        for f in u.get("functions", []):
            if f.get("fuzzy_match_percent", 0) >= 100.0 and f.get("name", "").startswith("?"):
                out.add(f["name"])
    return out


def audit(mm, args):
    """Compare, per reconstructed function, the MFC class our source NAMES at each
    call site with the class of the address retail ACTUALLY calls there.

    Our base COMDAT emits `call ??0CObList@@QAE@XZ` as a REL32 reloc at offset o.
    Retail's function has, at the same offset (the bytes match up to the masked
    fixup), a rel32 whose target = rva + o + 4 + disp.  Which band that target lands
    in is the class retail really uses.  Disagreement = a wrong-symbol binding that
    objdiff cannot see."""
    img = mm.img
    binds = load_src_bindings()
    exact = load_exact_names()
    # LIVE units only: build/objdiff/base/ keeps the .obj of a unit that has since
    # been renamed/removed from units.toml, and a stale COMDAT reports long-fixed
    # defects forever (ddrawsubmgrleafscan.obj / playdtor.obj did exactly that).
    live = {u["unit"] for u in tomllib.load(open(REPO / "config/units.toml", "rb"))["unit"]}
    objs = sorted(o for o in glob.glob(str(REPO / "build/objdiff/base/*.obj"))
                  if Path(o).stem in live)
    if not objs:
        sys.exit("mfc_class: no build/objdiff/base/*.obj - run `gruntz build --fast` first")

    verdicts = []               # (unit, fn, off, our_sym, our_cls, tgt, real_cls, ok)
    per_fn = {}                 # fn -> [ours, retail, unit]

    for o in objs:
        unit = Path(o).stem
        try:
            c = Coff(o)
        except Exception:
            continue
        symtab = c.symbols
        for si, fname in c.comdat_functions():
            if fname not in binds:
                continue
            frva, fsize, _u = binds[fname]
            if not fsize:
                continue
            sec = c.sections[si]
            for (va, symi, typ) in c.relocs(si):
                if symi >= len(symtab) or symtab[symi] is None:
                    continue
                sym = symtab[symi]["name"]
                cls = mangled_class(sym)
                if cls not in FOLD_PRONE:
                    continue
                per_fn.setdefault(fname, [set(), set(), unit])[0].add(cls)
                # offset-paired verdict: ONLY where our bytes are exact (see
                # load_exact_names) - otherwise the offsets have desynced.
                if fname not in exact or typ != REL32:
                    continue
                if va + 4 > min(sec["size"], fsize):
                    continue
                disp = img.i32(frva + va)
                if disp is None:
                    continue
                tgt = frva + va + 4 + disp
                if not img.is_text(tgt):
                    continue
                real = mm.band_of(tgt)
                realcls = real[2] if real else None
                verdicts.append((unit, fname, va, sym, cls, tgt, realcls, realcls == cls))

    # The SET test: which container BANDS does the retail function actually call
    # into?  Offset-independent, so it works below 100% too - and it is what catches
    # a whole class being wrong (we call CObList; retail only ever enters CPtrList).
    for fname, row in per_fn.items():
        frva, fsize, _u = binds[fname]
        row[1] |= retail_container_classes(mm, frva, fsize)

    bad = [v for v in verdicts if not v[7]]
    print("=== mfc_class --audit ===")
    print()
    print("A. OFFSET-PAIRED (byte-exact functions only: our reloc record at COMDAT")
    print("   offset o vs retail's rel32 at rva+o -> the address retail really calls)")
    print("   sites checked : %d" % len(verdicts))
    print("   AGREE         : %d" % (len(verdicts) - len(bad)))
    print("   WRONG CLASS   : %d   <- wrong-symbol bindings objdiff cannot see" % len(bad))
    print()
    for (unit, fn, off, sym, cls, tgt, realcls, _ok) in sorted(bad):
        print("     [%s] %s +0x%x" % (unit, fn, off))
        print("         we emit  %s  (source says %s)" % (sym, cls))
        print("         retail   calls 0x%06x = %s" % (tgt, realcls or "(not an MFC band)"))
    print()
    print("B. CLASS-SET (every reconstructed fn, exact or not: the SET of container")
    print("   bands retail's body calls into vs the SET our source names)")
    print()
    rows = []
    for fname in sorted(per_fn):
        ours, retail, unit = per_fn[fname]
        if not ours and not retail:
            continue
        if ours == retail and not args.all:
            continue
        rows.append((unit, fname, ours, retail))
    for unit, fname, ours, retail in sorted(rows):
        missing = ours - retail
        extra = retail - ours
        # retail==empty usually just means the retail body inlines nothing we can
        # see (an empty/thin fn, or the calls are in a sibling); flag only the
        # substantive disagreements.
        tag = "WRONG-CLASS" if (missing and retail) else ("unconfirmed" if not retail
                                                          else "partial")
        print("   [%-11s] %-8s %s" % (tag, unit, fname))
        print("        ours   : %s" % (", ".join(sorted(ours)) or "-"))
        print("        retail : %s" % (", ".join(sorted(retail)) or "-"))
        if missing and retail:
            print("        >>> we name %s; retail never enters that band (it enters %s)"
                  % (", ".join(sorted(missing)), ", ".join(sorted(retail))))
    n_wrong = sum(1 for _u, _f, o, r in rows if (o - r) and r)
    print()
    print("   functions whose class SET disagrees with retail : %d" % n_wrong)
    return 1 if (bad or n_wrong) else 0


def retail_container_classes(mm, rva, size):
    """The set of fold-prone MFC classes the retail function at [rva,rva+size) calls
    into: decode every E8/E9 rel32 in the body and band-classify the target."""
    img = mm.img
    out = set()
    o = img.off(rva)
    if o is None or size <= 0:
        return out
    body = img.d[o:o + size]
    for i in range(len(body) - 4):
        if body[i] in (0xE8, 0xE9):
            disp = struct.unpack_from("<i", body, i + 1)[0]
            tgt = rva + i + 5 + disp
            b = mm.band_of(tgt)
            if b and b[2] in FOLD_PRONE:
                out.add(b[2])
    return out


# ------------------------------------------------------------- FID labels


def label_rows():
    p = REPO / "config/library_labels.csv"
    rows = list(csv.reader(open(p)))
    return rows[0], rows[1:]


# Non-container symbols that legitimately live INSIDE a container's .obj: the CPlex
# block allocator every collection shares, and the ConstructElements/DestructElements/
# CopyElements element helpers the array/list templates instantiate per element type.
# They are correctly named already - the band owns the .obj, not every symbol in it.
def _band_neutral(name, cls):
    return cls in (None, "CPlex") or name.startswith(("?ConstructElements@", "?DestructElements@",
                                                      "?CopyElements@", "?SerializeElements@"))


def _rowkind(mm, name, cls, real):
    """What KIND of wrong is this label row? (drives both --labels and --relabel)"""
    if cls == real:
        return "ok"
    if _band_neutral(name, cls):
        return "ok"                       # a shared helper in the class's .obj
    if cls in FOLD_PRONE:
        return "fold"                     # FID picked a byte-identical sibling container
    if cls in mm.rtclass.values() or cls in ("CInternetSession",):
        return "fid-noise"                # matched an unrelated library body of the same shape
    return "fabricated"                   # a GAME class name aliased onto a library body


def relabel(mm, write):
    """Resolve the NAFXCW rows the CRuntimeClass bands can name.

    Three kinds of wrong row live in the container bands, and FID can never fix any
    of them on its own (the four array classes / the two lists / the map families are
    byte-identical, so every signature match there is AMBIG):

      fold        the row names a byte-identical SIBLING container (??0CByteArray at
                  0x1b4b43, which is really CDWordArray's ctor).  Rewrite the class:
                  the fold guarantees the member SHAPE is right - only the class was
                  undecidable - so the mangled member/signature carries over.
      fid-noise   the row names an unrelated library class whose body happens to have
                  the same shape (??1CInternetSession for a container dtor, ??_GCWinApp
                  for a container's scalar-deleting dtor).  The dtor manglings carry no
                  other class token, so the class swap is exact.
      fabricated  the row names a GAME class (?SetSize@CStateStackZ, ?AddTail@GzObList).
                  NAFXCW defines no such symbol - these were hand-added to paper over a
                  fake view's unresolved externs.  The view is gone; DROP the row.

    Rewritten rows are marked `CRUNTIME` so they are never again mistaken for a
    byte-signature claim.
    """
    hdr, rows = label_rows()
    out, fixed, dropped = [], [], []
    for r in rows:
        if len(r) < 5 or not r[0].startswith("0x"):
            out.append(r)
            continue
        rva = int(r[0], 16)
        name, lib, conf, src = r[1], r[2], r[3], r[4]
        b = mm.band_of(rva)
        if lib != "NAFXCW" or not b or b[2] not in FOLD_PRONE:
            out.append(r)
            continue
        real = b[2]
        cls = mangled_class(name)
        kind = _rowkind(mm, name, cls, real)
        if kind == "ok":
            out.append([r[0], name, lib, "CRUNTIME" if cls == real else conf, src])
            continue
        if kind == "fabricated":
            dropped.append((rva, name))
            continue
        new = name.replace("@%s@@" % cls, "@%s@@" % real, 1)
        if new == name:
            # ctor/dtor/vtable manglings put the class right after the ??N tag
            new = re.sub(r"^(\?\?(?:[0-9]|_[A-Z]|_7))%s@@" % re.escape(cls),
                         r"\g<1>%s@@" % real, name)
        if new == name:
            out.append(r)                 # can't re-spell it; leave + report
            continue
        fixed.append((rva, name, new, real, kind))
        out.append([r[0], new, lib, "CRUNTIME", src])

    print("=== mfc_class --relabel: NAFXCW rows the CRuntimeClass bands re-class ===")
    print("rewritten: %d   dropped (fabricated game-class aliases): %d"
          % (len(fixed), len(dropped)))
    print()
    for rva, old, new, real, kind in fixed:
        print("  %-10s 0x%06x  %-44s -> %s" % (kind, rva, old, new))
    for rva, old in dropped:
        print("  %-10s 0x%06x  %-44s -> (dropped: NAFXCW defines no such symbol)"
              % ("fabricated", rva, old))
    if write:
        p = REPO / "config/library_labels.csv"
        with open(p, "w", newline="") as f:
            w = csv.writer(f, lineterminator="\n")
            w.writerow(hdr)
            w.writerows(out)
        print("\nwrote %s" % p)
    else:
        print("\n(dry run - pass --write to rewrite config/library_labels.csv)")
    return 0


def show_labels(mm):
    """Every library_labels row inside a recovered MFC container band, with the
    class the binary says owns it."""
    _hdr, rows = label_rows()
    print("=== library_labels.csv rows inside a recovered MFC container band ===")
    print("%-10s %-8s %-48s %-16s %s" % ("rva", "conf", "label", "band", "verdict"))
    n = ok = 0
    for r in rows:
        if len(r) < 5 or not r[0].startswith("0x"):
            continue
        rva = int(r[0], 16)
        b = mm.band_of(rva)
        if not b or b[2] not in FOLD_PRONE or r[2] != "NAFXCW":
            continue
        cls = mangled_class(r[1])
        kind = _rowkind(mm, r[1], cls, b[2])
        ok += (kind == "ok")
        n += 1
        print("%-10s %-8s %-48s %-16s %s" % (r[0], r[3], r[1], b[2],
                                             "" if kind == "ok" else "<- " + kind.upper()))
    print("\n%d NAFXCW rows in container bands; %d correct, %d wrong" % (n, ok, n - ok))
    return 0


# -------------------------------------------------------------------- map


def show_map(mm):
    img = mm.img
    print("=== 1. CRuntimeClass objects (the class names, straight out of the binary) ===")
    for crt in sorted(mm.rtclass):
        nm = mm.rtclass[crt]
        print("  classC @0x%06x  %-24s objsize=0x%-4x schema=%-6d base=%s"
              % (crt, '"%s"' % nm, img.u32(crt + 4) or 0, img.u32(crt + 8) or 0,
                 mm.base_of.get(nm) or "-"))
    print("\n=== 2. vtables (slot0 -> GetRuntimeClass -> CRuntimeClass) ===")
    for n in sorted(mm.vt_of):
        vs = mm.vt_of[n]
        tag = "" if len(vs) == 1 else ("   <- %d vtables share this runtime class "
                                       "(derived classes without DECLARE_DYNAMIC; "
                                       "cannot anchor a band)" % len(vs))
        print("  %-24s %s%s" % (n, " ".join("0x%06x" % v for v in vs[:6]), tag))
    print("\n=== 3. code bands (one .obj per class => contiguous code) ===")
    print("  THE FOLD-PRONE CONTAINERS - the ones FID can ONLY ever call AMBIG:")
    for lo, hi, cls in mm.bands:
        if cls in FOLD_PRONE:
            print("    %-18s [0x%06x, 0x%06x)  %5d B   ctor 0x%06x  vtbl 0x%06x"
                  % (cls, lo, hi, hi - lo, lo, mm.vt_of[cls][0]))
    print("\n  (the rest - not fold-prone, FID names these fine)")
    for lo, hi, cls in mm.bands:
        if cls not in FOLD_PRONE:
            print("    %-24s [0x%06x, 0x%06x)  %5d B" % (cls, lo, hi, hi - lo))
    return 0


# -------------------------------------------------------------------- cli


def main():
    ap = argparse.ArgumentParser(
        prog="python -m gruntz.audit.mfc_class",
        description="which MFC class owns this RVA? (asks the binary via CRuntimeClass)")
    ap.add_argument("rva", nargs="*", help="RVA(s): a function, a vtable, a call target")
    ap.add_argument("--map", action="store_true", help="dump the whole recovered map")
    ap.add_argument("--audit", action="store_true",
                    help="audit the tree's MFC container bindings against retail")
    ap.add_argument("--labels", action="store_true",
                    help="library_labels.csv rows in MFC container bands, right vs wrong")
    ap.add_argument("--relabel", action="store_true",
                    help="rewrite the wrong ones (--write to commit them)")
    ap.add_argument("--write", action="store_true", help="with --relabel: write the CSV")
    ap.add_argument("--all", action="store_true", help="with --audit: show agreeing fns too")
    a = ap.parse_args()

    mm = MfcMap(Image(_exe_path()))

    if a.map:
        return show_map(mm)
    if a.labels:
        return show_labels(mm)
    if a.relabel:
        return relabel(mm, a.write)
    if a.audit:
        return audit(mm, a)
    if not a.rva:
        ap.print_help()
        return 2

    rc = 0
    for s in a.rva:
        rva = int(s, 16)
        if rva >= IMAGEBASE:
            rva -= IMAGEBASE
        cls, ev = mm.classify(rva)
        print("RVA 0x%06x  (%s)" % (rva, mm.img.section_of(rva) or "?"))
        for line in ev:
            print("    %s" % line)
        if cls:
            print("  => %s%s" % (cls, "   [fold-prone: FID can only say AMBIG here]"
                                 if cls in FOLD_PRONE else ""))
        else:
            print("  => UNKNOWN - not MFC, or not covered by a recovered band")
            rc = 1
        print()
    return rc


if __name__ == "__main__":
    sys.exit(main())
