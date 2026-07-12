#!/usr/bin/env python3
"""reloc_fidelity.py - the metric objdiff cannot see.

A relocation is byte-masked in match scoring: at a fixup site objdiff compares
nothing, so a source reference bound to NO retail address (a declared-only "fake"
function, often living in a fake view) or bound to the WRONG address still scores
100%. This tool makes reloc correctness a gameable metric with a zero target.

Per matched function it aligns, BY FUNCTION-RELATIVE OFFSET (both sides' reloc
records are function-relative, so they line up when the bytes match):
  retail = the target RVA the retail .reloc / rel32 operand actually uses at
           each fixup offset inside the function
  ours   = our base COMDAT's relocation record at the same offset, its named
           symbol resolved to an RVA through the RVA()/DATA() bindings
Per site verdict:
  CORRECT   our symbol resolves to exactly the retail target RVA
  MISBOUND  resolves to a DIFFERENT real RVA  <- actively wrong, the toxic case
  UNBOUND   our symbol has no RVA()/DATA() binding (fake/undefined decl) - and
            retail TELLS us the address it should be (a precise bind-to worklist)
  EXEMPT    library / import / string-pool / EH / local-label symbol (we don't
            bind those; they're the lib's job)
A function is FAITHFUL iff it has zero MISBOUND and zero UNBOUND sites.

Usage:
  python -m gruntz.analysis.reloc_fidelity              # headline + worst offenders
  python -m gruntz.analysis.reloc_fidelity --worklist   # per-site fix list (CSV)
  python -m gruntz.analysis.reloc_fidelity --unit grunt # one unit
"""
import argparse
import bisect
import csv
import glob
import os
import re
import struct
import sys
from collections import Counter, defaultdict
from pathlib import Path

# Resolve REPO from the CWD first, not __file__: in a worktree the shell's
# PYTHONPATH can point at MAIN's scripts/, so `python -m ...` loads main's module
# and __file__ would mis-resolve to main ([[worktree-pythonpath-leaks-to-main]]).
# The CWD is the worktree the user is actually in.
def _find_repo():
    for base in (Path.cwd(), Path(__file__).resolve().parent):
        for p in (base, *base.parents):
            if (p / "flake.nix").exists() and (p / "build" / "objdiff").exists():
                return p
    return next((p for p in Path(__file__).resolve().parents
                 if (p / "flake.nix").exists()), Path(__file__).resolve().parents[3])


REPO = _find_repo()
EXE = os.environ.get("GRUNTZ_EXE", "")
IMG = 0x400000
TEXT = (0x1000, 0x1e626b)

D = open(EXE, "rb").read() if EXE and os.path.isfile(EXE) else None


def fo(r):
    if r < 0x1e6800:
        return r - 0x1000 + 0x400
    if r < 0x208000:
        return 0x1e5800 + (r - 0x1e7000)
    return 0x206800 + (r - 0x208000)


def resolve_thunk(t, d=0):
    while d < 4 and TEXT[0] <= t < TEXT[1] and D[fo(t)] == 0xE9:
        t = t + 5 + struct.unpack("<i", D[fo(t) + 1:fo(t) + 5])[0]
        d += 1
    return t


_RELOC_VTBL_RE = re.compile(r"\bRELOC_VTBL\s*\(\s*([\w:]+)\s*,\s*(0x[0-9a-fA-F]+)\s*\)")


def reloc_vtbl_vtable_names():
    """{??_7<class>@@6B@ -> rva} for every RELOC_VTBL(class, addr) tree-wide. The
    placeholder class's cl-emitted vtable symbol reloc-masks the retail vtable at
    `addr` (which a DIFFERENT real class already binds via VTBL(), so it can't ALSO
    live in symbol_names.csv - the per-rva dedup keeps the real name). The vptr-store
    reference IS correctly bound to `addr` by design; without this it false-reports as
    UNBOUND. A WRONG addr still surfaces as MISBOUND (have != the retail want)."""
    out = {}
    for pat in ("src/**/*.cpp", "src/**/*.h", "include/**/*.h"):
        for p in glob.glob(str(REPO / pat), recursive=True):
            try:
                txt = open(p, "r", errors="ignore").read()
            except OSError:
                continue
            for m in _RELOC_VTBL_RE.finditer(txt):
                cls = m.group(1).split("::")[-1]
                out.setdefault("??_7%s@@6B@" % cls, int(m.group(2), 16))
    return out


def load_bindings():
    name2rva, funcs = {}, []
    for r in csv.DictReader(open(REPO / "build/gen/symbol_names.csv")):
        a = int(r["rva"], 16)
        name2rva[r["name"]] = a
        if r["kind"] == "func":
            funcs.append((a, int(r["size"] or "0", 16), r["name"], r["unit"]))
    # RELOC_VTBL placeholder vtable symbols aren't in symbol_names.csv (their rva is
    # bound under the real class's ??_7); resolve them so their vptr-store DIR32s are
    # counted CORRECT/MISBOUND against the retail target, not blanket-UNBOUND.
    for nm, a in reloc_vtbl_vtable_names().items():
        name2rva.setdefault(nm, a)
    funcs.sort()
    return name2rva, funcs


def load_exact_names():
    """Mangled names of functions objdiff scores 100% (fuzzy). The metric is only
    meaningful here: a matching function's bytes align, so a DIR32 offset lines up
    and any mismatch is a defect objdiff CANNOT see. Below 100% the bytes differ,
    offsets desync, and objdiff already flags the function - not the hidden case."""
    exact = set()
    p = REPO / "config/match_baseline.tsv"
    for line in open(p):
        if line.startswith("#"):
            continue
        f = line.rstrip("\n").split("\t")
        if len(f) >= 3 and f[1].startswith("?") and f[2] == "100.0000":
            exact.add(f[1])
    return exact


def load_libnames():
    lib = set()
    p = REPO / "config/library_labels.csv"
    if p.is_file():
        for r in csv.reader(open(p)):
            if r and r[0].startswith("0x"):
                lib.add(r[1])
    return lib


def is_exempt(sym, lib):
    return (sym in lib or sym.startswith((
        "__imp_", "$L", "$S", "$T", "??_C@", "__real@", "__xmm@", "_CxxThrow",
        "__CxxFrameHandler", "__except_list", "__load_config", "?__L", "__ehhandler"))
        or sym in ("___security_cookie",))


def retail_sites(rva, size):
    """{function-relative offset -> target RVA} for the ABSOLUTE (DIR32) fixups
    inside the function. Absolutes require a base-reloc, so retail's .reloc table
    is the authoritative, complete, offset-exact record — every DIR32 our COMDAT
    emits has a retail entry at the same offset iff the bytes match. (rel32 call
    displacements carry NO base-reloc and need a real disassembler to recover
    without desync; they are byte-visible to objdiff anyway, so out of scope.)"""
    out = {}
    lo = bisect.bisect_left(_RSITES, rva)
    hi = bisect.bisect_left(_RSITES, rva + size)
    for s in _RSITES[lo:hi]:
        out[s - rva] = _RELOC[s]
    return out


class Coff:
    def __init__(self, path):
        b = self.b = open(path, "rb").read()
        self.nsec = struct.unpack_from("<H", b, 2)[0]
        self.symptr = struct.unpack_from("<I", b, 8)[0]
        self.nsym = struct.unpack_from("<I", b, 12)[0]
        opt = struct.unpack_from("<H", b, 16)[0]
        self.strtab = self.symptr + self.nsym * 18
        self.secs = []
        for i in range(self.nsec):
            o = 20 + opt + i * 40
            rawsize, rawptr = struct.unpack_from("<II", b, o + 16)
            relptr = struct.unpack_from("<I", b, o + 24)[0]
            nrel = struct.unpack_from("<H", b, o + 32)[0]
            self.secs.append((relptr, nrel, rawptr, rawsize))

    def addend(self, sec, off):
        """The 4-byte DIR32 addend stored in the section at `off`. Our COMDAT
        encodes a constant array index there (g_bfP[1] -> addend 4), so the real
        target is name2rva[sym] + addend, not the bare symbol base."""
        _rp, _nr, rawptr, rawsize = self.secs[sec - 1]
        if rawptr and 0 <= off + 4 <= rawsize:
            return struct.unpack_from("<i", self.b, rawptr + off)[0]
        return 0

    def name(self, idx):
        base = self.symptr + idx * 18
        if struct.unpack_from("<I", self.b, base)[0] == 0:
            off = struct.unpack_from("<I", self.b, base + 4)[0]
            e = self.b.index(b"\0", self.strtab + off)
            return self.b[self.strtab + off:e].decode("latin1")
        return self.b[base:base + 8].split(b"\0")[0].decode("latin1")

    def defined_text(self):
        out = {}
        i = 0
        while i < self.nsym:
            base = self.symptr + i * 18
            _v, sec, _t, scl, naux = struct.unpack_from("<IhHBB", self.b, base + 8)
            if sec >= 1 and scl == 2:
                out.setdefault(self.name(i), sec)
            i += 1 + naux
        return out

    def reloc_sites(self, sec):
        """{operand offset -> (symbol name, kind)} for DIR32 + REL32 relocs.
        kind 'D'=DIR32 absolute (data/vtable/fn-ptr), 'C'=REL32 call/jmp. Our
        COMDAT names the callee and marks the exact operand offset, so for a
        byte-exact function retail's displacement at that same offset gives the
        real callee - no disassembler needed."""
        relptr, nrel = self.secs[sec - 1][:2]
        out = {}
        for r in range(nrel):
            ro = relptr + r * 10
            vaddr, symidx, typ = struct.unpack_from("<IIH", self.b, ro)
            if typ == 6:
                out[vaddr] = (self.name(symidx), "D")
            elif typ == 20:
                out[vaddr] = (self.name(symidx), "C")
        return out


def _current_unit_objs():
    """Only objs for CURRENT units - build/objdiff/base/ accumulates STALE ORPHAN
    objs from retired/renamed units (gitignored artifacts), and an orphan's stale
    COMDAT copy shadows the live one under the plain glob, false-reporting a fixed
    symbol as still-defective. units.toml is the authoritative current set."""
    try:
        import tomllib
        units = {u["unit"] for u in
                 tomllib.loads((REPO / "config/units.toml").read_text()).get("unit", [])}
    except Exception:
        units = None
    objs = []
    for p in glob.glob(str(REPO / "build/objdiff/base/*.obj")):
        stem = os.path.basename(p)[:-4]
        if units is None or stem in units:
            objs.append(p)
    return objs


def build_comdat_index():
    name2rva = _NAME2RVA
    idx = {}
    # newest-first so a live obj wins over any same-named straggler
    for p in sorted(_current_unit_objs(), key=lambda q: -os.path.getmtime(q)):
        c = Coff(p)
        for nm, sec in c.defined_text().items():
            if nm in name2rva and nm not in idx:
                idx[nm] = (c, sec)
    return idx


def analyze(funcs, comdat, lib, exact, unit_filter=None):
    rows = []
    for rva, size, nm, unit in funcs:
        if unit_filter and unit != unit_filter:
            continue
        if nm not in exact:           # only byte-exact functions align offset-wise
            continue
        cd = comdat.get(nm)
        if not cd or size == 0:
            continue
        c, sec = cd
        rt = retail_sites(rva, size)     # DIR32 absolute targets (authoritative)
        fb = D[fo(rva):fo(rva) + size]
        sites = []
        for voff, (sym, kind) in c.reloc_sites(sec).items():
            if is_exempt(sym, lib):
                continue
            base = _NAME2RVA.get(sym)     # our binding for the symbol (or None)
            if kind == "D":
                want = rt.get(voff)      # retail absolute target at this offset
                # fold the DIR32 addend: g_arr[k] emits a reloc naming g_arr with
                # addend k*elemsize in the section bytes, so our real target is
                # base+addend - else every constant array index false-reports.
                have = None if base is None else base + c.addend(sec, voff)
            else:                        # REL32 call: retail callee from displacement
                if voff + 4 <= len(fb):
                    disp = struct.unpack("<i", fb[voff:voff + 4])[0]
                    t = rva + voff + 4 + disp
                    want = resolve_thunk(t) if TEXT[0] <= t < TEXT[1] else None
                else:
                    want = None
                have = base
            if have is None:
                v = "UNBOUND"
            elif want is None:
                v = "MISBOUND"           # we reference where retail has no ref
            elif have == want:
                v = "CORRECT"
            else:
                v = "MISBOUND"
            sites.append((voff, sym, have, want, v, kind))
        bad = [s for s in sites if s[4] in ("UNBOUND", "MISBOUND")]
        rows.append({"rva": rva, "size": size, "name": nm, "unit": unit,
                     "n": len(sites), "correct": sum(1 for s in sites if s[4] == "CORRECT"),
                     "misbound": sum(1 for s in sites if s[4] == "MISBOUND"),
                     "unbound": sum(1 for s in sites if s[4] == "UNBOUND"),
                     "call_bad": sum(1 for s in sites if s[5] == "C" and s[4] != "CORRECT"),
                     "data_bad": sum(1 for s in sites if s[5] == "D" and s[4] != "CORRECT"),
                     "sites": sites, "faithful": not bad})
    return rows


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--worklist", action="store_true")
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--unit")
    args = ap.parse_args()
    if D is None:
        sys.exit("reloc_fidelity: $GRUNTZ_EXE not set")

    global _NAME2RVA, _RELOC, _RSITES
    _NAME2RVA, funcs = load_bindings()
    lib = load_libnames()

    _RELOC = {}
    off, end = 0x249a00, 0x249a00 + 0x1b9d1
    while off + 8 <= end:
        page, bsz = struct.unpack("<II", D[off:off + 8])
        if bsz < 8:
            break
        for i in range(8, bsz, 2):
            e, = struct.unpack("<H", D[off + i:off + i + 2])
            if e >> 12 == 3:
                s = page + (e & 0xFFF)
                if TEXT[0] <= s < TEXT[1]:
                    _RELOC[s] = struct.unpack("<I", D[fo(s):fo(s) + 4])[0] - IMG
        off += bsz
    _RSITES = sorted(_RELOC)

    comdat = build_comdat_index()
    exact = load_exact_names()
    rows = analyze(funcs, comdat, lib, exact, args.unit)

    if args.worklist:
        w = csv.writer(sys.stdout)
        w.writerow(["rva", "unit", "function", "offset", "symbol",
                    "our_rva", "retail_rva", "verdict", "kind"])
        for r in rows:
            for voff, sym, have, want, v, kind in r["sites"]:
                if v in ("MISBOUND", "UNBOUND"):
                    w.writerow(["0x%08x" % r["rva"], r["unit"], r["name"], "0x%x" % voff,
                                sym, "0x%x" % have if have else "",
                                "0x%x" % want if want else "", v,
                                "CALL" if kind == "C" else "DATA"])
        return

    if args.json:
        import json
        byu = defaultdict(lambda: [0, 0, 0, 0, 0])  # mis, unb, call_bad, data_bad, n
        for r in rows:
            u = byu[r["unit"]]
            u[0] += r["misbound"]; u[1] += r["unbound"]
            u[2] += r["call_bad"]; u[3] += r["data_bad"]; u[4] += 1
        out = {
            "n": len(rows),
            "faithful": sum(1 for r in rows if r["faithful"]),
            "sites": sum(r["n"] for r in rows),
            "misbound": sum(r["misbound"] for r in rows),
            "unbound": sum(r["unbound"] for r in rows),
            "call_bad": sum(r["call_bad"] for r in rows),
            "data_bad": sum(r["data_bad"] for r in rows),
            "units": [{"unit": u, "misbound": v[0], "unbound": v[1],
                       "call_bad": v[2], "data_bad": v[3], "n": v[4]}
                      for u, v in sorted(byu.items(),
                                         key=lambda kv: -(kv[1][0] + kv[1][1]))],
            "worst": [{"unit": r["unit"], "name": r["name"], "misbound": r["misbound"],
                       "unbound": r["unbound"], "call_bad": r["call_bad"],
                       "data_bad": r["data_bad"]}
                      for r in sorted(rows, key=lambda r: -(r["misbound"] + r["unbound"]))
                      if not r["faithful"]][:40]}
        print(json.dumps(out))
        return

    n = len(rows)
    faithful = sum(1 for r in rows if r["faithful"])
    mis = sum(r["misbound"] for r in rows)
    unb = sum(r["unbound"] for r in rows)
    tot = sum(r["n"] for r in rows)
    print("reloc fidelity over %d matched functions with a base COMDAT:" % n)
    print("  FAITHFUL functions (0 misbound, 0 unbound): %d (%d%%)"
          % (faithful, 100 * faithful // max(n, 1)))
    print("  reloc sites: %d total, %d correct, %d MISBOUND, %d UNBOUND"
          % (tot, tot - mis - unb, mis, unb))
    print("  MISBOUND is the toxic case (points at a real but WRONG rva);")
    print("  UNBOUND = a declared-only/fake symbol; retail names the target to bind to.")
    cb = sum(r["call_bad"] for r in rows)
    db = sum(r["data_bad"] for r in rows)
    print("  by kind: %d CALL-target defects (fake/misbound FUNCTIONS, rel32), "
          "%d DATA defects (globals/vtables, DIR32)" % (cb, db))
    print("\nworst 25 functions (misbound first):")
    for r in sorted(rows, key=lambda r: (-r["misbound"], -r["unbound"]))[:25]:
        ex = [s[1] for s in r["sites"] if s[4] == "MISBOUND"][:2]
        print("  %-20s %-46s mis=%d unb=%d %s"
              % (r["unit"], r["name"][:46], r["misbound"], r["unbound"], ex))
    # per-unit rollup
    byu = defaultdict(lambda: [0, 0, 0])
    for r in rows:
        u = byu[r["unit"]]
        u[0] += r["misbound"]
        u[1] += r["unbound"]
        u[2] += 1
    print("\ntop units by misbound:")
    for u, (m, ub, c) in sorted(byu.items(), key=lambda kv: -kv[1][0])[:12]:
        print("  %-24s misbound=%d unbound=%d (%d fns)" % (u, m, ub, c))


if __name__ == "__main__":
    main()
