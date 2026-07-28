#!/usr/bin/env python3
r"""objbind.py - lay out a compiled COFF object and bind its relocations to the
RESTORED GAME IMAGE, so the replay can run a function that references things.

    python recomp/replay/objbind.py ddrawshadeblit -o build/replay/mod.objmod
    python recomp/replay/objbind.py --list ddrawshadeblit      # what would bind

The replay used to LINK our object into replay.exe. A global the function
touches then resolves to replay.exe's own copy instead of the game's, and a call
to a sibling function resolves to a /FORCE:UNRESOLVED null - so only bodies with
ZERO relocations could be tested. That restriction, not the tier and not the
size of the state, was the ceiling.

Here every symbol the object names is looked up by its MANGLED name in
build/gen/symbol_names.csv (our own annotations - authoritative) and then in
config/library_labels.csv (FLIRT, HIGH confidence and unambiguous only), and the
reference is written to point at `image_base + rva`. The snapshot has already
restored those bytes, so our function reads the game's real globals and calls the
game's real code.

RETAIL WINS, ALWAYS. A call from our function to another of our functions binds
to retail's copy of the callee - a second unmatched body cannot contaminate the
verdict on the first. Only symbols with no retail identity fall through to our
loaded copy: intra-function jump-table labels (benign, and counted separately)
and `$SG` string literals (a caveat, and counted). A symbol that resolves to
neither is UNRESOLVED, and every function whose section contains one is marked
REFUSE rather than run - a function that jumps to a wrong address does not fail
honestly, it corrupts the comparison.

The output is a flat, already-relocated blob: replay.exe VirtualAllocs it at
`load_base` and memcpy's it. No loader logic runs in the replay process, which
matters because the replay's CRT is unusable for most of the sequence.
"""
import argparse
import csv
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = next(p for p in HERE.parents if (p / "flake.nix").exists())

OBJMOD_MAGIC = b"GRUNTZOM"
OBJMOD_VERSION = 1
HDR_FMT = "<8sIIIIIIIIIIII"
HDR_SIZE = struct.calcsize(HDR_FMT)
SYM_FMT = "<IIIIIIIII"
SYM_SIZE = struct.calcsize(SYM_FMT)

OBJSYM_FUNC = 0x0001
OBJSYM_OURS = 0x0002
OBJSYM_RETAIL = 0x0004
OBJSYM_REFUSE = 0x0008
OBJSYM_VOIDRET = 0x0010


def void_return(name):
    """True when the MANGLED name proves the function returns void.

    eax is part of the compared observable - it is the return value - but for a
    void function it is whatever the last instruction happened to leave there,
    and comparing it compares codegen. The mangled name settles it without a
    guess: after the qualifier `@@`, MSVC writes the function class, then (for a
    non-static member) a cv char, then the calling convention, then the RETURN
    TYPE, and `X` is void.

        ?ComputeCellFlags@CMapMgr@@QAE X HHH @Z    -> void __thiscall(int,int,int)
        ?Expand@CMapMgr@@QAE H PAU... @Z           -> int, so eax IS compared

    Under-claiming is the safe direction, so anything this cannot parse
    confidently returns False and eax stays in the comparison.
    """
    if not name.startswith("?") or not name.endswith("Z"):
        return False                    # _CDecl / C linkage: no type information
    # The qualifier `@@` also occurs INSIDE argument types (`PAUDSoundLink@@`),
    # so neither the first nor the last occurrence is reliably the right one.
    # Take the leftmost that parses as a whole signature: function class, then
    # the calling convention in the expected slot. Requiring the convention
    # char too is what rejects a `@@` that landed in the middle of a type.
    i = 0
    while True:
        i = name.find("@@", i + 1)
        if i < 0:
            return False
        p = name[i + 2:]
        if not p:
            return False
        cls = p[0]
        # Non-static members carry a cv char before the convention; static
        # members (C/D, K/L, S/T) and free functions (Y/Z) do not.
        if cls in "ABEFGHIJMNOPQRUVWX":
            k = 3
        elif cls in "CDKLSTYZ":
            k = 2
        else:
            continue
        if len(p) <= k or p[k - 1] not in "ACEGIKMO":  # __cdecl/__thiscall/...
            continue
        return p[k] == "X"

# The blob's home. Chosen against build/replay/census.txt: the highest recorded
# region a snapshot restores is the injected recorder at 0x50000000+0xa0000, so
# this is the first 16 MB granule clear of everything the restore touches. The
# arena takes the rest of the window (0x52000000 up to wine's DLLs at
# 0x79ea0000), which a 114 MB in-game snapshot needs nearly half of.
DEFAULT_LOAD_BASE = 0x51000000
DEFAULT_IMAGE_BASE = 0x00400000

IMAGE_SCN_CNT_CODE = 0x00000020
IMAGE_SCN_CNT_INITIALIZED_DATA = 0x00000040
IMAGE_SCN_CNT_UNINITIALIZED_DATA = 0x00000080
IMAGE_SCN_LNK_INFO = 0x00000200
IMAGE_SCN_LNK_REMOVE = 0x00000800

REL_DIR32 = 0x06
REL_DIR32NB = 0x07
REL_SECTION = 0x0A
REL_SECREL = 0x0B
REL_REL32 = 0x14

IMAGE_SYM_CLASS_EXTERNAL = 2
IMAGE_SYM_CLASS_STATIC = 3
IMAGE_SYM_CLASS_LABEL = 6

# Symbols that are ABSOLUTE in the libraries, so "bind to game_base + rva" is
# the wrong question for them: the value IS the answer.
#
# `__except_list` is LIBCMT's name for the TEB offset of the SEH chain, defined
# as IMAGE_SYM_ABSOLUTE 0. Every /GX prologue references it - the emitted bytes
# are `64 a1 <DIR32 __except_list>` = `mov eax, fs:[0]` - so leaving it
# unresolved refuses every function with a destructible stack local, which is a
# large slice of the game. Binding it to 0 reproduces retail's bytes exactly.
ABSOLUTE = {"__except_list": 0x00000000}


# --------------------------------------------------------------- COFF reading

class Obj(object):
    def __init__(self, path):
        self.path = Path(path)
        self.data = self.path.read_bytes()
        d = self.data
        nsec = struct.unpack_from("<H", d, 2)[0]
        self.symptr, self.nsym = struct.unpack_from("<II", d, 8)
        self.strtab = self.symptr + self.nsym * 18
        self.secs = []
        for s in range(nsec):
            h = 20 + s * 40
            name = d[h:h + 8].rstrip(b"\0").decode("latin1")
            (vsz, va, rawsz, rawptr, relptr, lineptr, nrel, nline,
             chars) = struct.unpack_from("<IIIIIIHHI", d, h + 8)
            self.secs.append(dict(idx=s + 1, name=name, size=rawsz, rawptr=rawptr,
                                  relptr=relptr, nrel=nrel, chars=chars, addr=0,
                                  keep=False, blob_off=0))
        self.syms = []
        i = 0
        while i < self.nsym:
            b = self.symptr + i * 18
            val, sec, typ, cls, naux = struct.unpack_from("<IhHBB", d, b + 8)
            self.syms.append(dict(i=i, name=self._name(i), val=val, sec=sec,
                                  typ=typ, cls=cls, naux=naux))
            i += 1 + naux
        self.byidx = {s["i"]: s for s in self.syms}

    def _name(self, idx):
        d = self.data
        b = self.symptr + idx * 18
        nm = d[b:b + 8]
        if nm[:4] == b"\0\0\0\0":
            off = struct.unpack_from("<I", nm, 4)[0]
            end = d.index(b"\0", self.strtab + off)
            return d[self.strtab + off:end].decode("latin1")
        return nm.rstrip(b"\0").decode("latin1")

    def relocs(self, sec):
        for r in range(sec["nrel"]):
            va, idx, typ = struct.unpack_from("<IIH", self.data, sec["relptr"] + r * 10)
            yield va, idx, typ


def sec_align(chars):
    a = (chars >> 20) & 0xF
    return max(1 << (a - 1), 16) if a else 16


def sec_wanted(sec):
    """Sections that become part of the loaded image."""
    if sec["chars"] & (IMAGE_SCN_LNK_INFO | IMAGE_SCN_LNK_REMOVE):
        return False
    if sec["name"].startswith(".debug") or sec["name"] == ".drectve":
        return False
    return bool(sec["chars"] & (IMAGE_SCN_CNT_CODE
                                | IMAGE_SCN_CNT_INITIALIZED_DATA
                                | IMAGE_SCN_CNT_UNINITIALIZED_DATA))


# ------------------------------------------------------------- the name -> rva

def load_rvamap(verbose=False):
    """mangled name -> rva. Our own annotations first; FLIRT labels only where
    they are HIGH confidence AND name a single address (`_memcpy` matches two
    bodies, so it names neither)."""
    m = {}
    p = REPO / "build" / "gen" / "symbol_names.csv"
    if not p.exists():
        raise SystemExit("objbind: no %s - run `gruntz build` first" % p)
    with p.open() as f:
        for row in csv.DictReader(f):
            m[row["name"]] = int(row["rva"], 16)
    ours = len(m)

    lib = {}
    p = REPO / "config" / "library_labels.csv"
    if p.exists():
        with p.open() as f:
            for row in csv.DictReader(f):
                if row.get("confidence") != "HIGH":
                    continue
                lib.setdefault(row["name"], set()).add(int(row["rva"], 16))
    added = 0
    for name, rvas in lib.items():
        if len(rvas) == 1 and name not in m:
            m[name] = next(iter(rvas))
            added += 1
    if verbose:
        print("[objbind] name map: %d from symbol_names.csv, %d from library_labels "
              "(HIGH + unambiguous), %d dropped as ambiguous"
              % (ours, added, sum(1 for r in lib.values() if len(r) > 1)))
    return m


# ------------------------------------------------------------------ the bind

class Module(object):
    def __init__(self, objs, rvamap, load_base, image_base):
        self.objs = objs
        self.rvamap = rvamap
        self.load_base = load_base
        self.image_base = image_base
        self.blob = bytearray()
        self.unresolved = {}   # name -> count
        self.foreign = {}      # name -> count
        self.symbols = []      # exported ObjModSym rows

    # -- layout ------------------------------------------------------------
    def layout(self):
        cur = 0
        for o in self.objs:
            for s in o.secs:
                if not sec_wanted(s):
                    continue
                al = sec_align(s["chars"])
                cur = (cur + al - 1) & ~(al - 1)
                s["keep"] = True
                s["blob_off"] = cur
                s["addr"] = self.load_base + cur
                cur += s["size"]
        self.blob = bytearray(cur)
        for o in self.objs:
            for s in o.secs:
                if s["keep"] and s["rawptr"] and not (
                        s["chars"] & IMAGE_SCN_CNT_UNINITIALIZED_DATA):
                    self.blob[s["blob_off"]:s["blob_off"] + s["size"]] = \
                        o.data[s["rawptr"]:s["rawptr"] + s["size"]]

    # -- symbol addresses --------------------------------------------------
    def addr_ours(self, o, sym):
        if sym["sec"] <= 0 or sym["sec"] > len(o.secs):
            return 0
        s = o.secs[sym["sec"] - 1]
        return (s["addr"] + sym["val"]) if s["keep"] else 0

    def addr_retail(self, name):
        rva = self.rvamap.get(name)
        return (self.image_base + rva) if rva is not None else 0

    # -- relocations -------------------------------------------------------
    def bind(self):
        for o in self.objs:
            for s in o.secs:
                if not s["keep"] or not s["nrel"]:
                    continue
                s["stats"] = self.bind_section(o, s)
        self.export_symbols()

    def bind_section(self, o, s):
        st = dict(n=0, retail=0, intra=0, foreign=0, unres=0, unres_names=[],
                  foreign_names=[])
        for va, idx, typ in o.relocs(s):
            st["n"] += 1
            sym = o.byidx.get(idx)
            if sym is None:
                st["unres"] += 1
                st["unres_names"].append("idx%d" % idx)
                continue
            name = sym["name"]
            if name in ABSOLUTE:
                self.patch(s, va, typ, ABSOLUTE[name], name, "absolute")
                st["retail"] += 1
                continue
            target = self.addr_retail(name)
            kind = "retail"
            if target:
                st["retail"] += 1
            else:
                target = self.addr_ours(o, sym)
                if not target:
                    st["unres"] += 1
                    st["unres_names"].append(name)
                    self.unresolved[name] = self.unresolved.get(name, 0) + 1
                    continue
                if sym["sec"] == s["idx"]:
                    st["intra"] += 1
                    kind = "intra"
                else:
                    st["foreign"] += 1
                    st["foreign_names"].append(name)
                    self.foreign[name] = self.foreign.get(name, 0) + 1
                    kind = "foreign"
            self.patch(s, va, typ, target, name, kind)
        return st

    def patch(self, s, va, typ, target, name, kind):
        off = s["blob_off"] + va
        if off + 4 > len(self.blob):
            raise SystemExit("objbind: relocation at %s+%#x is past the section"
                             % (s["name"], va))
        inline = struct.unpack_from("<I", self.blob, off)[0]
        if typ == REL_DIR32:
            val = (target + inline) & 0xFFFFFFFF
        elif typ == REL_REL32:
            val = (target + inline - (s["addr"] + va + 4)) & 0xFFFFFFFF
        elif typ == REL_DIR32NB:
            val = (target + inline - self.image_base) & 0xFFFFFFFF
        else:
            raise SystemExit("objbind: relocation type %#x (at %s+%#x, %s) is not "
                             "handled - it does not appear in MSVC5 /O2 code "
                             "sections, so seeing one means the section filter is "
                             "wrong" % (typ, s["name"], va, name))
        struct.pack_into("<I", self.blob, off, val)

    # -- exported symbol table ---------------------------------------------
    def export_symbols(self):
        for o in self.objs:
            for sym in o.syms:
                if sym["cls"] != IMAGE_SYM_CLASS_EXTERNAL:
                    continue
                ours = self.addr_ours(o, sym)
                retail = self.addr_retail(sym["name"])
                flags = 0
                st = None
                if ours:
                    flags |= OBJSYM_OURS
                    s = o.secs[sym["sec"] - 1]
                    if s["chars"] & IMAGE_SCN_CNT_CODE:
                        flags |= OBJSYM_FUNC
                    st = s.get("stats")
                if retail:
                    flags |= OBJSYM_RETAIL
                if st and st["unres"]:
                    flags |= OBJSYM_REFUSE
                if void_return(sym["name"]):
                    flags |= OBJSYM_VOIDRET
                self.symbols.append(dict(
                    name=sym["name"], ours=ours, retail=retail, flags=flags,
                    n=st["n"] if st else 0,
                    retail_n=st["retail"] if st else 0,
                    intra=st["intra"] if st else 0,
                    foreign=st["foreign"] if st else 0,
                    unres=st["unres"] if st else 0,
                    unres_names=st["unres_names"] if st else [],
                    foreign_names=st["foreign_names"] if st else []))

    # -- output ------------------------------------------------------------
    def write(self, path):
        strtab = bytearray(b"\0")
        offs = {}

        def intern(name):
            if name not in offs:
                offs[name] = len(strtab)
                strtab.extend(name.encode("latin1") + b"\0")
            return offs[name]

        rows = b""
        for s in self.symbols:
            rows += struct.pack(SYM_FMT, intern(s["name"]), s["ours"], s["retail"],
                                s["flags"], s["n"], s["retail_n"], s["intra"],
                                s["foreign"], s["unres"])
        blob_off = HDR_SIZE
        sym_off = blob_off + len(self.blob)
        sym_off = (sym_off + 15) & ~15
        str_off = sym_off + len(rows)
        total = str_off + len(strtab)
        hdr = struct.pack(HDR_FMT, OBJMOD_MAGIC, OBJMOD_VERSION, HDR_SIZE, SYM_SIZE,
                          self.load_base, self.image_base, blob_off, len(self.blob),
                          sym_off, len(self.symbols), str_off, len(strtab), total)
        out = bytearray(total)
        out[0:HDR_SIZE] = hdr
        out[blob_off:blob_off + len(self.blob)] = self.blob
        out[sym_off:sym_off + len(rows)] = rows
        out[str_off:str_off + len(strtab)] = strtab
        Path(path).parent.mkdir(parents=True, exist_ok=True)
        Path(path).write_bytes(bytes(out))
        return total


# ------------------------------------------------------------------- driver

def obj_path(unit):
    p = Path(unit)
    if p.exists():
        return p
    p = REPO / "build" / "objdiff" / "base" / (unit + ".obj")
    if not p.exists():
        raise SystemExit("objbind: no object for '%s' (%s)" % (unit, p))
    return p


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("unit", nargs="+", help="unit name (build/objdiff/base/<u>.obj) "
                                            "or a path to an .obj")
    ap.add_argument("-o", "--out", default=None, help="write the .objmod here")
    ap.add_argument("--load-base", type=lambda s: int(s, 0), default=DEFAULT_LOAD_BASE)
    ap.add_argument("--image-base", type=lambda s: int(s, 0), default=DEFAULT_IMAGE_BASE)
    ap.add_argument("--fn", default=None, help="report on this symbol only")
    ap.add_argument("--list", action="store_true",
                    help="print the per-function binding report and exit")
    ap.add_argument("--runnable", action="store_true",
                    help="print only the functions that bind cleanly, one per line")
    a = ap.parse_args()

    rvamap = load_rvamap(verbose=not a.runnable)
    objs = [Obj(obj_path(u)) for u in a.unit]
    m = Module(objs, rvamap, a.load_base, a.image_base)
    m.layout()
    m.bind()

    funcs = [s for s in m.symbols if s["flags"] & OBJSYM_FUNC]
    funcs.sort(key=lambda s: s["retail"] or 0xFFFFFFFF)

    if a.runnable:
        for s in funcs:
            if not (s["flags"] & OBJSYM_REFUSE) and s["retail"]:
                print("%08x %s" % (s["retail"] - a.image_base, s["name"]))
        return 0

    print("[objbind] %d object(s), blob %d bytes at %08x, image_base %08x"
          % (len(objs), len(m.blob), a.load_base, a.image_base))
    for s in funcs:
        if a.fn and s["name"] != a.fn:
            continue
        mark = "REFUSE" if s["flags"] & OBJSYM_REFUSE else "ok    "
        print("  %s %08x -> ours %08x  reloc %d (retail %d, intra %d, foreign %d, "
              "unresolved %d)  %s"
              % (mark, (s["retail"] - a.image_base) if s["retail"] else 0, s["ours"],
                 s["n"], s["retail_n"], s["intra"], s["foreign"], s["unres"],
                 s["name"]))
        for n in sorted(set(s["unres_names"])):
            print("        UNRESOLVED  %s" % n)
        for n in sorted(set(s["foreign_names"])):
            print("        our own copy %s" % n)
    if m.unresolved:
        print("[objbind] %d distinct unresolved symbol(s):" % len(m.unresolved))
        for n, c in sorted(m.unresolved.items(), key=lambda kv: -kv[1]):
            print("    %4d  %s" % (c, n))
    if a.list:
        return 0
    if not a.out:
        raise SystemExit("objbind: -o is required unless --list/--runnable")
    n = m.write(a.out)
    print("[objbind] wrote %s (%d bytes, %d symbols)" % (a.out, n, len(m.symbols)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
