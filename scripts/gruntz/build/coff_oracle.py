"""coff_oracle.py - recover cl.exe's string-pool names for the delinked target.

cl.exe pools a string literal "Font" into a COMDAT symbol named
`??_C@_04CLJC@Font?$AA@` (length + a VC5 16-bit checksum + the text). Ghidra,
and so the synthesised PDB, instead labels the same bytes `DAT_<addr>` /
`s_<text>_<addr>`, so the delinked target object's reloc points at a name that
can never match a recompiled source literal.

We do not reimplement VC5's checksum. The base objects ARE cl.exe output, so
every `??_C@...` symbol there is the exact name paired with its bytes. This
module reads those names (`build_string_map`) and reads a candidate symbol's
real bytes from the EXE (`Exe.cstring`); synth_pdb uses the pair to rename
string constants before handing the PDB to the delinker.
"""

import struct
from pathlib import Path


class Exe:
    """Read C strings from the retail PE image by virtual address."""

    def __init__(self, path: Path):
        data = path.read_bytes()
        self.data = data
        pe = struct.unpack_from("<I", data, 0x3C)[0]
        nsec = struct.unpack_from("<H", data, pe + 6)[0]
        opt = struct.unpack_from("<H", data, pe + 20)[0]
        self.base = struct.unpack_from("<I", data, pe + 24 + 28)[0]
        self.secs = []
        so = pe + 24 + opt
        for i in range(nsec):
            o = so + i * 40
            vsize, vaddr, rawsize, rawptr = struct.unpack_from("<IIII", data, o + 8)
            self.secs.append((vaddr, max(vsize, rawsize), rawptr, rawsize))

    def cstring(self, va: int, limit: int = 512):
        rva = va - self.base
        for vaddr, vspan, rawptr, rawsize in self.secs:
            if vaddr <= rva < vaddr + vspan:
                off = rawptr + (rva - vaddr)
                cap = min(rawptr + rawsize, off + limit)
                end = off
                while end < cap and self.data[end] != 0:
                    end += 1
                return self.data[off:end] if end < cap else None
        return None


def section_alignment(characteristics: int) -> int:
    """The IMAGE_SCN_ALIGN_* nibble decoded to a byte count (1 when unset)."""
    a = (characteristics & 0x00F00000) >> 20
    return 1 << (a - 1) if a else 1


class _Coff:
    """Minimal read-only i386 COFF object reader (symbols + section bytes).

    `sections` keeps the raw (rawptr, rawsize) tuples the string oracle needs;
    `section_table` carries the full per-section topology (name, characteristics,
    COMDAT selection/associate, the aux `scnlen` that is authoritative for .bss)
    that the candidate section manifest is derived from.
    """

    def __init__(self, path: Path):
        self.buf = path.read_bytes()
        b = self.buf
        self.nsec = struct.unpack_from("<H", b, 2)[0]
        self.symptr = struct.unpack_from("<I", b, 8)[0]
        self.nsym = struct.unpack_from("<I", b, 12)[0]
        opt = struct.unpack_from("<H", b, 16)[0]
        self.strtab_off = self.symptr + self.nsym * 18
        self.sections = []
        self.section_table = []
        for i in range(self.nsec):
            o = 20 + opt + i * 40
            name = b[o:o + 8].split(b"\0")[0].decode("latin1")
            if name.startswith("/"):        # long name -> string table offset
                off = int(name[1:])
                end = b.index(b"\0", self.strtab_off + off)
                name = b[self.strtab_off + off:end].decode("latin1")
            vsize, _vaddr, rawsize, rawptr = struct.unpack_from("<IIII", b, o + 8)
            chars = struct.unpack_from("<I", b, o + 36)[0]
            self.sections.append((rawptr, rawsize))
            self.section_table.append({
                "index": i + 1, "name": name, "characteristics": chars,
                "alignment": section_alignment(chars),
                "size": rawsize or vsize, "comdat": 0, "assoc": 0,
            })
        # The section-definition aux record carries the authoritative length
        # (.bss has rawsize 0) plus the COMDAT selection/associate.
        for idx, _value, secnum in self.iter_symbols():
            base = self.symptr + idx * 18
            _scl, naux = struct.unpack_from("<BB", b, base + 16)
            if _scl != 3 or not naux or not (1 <= secnum <= self.nsec):
                continue
            a = self.symptr + (idx + 1) * 18
            scnlen, _nr, _nl, _ck, assoc, sel = struct.unpack_from("<IHHIHB", b, a)
            sec = self.section_table[secnum - 1]
            sec["size"] = scnlen
            sec["comdat"] = sel
            sec["assoc"] = assoc

    def sym_name(self, idx: int) -> str:
        base = self.symptr + idx * 18
        if struct.unpack_from("<I", self.buf, base)[0] == 0:
            off = struct.unpack_from("<I", self.buf, base + 4)[0]
            end = self.buf.index(b"\0", self.strtab_off + off)
            return self.buf[self.strtab_off + off:end].decode("latin1")
        return self.buf[base:base + 8].split(b"\0")[0].decode("latin1")

    def iter_symbols(self):
        i = 0
        while i < self.nsym:
            base = self.symptr + i * 18
            value, secnum, _typ, _scl, naux = struct.unpack_from("<IhHBB", self.buf, base + 8)
            yield i, value, secnum
            i += 1 + naux

    def defined_symbols(self, secnum: int):
        """[(offset, name)] of the class-EXTERNAL symbols defined in one section."""
        out = []
        for idx, value, sn in self.iter_symbols():
            if sn != secnum:
                continue
            scl = struct.unpack_from("<B", self.buf, self.symptr + idx * 18 + 16)[0]
            if scl == 2:                      # IMAGE_SYM_CLASS_EXTERNAL
                out.append((value, self.sym_name(idx)))
        out.sort()
        return out

    def cstring(self, secnum: int, value: int, limit: int = 512):
        if secnum < 1 or secnum > self.nsec:
            return None
        rawptr, rawsize = self.sections[secnum - 1]
        if not rawptr:
            return None
        start = rawptr + value
        cap = min(rawptr + rawsize, start + limit)
        if not (rawptr <= start < rawptr + rawsize):
            return None
        end = start
        while end < cap and self.buf[end] != 0:
            end += 1
        return bytes(self.buf[start:end]) if end < cap else None


def build_string_map(base_dir: Path) -> dict:
    """{string bytes (sans NUL) -> ??_C@... name} from every base object."""
    out = {}
    for obj in sorted(Path(base_dir).glob("*.obj")):
        try:
            c = _Coff(obj)
        except Exception:
            continue
        for idx, value, secnum in c.iter_symbols():
            name = c.sym_name(idx)
            if name.startswith("??_C@") and secnum >= 1:
                cs = c.cstring(secnum, value)
                if cs is not None:
                    out.setdefault(cs, name)
    return out
