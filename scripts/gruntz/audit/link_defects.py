#!/usr/bin/env python3
"""link_defects.py - the defect shapes that break a LINK, hunted tree-wide.

objdiff scores a relocation site by MASKING it: a reference bound to a fake symbol,
or a symbol defined twice with different bytes, still scores 100%. All of these are
invisible to every match metric we have - and all make the reconstruction unlinkable.
Retail LINKED, so none of them can exist in the real source.

  (1) UNRESOLVED  a symbol some obj REFERENCES that NO obj defines and NO shippable
                  .LIB can supply -> `unresolved external symbol` at link. Split by
                  what the REMEDY is, because the three halves need opposite work:

      PHANTOM         a FUNCTION whose owning class does not exist in retail: it owns no
                      retail address at all, OR the binary's own RTTI REFUTES it (see
                      fabricated_classes()). The class is a fabricated per-TU view, so the
                      member can never resolve. Fix = recover the real class.

      UNDEFINED DATA  a VARIABLE (not a function) that nothing defines. This is a
                      DIFFERENT defect with a DIFFERENT fix, and it used to be
                      silently filed under "backlog" - which was WRONG, and hid ~450
                      guaranteed link failures behind a bucket labelled "resolves for
                      free as bodies land". It does NOT: a data symbol is never
                      produced by reconstructing a function. Somebody has to WRITE
                      THE DEFINITION. Typical cause: the tree declares a global in N
                      TUs (`DATA(0x..) extern i32 g_x;`) and defines it in none - or
                      defines it once under a DIFFERENT name/linkage, so the C++-
                      mangled alias resolves to nothing (?g_pathStepSeed@@3HA vs the
                      extern-"C" _g_645584 that actually has storage).
                      Fix = one real definition in the xref-proven owner TU.

      BACKLOG         a function of a class retail plausibly HAS, simply not reconstructed
                      yet. Every unreconstructed engine callee is deliberately declared-only
                      so its rel32 reloc-masks; it resolves the day someone writes the body.

                      Read this bucket as UNPROVEN, never as "fine". NOT ONE symbol in it is
                      bound to a retail address (if it were, it would be reconstructed, and
                      hence defined), so "it will resolve" is always an inference from its
                      CLASS, never from the symbol. That inference used to be pure tree SELF-
                      CERTIFICATION - "some sibling member of this class has an RVA claim" -
                      and fabricated classes walked straight through it: CSbConfigItem
                      laundered TWENTY guaranteed link failures in here on the strength of
                      two invented-name RVA rows. fabricated_classes() now catches the ones
                      the BINARY can refute. That is a FLOOR, not a ceiling - more are still
                      hiding in here, and this bucket is where you look for them.

  (2) MULTIPLY-DEFINED
                  one name DEFINED, in a NON-COMDAT section, by more than one obj ->
                  `LNK2005: already defined`. The link FAILS. Full stop - whether or
                  not the bytes agree.

                  This bucket exists because DIVERGENT (below) structurally COULD NOT
                  SEE it: DIVERGENT requires >1 distinct byte SHAPE, so N *identical*
                  definitions of the same global sailed straight through. g_coordPool
                  was defined SIX times (GameText, Grunt, GruntCombat, GruntSteps,
                  GruntEntranceMove, GruntEntranceArrival) - six .bss objects for one
                  global - and nothing in the tree counted it. A whole class of link
                  failure was uncounted.

                  COMDATs are EXCLUDED, and that exclusion is what makes the bucket
                  usable: C++ deliberately emits inline/member functions, vtables,
                  template instantiations and string literals as COMDATs from every TU
                  that uses them, and the linker keeps exactly one. Flagging those
                  would bury the real defects under thousands of legal duplicates.
                  A COMMON (tentative def, sec==0/val==size) is likewise merged by the
                  linker, so it is not a definition either. See Coff.is_comdat().

                  Fix = ONE definition in the xref-proven owner TU; every other TU
                  externs it (and drops its DATA()/RVA() pin with the declaration).

  (3) DIVERGENT   one mangled name DEFINED in >1 obj with DIFFERENT bytes -> an ODR
                  landmine. MSVC keeps ONE COMDAT copy per name, chosen arbitrarily
                  (first-seen), so the linker may pick a 5-byte `jmp <view-dtor>`
                  stub OVER the real teardown body. Typical cause: a class whose dtor
                  is left IMPLICIT in a view header, so cl5 synthesises a per-TU inline
                  copy under the same name as the real out-of-line body.
                  (N objs emitting the SAME bytes is a normal COMDAT - not a defect.)

Ground truth, not heuristics: "can the linker resolve it" is answered by reading the
symbol tables of the real toolchain archives ($MSVC_DIR/lib/*.LIB + $DXSDK_DIR/Lib),
so CRT/MFC/Win32-import symbols are never mistaken for defects. "Is it data or a
function" is answered by the MSVC mangling grammar, and for extern-"C" names (which
carry no type at all) by which SECTION the retail address lands in - NOT by
symbol_names.csv's `kind` column, which records the MACRO used, not the symbol's
nature. See is_data_symbol().

Usage:
  python -m gruntz.audit.link_defects            # headline + all inventories
  python -m gruntz.audit.link_defects --csv      # machine-readable worklist
  python -m gruntz.audit.link_defects --data     # ONLY the undefined-data worklist
  python -m gruntz.audit.link_defects --unit sbi_image
"""
import argparse
import glob
import hashlib
import os
import re
import struct
import subprocess
import sys
from collections import defaultdict
from pathlib import Path


def _find_repo():
    # CWD first: in a worktree the shell PYTHONPATH can point at MAIN's scripts/
    # ([[worktree-pythonpath-leaks-to-main]]), so __file__ would mis-resolve to main.
    for base in (Path.cwd(), Path(__file__).resolve().parent):
        for p in (base, *base.parents):
            if (p / "flake.nix").exists() and (p / "build" / "objdiff").exists():
                return p
    return next(p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists())


REPO = _find_repo()

# Symbols the CRT/EH machinery or the compiler itself emits per-TU; they are either
# supplied by a lib or are local scaffolding, never a reconstruction defect.
_EXEMPT_PREFIX = (
    "__imp_", "$L", "$S", "$T", "??_C@", "__real@", "__xmm@",
    "__except_list", "__load_config", "__ehhandler", "?__L",
)


class Coff:
    """Minimal COFF reader: defined/undefined symbols + per-section content shape."""

    def __init__(self, path):
        b = self.b = open(path, "rb").read()
        self.path = path
        self.nsec = struct.unpack_from("<H", b, 2)[0]
        self.symptr = struct.unpack_from("<I", b, 8)[0]
        self.nsym = struct.unpack_from("<I", b, 12)[0]
        opt = struct.unpack_from("<H", b, 16)[0]
        self.strtab = self.symptr + self.nsym * 18
        self.secs = []
        self.comdat = set()  # 1-based section numbers that are COMDATs
        for i in range(self.nsec):
            o = 20 + opt + i * 40
            rawsize, rawptr = struct.unpack_from("<II", b, o + 16)
            relptr = struct.unpack_from("<I", b, o + 24)[0]
            nrel = struct.unpack_from("<H", b, o + 32)[0]
            chars = struct.unpack_from("<I", b, o + 36)[0]
            if chars & 0x00001000:  # IMAGE_SCN_LNK_COMDAT
                self.comdat.add(i + 1)
            self.secs.append((relptr, nrel, rawptr, rawsize))

    def name(self, idx):
        base = self.symptr + idx * 18
        if struct.unpack_from("<I", self.b, base)[0] == 0:
            off = struct.unpack_from("<I", self.b, base + 4)[0]
            e = self.b.index(b"\0", self.strtab + off)
            return self.b[self.strtab + off:e].decode("latin1")
        return self.b[base:base + 8].split(b"\0")[0].decode("latin1")

    def _relocs(self, sec):
        relptr, nrel = self.secs[sec - 1][:2]
        out = []
        for r in range(nrel):
            ro = relptr + r * 10
            vaddr, symidx, typ = struct.unpack_from("<IIH", self.b, ro)
            out.append((vaddr, self.name(symidx), typ))
        return sorted(out)

    def shape(self, sec):
        """A link-relevant fingerprint of a section: its raw bytes plus every reloc as
        (offset, TARGET NAME, type). Two objs emitting the SAME COMDAT hash the same; a
        5-byte `jmp view_dtor` stub vs the real body does not.

        Compiler-LOCAL reloc targets are normalised away first. cl5 numbers its per-TU
        scaffolding ($L EH scope tables, $T/$S temporaries, ??_C@ string constants)
        sequentially per TU, so the SAME inline body emitted in two TUs legitimately
        references $L18650 in one and $L20123 in the other. Hashing those raw names
        would report every ordinary multi-TU COMDAT as divergent (they are not - the
        linker keeps one copy plus its own scaffolding). Only EXTERNAL targets - the
        real callees/vtables that decide what the code DOES - are identity-bearing."""
        _rp, _nr, rawptr, rawsize = self.secs[sec - 1]
        raw = self.b[rawptr:rawptr + rawsize] if rawptr else b""
        h = hashlib.md5()
        h.update(raw)
        for vaddr, nm, typ in self._relocs(sec):
            if nm.startswith(("$L", "$T", "$S", "??_C@", "__real@", "__xmm@")):
                nm = "<local>"
            h.update(b"|%d|%s|%d" % (vaddr, nm.encode("latin1"), typ))
        return h.hexdigest(), rawsize

    def symbols(self):
        """(defined {name: section}, undefined {names}). scl==2 is EXTERNAL; section 0
        with a nonzero value is a COMMON, with zero value an undefined import.

        A COMMON (sec==0, val==size) is a TENTATIVE definition: the linker merges any
        number of them into one, so it is neither a definition to report nor an
        unresolved reference. It is deliberately in neither set."""
        defined, undef = {}, set()
        i = 0
        while i < self.nsym:
            base = self.symptr + i * 18
            val, sec, _t, scl, naux = struct.unpack_from("<IhHBB", self.b, base + 8)
            nm = self.name(i)
            if scl == 2:
                if sec >= 1:
                    defined.setdefault(nm, sec)
                elif sec == 0 and val == 0:
                    undef.add(nm)
            i += 1 + naux
        return defined, undef

    def is_comdat(self, sec):
        """Is this section a COMDAT? A COMDAT symbol may legally be emitted by MANY objs
        (inline/member fns, vtables, template instantiations, string literals): the linker
        keeps ONE and discards the rest. Duplicates of a NON-COMDAT symbol are LNK2005."""
        return sec in self.comdat


def current_unit_objs():
    """Only objs of CURRENT units - build/objdiff/base/ accumulates stale orphan objs
    from retired/renamed units, and a stale COMDAT would fake a divergence."""
    try:
        import tomllib
        units = {u["unit"] for u in
                 tomllib.loads((REPO / "config/units.toml").read_text()).get("unit", [])}
    except Exception:
        units = None
    out = []
    for p in sorted(glob.glob(str(REPO / "build/objdiff/base/*.obj"))):
        stem = os.path.basename(p)[:-4]
        if units is None or stem in units:
            out.append((stem, p))
    return out


def lib_symbols():
    """Every symbol the real toolchain archives can supply (CRT, MFC, Win32 imports,
    DirectX). Read straight out of the .LIB symbol tables - the linker's own answer to
    'can this resolve', so no CRT/MFC symbol is ever miscalled a defect."""
    cache = REPO / "build" / "gen" / "lib_symbols.txt"
    if cache.is_file():
        return set(cache.read_text(errors="ignore").split("\n"))
    libs = []
    for env in ("MSVC_DIR", "DXSDK_DIR"):
        d = os.environ.get(env)
        if d:
            for sub in ("lib", "Lib"):
                libs += glob.glob(os.path.join(d, sub, "*"))
    libs = [p for p in libs if p.lower().endswith(".lib")]
    syms = set()
    for i in range(0, len(libs), 24):                       # batch: llvm-nm is slow to spawn
        try:
            out = subprocess.run(["llvm-nm", "--defined-only", "--no-sort", *libs[i:i + 24]],
                                 capture_output=True, text=True, timeout=600).stdout
        except Exception:
            continue
        for line in out.split("\n"):
            f = line.split(" ", 2)
            if len(f) == 3 and f[1] not in ("", "U"):
                syms.add(f[2].strip())
            elif len(f) == 2 and f[0] in ("T", "D", "B", "R", "t"):
                syms.add(f[1].strip())
    cache.parent.mkdir(parents=True, exist_ok=True)
    cache.write_text("\n".join(sorted(syms)))
    return syms


def is_exempt(sym, libs):
    return sym.startswith(_EXEMPT_PREFIX) or sym in libs or sym.lstrip("_") in libs


_CLASS_RE = re.compile(r"^\?\?[01][A-Za-z_]\w*@@|^\?\w+@(\w+)@@")


def owning_class(sym):
    """The class a mangled member symbol belongs to (best effort, for grouping)."""
    m = re.match(r"^\?\?[01]([A-Za-z_]\w*)@@", sym)        # ctor/dtor: ??0Foo@@ / ??1Foo@@
    if m:
        return m.group(1)
    m = re.match(r"^\?[\w@]+?@([A-Za-z_]\w*)@@", sym)      # method:   ?Meth@Foo@@
    if m:
        return m.group(1)
    m = re.match(r"^\?\?_[7GE]([A-Za-z_]\w*)@@", sym)      # vtable / deleting dtors
    if m:
        return m.group(1)
    return ""


# The retail .text span (same section map assert_relocs uses). An address inside it is
# CODE; anything above is .rdata/.data/.bss.
_TEXT = (0x1000, 0x1E626B)


def symbol_rvas():
    """{symbol -> retail rva} from symbol_names.csv.

    NOTE the `kind` column is deliberately NOT used to decide data-vs-function. It records
    which MACRO the source used (DATA() vs RVA()), not what the symbol IS - and the tree
    binds some FUNCTIONS with DATA() at their ILT-thunk address, so it reports
    `??0CGruntzMgrOptions@@QAE@XZ ... ,data` for a constructor. Trusting it put ctors,
    dtors and plain methods in the undefined-DATA bucket. The RVA cannot lie the same way:
    which SECTION the address lands in settles it."""
    rvas = {}
    p = REPO / "build/gen/symbol_names.csv"
    if p.is_file():
        import csv as _csv
        for r in _csv.DictReader(open(p, encoding="latin-1")):
            try:
                rvas[r["name"]] = int(r["rva"], 16)
            except (ValueError, KeyError, TypeError):
                pass
    return rvas


def _mangled_is_data(sym):
    """True iff an MSVC-mangled name denotes a VARIABLE rather than a function.

    The grammar: `?<name>@<scope>@...@@<code>...`. The qualified name ends at the FIRST
    `@@`; the character straight after it is the storage/type code, and it is a DIGIT for
    data (`3` plain global, `2`/`1`/`0` public/protected/private static member, ...) and a
    LETTER for a function (`Q`/`A`/`I`/`U`/`E`/`Y`/... = access + calling convention).

        ?g_wildcard@@3PBDB          -> '3' -> data (const char* g_wildcard)
        ?s_tbl@CFoo@@2PAHA          -> '2' -> data (static member)
        ?Meth@CFoo@@QAEXXZ          -> 'Q' -> function

    Anchoring on the FIRST `@@` is load-bearing: a naive `.*@@[0-8]` search matches an
    INNER `@@0`, because MSVC back-references a repeated argument type as a digit -
    `?BigDraw@ObjSink2e4@@QAEHHHHHHUVec4@@00000HHHH@Z` (the `@@00000` is five repeats of
    `UVec4`), which mis-filed 53 plain functions as data.

    `??`-prefixed names (ctors/dtors ??0/??1, vftables ??_7, string literals ??_C@) are
    deliberately NOT data here: the compiler emits ??_7/??_8/??_R itself into the obj that
    needs them, and a truly unresolved one is a CLASS-identity defect (PHANTOM), not a
    missing variable definition."""
    if not sym.startswith("?") or sym.startswith("??"):
        return False
    i = sym.find("@@")
    return i >= 0 and i + 2 < len(sym) and sym[i + 2].isdigit()


def is_data_symbol(sym, rvas):
    """Is this undefined symbol a VARIABLE (needs a definition) or a function?

    Two authorities, in priority order, neither of which is a guess:

      1. the MSVC MANGLING GRAMMAR - decisive for every C++-linkage name (`?`-prefixed),
         and it OVERRIDES everything else: if the grammar says `?Reset@CMapLogic@@QAEXXZ`
         is a method, it is a method no matter how the tree annotated it.

      2. for an extern-"C" name (`_g_645584`) the name carries no type at all, so fall
         back to the RETAIL ADDRESS: a symbol bound into .text is code, anything else is
         data. That is read from the binary's section map, not from an annotation.

    A symbol neither can classify (extern-"C", no retail rva) stays OUT of this bucket:
    an honest gap beats a guess, and over-counting here would be as dishonest as the
    under-counting this bucket exists to fix."""
    if sym.startswith("?"):
        return _mangled_is_data(sym)
    r = rvas.get(sym)
    if r is None:
        return False
    return not (_TEXT[0] <= r < _TEXT[1])


def real_classes():
    """Classes PROVEN to exist in retail: they own at least one symbol bound to a real
    retail RVA (symbol_names.csv) or a catalogued vtable. Used to split the unresolved
    set in two, because those two halves need opposite responses:

      BACKLOG  the class is real, this member just is not reconstructed yet. Every
               unreconstructed engine callee is deliberately declared-only so its
               rel32 reloc-masks - the campaign's standard technique. It resolves for
               free the day someone reconstructs it. NOT a modelling defect.

      PHANTOM  the owning class owns NO retail address anywhere. The class itself is a
               fabricated per-TU VIEW, so this member can NEVER resolve - retail has no
               such function to bind it to. THIS is the real link-blocker (the
               ??1CSbConfigItem shape): it must be fixed by modelling the real class,
               not by reconstructing anything.

    A class counts as REAL only if at least one of ITS OWN METHODS is bound to a retail
    rva in symbol_names.csv.

    A VTBL() binding deliberately does NOT count, and this used to be a hole that
    UNDERCOUNTED the metric. VTBL() only says the class's VTABLE has a retail address -
    it says nothing about whether any given declared-only METHOD NAME corresponds to a
    real retail function. A hand-rolled stand-in for a library class is exactly that
    case: <Gruntz/Wnd.h>'s CWnd carries VTBL(CWnd, 0x1eb5c4) (the real MFC vtable IS at
    that address) while every one of its ~26 methods is a fabricated WndVslN placeholder
    that no obj and no .LIB defines and that NOTHING can ever define, because the real
    body in NAFXCW.LIB is exported under a different mangled name. Those are phantoms in
    the strictest sense, and the VTBL() was laundering them into the "backlog" bucket."""
    real = set()
    p = REPO / "build/gen/symbol_names.csv"
    if p.is_file():
        import csv as _csv
        for r in _csv.DictReader(open(p)):
            c = owning_class(r["name"])
            if c:
                real.add(c)
    return real - fabricated_classes()


def retail_rtti_classes():
    """Every class name retail's RTTI type descriptors (`??_R0`) attest to, read straight
    out of the EXE: the `.?AVFoo@@` / `.?AUFoo@@` decorated-name field of each descriptor.

    This is BINARY ground truth, not a tree claim - but read what it does and does NOT
    prove. Retail is five separate projects (C:\\Proj\\{DDrawMgr,DinMgr2,Dsndmgr,NetMgr,
    Gruntz}) and /GR was on for only SOME of them, so:

      PRESENT  -> the class exists in retail.                       (proof)
      ABSENT   -> it may still be a real class compiled WITHOUT /GR (the whole engine side:
                  CGameLevel, CDDSurface, CStatusBarMgr, CRezFile, the faders, the sound
                  and net classes ... all real, all descriptor-less). ABSENCE PROVES
                  NOTHING ON ITS OWN and must never be used as a fabrication test - doing
                  so mis-flags ~145 real classes."""
    cache = REPO / "build" / "gen" / "rtti_classes.txt"
    if cache.is_file():
        return set(cache.read_text().split())
    exe = os.environ.get("GRUNTZ_EXE", "")
    if not exe or not os.path.isfile(exe):
        return set()
    blob = open(exe, "rb").read()
    names = {m.group(1).decode("latin1")
             for m in re.finditer(rb"\.\?A[VU]([A-Za-z_]\w*)@@", blob)}
    cache.parent.mkdir(parents=True, exist_ok=True)
    cache.write_text("\n".join(sorted(names)))
    return names


_DERIVES_RE = re.compile(
    r"^\s*(?:class|struct)\s+([A-Za-z_]\w*)\s*:\s*"
    r"(?:public\s+|protected\s+|private\s+|virtual\s+)*([A-Za-z_][\w:]*)\s*([<{,])?")


def declared_bases():
    """{base -> {(derived, file)}} for every `class D : public B` our own source declares.
    Template bases are skipped (a `?$`-mangled descriptor is not in the plain-name set, so
    they cannot be attested and would false-positive)."""
    out = defaultdict(set)
    for pat in ("include/**/*.h", "src/**/*.h", "src/**/*.cpp"):
        for p in REPO.glob(pat):
            if "vendor" in p.parts:
                continue
            for line in p.read_text(errors="ignore").split("\n"):
                m = _DERIVES_RE.match(line)
                if m and m.group(3) != "<":                 # skip template bases
                    out[m.group(2).split("::")[-1]].add(
                        (m.group(1), str(p.relative_to(REPO))))
    return out


def fabricated_classes():
    """Classes the BINARY ITSELF REFUTES - proven not to exist in retail, so every member
    of them is a permanent `unresolved external symbol`.

    The proof is a sound consequence of how MSVC lays out RTTI: a /GR class's
    RTTIClassHierarchyDescriptor carries a base-class array naming EVERY base in its
    hierarchy, and each entry points at that base's OWN `??_R0` type descriptor. So a base
    of an attested class is necessarily attested too - and the module-scoped /GR that makes
    bare absence useless (see retail_rtti_classes) does NOT weaken this: if the DERIVED
    class got a descriptor, its whole base chain got one, engine-side or not.

        D is RTTI-attested  AND  our source says `class D : public B`  AND  B has no
        descriptor   =>   B is not D's retail base   =>   B DOES NOT EXIST.

    This is the hole that let ~30 guaranteed link failures hide in the "backlog" bucket.
    The old real_classes() test was `the class owns >=1 symbol_names row`, which is TREE
    SELF-CERTIFICATION: the tree minted the name AND the RVA claim. CSbConfigItem - a class
    with no descriptor, refuted as the base of five attested CSBI_* classes whose real root
    RTTI names as CStatusBarItem - had exactly TWO members pinned to real retail addresses
    under invented names, and those two rows laundered its other twenty (a ctor, thirteen
    virtuals, ...) into "resolves for free as those bodies land". They cannot: nobody will
    ever write CSbConfigItem::Configure, because retail has no CSbConfigItem. Same shape:
    CGruntMovingBase (RTTI: CGrunt's real base is CMovingLogic).

    NOTE this is a floor, not a ceiling. It catches a fabricated class only when an
    RTTI-attested class DERIVES from it. A fabricated class nobody inherits from - or one
    under a /GR-less engine hierarchy - is still invisible here, so the true PHANTOM count
    is >= what this reports. An honest floor beats a false zero."""
    rtti = retail_rtti_classes()
    if not rtti:
        return set()
    libs = {owning_class(s) for s in lib_symbols()}
    libs.discard("")
    bad = set()
    for base, derived in declared_bases().items():
        if base in rtti or base in libs:
            continue
        if any(d in rtti for d, _f in derived):
            bad.add(base)
    return bad


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--csv", action="store_true")
    ap.add_argument("--unit", help="restrict the report to defects touching this unit")
    ap.add_argument("--phantom-only", action="store_true",
                    help="only unresolved symbols of classes with NO retail address "
                         "(the can-never-link set); hides the reconstruct-me backlog")
    ap.add_argument("--data", action="store_true",
                    help="ONLY the undefined-DATA worklist (referenced variables that "
                         "nothing defines); each needs a definition in its owner TU")
    args = ap.parse_args()

    libs = lib_symbols()
    objs = current_unit_objs()
    real = real_classes()
    rvas = symbol_rvas()

    defined = defaultdict(list)     # name -> [(unit, shape_hash, size)]
    referenced = defaultdict(set)   # name -> {units}
    hard_defs = defaultdict(list)   # name -> [unit] for NON-COMDAT definitions only
    for unit, path in objs:
        c = Coff(path)
        d, u = c.symbols()
        for nm, sec in d.items():
            h, sz = c.shape(sec)
            defined[nm].append((unit, h, sz))
            if not c.is_comdat(sec):
                hard_defs[nm].append(unit)
        for nm in u:
            referenced[nm].add(unit)

    # (1) UNRESOLVED: referenced, defined by no obj, and no .LIB can supply it.
    # Three-way split by REMEDY: UNDEFINED DATA (write the definition) / PHANTOM (recover
    # the class) / BACKLOG (reconstruct the body). DATA is tested FIRST: a variable needs a
    # definition no matter whose class it hangs off, and filing it under either of the
    # other two buckets misdescribes the work (that is exactly how ~450 of them hid).
    unresolved, backlog, undef_data = [], [], []
    for nm, units in sorted(referenced.items()):
        if nm in defined or is_exempt(nm, libs):
            continue
        if is_data_symbol(nm, rvas):
            undef_data.append((nm, sorted(units)))
            continue
        cls = owning_class(nm)
        (unresolved if (cls and cls not in real) else backlog).append((nm, sorted(units)))

    # (2b) MULTIPLY-DEFINED: one name, defined NON-COMDAT by >1 obj. This is LNK2005, full
    # stop - whether or not the bytes agree. It is NOT the DIVERGENT bucket: that one needs
    # >1 distinct byte SHAPE, so N identical definitions of the same global slipped straight
    # through it (g_coordPool was defined SIX times - six .bss objects for one global - and
    # nothing counted it). A COMDAT may legally be emitted by many objs (inline/member fns,
    # vtables, templates, literals): the linker keeps one. Only NON-COMDAT dupes are errors.
    multi_def = []
    for nm, units in sorted(hard_defs.items()):
        if len(units) > 1:
            multi_def.append((nm, sorted(units)))

    # (2) DIVERGENT: one name, >1 obj, >1 distinct byte shape.
    divergent = []
    for nm, defs in sorted(defined.items()):
        shapes = {h for _u, h, _s in defs}
        if len(defs) > 1 and len(shapes) > 1:
            by_shape = defaultdict(list)
            sizes = {}
            for u, h, s in defs:
                by_shape[h].append(u)
                sizes[h] = s
            variants = sorted(by_shape.items(), key=lambda kv: sizes[kv[0]])
            divergent.append((nm, [(sizes[h], sorted(us)) for h, us in variants]))

    if args.unit:
        unresolved = [r for r in unresolved if args.unit in r[1]]
        backlog = [r for r in backlog if args.unit in r[1]]
        undef_data = [r for r in undef_data if args.unit in r[1]]
        multi_def = [m for m in multi_def if args.unit in m[1]]
        divergent = [d for d in divergent
                     if any(args.unit in us for _sz, us in d[1])]

    # the retail rva each undefined global is annotated at (DATA()/@data-symbol), when known
    rva_of = {}
    p = REPO / "build/gen/symbol_names.csv"
    if p.is_file():
        import csv as _csv
        for r in _csv.DictReader(open(p, encoding="latin-1")):
            rva_of[r["name"]] = r.get("rva") or r.get("address") or ""

    if args.data:
        print("UNDEFINED DATA - %d referenced variable(s) that NO obj defines and no .LIB\n"
              "supplies. Each needs ONE real definition in its xref-proven owner TU.\n"
              % len(undef_data))
        for nm, units in undef_data:
            print("  %-46s %-12s <- %s"
                  % (nm[:46], rva_of.get(nm, ""), ",".join(units)[:60]))
        return 1 if undef_data else 0

    if args.csv:
        w = sys.stdout
        w.write("kind,symbol,class,detail\n")
        for nm, units in undef_data:
            w.write('UNDEFINED_DATA,%s,%s,"rva %s; referenced by: %s"\n'
                    % (nm, owning_class(nm), rva_of.get(nm, "?"), " ".join(units)))
        for nm, units in unresolved:
            w.write('UNRESOLVED,%s,%s,"referenced by: %s"\n'
                    % (nm, owning_class(nm), " ".join(units)))
        for nm, units in multi_def:
            w.write('MULTIPLY_DEFINED,%s,%s,"defined non-COMDAT in: %s"\n'
                    % (nm, owning_class(nm), " ".join(units)))
        for nm, variants in divergent:
            det = " | ".join("%dB in %s" % (sz, ",".join(us)) for sz, us in variants)
            w.write('DIVERGENT,%s,%s,"%s"\n' % (nm, owning_class(nm), det))
        return 1 if (unresolved or divergent or undef_data or multi_def) else 0

    print("link-defect scan over %d current-unit objs (%d lib symbols resolvable)\n"
          % (len(objs), len(libs)))

    print("=" * 78)
    print("(1) PHANTOM EXTERNALS - referenced; owning class does not exist in retail")
    print("    -> can NEVER link: retail has no such function. Fix = model the real")
    print("       class (these are fabricated per-TU views). %d symbol(s)." % len(unresolved))
    print("=" * 78)
    fab = fabricated_classes()
    if fab:
        bases = declared_bases()
        print("  [RTTI-REFUTED] %d class(es) the BINARY disproves: an RTTI-attested class"
              % len(fab))
        print("  derives from them, so retail would have emitted a ??_R0 for them - it did")
        print("  not. RVA claims by the tree do NOT make these real (that self-certification")
        print("  is what hid them in `backlog`):")
        for c in sorted(fab):
            ds = sorted(d for d, _f in bases.get(c, ()) if d in retail_rtti_classes())
            print("      %-24s refuted by attested derived: %s" % (c, ", ".join(ds)))
        print()
    by_cls = defaultdict(list)
    for nm, units in unresolved:
        by_cls[owning_class(nm) or "(free)"].append((nm, units))
    for cls, rows in sorted(by_cls.items(), key=lambda kv: -len(kv[1])):
        print("  %s  (%d)" % (cls, len(rows)))
        for nm, units in rows:
            print("      %-52s <- %s" % (nm, ",".join(units)))

    print()
    print("=" * 78)
    print("(2) UNDEFINED DATA - referenced VARIABLES that no obj defines and no .LIB")
    print("    supplies. A data symbol is NEVER produced by reconstructing a function,")
    print("    so these do NOT 'resolve for free' - each needs ONE real definition in")
    print("    its owner TU. %d symbol(s)." % len(undef_data))
    print("=" * 78)
    if not args.phantom_only:
        for nm, units in undef_data[:40]:
            print("  %-46s %-12s <- %s"
                  % (nm[:46], rva_of.get(nm, ""), ",".join(units)[:40]))
        if len(undef_data) > 40:
            print("  ... %d more (--data for the full worklist, --csv for machine-readable)"
                  % (len(undef_data) - 40))

    if not args.phantom_only:
        print()
        print("  [backlog] %d further unresolved symbol(s) are functions of classes retail"
              % len(backlog))
        print("            PLAUSIBLY has, not reconstructed yet (declared-only so their rel32")
        print("            reloc-masks). UNPROVEN, not fine: none is bound to a retail address,")
        print("            so this is an inference from the CLASS. It is where fabricated")
        print("            classes hide - the 3 RTTI-refuted ones above were in here.")

    print()
    print("=" * 78)
    print("(3) MULTIPLY-DEFINED - one name, defined NON-COMDAT by >1 obj -> LNK2005")
    print("    -> the link FAILS: N .bss/.data objects for one global. Fix = ONE")
    print("       definition in the owner TU; every other TU externs it. %d symbol(s)."
          % len(multi_def))
    print("=" * 78)
    for nm, units in multi_def[:40]:
        print("  %-46s %-10s  defined in %d objs: %s"
              % (nm[:46], rva_of.get(nm, ""), len(units), ",".join(units)[:44]))
    if len(multi_def) > 40:
        print("  ... %d more (--csv for the full list)" % (len(multi_def) - 40))

    print()
    print("=" * 78)
    print("(4) DIVERGENT DUPLICATE COMDATs - one name, different bytes in >1 obj")
    print("    -> ODR landmine: the linker may pick the stub over the real body.")
    print("       %d symbol(s)." % len(divergent))
    print("=" * 78)
    for nm, variants in divergent:
        print("  %s" % nm)
        for sz, us in variants:
            print("      %5d B  %s" % (sz, ",".join(us)))

    # The headline: every bucket here is a defect that PREVENTS A SUCCESSFUL LINK.
    # UNDEFINED-DATA is counted HERE, not hidden in `backlog`: every one is an `unresolved
    # external symbol` today, exactly like a PHANTOM. MULTIPLY-DEFINED is counted here too -
    # it is an LNK2005 that the DIVERGENT bucket structurally could not see (that one needs
    # >1 byte SHAPE, so N *identical* definitions of one global were invisible). Both
    # additions make the number WORSE and TRUE; that is the point of the number.
    print("\nTOTAL: %d PHANTOM, %d UNDEFINED-DATA, %d MULTIPLY-DEFINED, %d DIVERGENT"
          "  (target: 0 / 0 / 0 / 0)"
          % (len(unresolved), len(undef_data), len(multi_def), len(divergent)))
    print("       defects that PREVENT A SUCCESSFUL LINK = %d  [+%d backlog fns, UNPROVEN]"
          % (len(unresolved) + len(undef_data) + len(multi_def), len(backlog)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
