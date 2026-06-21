#!/usr/bin/env python3
"""extern_harvest.py - inventory the unresolved externals matched code references.

A matched function calls/loads symbols we have NOT given an address: the call
`?Find@CButeTree@@QAEPAXPBD@Z` is an undefined external (`U`) in the base obj, so
the delinked target obj names that retail address with Ghidra's `FUN_<rva>`
placeholder and the reloc fails to pair by name (fuzzy, not exact).

This tool finds every such external and recovers its retail address by CORRELATION
- it is read-only and emits no stubs. For each matched function F (RVA known from
symbol_names.csv) it reads F's base-obj relocations; for each reloc whose target is
defined NOWHERE in our base objs, it reads the wired displacement/address from the
RETAIL EXE at `F_rva + reloc_offset` to compute the target RVA:

    REL32 (call/jmp):  target = F_rva + off + 4 + int32(exe[F_rva+off])
    DIR32 (data/&sym): target = uint32(exe[F_rva+off]) - imagebase

The address is a FACT (the real retail target); the name is our existing RE claim
(the caller's declaration). A symbol resolves to exactly one address, so every
matched caller of one external must correlate to the SAME RVA - a disagreement is a
genuine mislabel, surfaced as a CONFLICT (a free audit before any stub is written).

Output: a summary, the conflict audit, and the ranked inventory (callers, kind,
ghidra-name, rva, size, name) - the worklist for the stub migration.

Run: python -m gruntz.analysis.extern_harvest
"""

import argparse
import csv
import os
import struct
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
REPO = next((p for p in SCRIPT_DIR.parents if (p / "flake.nix").exists()), SCRIPT_DIR)

DIR32 = 0x0006   # IMAGE_REL_I386_DIR32  (absolute VA in the field)
REL32 = 0x0014   # IMAGE_REL_I386_REL32  (pc-relative, from end of field)

# StorageClass
C_EXTERNAL = 2


# --- retail PE: read a u32 at an RVA -----------------------------------------
class Exe:
    def __init__(self, path: Path):
        d = path.read_bytes()
        self.data = d
        pe = struct.unpack_from("<I", d, 0x3C)[0]
        nsec = struct.unpack_from("<H", d, pe + 6)[0]
        opt = struct.unpack_from("<H", d, pe + 20)[0]
        self.base = struct.unpack_from("<I", d, pe + 24 + 28)[0]
        self.secs = []
        so = pe + 24 + opt
        for i in range(nsec):
            o = so + i * 40
            vsize, vaddr, rawsize, rawptr = struct.unpack_from("<IIII", d, o + 8)
            self.secs.append((vaddr, max(vsize, rawsize), rawptr, rawsize))

        self.image_end = max(v + s for v, s, _p, _r in self.secs)
        # .text bounds (call/jmp targets live here): first executable-ish section.
        self.text_lo, self.text_hi = self.secs[0][0], self.secs[0][0] + self.secs[0][1]

    def u32_at_rva(self, rva: int):
        for vaddr, span, rawptr, rawsize in self.secs:
            if vaddr <= rva < vaddr + span:
                off = rawptr + (rva - vaddr)
                if off + 4 <= rawptr + rawsize:
                    return struct.unpack_from("<I", self.data, off)[0]
        return None

    def u8_at_rva(self, rva: int):
        for vaddr, span, rawptr, rawsize in self.secs:
            if vaddr <= rva < vaddr + span:
                off = rawptr + (rva - vaddr)
                if off < rawptr + rawsize:
                    return self.data[off]
        return None


# --- COFF object: symbols, per-section relocs, section-leader functions ------
class Coff:
    def __init__(self, path: Path):
        b = path.read_bytes()
        self.buf = b
        self.nsec = struct.unpack_from("<H", b, 2)[0]
        self.symptr = struct.unpack_from("<I", b, 8)[0]
        self.nsym = struct.unpack_from("<I", b, 12)[0]
        opt = struct.unpack_from("<H", b, 16)[0]
        self.strtab = self.symptr + self.nsym * 18
        # section headers
        self.sec_reloc = []           # 1-based sec -> (relptr, nreloc)
        self.sec_raw = []             # 1-based sec -> (rawptr, rawsize)
        for i in range(self.nsec):
            o = 20 + opt + i * 40
            rawsize, rawptr = struct.unpack_from("<II", b, o + 16)
            relptr = struct.unpack_from("<I", b, o + 24)[0]
            nreloc = struct.unpack_from("<H", b, o + 32)[0]
            self.sec_reloc.append((relptr, nreloc))
            self.sec_raw.append((rawptr, rawsize))
        # symbol table (cache name + (value, secnum, sclass) per index)
        self.syms = []                # index -> (name, value, secnum, sclass)
        i = 0
        while i < self.nsym:
            base = self.symptr + i * 18
            value, secnum, _typ, sclass, naux = struct.unpack_from("<IhHBB", b, base + 8)
            self.syms.append((self._name(base), value, secnum, sclass))
            for j in range(1, 1 + naux):
                self.syms.append(("", 0, 0, 0))   # placeholder for aux slot
            i += 1 + naux

    def _name(self, base):
        if struct.unpack_from("<I", self.buf, base)[0] == 0:
            off = struct.unpack_from("<I", self.buf, base + 4)[0]
            end = self.buf.index(b"\0", self.strtab + off)
            return self.buf[self.strtab + off:end].decode("latin1")
        return self.buf[base:base + 8].split(b"\0")[0].decode("latin1")

    def defined_names(self):
        return {n for (n, _v, sec, _c) in self.syms if n and sec > 0}

    def funcs_in_section(self, sec):
        """[(value, name)] external symbols defined at section `sec`, sorted by value -
        the candidate function leaders (skip $-labels)."""
        out = [(v, n) for (n, v, s, c) in self.syms
               if s == sec and c == C_EXTERNAL and n and not n.startswith("$")]
        out.sort()
        return out

    def relocs(self, sec):
        """[(offset, symidx, type)] for 1-based section `sec`."""
        relptr, nreloc = self.sec_reloc[sec - 1]
        out = []
        for k in range(nreloc):
            o = relptr + k * 10
            vaddr, symidx, typ = struct.unpack_from("<IIH", self.buf, o)
            out.append((vaddr, symidx, typ))
        return out

    def addend_at(self, sec, off):
        """Signed i32 stored in the section's raw bytes at `off` - the reloc addend
        cl.exe writes into the field (0 for a call/&symbol; N for `&arr[N]`/`&s.f`)."""
        rawptr, rawsize = self.sec_raw[sec - 1]
        if not rawptr or off + 4 > rawsize:
            return 0
        v = struct.unpack_from("<i", self.buf, rawptr + off)[0]
        return v


# --- inputs ------------------------------------------------------------------
def load_symbol_names(path):
    """name -> rva (matched functions + data, from build/gen/symbol_names.csv)."""
    m = {}
    with open(path, newline="") as f:
        for row in csv.DictReader(ln for ln in f if not ln.lstrip().startswith("#")):
            if row.get("rva") and row.get("name"):
                m[row["name"].strip()] = int(row["rva"], 16)
    return m


def load_exact(path):
    """{name} of functions at fuzzy_match_percent == 100 (offsets align with retail).

    Correlation is only trustworthy from a byte-aligned function: a fuzzy (<100)
    caller can have an inserted/deleted instruction, shifting the reloc offset off
    the real retail site and yielding a garbage target. The reloc-NAME diff this
    tool exists to fix does not lower fuzzy %, so the set we want is exactly the
    fuzzy-100 functions.
    """
    import json
    exact = set()
    if not Path(path).exists():
        return exact
    rep = json.loads(Path(path).read_text())
    for u in rep.get("units", []):
        for fn in u.get("functions", []):
            if fn.get("fuzzy_match_percent") == 100.0 and fn.get("name"):
                exact.add(fn["name"])
    return exact


def load_ghidra_functions(path):
    """rva -> (size, name) from ghidra functions.csv."""
    m = {}
    if not Path(path).exists():
        return m
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            m[int(row["entry_rva"], 16)] = (int(row["byte_size"]), row["name"])
    return m


def load_ghidra_symbols(path):
    """rva -> name from ghidra symbols.csv (data/strings)."""
    m = {}
    if not Path(path).exists():
        return m
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            a = row["address_rva"]
            if a.startswith("0x-"):
                continue
            m.setdefault(int(a, 16), row["name"])
    return m


# --- classification (engine vs library, for the report only) -----------------
_MFC = ("CObject", "CObList", "CMapStringToOb", "CString", "CArchive", "CFile",
        "CDialog", "CWnd", "CByteArray", "CPtrArray", "CPtrList", "CDC", "CWinApp",
        "CException", "CRuntimeClass", "CGdiObject", "CDWordArray", "CStringArray",
        "__POSITION", "CMemFile", "CTime", "CWinThread", "CCmdTarget", "CScrollView")
_CRT_EXACT = {"_atexit", "_calloc", "_malloc", "_free", "_realloc", "__fltused",
              "___CxxFrameHandler", "__EH_prolog", "__except_list", "__setjmp3",
              "_memcpy", "_memset", "_memcmp", "_memmove", "_strlen", "_strcpy",
              "_strcmp", "_strcat", "_sprintf", "_atoi", "_atol", "__ftol", "_abort",
              "__purecall", "_rand", "_srand", "__CIfmod", "__CIsqrt", "_qsort"}


def classify(name):
    # global operator new/delete + vector new/delete are CRT/MFC, not engine.
    if name.startswith(("??2@", "??3@", "??_U", "??_V")):
        return "lib"
    if name in _CRT_EXACT or name.startswith("_Afx") or name.startswith("__"):
        return "lib"
    if any(("@" + c + "@") in name or ("@" + c + "@@") in name or name.endswith("@" + c + "@@")
           for c in _MFC):
        return "lib"
    if "@@3" in name:           # `?g_foo@@3...` global variable -> data
        return "data"
    return "eng"


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--base-dir", default=str(REPO / "build/objdiff/base"))
    ap.add_argument("--exe", default=str(REPO / "build/exe/GRUNTZ.EXE"))
    ap.add_argument("--names", default=str(REPO / "build/gen/symbol_names.csv"))
    ap.add_argument("--gh-funcs", default=str(REPO / "build/ghidra-enrich/exports/functions.csv"))
    ap.add_argument("--gh-syms", default=str(REPO / "build/ghidra-enrich/exports/symbols.csv"))
    ap.add_argument("--report", default=str(REPO / "build/objdiff/report.json"),
                    help="objdiff report.json; correlate only from fuzzy-100 callers.")
    ap.add_argument("--out", help="write the full inventory as TSV here too.")
    ap.add_argument("--limit", type=int, default=40, help="rows to print per section.")
    args = ap.parse_args()

    exe = Exe(Path(args.exe))
    names = load_symbol_names(args.names)
    exact = load_exact(args.report)
    gh_funcs = load_ghidra_functions(args.gh_funcs)
    gh_syms = load_ghidra_symbols(args.gh_syms)

    objs = sorted(Path(args.base_dir).glob("*.obj"))
    coffs = [(o, Coff(o)) for o in objs]

    # 1. global defined-symbol set across all base objs.
    defined = set()
    for _o, c in coffs:
        defined |= c.defined_names()

    # 2. correlate: external -> {rva -> [callers]} and bad (uncomputable) sites.
    ext = {}            # name -> {target_rva: set(caller_fn)}
    no_target = {}      # name -> count of sites where the exe read failed
    for _o, c in coffs:
        for sec in range(1, c.nsec + 1):
            leaders = c.funcs_in_section(sec)        # [(value, name)] sorted
            if not leaders:
                continue
            rl = c.relocs(sec)
            if not rl:
                continue
            for off, symidx, typ in rl:
                if symidx >= len(c.syms):
                    continue
                target = c.syms[symidx][0]
                if not target or target in defined:
                    continue                          # resolved internally
                # containing function = greatest leader value <= off
                fn = None
                for v, n in leaders:
                    if v <= off:
                        fn = n
                    else:
                        break
                if fn is None or fn not in names:
                    continue                          # caller RVA unknown
                if exact and fn not in exact:
                    continue                          # only byte-aligned callers
                fn_rva = names[fn]
                field = exe.u32_at_rva(fn_rva + off)
                if field is None:
                    no_target[target] = no_target.get(target, 0) + 1
                    continue
                if typ == REL32:
                    # the field must be the disp of a `call`/`jmp rel32`.
                    op = exe.u8_at_rva(fn_rva + off - 1)
                    if op not in (0xE8, 0xE9):
                        no_target[target] = no_target.get(target, 0) + 1
                        continue
                    disp = field if field < 0x80000000 else field - 0x100000000
                    trva = fn_rva + off + 4 + disp
                    if not (exe.text_lo <= trva < exe.text_hi):
                        no_target[target] = no_target.get(target, 0) + 1
                        continue
                elif typ == DIR32:
                    # field = (symbol_va + addend); subtract the base-obj addend so
                    # `&arr[N]` / `&s.field` resolve to the symbol base, not element N.
                    trva = field - c.addend_at(sec, off) - exe.base
                    if not (0 <= trva < exe.image_end):
                        no_target[target] = no_target.get(target, 0) + 1
                        continue
                else:
                    continue
                ext.setdefault(target, {}).setdefault(trva, set()).add(fn)

    # 3. consensus + ghidra join + classify.
    #   `state` is what the delinked obj names this RVA today: "match" (Ghidra
    #   already gives our exact name -> no mismatch, no stub needed) vs an
    #   actionable placeholder (FUN_ / DAT_,s_ / a different name / absent).
    rows = []           # (callers, kind, state, actionable, rva, size, name)
    conflicts = []      # (name, {rva: ncallers})
    for name, by_rva in ext.items():
        callers = sum(len(s) for s in by_rva.values())
        if len(by_rva) > 1:
            conflicts.append((name, {r: len(s) for r, s in by_rva.items()}))
            continue
        rva = next(iter(by_rva))
        kind = classify(name)
        if rva in gh_funcs:
            size, gh = gh_funcs[rva]
        elif rva in gh_syms:
            size, gh = 0, gh_syms[rva]
        else:
            size, gh = 0, None
        if gh == name:
            state = "match"
        elif gh is None:
            state = "absent"
        elif gh.startswith("FUN_"):
            state = "FUN_"
        elif gh.startswith(("DAT_", "s_")):
            state = "DAT_/s_"
        else:
            state = "other"
        rows.append((callers, kind, state, state != "match", rva, size, name))

    rows.sort(key=lambda r: (-r[0], r[6]))
    act = [r for r in rows if r[3]]            # actionable (real mismatch today)

    # 4. report.
    def kc(rs, k):
        return sum(1 for r in rs if r[1] == k)
    print("=" * 78)
    print("UNRESOLVED EXTERNALS referenced by matched (fuzzy-100) code")
    print("=" * 78)
    print(f"  byte-aligned callers (exact set) : {len(exact)}")
    print(f"  distinct externals correlated    : {len(ext)}")
    print(f"  clean (single RVA, consensus)    : {len(rows)}")
    print(f"  CONFLICTS (>1 RVA, mislabel?)    : {len(conflicts)}")
    print(f"  dropped exe sites (sanity-failed): {len(no_target)}")
    print()
    print(f"  already match (Ghidra=our name): {len(rows) - len(act)}")
    print(f"  ACTIONABLE (mismatch today)    : {len(act)}")
    print(f"      engine fns {kc(act,'eng')}   data globals {kc(act,'data')}   library {kc(act,'lib')}")
    print(f"      -> stub-migration scope = engine fns + data globals = "
          f"{kc(act,'eng') + kc(act,'data')}")
    print()

    if conflicts:
        print("-" * 78)
        print("CONFLICTS - one symbol, multiple retail RVAs (LABELING BUG, fix first):")
        print("-" * 78)
        for name, by in sorted(conflicts):
            sites = "  ".join(f"0x{r:06x}(x{n})" for r, n in sorted(by.items()))
            print(f"  {name}\n      {sites}")
        print()

    print("-" * 78)
    print(f"ACTIONABLE INVENTORY (top {args.limit} by #callers; kind=eng|data|lib):")
    print(f"{'callers':>7}  {'kind':4}  {'now':8}  {'rva':>8}  {'size':>5}  name")
    print("-" * 78)
    for callers, kind, state, _a, rva, size, name in act[:args.limit]:
        print(f"{callers:>7}  {kind:4}  {state:8}  0x{rva:06x}  {size:>5}  {name}")
    if len(act) > args.limit:
        print(f"  ... {len(act) - args.limit} more actionable (use --out for all)")

    if args.out:
        with open(args.out, "w") as f:
            f.write("callers\tkind\tstate\tactionable\trva\tsize\tname\n")
            for callers, kind, state, a, rva, size, name in rows:
                f.write(f"{callers}\t{kind}\t{state}\t{int(a)}\t0x{rva:06x}\t{size}\t{name}\n")
        print(f"\n[wrote full inventory ({len(rows)} rows) -> {args.out}]")


if __name__ == "__main__":
    main()
