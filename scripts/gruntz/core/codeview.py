#!/usr/bin/env python3
"""codeview.py - read MSVC 5.0 (old-format) CodeView locals from a COFF object.

`cl /Z7` embeds CodeView debug info directly in the object's `.debug$S` sections
(one per function COMDAT) - no PDB, no mspdbsrv. MSVC 5.0 emits the PRE-VC7
CodeView format (symbol signature 0x2, 32-bit type indices, length-prefixed
"Pascal" names) that modern `llvm-readobj --codeview` cannot parse ("Invalid
data"). The records we need are simple and stable, so we parse them directly.

For each function we recover its named stack/register variables:
  * S_GPROC32 / S_LPROC32 (0x100b / 0x100a) - the procedure; its name is the
    DEMANGLED qualified name (e.g. "RezMgr::MakeImageKey"), which joins to the
    qualified name labels.py already derives per RVA (build/gen/functions.json).
  * S_BPREL32 (0x1006) - a frame-relative ("stack") local: signed frame offset,
    32-bit type index, name. Parameters sit at positive offsets, locals at
    negative ones (relative to the canonical frame base: [+0]=return addr).
  * S_REGISTER (0x1001) - an enregistered local: type index, register, name.

The .text bytes are byte-identical with/without /Z7 (verified), so a /Z7 build of
a matched TU describes the RETAIL function exactly - the locals harvested here are
the real source-level variable names for the byte-exact code.
"""

import struct

# CodeView symbol record types (old/ST 32-bit-type-index variants).
S_LPROC32 = 0x100a
S_GPROC32 = 0x100b
S_BPREL32 = 0x1006
S_REGISTER = 0x1001
S_END = 0x0006
S_OLDEND = 0x0007

# CodeView register numbers we care to name (x86); others kept numeric.
CV_REG = {
    1: "al", 2: "cl", 3: "dl", 4: "bl", 9: "ax", 10: "cx", 11: "dx", 12: "bx",
    17: "eax", 18: "ecx", 19: "edx", 20: "ebx", 21: "esp", 22: "ebp",
    23: "esi", 24: "edi",
}

# GPROC32 fixed-field size before the Pascal name, for the 32-bit-type-index
# layout MSVC 5.0 emits: pParent,pEnd,pNext,len,DbgStart,DbgEnd (6*u32) +
# typind(u32) + off(u32) + seg(u16) + flags(u8) = 35 bytes.
_GPROC_NAME_OFF = 35


class Local:
    __slots__ = ("name", "kind", "offset", "reg", "type_index")

    def __init__(self, name, kind, offset=None, reg=None, type_index=0):
        self.name = name
        self.kind = kind          # "stack" | "reg"
        self.offset = offset      # frame offset (stack)
        self.reg = reg            # register name (reg)
        self.type_index = type_index

    def __repr__(self):
        loc = ("bp%+#x" % self.offset) if self.kind == "stack" else self.reg
        return "Local(%r @ %s t=0x%x)" % (self.name, loc, self.type_index)


def _pascal(buf, p):
    n = buf[p]
    return buf[p + 1:p + 1 + n].decode("latin1")


def _iter_debug_s(data):
    """Yield each `.debug$S` section's raw bytes from a COFF object."""
    nsec = struct.unpack_from("<H", data, 2)[0]
    sec_off = 20
    for i in range(nsec):
        b = sec_off + i * 40
        name = data[b:b + 8].rstrip(b"\0").decode("latin1")
        raw_size, raw_ptr = struct.unpack_from("<II", data, b + 16)
        if name.startswith(".debug$S") and raw_size >= 4 and raw_ptr:
            yield data[raw_ptr:raw_ptr + raw_size]


def parse_locals(obj_path):
    """{qualified_proc_name: [Local, ...]} for every function in a /Z7 COFF obj.

    Only functions with at least one named local are returned non-empty, but every
    procedure encountered is keyed (possibly with an empty list).
    """
    with open(obj_path, "rb") as f:
        data = f.read()
    out = {}
    for sec in _iter_debug_s(data):
        p = 0
        if struct.unpack_from("<I", sec, 0)[0] in (1, 2, 4):
            p = 4  # CodeView signature on the module section; per-func sections lack it
        cur = None
        while p + 4 <= len(sec):
            rlen, rtyp = struct.unpack_from("<HH", sec, p)
            if rlen < 2:
                break
            body = sec[p + 4:p + 2 + rlen]
            try:
                if rtyp in (S_GPROC32, S_LPROC32):
                    cur = _pascal(body, _GPROC_NAME_OFF)
                    out.setdefault(cur, [])
                elif rtyp == S_BPREL32 and cur is not None:
                    off = struct.unpack_from("<i", body, 0)[0]
                    ti = struct.unpack_from("<I", body, 4)[0]
                    nm = _pascal(body, 8)
                    if nm:
                        out[cur].append(Local(nm, "stack", offset=off, type_index=ti))
                elif rtyp == S_REGISTER and cur is not None:
                    ti = struct.unpack_from("<I", body, 0)[0]
                    reg = struct.unpack_from("<H", body, 4)[0]
                    nm = _pascal(body, 6)
                    if nm:
                        out[cur].append(
                            Local(nm, "reg", reg=CV_REG.get(reg, "r%d" % reg),
                                  type_index=ti))
                elif rtyp in (S_END, S_OLDEND):
                    cur = None
            except Exception:
                pass  # tolerate an unexpected record; keep scanning
            p += 2 + rlen
    return out


# --- COFF line numbers (source line <-> code offset) -----------------------
#
# MSVC 5.0 /Z7 does NOT emit the modern C13 DEBUG_S_LINES subsection (the one
# `llvm-readobj --codeview` understands). It uses the CLASSIC COFF line-number
# mechanism instead: each function's `.text` section header carries
# PointerToLinenumbers / NumberOfLinenumbers pointing at a run of 6-byte
# IMAGE_LINENUMBER records
#     union { u32 SymbolTableIndex;   // when Linenumber == 0 (the anchor record)
#             u32 VirtualAddress; }   // when Linenumber != 0: code offset in section
#     u16 Linenumber;
# The anchor (Linenumber == 0) names the function via its symbol index. Every
# other record's Linenumber is stored RELATIVE to the function's begin line,
# which lives in the function's `.bf` symbol aux record (bytes [4:6]). So the
# real source line is  bf_line + stored.  (Verified against CState::CState:
# .bf=53, the stored-16 record at offset 0x48 is `mov [eax+0x16c],ecx` == the
# `m_16c = 0;` at GameMode.cpp:69 == 53+16.)  The .text bytes are byte-identical
# with/without /Z7, so these offsets line up 1:1 with the matching base obj's
# `llvm-objdump` offsets.

# Section-header field offsets (40-byte IMAGE_SECTION_HEADER).
_SECHDR = 40
_SH_PTRLINES = 28   # PointerToLinenumbers  (u32)
_SH_NLINES = 34     # NumberOfLinenumbers   (u16)


def _coff_sym_name(data, symptr, strtab_off, idx):
    b = symptr + idx * 18
    nm = data[b:b + 8]
    if nm[:4] == b"\0\0\0\0":                       # long name -> string table
        off = struct.unpack_from("<I", nm, 4)[0]
        end = data.index(b"\0", strtab_off + off)
        return data[strtab_off + off:end].decode("latin1")
    return nm.rstrip(b"\0").decode("latin1")


def parse_lines(obj_path):
    """{mangled_func_name: {"bf": begin_line, "lines": {code_off: src_line}}}.

    One entry per `.text` COMDAT that carries COFF line numbers; `lines` maps a
    code offset (relative to the function start, == the base obj's objdump
    offset) to its 1-based source line. Functions without line records (or a
    resolvable `.bf`) are simply absent.
    """
    with open(obj_path, "rb") as f:
        data = f.read()
    nsec = struct.unpack_from("<H", data, 2)[0]
    symptr, nsym = struct.unpack_from("<II", data, 8)
    strtab_off = symptr + nsym * 18
    out = {}
    for s in range(nsec):
        h = 20 + s * _SECHDR
        name = data[h:h + 8].rstrip(b"\0").decode("latin1")
        lptr = struct.unpack_from("<I", data, h + _SH_PTRLINES)[0]
        nline = struct.unpack_from("<H", data, h + _SH_NLINES)[0]
        if name != ".text" or not lptr or nline < 1:
            continue
        fidx, ln0 = struct.unpack_from("<IH", data, lptr)
        if ln0 != 0 or fidx >= nsym:               # first record must be the anchor
            continue
        # begin line: function symbol's aux TagIndex -> `.bf` symbol, aux[4:6].
        if data[symptr + fidx * 18 + 17] < 1:      # NumberOfAuxSymbols
            continue
        tag = struct.unpack_from("<I", data, symptr + (fidx + 1) * 18)[0]
        if not 0 < tag < nsym:
            continue
        bf = struct.unpack_from("<H", data, symptr + (tag + 1) * 18 + 4)[0]
        fname = _coff_sym_name(data, symptr, strtab_off, fidx)
        lines = {}
        for r in range(1, nline):
            off, stored = struct.unpack_from("<IH", data, lptr + r * 6)
            lines[off] = bf + stored
        out[fname] = {"bf": bf, "lines": lines}
    return out


if __name__ == "__main__":
    import sys
    if len(sys.argv) > 1 and sys.argv[1] == "--lines":
        for obj in sys.argv[2:]:
            for nm, info in parse_lines(obj).items():
                print(f"{nm}  bf={info['bf']}")
                for off, ln in sorted(info["lines"].items()):
                    print(f"    off {off:#06x}  line {ln}")
        raise SystemExit(0)
    for obj in sys.argv[1:]:
        procs = parse_locals(obj)
        nstack = sum(1 for v in procs.values() for l in v if l.kind == "stack")
        print("%s: %d procs, %d named stack locals" % (obj, len(procs), nstack))
        for nm, locs in procs.items():
            if locs:
                print("  " + nm)
                for l in locs:
                    print("    " + repr(l))
