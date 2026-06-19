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


if __name__ == "__main__":
    import sys
    for obj in sys.argv[1:]:
        procs = parse_locals(obj)
        nstack = sum(1 for v in procs.values() for l in v if l.kind == "stack")
        print("%s: %d procs, %d named stack locals" % (obj, len(procs), nstack))
        for nm, locs in procs.items():
            if locs:
                print("  " + nm)
                for l in locs:
                    print("    " + repr(l))
