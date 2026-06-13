#!/usr/bin/env python3
"""seh_extract.py - extract MSVC C++ exception-handling (EH) metadata from an
x86 PE and recover the code segments it describes.

PURPOSE
-------
GRUNTZ.EXE was built with Visual C++ 5.0 (cl 11.00). Its C++ exception support
leaves a dense, *self-describing* metadata web in `.rdata`: one `_s_FuncInfo`
record per function that contains a try/catch or has objects requiring unwind
(destructor) cleanup. That metadata names, with byte precision, the entry
points of every catch handler funclet and every unwind/destructor funclet, plus
the EH-state structure of each owning function.

We parse it directly to get an INDEPENDENT function-boundary source that does
NOT rely on Ghidra's control-flow recovery (which is flaky around switch/case
jump tables). The funclet entry RVAs recovered here are exactly the
"Catch@xxxx" / "Unwind@xxxx" labels Ghidra emits, so the overlap is a direct
correctness proof, and any funclet we find that Ghidra missed is a real
boundary Ghidra's analysis dropped.

THE x86 VC5 EH MODEL
--------------------
There is NO .pdata/.xdata on x86 (that is x64). Instead, a function that uses
C++ EH installs a frame handler at runtime via a tiny per-function THUNK in
`.text`:

        <thunk>:  B8 <imm32 FuncInfo VA>      mov  eax, offset _s_FuncInfo
                  E9 <rel32 __CxxFrameHandler> jmp  __CxxFrameHandler

The owning function's prologue does `push offset <thunk>; push fs:[0]; ...` to
chain an EXCEPTION_REGISTRATION node, so the OS dispatcher reaches
__CxxFrameHandler with eax = &FuncInfo. From FuncInfo the runtime drives the
state machine: unwind map (destructor cleanup actions, indexed by EH state) and
try-block map (try-state ranges + catch handler arrays + RTTI catch types).

EXTRACTION
----------
1. Parse the PE ourselves (stdlib `struct`): section table, image base, and the
   `.reloc` HIGHLOW set (the authoritative set of "this dword is an absolute
   relocated pointer" sites). The reloc set is used to validate that a decoded
   pointer field is a genuine reloc'd VA, killing false positives.
2. Scan `.rdata`/`.data` for the FuncInfo magic 0x19930520, decode `_s_FuncInfo`
   and follow `pUnwindMap` (maxState entries) and `pTryBlockMap` (nTryBlocks
   entries -> catch handler arrays -> RTTI TypeDescriptor names).
3. Recover catch-funclet and unwind-funclet entry RVAs.
4. Attribute each FuncInfo to its owning function: locate the `.text` thunk that
   loads the FuncInfo VA (`mov eax, imm32; jmp __CxxFrameHandler`), then the
   `push offset <thunk>` prologue site, and snap that to the nearest preceding
   known function start (best-effort; see ATTRIBUTION LIMITS below).

EHDATA.H PROVENANCE
-------------------
The canonical struct layouts live in Microsoft's `ehdata.h`. That header is NOT
shipped on VS97 Disc 3 (only the prebuilt `frame.obj` is). What the disc DOES
ship and what we extracted to build/seh/ref/ are the assembly-side definitions
that pin down the constants and the C-SEH scope-table model:
  * EXSUP.INC  -> MAGIC_NUMBER1 equ 019930520h   (confirms the FuncInfo magic)
  * EXCEPT.INC -> __EXCEPTIONREGISTRATIONRECORD, _JMP_BUF, _EH_UNWINDING, ...
  * TRNSCTRL.H -> the _M_IX86 EH funclet-calling contract referencing FuncInfo /
                  TryBlockMapEntry / __ehstate_t (names, not layout)
  * EH.H, HANDLER.CPP, TYPEINFO.H -> set_terminate / type_info surface.
The `_s_FuncInfo` / `_s_UnwindMapEntry` / `_s_TryBlockMapEntry` /
`_s_HandlerType` byte layouts below are the well-known VC5 (cl 11) layouts
(magicNumber, maxState, pUnwindMap, nTryBlocks, pTryBlockMap, then the two x86
IP-to-state fields which VC5 leaves zero). They were verified empirically
against this binary: pUnwindMap/pTryBlockMap fields land in `.reloc`, unwind
actions land in `.text`, catch handlers resolve to readable RTTI type names
(.PAVCException@@ etc.) and the recovered handler RVAs match Ghidra's Catch@.

ATTRIBUTION LIMITS / NUANCES
----------------------------
* VC5 x86 FuncInfo has NO usable IP-to-state map (nIPMapEntries / pIPtoStateMap
  are zero), so the owning function's EXACT byte extent is NOT encoded in EH
  metadata. We attribute via the thunk + push-prologue xref and snap to the
  nearest preceding function start; the extent is approximated as [start, next
  function start). Some FuncInfos (~19%) have no single `push offset thunk`
  immediate (shared/optimized prologue) and get no owning-func attribution -
  their funclets are still recovered, just unattributed.
* 0x19930520 is also the NLG debug cookie (_NLG_INFO.dwSig). Cookies sit in
  `.text` (or fail the maxState/nTry/pointer-in-reloc sanity), so they are
  filtered out; only well-formed `.rdata`/`.data` records are kept.
* __except_handler3 (C-style __try/__except) scope tables have no magic and are
  found only via xrefs to the handler thunk. They are SECONDARY and reported
  separately as a count of __except_handler3-style thunks; their scope-table
  walking is not implemented (the C++ FuncInfo path already yields the funclet
  boundaries this tool exists to cross-check).

USAGE
-----
    python3 scripts/seh_extract.py [--exe PATH] [--json PATH] [--summary PATH]
                                   [--ghidra-csv PATH] [--quiet]

Stdlib only. Outputs default to build/seh/seh_segments.json and
build/seh/summary.txt (both gitignored).
"""

from __future__ import annotations

import argparse
import bisect
import json
import os
import struct
import sys
from dataclasses import dataclass, field

# --------------------------------------------------------------------------
# VC5 ehdata.h constants / layout (see module docstring for provenance).
# --------------------------------------------------------------------------
FUNCINFO_MAGICS = (0x19930520, 0x19930521, 0x19930522)  # VC5 emits only 0520
SIZEOF_FUNCINFO = 0x1C          # magic,maxState,pUnwind,nTry,pTry,nIP,pIP
SIZEOF_UNWINDMAP_ENTRY = 8      # int toState; void(*action)()
SIZEOF_TRYBLOCKMAP_ENTRY = 20   # int tryLow,tryHigh,catchHigh,nCatches; HT* pHandlerArray
SIZEOF_HANDLERTYPE = 16         # uint adjectives; TD* pType; int dispCatchObj; void* addr

IMAGE_REL_BASED_HIGHLOW = 3

# x86 opcodes used in thunk / prologue recognition.
OP_MOV_EAX_IMM32 = 0xB8
OP_JMP_REL32 = 0xE9
OP_PUSH_IMM32 = 0x68

# Sanity bounds for FuncInfo fields (defensive false-positive rejection).
MAX_STATE_LIMIT = 5000
MAX_TRYBLOCKS_LIMIT = 2000
MAX_CATCHES_LIMIT = 256


# --------------------------------------------------------------------------
# Minimal PE32 parser
# --------------------------------------------------------------------------
@dataclass
class Section:
    name: str
    vaddr: int      # RVA
    vsize: int
    rawsize: int
    rawptr: int


class PE:
    """Just enough PE32 to map RVA<->file-offset and read the reloc table."""

    def __init__(self, data: bytes):
        self.data = data
        if data[:2] != b"MZ":
            raise ValueError("not an MZ image")
        e_lfanew = struct.unpack_from("<I", data, 0x3C)[0]
        if data[e_lfanew:e_lfanew + 4] != b"PE\x00\x00":
            raise ValueError("no PE signature")
        coff = e_lfanew + 4
        machine, nsec = struct.unpack_from("<HH", data, coff)
        if machine != 0x14C:
            raise ValueError("not x86 (machine=0x%x); x86 expected" % machine)
        opt_size = struct.unpack_from("<H", data, coff + 16)[0]
        opt = coff + 20
        opt_magic = struct.unpack_from("<H", data, opt)[0]
        if opt_magic != 0x10B:
            raise ValueError("not PE32 (opt magic=0x%x)" % opt_magic)
        self.image_base = struct.unpack_from("<I", data, opt + 28)[0]
        ndir = struct.unpack_from("<I", data, opt + 92)[0]
        self.data_dirs = []
        dd = opt + 96
        for i in range(ndir):
            va, sz = struct.unpack_from("<II", data, dd + i * 8)
            self.data_dirs.append((va, sz))

        self.sections: list[Section] = []
        sechdr = opt + opt_size
        for i in range(nsec):
            off = sechdr + i * 40
            name = data[off:off + 8].rstrip(b"\x00").decode("latin1")
            vsize, vaddr, rawsize, rawptr = struct.unpack_from("<IIII", data, off + 8)
            self.sections.append(Section(name, vaddr, vsize, rawsize, rawptr))

    # -- address helpers ---------------------------------------------------
    def section_of_rva(self, rva: int) -> Section | None:
        for s in self.sections:
            if s.vaddr <= rva < s.vaddr + max(s.vsize, s.rawsize):
                return s
        return None

    def rva_to_off(self, rva: int) -> int | None:
        s = self.section_of_rva(rva)
        if s is None:
            return None
        delta = rva - s.vaddr
        if delta >= s.rawsize:
            return None  # in uninitialized tail (e.g. .bss-like .data)
        return s.rawptr + delta

    def off_to_rva(self, off: int) -> int | None:
        for s in self.sections:
            if s.rawptr <= off < s.rawptr + s.rawsize:
                return s.vaddr + (off - s.rawptr)
        return None

    def va_to_rva(self, va: int) -> int:
        return va - self.image_base

    def rva_to_va(self, rva: int) -> int:
        return rva + self.image_base

    def read_at_rva(self, rva: int, n: int) -> bytes | None:
        off = self.rva_to_off(rva)
        if off is None:
            return None
        return self.data[off:off + n]

    def u32_at_rva(self, rva: int) -> int | None:
        b = self.read_at_rva(rva, 4)
        if b is None or len(b) < 4:
            return None
        return struct.unpack("<I", b)[0]

    def i32_at_rva(self, rva: int) -> int | None:
        b = self.read_at_rva(rva, 4)
        if b is None or len(b) < 4:
            return None
        return struct.unpack("<i", b)[0]

    # -- base relocations --------------------------------------------------
    def highlow_reloc_sites(self) -> set[int]:
        """Return the set of RVAs that hold a HIGHLOW (absolute dword) reloc."""
        sites: set[int] = set()
        if len(self.data_dirs) <= 5:
            return sites
        rva, size = self.data_dirs[5]
        if not rva or not size:
            return sites
        off = self.rva_to_off(rva)
        if off is None:
            return sites
        end = off + size
        p = off
        d = self.data
        while p + 8 <= end:
            page, blk = struct.unpack_from("<II", d, p)
            if blk < 8 or p + blk > end + 8:
                break
            n = (blk - 8) // 2
            for k in range(n):
                ent = struct.unpack_from("<H", d, p + 8 + k * 2)[0]
                if (ent >> 12) == IMAGE_REL_BASED_HIGHLOW:
                    sites.add(page + (ent & 0xFFF))
            p += blk
        return sites


# --------------------------------------------------------------------------
# EH structures (decoded), addresses stored as RVAs
# --------------------------------------------------------------------------
@dataclass
class Catch:
    adjectives: int
    type_rva: int        # RVA of TypeDescriptor, 0 == catch(...)
    type_name: str
    disp_catch_obj: int
    handler_rva: int     # catch funclet entry


@dataclass
class TryBlock:
    try_low: int
    try_high: int
    catch_high: int
    catches: list[Catch] = field(default_factory=list)


@dataclass
class FuncInfo:
    funcinfo_rva: int
    magic: int
    max_state: int
    unwind_map_rva: int
    n_try_blocks: int
    try_block_map_rva: int
    try_blocks: list[TryBlock] = field(default_factory=list)
    unwind_funclets: list[int] = field(default_factory=list)  # RVAs
    catch_funclets: list[int] = field(default_factory=list)   # RVAs
    thunk_rva: int | None = None
    owning_func_rva: int | None = None
    owning_func_extent: list[int] | None = None  # [start_rva, end_rva)


# --------------------------------------------------------------------------
# RTTI TypeDescriptor name read
# --------------------------------------------------------------------------
def read_type_name(pe: PE, td_rva: int) -> str:
    """TypeDescriptor: void* pVFTable; void* spare; char name[]. Name starts
    at +8 and is a NUL-terminated decorated type name (e.g. .PAVCException@@)."""
    off = pe.rva_to_off(td_rva)
    if off is None:
        return ""
    raw = pe.data[off + 8:off + 8 + 256]
    end = raw.find(b"\x00")
    if end >= 0:
        raw = raw[:end]
    try:
        return raw.decode("ascii")
    except UnicodeDecodeError:
        return raw.decode("latin1")


# --------------------------------------------------------------------------
# FuncInfo scan + decode
# --------------------------------------------------------------------------
def find_magic_offsets(data: bytes, magic: int) -> list[int]:
    needle = struct.pack("<I", magic)
    out = []
    start = 0
    while True:
        i = data.find(needle, start)
        if i < 0:
            break
        out.append(i)
        start = i + 1
    return out


def decode_funcinfo(pe: PE, reloc_sites: set[int], fi_rva: int) -> FuncInfo | None:
    """Decode and validate a candidate _s_FuncInfo at fi_rva. Returns None if it
    fails the structural sanity checks (i.e. is an NLG cookie / false hit)."""
    magic = pe.u32_at_rva(fi_rva)
    if magic not in FUNCINFO_MAGICS:
        return None
    max_state = pe.i32_at_rva(fi_rva + 0x04)
    p_unwind = pe.u32_at_rva(fi_rva + 0x08)
    n_try = pe.u32_at_rva(fi_rva + 0x0C)
    p_try = pe.u32_at_rva(fi_rva + 0x10)
    if None in (max_state, p_unwind, n_try, p_try):
        return None
    if not (0 <= max_state < MAX_STATE_LIMIT):
        return None
    if not (0 <= n_try < MAX_TRYBLOCKS_LIMIT):
        return None

    unwind_rva = pe.va_to_rva(p_unwind) if p_unwind else 0
    try_rva = pe.va_to_rva(p_try) if p_try else 0

    # The pointer FIELDS must themselves be relocated dwords pointing in-image.
    if max_state > 0:
        if not p_unwind or pe.section_of_rva(unwind_rva) is None:
            return None
        if (fi_rva + 0x08) not in reloc_sites:
            return None
    if n_try > 0:
        if not p_try or pe.section_of_rva(try_rva) is None:
            return None
        if (fi_rva + 0x10) not in reloc_sites:
            return None

    fi = FuncInfo(
        funcinfo_rva=fi_rva,
        magic=magic,
        max_state=max_state,
        unwind_map_rva=unwind_rva,
        n_try_blocks=n_try,
        try_block_map_rva=try_rva,
    )

    # Unwind map: maxState entries of {int toState; void(*action)();}
    if max_state > 0 and unwind_rva:
        for s in range(max_state):
            ent = pe.read_at_rva(unwind_rva + s * SIZEOF_UNWINDMAP_ENTRY,
                                 SIZEOF_UNWINDMAP_ENTRY)
            if ent is None or len(ent) < SIZEOF_UNWINDMAP_ENTRY:
                break
            _to_state, action = struct.unpack("<iI", ent)
            if action:
                a_rva = pe.va_to_rva(action)
                sec = pe.section_of_rva(a_rva)
                if sec and sec.name == ".text":
                    fi.unwind_funclets.append(a_rva)

    # Try-block map: nTry entries; each -> handler array -> catch funclets.
    if n_try > 0 and try_rva:
        for t in range(n_try):
            ent = pe.read_at_rva(try_rva + t * SIZEOF_TRYBLOCKMAP_ENTRY,
                                 SIZEOF_TRYBLOCKMAP_ENTRY)
            if ent is None or len(ent) < SIZEOF_TRYBLOCKMAP_ENTRY:
                break
            try_low, try_high, catch_high, n_catch, p_handlers = struct.unpack(
                "<iiiiI", ent)
            tb = TryBlock(try_low=try_low, try_high=try_high, catch_high=catch_high)
            if 0 < n_catch <= MAX_CATCHES_LIMIT and p_handlers:
                h_rva = pe.va_to_rva(p_handlers)
                if pe.section_of_rva(h_rva) is not None:
                    for c in range(n_catch):
                        hent = pe.read_at_rva(h_rva + c * SIZEOF_HANDLERTYPE,
                                              SIZEOF_HANDLERTYPE)
                        if hent is None or len(hent) < SIZEOF_HANDLERTYPE:
                            break
                        adj, p_type, disp, handler = struct.unpack("<IIiI", hent)
                        type_rva = pe.va_to_rva(p_type) if p_type else 0
                        type_name = read_type_name(pe, type_rva) if type_rva else ""
                        handler_rva = pe.va_to_rva(handler) if handler else 0
                        tb.catches.append(Catch(
                            adjectives=adj,
                            type_rva=type_rva,
                            type_name=type_name,
                            disp_catch_obj=disp,
                            handler_rva=handler_rva,
                        ))
                        if handler_rva and pe.section_of_rva(handler_rva) \
                                and pe.section_of_rva(handler_rva).name == ".text":
                            fi.catch_funclets.append(handler_rva)
            fi.try_blocks.append(tb)

    return fi


# --------------------------------------------------------------------------
# Owning-function attribution via the .text EH thunk + push prologue xref
# --------------------------------------------------------------------------
def attribute_owning_functions(pe: PE, funcs: list[FuncInfo],
                               func_starts: list[int]) -> dict:
    """For each FuncInfo, find `mov eax, <FuncInfo VA>; jmp __CxxFrameHandler`
    in .text (the thunk), then `push offset <thunk>` in .text, and snap to the
    nearest preceding known function start. Returns stats."""
    text = next((s for s in pe.sections if s.name == ".text"), None)
    if text is None:
        return {}
    tb = pe.data[text.rawptr:text.rawptr + text.rawsize]
    base = pe.image_base
    tva = text.vaddr

    def find_mov_thunk(target_va: int) -> int | None:
        needle = struct.pack("<I", target_va)
        s = 0
        while True:
            i = tb.find(needle, s)
            if i < 0:
                return None
            if i >= 1 and tb[i - 1] == OP_MOV_EAX_IMM32:
                return tva + (i - 1)   # thunk start RVA
            s = i + 1

    def find_push_site(thunk_rva: int) -> int | None:
        needle = struct.pack("<I", thunk_rva + base)
        s = 0
        while True:
            i = tb.find(needle, s)
            if i < 0:
                return None
            if i >= 1 and tb[i - 1] == OP_PUSH_IMM32:
                return tva + (i - 1)   # push instruction RVA
            s = i + 1

    def owning(rva: int) -> int | None:
        idx = bisect.bisect_right(func_starts, rva) - 1
        return func_starts[idx] if idx >= 0 else None

    stats = {"thunk_found": 0, "push_found": 0, "owning_attributed": 0,
             "cxx_frame_handler_rva": None}
    handler_targets: dict[int, int] = {}

    for fi in funcs:
        thunk_rva = find_mov_thunk(pe.rva_to_va(fi.funcinfo_rva))
        if thunk_rva is None:
            continue
        fi.thunk_rva = thunk_rva
        stats["thunk_found"] += 1
        # the jmp rel32 right after the imm32 names __CxxFrameHandler
        off = pe.rva_to_off(thunk_rva)
        if off is not None and pe.data[off] == OP_MOV_EAX_IMM32 \
                and pe.data[off + 5] == OP_JMP_REL32:
            rel = struct.unpack_from("<i", pe.data, off + 6)[0]
            tgt = (thunk_rva + 5) + 5 + rel
            handler_targets[tgt] = handler_targets.get(tgt, 0) + 1

        push_rva = find_push_site(thunk_rva)
        if push_rva is None:
            continue
        stats["push_found"] += 1
        of = owning(push_rva)
        if of is not None:
            fi.owning_func_rva = of
            stats["owning_attributed"] += 1

    # Resolve owning extents [start, next start) for attributed functions.
    for fi in funcs:
        if fi.owning_func_rva is not None:
            idx = bisect.bisect_right(func_starts, fi.owning_func_rva)
            end = func_starts[idx] if idx < len(func_starts) else None
            fi.owning_func_extent = [fi.owning_func_rva, end]

    if handler_targets:
        stats["cxx_frame_handler_rva"] = max(handler_targets,
                                             key=handler_targets.get)
    return stats


# --------------------------------------------------------------------------
# Ghidra ground-truth validation
# --------------------------------------------------------------------------
def load_ghidra(csv_path: str):
    """Return (all_func_starts, funclet_rvas, name_by_rva) from functions.csv.
    Funclets = names matching ^Unwind@ or ^Catch@. Plain csv parse (col0 hex
    rva, col1 name)."""
    func_starts = []
    funclet_rvas = set()
    name_by_rva: dict[int, str] = {}
    with open(csv_path, "r", encoding="utf-8", errors="replace") as f:
        header = f.readline()  # skip header
        for line in f:
            line = line.rstrip("\n")
            if not line:
                continue
            # name may contain no commas in this export; split on first comma
            parts = line.split(",")
            try:
                rva = int(parts[0], 16)
            except (ValueError, IndexError):
                continue
            name = parts[1] if len(parts) > 1 else ""
            func_starts.append(rva)
            name_by_rva[rva] = name
            if name.startswith("Unwind@") or name.startswith("Catch@"):
                funclet_rvas.add(rva)
    func_starts.sort()
    return func_starts, funclet_rvas, name_by_rva


# --------------------------------------------------------------------------
# Main extraction driver
# --------------------------------------------------------------------------
def extract(pe: PE):
    reloc_sites = pe.highlow_reloc_sites()
    data = pe.data

    # Candidate magic offsets, restricted to initialized .rdata/.data bytes.
    candidate_offs = []
    for magic in FUNCINFO_MAGICS:
        candidate_offs.extend(find_magic_offsets(data, magic))
    candidate_offs.sort()

    funcs: list[FuncInfo] = []
    rejected = 0
    for off in candidate_offs:
        sec = None
        for s in pe.sections:
            if s.rawptr <= off < s.rawptr + s.rawsize:
                sec = s
                break
        if sec is None or sec.name not in (".rdata", ".data"):
            rejected += 1
            continue
        fi_rva = pe.off_to_rva(off)
        fi = decode_funcinfo(pe, reloc_sites, fi_rva)
        if fi is None:
            rejected += 1
            continue
        funcs.append(fi)

    return funcs, reloc_sites, rejected, len(candidate_offs)


def build_json(funcs: list[FuncInfo], pe: PE) -> list[dict]:
    out = []
    for fi in funcs:
        out.append({
            "funcinfo_rva": fi.funcinfo_rva,
            "funcinfo_va": pe.rva_to_va(fi.funcinfo_rva),
            "magic": fi.magic,
            "max_state": fi.max_state,
            "thunk_rva": fi.thunk_rva,
            "owning_func_rva": fi.owning_func_rva,
            "owning_func_extent": fi.owning_func_extent,
            "try_blocks": [
                {
                    "try_low": tb.try_low,
                    "try_high": tb.try_high,
                    "catch_high": tb.catch_high,
                    "catches": [
                        {
                            "handler_rva": c.handler_rva,
                            "type": c.type_name,
                            "type_rva": c.type_rva,
                            "adjectives": c.adjectives,
                            "disp_catch_obj": c.disp_catch_obj,
                        } for c in tb.catches
                    ],
                } for tb in fi.try_blocks
            ],
            "unwind_funclets": sorted(set(fi.unwind_funclets)),
            "catch_funclets": sorted(set(fi.catch_funclets)),
        })
    return out


def hx(v):
    return ("0x%08x" % v) if v is not None else "None"


def main(argv=None):
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.dirname(here)
    ap = argparse.ArgumentParser(description="Extract MSVC x86 C++ EH metadata "
                                 "from a PE and recover funclet boundaries.")
    ap.add_argument("--exe", default=os.path.join(root, "binaries", "retail_en",
                                                   "GRUNTZ.EXE"))
    ap.add_argument("--json", default=os.path.join(root, "build", "seh",
                                                    "seh_segments.json"))
    ap.add_argument("--summary", default=os.path.join(root, "build", "seh",
                                                       "summary.txt"))
    ap.add_argument("--ghidra-csv", default=os.path.join(
        root, "build", "ghidra-enrich", "exports", "functions.csv"))
    ap.add_argument("--quiet", action="store_true")
    args = ap.parse_args(argv)

    with open(args.exe, "rb") as f:
        pe = PE(f.read())

    funcs, reloc_sites, rejected, n_candidates = extract(pe)

    # Owning-function attribution needs a function-start list. Prefer Ghidra's
    # (for the extent snap); the funclet OVERLAP check below is independent of
    # this and is the real correctness proof.
    g_func_starts, g_funclets, g_names = ([], set(), {})
    have_ghidra = os.path.exists(args.ghidra_csv)
    if have_ghidra:
        g_func_starts, g_funclets, g_names = load_ghidra(args.ghidra_csv)
    attr_stats = attribute_owning_functions(pe, funcs, g_func_starts)

    # Aggregate funclets recovered by this parser.
    my_unwind = set()
    my_catch = set()
    for fi in funcs:
        my_unwind.update(fi.unwind_funclets)
        my_catch.update(fi.catch_funclets)
    my_funclets = my_unwind | my_catch

    n_owning = sum(1 for fi in funcs if fi.owning_func_rva is not None)
    n_with_try = sum(1 for fi in funcs if fi.n_try_blocks > 0)

    # Validation vs Ghidra funclet labels.
    val = {}
    if have_ghidra:
        overlap = my_funclets & g_funclets
        extras = my_funclets - g_funclets
        misses = g_funclets - my_funclets
        val = {
            "ghidra_funclets": len(g_funclets),
            "my_funclets": len(my_funclets),
            "overlap": len(overlap),
            "extras": sorted(extras),
            "misses": sorted(misses),
            "overlap_pct_of_ghidra": (100.0 * len(overlap) / len(g_funclets))
            if g_funclets else 0.0,
        }

    # Write JSON output.
    os.makedirs(os.path.dirname(args.json), exist_ok=True)
    payload = {
        "image_base": pe.image_base,
        "exe": os.path.relpath(args.exe, root),
        "funcinfo_count": len(funcs),
        "cxx_frame_handler_rva": attr_stats.get("cxx_frame_handler_rva"),
        "func_infos": build_json(funcs, pe),
    }
    with open(args.json, "w", encoding="utf-8") as f:
        json.dump(payload, f, indent=2)

    # Write summary.
    lines = []
    L = lines.append
    L("MSVC x86 C++ EH extraction - %s" % os.path.basename(args.exe))
    L("=" * 60)
    L("image base                 : %s" % hx(pe.image_base))
    L("FuncInfo magic candidates  : %d (0x19930520 only on VC5)" % n_candidates)
    L("  rejected (NLG/non-rdata) : %d" % rejected)
    L("valid _s_FuncInfo records  : %d" % len(funcs))
    L("  with try/catch blocks    : %d" % n_with_try)
    L(".reloc HIGHLOW sites       : %d" % len(reloc_sites))
    L("__CxxFrameHandler thunk    : %s" % hx(attr_stats.get("cxx_frame_handler_rva")))
    L("")
    L("Funclets recovered (independent of Ghidra):")
    L("  unwind/destructor funclets : %d" % len(my_unwind))
    L("  catch handler funclets     : %d" % len(my_catch))
    L("  total distinct funclets    : %d" % len(my_funclets))
    L("")
    L("Owning-function attribution (thunk + push xref, best-effort):")
    L("  thunks located in .text    : %d / %d" % (attr_stats.get("thunk_found", 0),
                                                  len(funcs)))
    L("  push-prologue site found   : %d" % attr_stats.get("push_found", 0))
    L("  owning function attributed : %d (%.1f%%)" % (
        n_owning, 100.0 * n_owning / len(funcs) if funcs else 0.0))
    L("  (extent approximated as [start, next func start); VC5 x86 FuncInfo")
    L("   carries no IP-to-state map, so exact byte extent is not in EH data.)")
    L("")
    if have_ghidra:
        L("Validation vs Ghidra Catch@/Unwind@ ground truth:")
        L("  Ghidra funclet labels      : %d" % val["ghidra_funclets"])
        L("  my funclets                : %d" % val["my_funclets"])
        L("  overlap (proof of correct) : %d  (%.2f%% of Ghidra)" % (
            val["overlap"], val["overlap_pct_of_ghidra"]))
        L("  extras (Ghidra MISSED)     : %d" % len(val["extras"]))
        L("  misses (we did not find)   : %d" % len(val["misses"]))
        if val["extras"]:
            L("  extra funclet RVAs (first 40):")
            for r in val["extras"][:40]:
                L("    %s" % hx(r))
        if val["misses"]:
            L("  missed funclet RVAs (first 40):")
            for r in val["misses"][:40]:
                L("    %s  %s" % (hx(r), g_names.get(r, "")))
    else:
        L("Ghidra functions.csv not found at %s - validation skipped." %
          args.ghidra_csv)
    L("")
    L("Outputs: %s" % os.path.relpath(args.json, root))
    L("         %s" % os.path.relpath(args.summary, root))

    text = "\n".join(lines) + "\n"
    os.makedirs(os.path.dirname(args.summary), exist_ok=True)
    with open(args.summary, "w", encoding="utf-8") as f:
        f.write(text)

    if not args.quiet:
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
