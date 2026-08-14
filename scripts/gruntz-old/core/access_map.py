#!/usr/bin/env python3
"""gruntz.core.access_map - THE static map of every data access retail's code makes.

The score cannot answer "which bytes does retail's code actually touch, and how
wide?", because WE choose the extent of every claim: a too-small claim always
scores 100 (objdiff only compares what we told it to compare). Model `float x`
where retail has `struct { float x, y; }` and the four bytes match, the section
reports exact, and `y` is silently unmodelled. `?g_idleGeom@@3PAUBzGeomPair@@A`
was an invented object with its members in the wrong order that matched X by
coincidence and was ultimately a phantom.

This module reads the RETAIL image and records, per instruction, WHAT data byte
range it touches and HOW WIDE. That is the evidence the score structurally
cannot produce.

SITE ENUMERATION IS THE .reloc HIGHLOW TABLE, not a from-scratch disassembly.
Every absolute operand in the image is relocated, so `.reloc` is a complete and
reliable index of *where* the absolute references are; we disassemble only at
each site, to learn the width and the addressing form. That sidesteps needing a
correct whole-image disassembly and it covers code we have not reconstructed -
which is the point.

DECODE. Two objdump passes over .text:
  linear    the whole section decoded from its start (desyncs on data-in-text)
  anchored  the same bytes with every byte OUTSIDE a recovered function extent
            overwritten by 0x90, so the decoder re-syncs exactly on each
            function start instead of drifting through an embedded jump table
A site is decoded by the anchored pass when it lies inside a recovered function,
else by the linear pass; a site neither pass can place stays `undecoded` and is
counted, never guessed.

FORMS (what the map can and cannot see):
  direct        `ds:addr`               reloc-anchored, the datum itself
  indexed       `[reg*s+addr]`          reloc-anchored, addr is a TABLE base and
                                        s is a hard element-size witness
  derived-disp  `[reg+disp]` after a
                proven `mov reg,&sym`   NOT in .reloc - recovered by a bounded,
                                        single-block forward propagation
  lea / imm     address-taken           the object escapes; width unknown
  indcall       `call/jmp [addr]`       the cell holds a function pointer
  iat           indcall into the IAT    an import, excluded from symbol evidence

STRUCTURAL BLIND SPOT, stated honestly: an access through a base register whose
value did not come from an absolute operand in the same basic block carries no
relocation and no local provenance, so it is invisible here. That is every
`this`-relative field access, every access through a pointer loaded FROM memory,
and every escape through a call argument. `derived-disp` recovers only the
single-block case. The map is exhaustive for ABSOLUTE references and partial for
register-relative ones; `coverage()` reports both numbers.
"""
from __future__ import annotations

import bisect
import re
import shutil
import sqlite3
import struct
import subprocess
from collections import Counter, defaultdict
from pathlib import Path

from gruntz.core.pe import IMAGEBASE, REPO

OBJDUMP = shutil.which("objdump") or "objdump"

SQLITE = REPO / "build/gen/data_access_map.sqlite"
TSV = REPO / "build/gen/data_access_map.tsv"

_INSN = re.compile(r"^\s*([0-9a-f]+):\s*((?:[0-9a-f]{2} )+)\s*(\S.*)?$")
_WIDTH = {"BYTE": 1, "WORD": 2, "DWORD": 4, "QWORD": 8, "TBYTE": 10, "FWORD": 6}
_WPTR = re.compile(r"\b(BYTE|WORD|DWORD|QWORD|TBYTE|FWORD) PTR\b")
_R32 = ("eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi")
_REG = re.compile(r"\b(e[abcd]x|e[sd]i|e[bs]p|[abcd][lhx]|[sd]i|[bs]p)\b")
_TERM = re.compile(r"([+-])?\s*([a-z0-9]+)(?:\*([1248]))?")
_REGWIDTH = {**{r: 4 for r in _R32},
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
# instructions that end a basic block for the derived-disp propagation
_XFER = re.compile(r"^(j\w+|call|ret\w*|loop\w*|iret\w*|int3?|hlt|leave)$")
# implicit register clobbers: the destination is not an operand, so the
# "ops[0] is our register" test cannot see them
_CLOBBER = {"mul": "eax edx", "imul": "eax edx", "div": "eax edx",
            "idiv": "eax edx", "cdq": "edx", "cwd": "edx", "cbw": "eax",
            "cwde": "eax", "lodsb": "eax esi", "lodsd": "eax esi",
            "lodsw": "eax esi", "stosb": "edi", "stosd": "edi", "stosw": "edi",
            "movsb": "esi edi", "movsd": "esi edi", "movsw": "esi edi",
            "scasb": "edi", "scasd": "edi", "scasw": "edi"}


class Access:
    """One decoded data reference. `width` 0 means the instruction takes the
    ADDRESS (lea/imm/push) rather than touching bytes, so it covers no range."""
    __slots__ = ("insn_rva", "insn_len", "mnemonic", "site_rva", "target_rva",
                 "width", "rw", "form", "base_reg", "index_reg", "scale", "disp",
                 "fpu", "ext", "origin", "text")

    def __init__(self, **kw):
        for k in self.__slots__:
            setattr(self, k, kw.get(k))

    @property
    def end_rva(self):
        return self.target_rva + (self.width or 0)


# --- disassembly --------------------------------------------------------------
class Decode:
    """A decoded view of .text: instruction starts, text, and branch targets."""

    def __init__(self, starts, lines, targets):
        self.starts, self.lines, self.targets = starts, lines, targets

    def at(self, rva):
        """(index, start, text) of the instruction CONTAINING rva, or None."""
        k = bisect.bisect_right(self.starts, rva) - 1
        return k if k >= 0 else None

    def length(self, k):
        return (self.starts[k + 1] - self.starts[k]
                if k + 1 < len(self.starts) else 1)


_JMP = re.compile(r"^(j\w+|loop\w*)\s+0x([0-9a-f]+)")


def _objdump(blob: bytes, vma: int, tag: str) -> Decode:
    tmp = REPO / "build" / f".am_{tag}.bin"
    tmp.parent.mkdir(exist_ok=True)
    tmp.write_bytes(blob)
    try:
        out = subprocess.run(
            [OBJDUMP, "-D", "-b", "binary", "-m", "i386", "-Mintel",
             f"--adjust-vma=0x{vma:x}", str(tmp)],
            capture_output=True, text=True, check=True).stdout
    finally:
        tmp.unlink(missing_ok=True)
    starts, lines, targets = [], [], set()
    for ln in out.splitlines():
        m = _INSN.match(ln)
        if not m or m.group(3) is None:      # byte-continuation of the previous
            continue
        starts.append(int(m.group(1), 16))
        text = m.group(3).strip()
        lines.append(text)
        j = _JMP.match(text)
        if j:
            targets.add(int(j.group(2), 16))
    return Decode(starts, lines, targets)


def disasm_text(pe, fstarts=None, fsize=None):
    """(linear, anchored) decodes of .text.

    `anchored` overwrites every byte outside a recovered function extent with
    0x90 so the decoder cannot drift past a function start; 0x90 is 1 byte, so
    every function start stays on an instruction boundary no matter the gap
    parity. Returns (linear, None) when no function inventory is supplied.
    """
    name, va, vsz, rp, rsz = next(s for s in pe.secs if s[0] == ".text")
    blob = pe.data[rp:rp + rsz]
    linear = _objdump(blob, va, "lin")
    if not fstarts:
        return linear, None
    keep = bytearray(b"\x90" * len(blob))
    hi = va + len(blob)
    for f in fstarts:
        if not (va <= f < hi):
            continue
        n = fsize.get(f) or 0
        if not n:
            continue
        a, b = f - va, min(f - va + n, len(blob))
        keep[a:b] = blob[a:b]
    return linear, _objdump(bytes(keep), va, "anc")


# --- operand decoding ---------------------------------------------------------
def split_operands(asm):
    """(mnemonic, [operand, ...]) - Intel syntax, no commas inside brackets."""
    asm = asm.split(" <")[0]                 # strip objdump's <offset+N> notes
    asm = asm.split("#")[0].strip()          # strip objdump's trailing comment
    parts = asm.split(None, 1)
    if len(parts) == 1:
        return parts[0], []
    mnem = parts[0]
    if mnem in ("rep", "repz", "repnz", "repe", "repne", "lock"):
        return split_operands(parts[1])
    return mnem, [o.strip() for o in parts[1].split(",")]


def parse_mem(op):
    """(width, base, index, scale, disp) for one memory operand, else None.

    `disp` is the SUM of every numeric term in the effective address (the
    absolute VA plus any literal displacement), so a `[eax*4+0x5f1234]` and a
    `[eax+0x8]` are described by the same three fields."""
    if "[" not in op and "ds:" not in op and "PTR" not in op:
        return None
    m = _WPTR.search(op)
    width = _WIDTH[m.group(1)] if m else 0
    if "[" in op:
        inner = op[op.index("[") + 1:op.rindex("]")]
    else:
        inner = op.split("ds:")[-1].strip()
    base = index = ""
    scale = 0
    disp = 0
    for sign, tok, sc in _TERM.findall(inner.replace(" ", "")):
        if not tok:
            continue
        neg = sign == "-"
        if tok in _REGWIDTH:
            if sc:
                index, scale = tok, int(sc)
            elif not base:
                base = tok
            else:
                index, scale = tok, 1
            continue
        try:
            v = int(tok, 16) if tok.startswith("0x") else int(tok, 16)
        except ValueError:
            continue
        disp += -v if neg else v
    return width, base, index, scale, disp


def _direction(mnem, opidx, ops):
    if mnem in ("movzx", "movsx"):
        return "r"
    if mnem in _FPU_STORE:
        return "w"
    if mnem in _FPU_LOAD:
        return "r"
    if opidx != 0:
        return "r"
    if mnem in _READ_FIRST:
        return "r"
    if mnem in _RMW:
        return "rw"
    if mnem == "pop" or mnem.startswith("set") or mnem.startswith("mov"):
        return "w"
    return "rw"                              # unknown mnemonic, memory dest


def _fpu_tag(mnem, width):
    if mnem in _FPU_INT:
        return f"i{width * 8}" if width else "i?"
    return {4: "f32", 8: "f64", 10: "f80"}.get(width, "f?")


_ANDMASK = {"0xffff": 2, "0xff": 1}


def _and_mask(dec, k, reg, window=6):
    """`mov r32,[mem]` + `and r32,0xffff` is MSVC5's movzx-avoidance.

    cl 5.0 loads a narrow global with a FULL-WIDTH read and masks the register
    (movzx was slow on the Pentium), so the 4-byte access is a 2-byte one in
    disguise. Return the masked width, or 0 if the register is not masked
    before it is redefined / the block ends. Ambiguous by construction - a real
    `u32 & 0xffff` looks the same - so callers must SUPPRESS on it, never
    rewrite the recorded width."""
    for j in range(k + 1, min(k + 1 + window, len(dec.starts))):
        rva, asm = dec.starts[j], dec.lines[j]
        if rva in dec.targets:
            break
        mnem, ops = split_operands(asm)
        if _XFER.match(mnem):
            break
        if len(ops) == 2 and ops[0] == reg:
            if mnem == "and":
                return _ANDMASK.get(ops[1], 0)
            return 0                         # redefined without a mask
        if reg in _CLOBBER.get(mnem, ""):
            break
    return 0


def classify(site, stored, dec, k, iat):
    """Decode the instruction at index `k` into an Access for reloc `site`."""
    target = stored - IMAGEBASE
    insn_rva, asm = (dec.starts[k], dec.lines[k]) if k is not None else (site, "")
    ac = Access(insn_rva=insn_rva, insn_len=dec.length(k) if k is not None else 0,
                mnemonic="", site_rva=site, target_rva=target, width=0, rw="-",
                form="undecoded", base_reg="", index_reg="", scale=0, disp=0,
                fpu="", ext="", origin="reloc", text=asm)
    va_hex = f"0x{stored:x}"
    if va_hex not in asm:
        return ac                            # desync / data-in-text: never guess
    mnem, ops = split_operands(asm)
    ac.mnemonic = mnem
    opidx = next((i for i, o in enumerate(ops) if va_hex in o), None)
    if opidx is None:
        return ac
    op = ops[opidx]
    if mnem == "lea":
        mem = parse_mem(op)
        ac.form, ac.rw = "lea", "-"
        if mem:
            _w, ac.base_reg, ac.index_reg, ac.scale, _d = mem
        return ac
    mem = parse_mem(op)
    if mem is None:
        ac.form, ac.rw = "imm", "-"          # push 0xVA / mov reg,0xVA
        return ac
    width, base, index, scale, disp = mem
    if not width and len(ops) == 2:
        width = _REGWIDTH.get(ops[1 - opidx], 0)   # moffs prints no PTR keyword
    ac.width, ac.base_reg, ac.index_reg, ac.scale = width, base, index, scale
    ac.disp = disp - stored                  # literal displacement past the VA
    ac.form = "indexed" if (base or index) else "direct"
    if mnem in ("call", "jmp"):
        ac.form = "iat" if iat[0] <= target < iat[1] else "indcall"
        ac.rw = "r"
        return ac
    ac.rw = _direction(mnem, opidx, ops)
    if mnem in ("movzx", "movsx"):
        ac.ext = "u" if mnem == "movzx" else "i"
    elif mnem == "mov" and opidx == 1 and ac.rw == "r" and width == 4 \
            and k is not None and _REGWIDTH.get(ops[0]) == 4:
        m = _and_mask(dec, k, ops[0])
        if m:
            ac.ext = f"m{m}"                 # movzx-avoidance: a narrow load
    if mnem in _FPU_LOAD or mnem in _FPU_STORE:
        ac.fpu = _fpu_tag(mnem, width)
    return ac


# --- derived (register-relative) accesses -------------------------------------
_MOVIMM = re.compile(r"^mov\s+(e[a-z][a-z]),\s*0x([0-9a-f]+)$")


def derive(dec, seeds, in_data, stop=frozenset(), budget=48):
    """Recover `[reg+disp]` accesses whose base came from an absolute operand.

    A seed is a `mov r32,&sym` / `lea r32,[&sym]` at instruction index k. We walk
    forward inside the SAME basic block - stopping at any control transfer, at
    any instruction that is a branch target or a function start, and at any
    write to the base register (explicit operand OR implicit clobber) - and emit
    an Access for each memory operand based on that register. Single-block only,
    no register copies: deliberately conservative, because a wrong provenance
    would INVENT a data reference, which is worse than missing one.
    """
    out = []
    for k, reg, addr in seeds:
        for j in range(k + 1, min(k + 1 + budget, len(dec.starts))):
            rva, asm = dec.starts[j], dec.lines[j]
            if rva in dec.targets or rva in stop:
                break                        # a join: the register may differ
            mnem, ops = split_operands(asm)
            if _XFER.match(mnem):
                break
            if reg in _CLOBBER.get(mnem, ""):
                break
            for i, op in enumerate(ops):
                mem = parse_mem(op)
                if not mem or mem[1] != reg or mem[2]:
                    continue                 # base must be OUR reg, no index
                width, base, index, scale, disp = mem
                t = addr + disp
                if not in_data(t):
                    continue
                if not width and len(ops) == 2:
                    width = _REGWIDTH.get(ops[1 - i], 0)
                out.append(Access(
                    insn_rva=rva, insn_len=dec.length(j), mnemonic=mnem,
                    site_rva=0, target_rva=t, width=width,
                    rw=_direction(mnem, i, ops), form="derived-disp",
                    base_reg=reg, index_reg="", scale=0, disp=disp,
                    fpu=_fpu_tag(mnem, width)
                    if mnem in _FPU_LOAD or mnem in _FPU_STORE else "",
                    ext="u" if mnem == "movzx" else
                        "i" if mnem == "movsx" else "",
                    origin="derived", text=asm))
            # the register is redefined -> our provenance ends
            if ops and ops[0].strip() == reg and mnem not in _READ_FIRST:
                break
    return out


def seeds_from(dec, accesses):
    """Seed list for derive(): every lea/imm access that lands an address in a
    register. `push 0xVA` and `mov [mem],0xVA` are excluded - the address leaves
    the block, so its later use has no local provenance."""
    idx = {r: i for i, r in enumerate(dec.starts)}
    out = []
    for ac in accesses:
        if ac.form not in ("lea", "imm"):
            continue
        k = idx.get(ac.insn_rva)
        if k is None:
            continue
        mnem, ops = split_operands(ac.text)
        if not ops:
            continue
        dst = ops[0].strip()
        if dst not in _R32:
            continue
        if mnem == "lea" and (ac.base_reg or ac.index_reg):
            continue                         # lea r,[base+&sym]: not a pure addr
        if mnem not in ("lea", "mov"):
            continue
        out.append((k, dst, ac.target_rva))
    return out


# --- the sweep ----------------------------------------------------------------
def _iat_range(pe):
    rva = struct.unpack_from("<I", pe.data, pe._opt + 96 + 12 * 8)[0]
    sz = struct.unpack_from("<I", pe.data, pe._opt + 96 + 12 * 8 + 4)[0]
    return (rva, rva + sz) if rva else (0, 0)


def data_ranges(pe):
    out = []
    for name, va, vsz, rp, rsz in pe.secs:
        if name in (".rdata", ".data", ".bss", ".data1", ".idata"):
            out.append((name, va, va + max(vsz, rsz)))
    return out


def sweep(ctx):
    """(accesses, cells, stats) over the whole retail image."""
    pe = ctx.pe
    db = ctx.symbols
    dr = data_ranges(pe)
    iat = _iat_range(pe)
    tname, tva, tvsz, trp, trsz = next(s for s in pe.secs if s[0] == ".text")
    tlo, thi = tva, tva + max(tvsz, trsz)

    lows = [lo for _, lo, _ in dr]
    def in_data(rva):
        k = bisect.bisect_right(lows, rva) - 1
        return k >= 0 and rva < dr[k][2]

    linear, anchored = disasm_text(pe, db.fstarts, db.fsize)
    fstarts = db.fstarts

    def pick(site):
        """(decode, index) - the anchored decode inside a recovered function,
        the linear one otherwise; whichever actually places the site wins."""
        i = bisect.bisect_right(fstarts, site) - 1
        inside = False
        if i >= 0:
            n = db.fsize.get(fstarts[i]) or 0
            inside = bool(n) and site < fstarts[i] + n
        order = ((anchored, linear) if inside and anchored else (linear, anchored))
        for dec in order:
            if dec is None:
                continue
            k = dec.at(site)
            if k is None:
                continue
            yield dec, k

    def in_fn(rva):
        i = bisect.bisect_right(fstarts, rva) - 1
        if i < 0:
            return False
        n = db.fsize.get(fstarts[i]) or 0
        return bool(n) and rva < fstarts[i] + n

    stats = Counter()
    accesses, cells = [], []
    for site, stored in pe.relocs_in(tlo, thi):
        target = stored - IMAGEBASE
        if not in_data(target):
            if iat[0] <= target < iat[1]:
                stats["to-iat"] += 1
            elif tlo <= target < thi:
                stats["to-text"] += 1
            else:
                stats["to-other"] += 1
            continue
        best = None
        for dec, k in pick(site):
            ac = classify(site, stored, dec, k, iat)
            if ac.form != "undecoded":
                best = ac
                break
            best = best or ac
        if best.form == "undecoded" and not in_fn(site):
            # not an instruction operand at all: a relocated POINTER CELL that
            # lives in .text - a data table the linker placed in the code
            # section. It does not TOUCH the target, it HOLDS its address.
            cells.append({"site": site, "target": target, "kind": "dataptr",
                          "where": ".text"})
            stats["cell-in-text"] += 1
            continue
        accesses.append(best)
        stats[f"form-{best.form}"] += 1

    # derived pass: run on BOTH decodes' seeds, dedup by (insn_rva, target)
    seen = set()
    derived = []
    stop = frozenset(fstarts)
    for dec in (anchored, linear):
        if dec is None:
            continue
        sd = seeds_from(dec, accesses)
        if dec is (anchored or linear):
            stats["seed-total"] = len(sd)
            # a register load followed within 3 instructions by a call is an
            # object handed to a CALLEE: every field access it makes is
            # `this`-relative and therefore invisible to this map
            stats["seed-handed-to-callee"] = sum(
                1 for k, _r, _a in sd
                if any(dec.lines[j].startswith("call")
                       for j in range(k + 1, min(k + 4, len(dec.lines)))))
        for ac in derive(dec, sd, in_data, stop):
            key = (ac.insn_rva, ac.target_rva, ac.width)
            if key in seen:
                continue
            seen.add(key)
            derived.append(ac)
    stats["form-derived-disp"] = len(derived)
    accesses.extend(derived)

    for sec, lo, hi in dr:
        for site, stored in pe.relocs_in(lo, hi):
            t = stored - IMAGEBASE
            kind = ("fnptr" if tlo <= t < thi
                    else "dataptr" if in_data(t) else "otherptr")
            cells.append({"site": site, "target": t, "kind": kind, "where": sec})
            stats[f"cell-{kind}"] += 1
    return accesses, cells, stats


# --- claims + a flattened field map -------------------------------------------
_ARRAY = re.compile(r"\[(\d*)\]")


class Types:
    """structs.json + enums.json + the primitive table, flattened to
    (offset, size, path, type, resolved) rows.

    `resolved` is the honesty flag. When a field's type cannot be sized, its
    extent is inferred from the NEXT field's offset (or the struct size) so the
    field map stays complete - but the row is marked unresolved and every width
    verdict skips it. Accusing a declared type we cannot read would be a
    fabricated finding."""
    PRIM = {"i8": 1, "u8": 1, "char": 1, "BYTE": 1, "bool": 1, "CHAR": 1,
            "i16": 2, "u16": 2, "short": 2, "WORD": 2, "SHORT": 2, "USHORT": 2,
            "i32": 4, "u32": 4, "int": 4, "long": 4, "DWORD": 4, "UINT": 4,
            "LONG": 4, "ULONG": 4, "HRESULT": 4, "BOOL": 4, "COLORREF": 4,
            "i64": 8, "u64": 8, "float": 4, "double": 8}
    BYTEISH = {"i8", "u8", "char", "BYTE", "CHAR", "bool"}

    def __init__(self, path, enums=None):
        import json
        self.structs = {}
        self.enums = set()
        p = Path(path)
        if p.exists():
            for s in json.loads(p.read_text()):
                self.structs[s["name"]] = s
        e = Path(enums) if enums else Path(path).with_name("enums.json")
        if e.exists():
            # MSVC 5.0 sizes every enum as 4 bytes (docs/enum-modeling-plan.md)
            self.enums = {x["name"] for x in json.loads(e.read_text())}

    @staticmethod
    def strip(t):
        t = re.sub(r"\b(const|struct|class|union|unsigned|signed|volatile)\b",
                   "", t or "").strip()
        return re.sub(r"\s+", " ", t).strip()

    @staticmethod
    def dims(t):
        """([n, ...], element_type_text) for `T[8][4]`."""
        return [int(d) if d else 0 for d in _ARRAY.findall(t or "")], \
            _ARRAY.sub("", t or "").strip()

    def sizeof(self, t):
        t = self.strip(t)
        dims, base = self.dims(t)
        n = 1
        for d in dims:
            n *= (d or 1)
        b = self.base_size(base)
        return None if b is None else b * n

    def base_size(self, base):
        base = self.strip(base)
        if base.endswith("*") or "(" in base:
            return 4
        if base in self.PRIM:
            return self.PRIM[base]
        if base in self.enums:
            return 4
        s = self.structs.get(base)
        return s["size"] if s else None

    def is_byteish(self, t):
        return self.strip(self.dims(t)[1]) in self.BYTEISH

    def is_ptr(self, t):
        t = self.strip(t)
        return t.endswith("*") or "(" in t

    def is_float(self, t):
        return self.strip(self.dims(t)[1]) in ("float", "double")

    def field_at(self, t, off, depth=0, path=""):
        """Resolve a byte offset into a declared type ARITHMETICALLY.

        Returns (field_off, size, path, leaf_type, resolved) for the field that
        covers `off`, or one of the sentinels:
          ("out",   ...) off is past the declared type
          ("hole",  ...) off falls in a gap BETWEEN declared fields
          ("vptr",  ...) off precedes the first declared field - structs.json
                         systematically omits the vptr of a polymorphic class,
                         so this is a tool blind spot, not a layout defect
        Arithmetic, not table lookup, so a 196610-byte array resolves at any
        offset without flattening 196610 rows."""
        t = self.strip(t)
        dims, base = self.dims(t)
        bsz = self.base_size(base)
        if bsz is None or bsz <= 0 or depth > 4:
            return None
        total = 1
        for d in dims:
            total *= (d or 1)
        if off < 0 or off >= bsz * total:
            return ("out", off, 0, path, base, 1)
        i, rem = divmod(off, bsz)
        idx = ""
        if dims:
            r, parts = i, []
            for d in reversed(dims):
                parts.append(r % (d or 1))
                r //= (d or 1)
            idx = "".join(f"[{p}]" for p in reversed(parts))
        sub = self.structs.get(base)
        if sub and sub.get("fields"):
            fs = sorted(sub["fields"], key=lambda f: f["offset"])
            if rem < fs[0]["offset"]:
                return ("vptr", off - rem, fs[0]["offset"], f"{path}{idx}",
                        base, 1)
            k = 0
            for j, f in enumerate(fs):
                if f["offset"] <= rem:
                    k = j
            f = fs[k]
            nxt = fs[k + 1]["offset"] if k + 1 < len(fs) else sub.get("size", 0)
            inner = self.field_at(f["type"], rem - f["offset"], depth + 1,
                                  f"{path}{idx}.{f['name']}")
            if inner is None:
                # unsizeable member: it owns everything up to the next field
                span = max(nxt - f["offset"], 0)
                if rem - f["offset"] < span:
                    return (off - (rem - f["offset"]), span,
                            f"{path}{idx}.{f['name']}", f["type"], 0)
                return ("hole", off, 0, f"{path}{idx}", base, 1)
            if inner[0] == "out":
                return ("hole", off, 0, f"{path}{idx}", base, 1)
            if isinstance(inner[0], str):
                return inner
            foff, fsz, fpath, fty, res = inner
            return (off - rem + f["offset"] + foff, fsz, fpath, fty, res)
        return (off - rem, bsz, f"{path}{idx}" or ".", base, 1)

    def flatten(self, t, base_off=0, depth=0, path="", hint=None):
        """[(offset, size, path, leaf_type, resolved)] for a declared type.

        Arrays expand element by element with an `[i]` path so a stride or a
        per-element width check has a shape to compare against (bounded, so a
        huge table does not explode the map). A field whose type cannot be
        sized still gets a row - sized by `hint`, the distance to the next
        declared field - flagged `resolved=0` so no verdict fires through it."""
        t = self.strip(t)
        dims, base = self.dims(t)
        bsz = self.base_size(base)
        resolved = bsz is not None
        if bsz is None:
            bsz = hint or 0
            if bsz <= 0:
                return []
        total = 1
        for d in dims:
            total *= (d or 1)
        rows = []
        cap = 2048                            # never explode a huge table
        for i in range(min(total, cap)):
            off = base_off + i * bsz
            idx = ""
            if dims:
                rem, parts = i, []
                for d in reversed(dims):
                    parts.append(rem % (d or 1))
                    rem //= (d or 1)
                idx = "".join(f"[{p}]" for p in reversed(parts))
            sub = self.structs.get(base) if resolved else None
            if sub and depth < 4 and sub.get("fields"):
                fs = sorted(sub["fields"], key=lambda f: f["offset"])
                for j, f in enumerate(fs):
                    nxt = (fs[j + 1]["offset"] if j + 1 < len(fs)
                           else sub.get("size") or f["offset"])
                    rows.extend(self.flatten(
                        f["type"], off + f["offset"], depth + 1,
                        f"{path}{idx}.{f['name']}", max(nxt - f["offset"], 0)))
            else:
                rows.append((off, bsz, f"{path}{idx}" or ".", base,
                             int(resolved)))
        return rows


# --- claims (our side) --------------------------------------------------------
class Claim:
    """One DATA() symbol with its declared extent and flattened field map."""
    __slots__ = ("rva", "name", "unit", "type", "section", "extent",
                 "extent_src", "fields", "pct")

    def __init__(self, **kw):
        for k in self.__slots__:
            setattr(self, k, kw.get(k))

    @property
    def end(self):
        return self.rva + self.extent


def build_claims(types, pe):
    """Every DATA() symbol, extent-resolved and field-flattened.

    Coverage = the reviewed symbol_names.csv size when present, else the
    declared type's sizeof, else the next-symbol gap; always clamped to the next
    symbol, because a declared size that crosses its neighbour is that symbol's
    own defect and must never swallow the neighbour's accesses."""
    from gruntz.core.data_audit import load_data_symbols
    syms = load_data_symbols()
    out = []
    for i, s in enumerate(syms):
        nxt = syms[i + 1].rva if i + 1 < len(syms) else None
        declared = s.reviewed_size or types.sizeof(s.type_decl)
        if declared:
            end, src = s.rva + declared, "declared"
            if nxt is not None and end > nxt:
                end, src = nxt, "declared-overlap"
        elif nxt is not None:
            end, src = nxt, "next-symbol-gap"
        else:
            end, src = s.rva + 4, "unknown"
        out.append(Claim(rva=s.rva, name=s.name, unit=s.unit,
                         type=s.type_decl or "", section=pe.sec_name(s.rva) or "",
                         extent=max(end - s.rva, 0), extent_src=src,
                         fields=types.flatten(s.type_decl) if s.type_decl else []))
    return out


# --- persistence --------------------------------------------------------------
SCHEMA = """
PRAGMA journal_mode=OFF;
CREATE TABLE meta   (key TEXT PRIMARY KEY, value TEXT);
CREATE TABLE access (
  id INTEGER PRIMARY KEY, insn_rva INT, insn_len INT, mnemonic TEXT,
  site_rva INT, target_rva INT, width INT, end_rva INT, rw TEXT, form TEXT,
  base_reg TEXT, index_reg TEXT, scale INT, disp INT, fpu TEXT, ext TEXT,
  origin TEXT, fn_rva INT, fn_name TEXT, fn_unit TEXT,
  sym_rva INT, sym_name TEXT, sym_off INT, in_extent INT, text TEXT);
CREATE TABLE cell   (
  id INTEGER PRIMARY KEY, site_rva INT, target_rva INT, kind TEXT, where_sec TEXT,
  sym_rva INT, sym_name TEXT, sym_off INT,
  tgt_sym_rva INT, tgt_sym_name TEXT, tgt_sym_off INT);
CREATE TABLE claim  (
  rva INT PRIMARY KEY, name TEXT, unit TEXT, type TEXT, section TEXT,
  extent INT, extent_src TEXT, sect_pct REAL,
  n_access INT, n_read INT, n_write INT, n_addr INT, n_cells INT);
CREATE TABLE field  (
  sym_rva INT, off INT, size INT, path TEXT, type TEXT, is_ptr INT, is_float INT,
  resolved INT);
CREATE TABLE finding(
  id INTEGER PRIMARY KEY, category TEXT, severity TEXT, sym_rva INT,
  sym_name TEXT, addr INT, detail TEXT, evidence TEXT);
"""
INDEXES = """
CREATE INDEX ix_acc_target ON access(target_rva);
CREATE INDEX ix_acc_sym    ON access(sym_rva);
CREATE INDEX ix_acc_fn     ON access(fn_rva);
CREATE INDEX ix_acc_insn   ON access(insn_rva);
CREATE INDEX ix_cell_site  ON cell(site_rva);
CREATE INDEX ix_cell_tgt   ON cell(target_rva);
CREATE INDEX ix_fld_sym    ON field(sym_rva);
CREATE INDEX ix_fnd_cat    ON finding(category);
CREATE INDEX ix_fnd_sym    ON finding(sym_rva);
"""

TSV_COLS = ["insn_rva", "insn_len", "mnemonic", "site_rva", "target_rva",
            "width", "end_rva", "rw", "form", "base_reg", "index_reg", "scale",
            "disp", "fpu", "ext", "origin", "fn_rva", "fn_name", "fn_unit",
            "sym_rva", "sym_name", "sym_off", "in_extent", "text"]


def _locate(starts, claims, rva):
    """(claim, offset, in_extent) for a data RVA, or (None, -1, 0)."""
    k = bisect.bisect_right(starts, rva) - 1
    if k < 0:
        return None, -1, 0
    c = claims[k]
    return c, rva - c.rva, int(rva < c.end)


def persist(ctx, types, accesses, cells, claims, stats, sqlite_path=SQLITE,
            tsv_path=TSV, findings=()):
    """Write the map. sqlite is the query index; the TSV is the grep-able,
    diffable copy of the access table (the artifact a human reads)."""
    from gruntz.core.symbols import owner
    db = ctx.symbols
    starts = [c.rva for c in claims]
    by_rva = {c.rva: c for c in claims}

    arows, per = [], defaultdict(Counter)
    for ac in accesses:
        f = owner(ac.insn_rva, db.fstarts, db.fsize)
        fname, funit = db.names.get(f, ("", "")) if f else ("", "")
        c, off, inx = _locate(starts, claims, ac.target_rva)
        arows.append((ac.insn_rva, ac.insn_len, ac.mnemonic or "", ac.site_rva,
                      ac.target_rva, ac.width or 0, ac.end_rva, ac.rw, ac.form,
                      ac.base_reg or "", ac.index_reg or "", ac.scale or 0,
                      ac.disp or 0, ac.fpu or "", ac.ext or "", ac.origin,
                      f or 0, fname or "", funit or "",
                      c.rva if c else 0, c.name if c else "", off, inx,
                      ac.text or ""))
        if c and inx:
            p = per[c.rva]
            if ac.form in ("lea", "imm"):
                p["addr"] += 1
            else:
                p["access"] += 1
                if "r" in ac.rw:
                    p["read"] += 1
                if "w" in ac.rw:
                    p["write"] += 1

    crows = []
    for cl in cells:
        c, off, inx = _locate(starts, claims, cl["site"])
        t, toff, tinx = _locate(starts, claims, cl["target"])
        if c and inx:
            per[c.rva]["cells"] += 1
        crows.append((cl["site"], cl["target"], cl["kind"], cl.get("where", ""),
                      c.rva if c and inx else 0, c.name if c and inx else "",
                      off if inx else -1,
                      t.rva if t and tinx else 0, t.name if t and tinx else "",
                      toff if tinx else -1))

    sqlite_path.parent.mkdir(parents=True, exist_ok=True)
    if sqlite_path.exists():
        sqlite_path.unlink()
    con = sqlite3.connect(sqlite_path)
    con.executescript(SCHEMA)
    con.executemany(
        "INSERT INTO access(insn_rva,insn_len,mnemonic,site_rva,target_rva,"
        "width,end_rva,rw,form,base_reg,index_reg,scale,disp,fpu,ext,origin,"
        "fn_rva,fn_name,fn_unit,sym_rva,sym_name,sym_off,in_extent,text) "
        "VALUES(" + ",".join("?" * 24) + ")", arows)
    con.executemany(
        "INSERT INTO cell(site_rva,target_rva,kind,where_sec,sym_rva,sym_name,"
        "sym_off,tgt_sym_rva,tgt_sym_name,tgt_sym_off) VALUES("
        + ",".join("?" * 10) + ")", crows)
    con.executemany(
        "INSERT INTO claim(rva,name,unit,type,section,extent,extent_src,"
        "sect_pct,n_access,n_read,n_write,n_addr,n_cells) VALUES("
        + ",".join("?" * 13) + ")",
        [(c.rva, c.name, c.unit, c.type, c.section, c.extent, c.extent_src,
          c.pct if c.pct is not None else -1.0,
          per[c.rva]["access"], per[c.rva]["read"], per[c.rva]["write"],
          per[c.rva]["addr"], per[c.rva]["cells"]) for c in claims])
    con.executemany(
        "INSERT INTO field(sym_rva,off,size,path,type,is_ptr,is_float,resolved) "
        "VALUES(?,?,?,?,?,?,?,?)",
        [(c.rva, off, sz, path, ty, int(types.is_ptr(ty)),
          int(types.is_float(ty)), res)
         for c in claims for off, sz, path, ty, res in c.fields])
    con.executemany("INSERT INTO meta(key,value) VALUES(?,?)",
                    [(k, str(v)) for k, v in stats.items()])
    if findings:
        con.executemany(
            "INSERT INTO finding(category,severity,sym_rva,sym_name,addr,"
            "detail,evidence) VALUES(?,?,?,?,?,?,?)", findings)
    con.executescript(INDEXES)
    con.commit()
    con.close()

    tsv_path.parent.mkdir(parents=True, exist_ok=True)
    with tsv_path.open("w") as f:
        f.write("\t".join(TSV_COLS) + "\n")
        hexcols = {"insn_rva", "site_rva", "target_rva", "end_rva", "fn_rva",
                   "sym_rva"}
        for r in sorted(arows, key=lambda r: (r[4], r[0])):
            cells_out = []
            for name, v in zip(TSV_COLS, r):
                cells_out.append(f"0x{v:x}" if name in hexcols and isinstance(v, int)
                                 else str(v))
            f.write("\t".join(cells_out) + "\n")
    return len(arows), len(crows)


def connect(path=SQLITE):
    if not Path(path).exists():
        raise SystemExit(f"no access map at {path} - run "
                         f"`gruntz audit data_access_map --build` first")
    con = sqlite3.connect(f"file:{path}?mode=ro", uri=True)
    con.row_factory = sqlite3.Row
    return con
