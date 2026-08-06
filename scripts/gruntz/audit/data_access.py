#!/usr/bin/env python3
"""gruntz.audit.data_access - the STATIC access map of .rdata/.data: every retail
instruction that touches a data RVA, decoded for width/mode/direction, aggregated
per declared DATA() symbol and diffed against its declared type.

The static half of the mis-typed-globals campaign (a dynamic debugger trace is the
deferred second producer into the same event schema). The .reloc section enumerates
EVERY absolute data reference in code - exhaustive, no gameplay coverage gap. Each
site's containing instruction (objdump, the producer sema disasm already trusts)
gives:

  width  BYTE/WORD/DWORD/QWORD/TBYTE memory operand = the access size
  mode   direct   `ds:addr`             the datum itself is read/written
         indexed  `[reg*s+addr]`        addr is a TABLE base; s = stride witness
         lea/imm  address-taken         the object escapes to a callee (opaque)
         indcall  `call/jmp [addr]`     the cell holds a FUNCTION POINTER
         iat      indcall into the import table (excluded from symbol evidence)
  rw     r / w / rw, from the mnemonic class
  ext    movzx = unsigned witness, movsx = signed witness
  fpu    fld/fst DWORD = f32, QWORD = f64; fild/fist = integer (not float)

Data-side relocs add content evidence: a HIGHLOW cell INSIDE a symbol's extent
holding a .text VA proves a fn-ptr field, a data VA proves a data-ptr field; a
cell POINTED AT interior offset N proves a subobject boundary at N.

Attribution: a symbol's coverage is its DECLARED size (reviewed
symbol_names.csv size, else globals.json sizeof), clamped to the next symbol;
accesses past coverage are never charged to it - they cluster into candidate
missing-global RUNS (string-pool runs down-ranked by a byte peek). Output:
build/gen/data_access.tsv + advisory verdict flags per symbol:

  decl-overlap     the declared size crosses the NEXT symbol -> merge/size bug
  fpu-on-nonfloat  FPU float access, declared type has no float/double
  fnptr-content    indcall through / .text-VA cells inside, type has no ptr
  ptr-content      relocated data-VA cells inside, declared type has no ptr
  wide-on-prim     access wider than the declared primitive (u16 read as u32)
  narrow-write     sub-dword STORE into an i32 -> a bool/u16 mismodel
  indexed-scalar   a <=4-byte scalar used as an indexed table base
  pointed-interior a data-VA reloc lands INSIDE the scalar -> subobject bound

Usage:
  python -m gruntz.audit.data_access                 # sweep -> TSV + summary
  python -m gruntz.audit.data_access --rva 0x4d2a44  # one symbol, every site
  python -m gruntz.audit.data_access --unclaimed     # unclaimed access runs
  python -m gruntz.audit.data_access --flagged       # only flagged symbols
"""
import argparse
import bisect
import csv
import json
import re
import shutil
import struct
import subprocess
import sys
from collections import Counter, defaultdict
from pathlib import Path

from gruntz.core import get_context
from gruntz.core.data_audit import GLOBALS, load_data_symbols
from gruntz.core.pe import IMAGEBASE, REPO

OUTPUT = REPO / "build/gen/data_access.tsv"
OBJDUMP = shutil.which("objdump") or "objdump"

_INSN = re.compile(r"^\s*([0-9a-f]+):\s*((?:[0-9a-f]{2}\s)+)\s*(\S.*)?$")
_WIDTH = {"BYTE": 1, "WORD": 2, "DWORD": 4, "QWORD": 8, "TBYTE": 10, "FWORD": 6}
_WPTR = re.compile(r"\b(BYTE|WORD|DWORD|QWORD|TBYTE|FWORD) PTR\b")
_REG = re.compile(r"\b(e[abcd]x|e[sd]i|e[bs]p|[abcd][lhx]|[sd]i|[bs]p)\b")
_SCALE = re.compile(r"\*([1248])\b")
_REGWIDTH = {**{r: 4 for r in ("eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp")},
             **{r: 2 for r in ("ax", "bx", "cx", "dx", "si", "di", "bp", "sp")},
             **{r: 1 for r in ("al", "bl", "cl", "dl", "ah", "bh", "ch", "dh")}}

# mnemonic -> how a MEMORY FIRST-operand is used (default: dest of a plain move)
_RMW = {"add", "sub", "adc", "sbb", "and", "or", "xor", "not", "neg", "inc",
        "dec", "sal", "shl", "sar", "shr", "rol", "ror", "rcl", "rcr", "xchg",
        "xadd", "btc", "btr", "bts"}
_READ_FIRST = {"cmp", "test", "push", "bt", "imul"}
_FPU_LOAD = {"fld", "fild", "fadd", "fsub", "fsubr", "fmul", "fdiv", "fdivr",
             "fcom", "fcomp", "ficom", "ficomp", "fiadd", "fisub", "fimul",
             "fidiv", "fidivr", "fisubr"}
_FPU_STORE = {"fst", "fstp", "fist", "fistp", "fisttp", "fbstp"}
_FPU_INT = {"fild", "fist", "fistp", "fisttp", "ficom", "ficomp", "fiadd",
            "fisub", "fisubr", "fimul", "fidiv", "fidivr"}


class Event:
    __slots__ = ("site", "insn_rva", "target", "mode", "width", "rw", "ext",
                 "fpu", "scale", "text")

    def __init__(self, **kw):
        for k in self.__slots__:
            setattr(self, k, kw.get(k))


# --- .text disassembly (one objdump pass, the dump_target producer) -----------
def disasm_text(pe):
    """(starts, lines): sorted insn-start RVAs + the asm text per insn."""
    name, va, vsz, rp, rsz = next(s for s in pe.secs if s[0] == ".text")
    blob = pe.data[rp:rp + rsz]
    tmp = REPO / "build" / ".da_text.bin"
    tmp.parent.mkdir(exist_ok=True)
    tmp.write_bytes(blob)
    try:
        out = subprocess.run(
            [OBJDUMP, "-D", "-b", "binary", "-m", "i386", "-Mintel",
             f"--adjust-vma=0x{va:x}", str(tmp)],
            capture_output=True, text=True, check=True).stdout
    finally:
        tmp.unlink(missing_ok=True)
    starts, lines = [], []
    for ln in out.splitlines():
        m = _INSN.match(ln)
        if not m:
            continue
        if m.group(3) is None:            # byte-continuation of the previous insn
            continue
        starts.append(int(m.group(1), 16))
        lines.append(m.group(3).strip())
    return starts, lines


# --- one .text site -> one classified Event -----------------------------------
def _split_operands(asm):
    """(mnemonic, [operand, ...]) - Intel syntax, no commas inside brackets."""
    asm = asm.split(" <")[0]              # strip objdump's <offset+N> annotations
    parts = asm.split(None, 1)
    if len(parts) == 1:
        return parts[0], []
    mnem = parts[0]
    if mnem in ("rep", "repz", "repnz", "lock"):
        return _split_operands(parts[1])
    return mnem, [o.strip() for o in parts[1].split(",")]


def classify_site(site, stored, starts, lines, iat):
    """Decode the instruction containing reloc `site` into an Event (or an
    'undecoded' one when the linear disasm desynced across a jump table)."""
    target = stored - IMAGEBASE
    k = bisect.bisect_right(starts, site) - 1
    insn_rva, asm = (starts[k], lines[k]) if k >= 0 else (site, "")
    ev = Event(site=site, insn_rva=insn_rva, target=target, text=asm,
               mode="undecoded", width=0, rw="-", ext="", fpu="", scale=0)
    va_hex = f"0x{stored:x}"
    if va_hex not in asm:
        return ev                          # desync / data-in-text: never guess
    mnem, ops = _split_operands(asm)
    opidx = next((i for i, o in enumerate(ops) if va_hex in o), None)
    if opidx is None:
        return ev
    op = ops[opidx]
    is_mem = "[" in op or "ds:" in op or "PTR" in op
    if mnem == "lea":
        ev.mode, ev.rw = "lea", "-"
        m = _SCALE.search(op)
        ev.scale = int(m.group(1)) if m else 0
        return ev
    if not is_mem:
        ev.mode, ev.rw = "imm", "-"        # push 0xVA / mov reg,0xVA / mov [..],0xVA
        return ev
    # memory operand: width, indexing, direction
    m = _WPTR.search(op)
    w = _WIDTH[m.group(1)] if m else 0
    if not w and len(ops) == 2:
        # moffs forms print no PTR keyword (mov eax,ds:0xVA): reg gives width
        other = ops[1 - opidx]
        w = _REGWIDTH.get(other, 0)
    ev.width = w
    inner = op[op.index("[") + 1:op.rindex("]")] if "[" in op else op.split("ds:")[-1]
    ev.mode = "indexed" if _REG.search(inner) else "direct"
    m = _SCALE.search(inner)
    ev.scale = int(m.group(1)) if m else 0
    if mnem in ("call", "jmp"):
        ev.mode = "iat" if iat[0] <= target < iat[1] else "indcall"
        ev.rw = "r"
        return ev
    if mnem in ("movzx", "movsx"):
        ev.rw, ev.ext = "r", ("u" if mnem == "movzx" else "i")
        return ev
    if mnem in _FPU_LOAD or mnem in _FPU_STORE:
        ev.rw = "w" if mnem in _FPU_STORE else "r"
        if mnem in _FPU_INT:
            ev.fpu = f"i{w * 8}"
        else:
            ev.fpu = {4: "f32", 8: "f64", 10: "f80"}.get(w, "f?")
        return ev
    if opidx == 0:
        if mnem in _READ_FIRST:
            ev.rw = "r"
        elif mnem in _RMW:
            ev.rw = "rw"
        elif mnem == "pop" or mnem.startswith("set") or mnem == "mov" \
                or mnem.startswith("mov"):
            ev.rw = "w"
        else:
            ev.rw = "rw"                   # unknown mnemonic with mem dest: assume RMW
    else:
        ev.rw = "r"
    return ev


# --- sweep --------------------------------------------------------------------
def _data_ranges(pe):
    out = []
    for name, va, vsz, rp, rsz in pe.secs:
        if name in (".rdata", ".data", ".bss", ".data1", ".idata"):
            out.append((name, va, va + max(vsz, rsz)))
    return out


def _iat_range(pe):
    rva = struct.unpack_from("<I", pe.data, pe._opt + 96 + 12 * 8)[0]
    sz = struct.unpack_from("<I", pe.data, pe._opt + 96 + 12 * 8 + 4)[0]
    return (rva, rva + sz) if rva else (0, 0)


def sweep(ctx):
    """(events, cells, stats): every .text access event, every data-side reloc
    cell {site, target, kind}, and the set-accounting stats."""
    pe = ctx.pe
    dranges = _data_ranges(pe)
    tname, tva, tvsz, trp, trsz = next(s for s in pe.secs if s[0] == ".text")
    tlo, thi = tva, tva + max(tvsz, trsz)
    iat = _iat_range(pe)

    def in_data(rva):
        return any(lo <= rva < hi for _, lo, hi in dranges)

    starts, lines = disasm_text(pe)
    events, cells = [], []
    stats = Counter()
    for site, stored in pe.relocs_in(tlo, thi):
        target = stored - IMAGEBASE
        if in_data(target):
            ev = classify_site(site, stored, starts, lines, iat)
            events.append(ev)
            stats[f"text-{ev.mode}"] += 1
        elif tlo <= target < thi:
            stats["text-to-text"] += 1     # fn-ptr immediates: xref's domain
        else:
            stats["text-to-other"] += 1
    for _, lo, hi in dranges:
        for site, stored in pe.relocs_in(lo, hi):
            target = stored - IMAGEBASE
            kind = ("fnptr" if tlo <= target < thi
                    else "dataptr" if in_data(target) else "otherptr")
            cells.append({"site": site, "target": target, "kind": kind})
            stats[f"data-{kind}"] += 1
    return events, cells, stats


# --- attribution + verdicts ---------------------------------------------------
def build_extents():
    """[(rva, cov_end, sym, src)] sorted. Coverage = the DECLARED size (reviewed
    symbol_names.csv size, else globals.json sizeof), clamped to the next symbol;
    with no declared size the whole next-symbol gap is the (uncertain) extent.
    Accesses PAST coverage are never charged to the symbol - they are candidate
    missing globals (the unclaimed runs report)."""
    syms = load_data_symbols()
    gsizes = {}
    if Path(GLOBALS).exists():
        for g in json.loads(Path(GLOBALS).read_text()):
            sz = (g.get("size") or "").strip()
            if sz:
                gsizes[int(g["rva"], 16)] = int(sz, 16)
    out = []
    for i, s in enumerate(syms):
        nxt = syms[i + 1].rva if i + 1 < len(syms) else None
        declared = s.reviewed_size or gsizes.get(s.rva)
        if declared:
            end, src = s.rva + declared, "decl"
            if nxt is not None and end > nxt:
                end, src = nxt, "decl-overlap"     # declared size crosses the
        elif nxt is not None:                      # next symbol: its own defect
            end, src = nxt, "gap"
        else:
            end, src = s.rva + 4, "unknown"
        out.append((s.rva, end, s, src))
    return out


def owner_of(extents, starts, rva):
    k = bisect.bisect_right(starts, rva) - 1
    if k < 0:
        return None
    lo, hi, sym, src = extents[k]
    return (sym, rva - lo, src, rva < hi)


def aggregate(events, cells, extents):
    starts = [e[0] for e in extents]
    per = defaultdict(lambda: {"events": [], "cells_in": [], "pointed_at": set()})
    unclaimed = []
    for ev in events:
        if ev.mode in ("undecoded", "iat"):
            continue
        hit = owner_of(extents, starts, ev.target)
        if hit is not None and hit[3]:
            per[hit[0].rva]["events"].append(ev)
        else:
            unclaimed.append(ev)
    for c in cells:
        hit = owner_of(extents, starts, c["site"])
        if hit and hit[3]:
            per[hit[0].rva]["cells_in"].append(c)
        t = owner_of(extents, starts, c["target"])
        if t and t[3] and t[1] > 0:
            per[t[0].rva]["pointed_at"].add(t[1])
    return per, unclaimed


class TypeOracle:
    """Resolve a declared-type string against structs.json: primitive sizes,
    and whether the type LEGITIMATELY contains float/pointer members (so an
    AFX_MSGMAP holding pointers or a ClipVtx holding floats never flags).
    Unknown type names get NO content flags - conservative, never accuse."""
    _PRIM = {"i8": 1, "u8": 1, "char": 1, "BYTE": 1, "bool": 1,
             "i16": 2, "u16": 2, "short": 2, "WORD": 2,
             "i32": 4, "u32": 4, "int": 4, "long": 4, "DWORD": 4, "UINT": 4,
             "LONG": 4, "ULONG": 4, "HRESULT": 4, "BOOL": 4,
             "i64": 8, "u64": 8, "float": 4, "double": 8}

    def __init__(self, path):
        self.structs = {}
        p = Path(path)
        if p.exists():
            for s in json.loads(p.read_text()):
                self.structs[s["name"]] = s

    @staticmethod
    def base(t):
        t = re.sub(r"\b(const|struct|unsigned|signed)\b", "", t or "").strip()
        return re.sub(r"\[[0-9]*\]", "", t).strip()    # array -> element type

    def is_ptr(self, t):
        return self.base(t).endswith("*") or "(" in (t or "")

    def prim_size(self, t):
        b = self.base(t)
        return 4 if b.endswith("*") else self._PRIM.get(b)

    def _fields(self, name, depth=0):
        s = self.structs.get(name)
        if not s or depth > 3:
            return
        for f in s["fields"]:
            yield f
            yield from self._fields(self.base(f["type"]), depth + 1)

    def known(self, t):
        b = self.base(t)
        return bool(t) and (b.endswith("*") or b in self._PRIM or b in self.structs)

    def _unresolved(self, t):
        """A field type we can't see through (e.g. the AFX_PMSG / VariantCallback
        fn-ptr typedefs): MAY be a pointer/float - never flag through it."""
        b = self.base(t)
        return not (b.endswith("*") or "(" in t or b in self._PRIM
                    or b in self.structs)

    def float_ok(self, t):
        b = self.base(t)
        return b in ("float", "double") or any(
            self.base(f["type"]) in ("float", "double") or self._unresolved(f["type"])
            for f in self._fields(b))

    def ptr_ok(self, t):
        return self.is_ptr(t) or any(
            self.is_ptr(f["type"]) or self._unresolved(f["type"])
            for f in self._fields(self.base(t)))


_ORACLE = None


def oracle():
    global _ORACLE
    if _ORACLE is None:
        _ORACLE = TypeOracle(REPO / "build/gen/structs.json")
    return _ORACLE


def verdicts(sym, extent, src, agg):
    """Advisory flags: declared type vs observed accesses."""
    ty = oracle()
    t = sym.type_decl or ""
    flags = []
    evs = agg["events"]
    offs = sorted({e.target - sym.rva for e in evs if e.mode == "direct"})
    widths = {e.width for e in evs if e.width}
    known = ty.known(t)
    prim = ty.prim_size(t) if t and "[" not in t else None   # scalar primitive
    is_char = ty.base(t) in ("char", "u8", "i8", "BYTE", "bool")
    if src == "decl-overlap":
        flags.append("decl-overlap")
    if known and not ty.float_ok(t) and \
            any(e.fpu.startswith("f") for e in evs if e.fpu):
        flags.append("fpu-on-nonfloat")
    fnptr = any(c["kind"] == "fnptr" for c in agg["cells_in"]) or \
        any(e.mode == "indcall" for e in evs)
    dataptr = any(c["kind"] == "dataptr" for c in agg["cells_in"])
    if known and (fnptr or dataptr) and not ty.ptr_ok(t):
        flags.append("fnptr-content" if fnptr else "ptr-content")
    if prim:
        # wide READS of a sub-dword prim are the MSVC5 zero-extend idiom
        # (mov r32, dword + and 0xffff) - only wide WRITES, or a symbol with
        # NO narrow access at all, indict the declared width. Indexed accesses
        # at the tail are usually base±k aliases of the NEXT symbol (the
        # negative-addend spelling assert_relocs knows) - not counted here.
        wide_w = any(e.width and e.width > prim and "w" in e.rw
                     and e.mode == "direct" for e in evs)
        narrow_seen = any(e.width and e.width <= prim for e in evs)
        if wide_w or (widths and max(widths) > prim and not narrow_seen):
            flags.append("wide-on-prim")           # access wider than the type
        if prim == 4 and not is_char and \
                any(e.width in (1, 2) and "w" in e.rw and e.mode == "direct"
                    for e in evs):
            flags.append("narrow-write")           # sub-dword store into an i32
        if any(e.mode == "indexed" and e.target == sym.rva for e in evs):
            flags.append("indexed-scalar")         # used as a table base
        if agg["pointed_at"] and not is_char:
            flags.append("pointed-interior")
    return flags, offs, widths


# --- reporting ----------------------------------------------------------------
def fmt_widths(evs):
    c = Counter(e.width for e in evs if e.width)
    return " ".join(f"{w}:{n}" for w, n in sorted(c.items()))


def write_ledger(per, extents, path):
    cols = ["rva", "name", "unit", "type", "extent", "extent_src", "sites",
            "direct", "indexed", "imm", "lea", "indcall", "reads", "writes",
            "widths", "max_off", "n_offs", "fpu", "cells_in", "pointed_at",
            "flags"]
    rows = []
    for lo, hi, sym, src in extents:
        agg = per.get(sym.rva)
        if not agg:
            continue
        evs = agg["events"]
        flags, offs, _w = verdicts(sym, hi - lo, src, agg)
        modes = Counter(e.mode for e in evs)
        rows.append({
            "rva": f"0x{sym.rva:x}", "name": sym.name, "unit": sym.unit,
            "type": sym.type_decl, "extent": f"0x{hi - sym.rva:x}",
            "extent_src": src, "sites": len(evs),
            "direct": modes["direct"], "indexed": modes["indexed"],
            "imm": modes["imm"], "lea": modes["lea"], "indcall": modes["indcall"],
            "reads": sum(1 for e in evs if "r" in e.rw),
            "writes": sum(1 for e in evs if "w" in e.rw),
            "widths": fmt_widths(evs),
            "max_off": f"0x{max(offs):x}" if offs else "",
            "n_offs": len(offs),
            "fpu": " ".join(sorted({e.fpu for e in evs if e.fpu})),
            "cells_in": len(agg["cells_in"]),
            "pointed_at": " ".join(f"0x{o:x}" for o in sorted(agg["pointed_at"])[:8]),
            "flags": " ".join(flags),
        })
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=cols, delimiter="\t")
        w.writeheader()
        w.writerows(rows)
    return rows


def unclaimed_runs(pe, unclaimed, extents):
    """Group unclaimed accessed RVAs into candidate-global runs; peek the bytes
    to down-rank pooled string literals (imm-only + printable = the unpinnable
    string pool, docs/patterns pooled-literals) from real missing globals."""
    starts = [e[0] for e in extents]
    by_rva = defaultdict(list)
    for ev in unclaimed:
        by_rva[ev.target].append(ev)
    runs, cur = [], None
    for rva in sorted(by_rva):
        if cur and rva - cur["end"] <= 16:
            cur["end"] = rva + max((e.width or 4) for e in by_rva[rva])
            cur["events"] += by_rva[rva]
        else:
            cur = {"start": rva, "end": rva + max((e.width or 4) for e in by_rva[rva]),
                   "events": list(by_rva[rva])}
            runs.append(cur)
    for r in runs:
        k = bisect.bisect_right(starts, r["start"]) - 1
        r["prev_sym"] = extents[k][2].name if k >= 0 else ""
        o = pe.off(r["start"])
        blob = pe.data[o:o + min(24, r["end"] - r["start"])] if o else b""
        printable = blob and all(32 <= b < 127 or b in (0, 9, 10, 13) for b in blob)
        imm_only = all(e.mode in ("imm", "lea") for e in r["events"])
        r["kind"] = "string-pool" if (printable and imm_only) else "data"
    return runs


def print_symbol(ctx, per, extents, rva):
    from gruntz.core.symbols import owner
    db = ctx.symbols
    hit = next(((lo, hi, s, src) for lo, hi, s, src in extents if s.rva == rva), None)
    if hit is None:
        print(f"no DATA() symbol at 0x{rva:x}")
        return 1
    lo, hi, sym, src = hit
    agg = per.get(rva) or {"events": [], "cells_in": [], "pointed_at": set()}
    flags, offs, widths = verdicts(sym, hi - lo, src, agg)
    print(f"{sym.name}  [{sym.unit}]  0x{rva:x}  extent 0x{hi - lo:x} ({src})")
    print(f"  type: {sym.type_decl or '?'}   flags: {' '.join(flags) or '-'}")
    print(f"  direct offsets: {' '.join(f'0x{o:x}' for o in offs) or '-'}")
    for c in agg["cells_in"]:
        print(f"  cell +0x{c['site'] - rva:x}: {c['kind']} -> 0x{c['target']:x}")
    if agg["pointed_at"]:
        print("  pointed-at interior: " +
              " ".join(f"+0x{o:x}" for o in sorted(agg["pointed_at"])))
    for ev in sorted(agg["events"], key=lambda e: e.site):
        f = owner(ev.insn_rva, db.fstarts, db.fsize)
        nm = db.names.get(f, ("<gap>", ""))[0] if f else "<gap>"
        print(f"  0x{ev.insn_rva:06x}  {ev.mode:8} {ev.rw:2} w{ev.width:<2} "
              f"{ev.fpu or ev.ext or '':4} +0x{ev.target - rva:<4x} {ev.text:44} {nm}")
    return 0


def print_run(ctx, runs, cells, rva):
    """One unclaimed run: bytes, cells, every access site resolved to its fn."""
    from gruntz.core.symbols import owner
    db = ctx.symbols
    r = next((r for r in runs if r["start"] <= rva < r["end"]), None)
    if r is None:
        print(f"no unclaimed run covers 0x{rva:x}")
        return 1
    span = r["end"] - r["start"]
    print(f"run 0x{r['start']:06x} +0x{span:x}  kind={r['kind']}  "
          f"sites={len(r['events'])}  after {r['prev_sym']}")
    o = ctx.pe.off(r["start"])
    if o is not None:
        for i in range(0, min(span, 64), 16):
            row = ctx.pe.data[o + i:o + min(i + 16, span)]
            hexs = " ".join(f"{b:02x}" for b in row)
            asc = "".join(chr(b) if 32 <= b < 127 else "." for b in row)
            print(f"  +0x{i:03x}  {hexs:<47}  {asc}")
    for c in cells:
        if r["start"] <= c["site"] < r["end"]:
            print(f"  cell +0x{c['site'] - r['start']:x}: {c['kind']} -> 0x{c['target']:x}")
    units = Counter()
    for ev in sorted(r["events"], key=lambda e: (e.target, e.site)):
        f = owner(ev.insn_rva, db.fstarts, db.fsize)
        nm, unit = db.names.get(f, (None, None)) if f else (None, None)
        units[unit or "<gap>"] += 1
        print(f"  0x{ev.insn_rva:06x}  {ev.mode:8} {ev.rw:2} w{ev.width:<2} "
              f"{ev.fpu or ev.ext or '':4} +0x{ev.target - r['start']:<4x} "
              f"{ev.text:44} {nm or '<gap>'}")
    print("  units: " + ", ".join(f"{u}={n}" for u, n in units.most_common()))
    return 0


def write_queue(ctx, runs, path):
    """The campaign worklist: every data run, fingerprint-grouped."""
    from gruntz.core.symbols import owner
    db = ctx.symbols
    fps = defaultdict(list)
    for r in runs:
        if r["kind"] != "data":
            continue
        fp = (r["end"] - r["start"],
              tuple(sorted(Counter(e.width for e in r["events"] if e.width).items())))
        fps[fp].append(r)
    rows = []
    for gi, (fp, group) in enumerate(
            sorted(fps.items(), key=lambda kv: -len(kv[1]))):
        for r in group:
            units = Counter()
            for e in r["events"]:
                f = owner(e.insn_rva, db.fstarts, db.fsize)
                units[db.names.get(f, ("", "<gap>"))[1] if f else "<gap>"] += 1
            modes = Counter(e.mode for e in r["events"])
            rows.append({
                "group": gi, "instances": len(group),
                "start": f"0x{r['start']:x}", "span": f"0x{fp[0]:x}",
                "sites": len(r["events"]),
                "modes": " ".join(f"{k}:{v}" for k, v in sorted(modes.items())),
                "widths": " ".join(f"{w}:{n}" for w, n in fp[1]),
                "units": " ".join(f"{u}:{n}" for u, n in units.most_common(3)),
                "after": r["prev_sym"],
            })
    with path.open("w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys()), delimiter="\t")
        w.writeheader()
        w.writerows(rows)
    print(f"[data-access] queue: {len(rows)} runs in {len(fps)} groups -> {path}")
    return 0


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--rva", help="one symbol: every access site, resolved")
    ap.add_argument("--run", help="one unclaimed run: bytes + every site resolved")
    ap.add_argument("--queue", action="store_true",
                    help="write the fingerprint-grouped run worklist TSV")
    ap.add_argument("--unclaimed", action="store_true",
                    help="list unclaimed access runs (candidate missing globals)")
    ap.add_argument("--flagged", action="store_true",
                    help="print only flagged symbols from the sweep")
    ap.add_argument("-o", "--output", type=Path, default=OUTPUT)
    args = ap.parse_args(argv)

    ctx = get_context()
    events, cells, stats = sweep(ctx)
    extents = build_extents()
    per, unclaimed = aggregate(events, cells, extents)

    if args.rva:
        return print_symbol(ctx, per, extents, int(args.rva, 16))
    if args.run:
        return print_run(ctx, unclaimed_runs(ctx.pe, unclaimed, extents), cells,
                         int(args.run, 16))
    if args.queue:
        return write_queue(ctx, unclaimed_runs(ctx.pe, unclaimed, extents),
                           REPO / "build/gen/data_access_queue.tsv")

    rows = write_ledger(per, extents, args.output)
    runs = unclaimed_runs(ctx.pe, unclaimed, extents)

    # set accounting: every .text->data reloc lands in exactly one bucket
    n_text = sum(v for k, v in stats.items() if k.startswith("text-")
                 and k not in ("text-to-text", "text-to-other"))
    attributed = sum(r["sites"] for r in rows)
    n_und = stats["text-undecoded"]
    n_iat = stats["text-iat"]
    print(f"[data-access] .text->data reloc sites: {n_text} "
          f"(attributed {attributed}, unclaimed {len(unclaimed)}, iat {n_iat}, "
          f"undecoded {n_und})")
    print("[data-access] modes: " + ", ".join(
        f"{k[5:]}={v}" for k, v in sorted(stats.items()) if k.startswith("text-")))
    print("[data-access] data cells: " + ", ".join(
        f"{k[5:]}={v}" for k, v in sorted(stats.items()) if k.startswith("data-")))
    flagged = [r for r in rows if r["flags"]]
    fc = Counter(f for r in flagged for f in r["flags"].split())
    print(f"[data-access] {len(rows)} accessed symbols, {len(flagged)} flagged: " +
          ", ".join(f"{k}={v}" for k, v in sorted(fc.items())))
    rk = Counter(r["kind"] for r in runs)
    print(f"[data-access] unclaimed runs: {len(runs)} "
          f"(data={rk['data']}, string-pool={rk['string-pool']}) -> {args.output}")

    if args.unclaimed:
        for r in sorted((r for r in runs if r["kind"] == "data"),
                        key=lambda r: -len(r["events"]))[:120]:
            ws = fmt_widths(r["events"])
            modes = Counter(e.mode for e in r["events"])
            ms = " ".join(f"{k}:{v}" for k, v in sorted(modes.items()))
            print(f"  0x{r['start']:06x} +0x{r['end'] - r['start']:<5x} "
                  f"sites={len(r['events']):<4} [{ms}] widths[{ws}]  "
                  f"after {r['prev_sym']}")
    if args.flagged:
        for r in sorted(flagged, key=lambda r: -r["sites"]):
            print(f"  0x{int(r['rva'], 16):06x} {r['name'][:52]:52} "
                  f"[{r['flags']}] sites={r['sites']} widths[{r['widths']}] "
                  f"type={r['type'][:40]}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
