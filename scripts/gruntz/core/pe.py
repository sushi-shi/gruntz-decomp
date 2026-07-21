"""gruntz.core.pe - the retail GRUNTZ.EXE, parsed once.

The PE primitives every binary-side tool shares: section table, .text access,
base-reloc sites, the ILT jmp-thunk band, the whole-.text E8/E9 call index and
the non-.text string table. Everything lazy + cached on the instance; get one
via gruntz.core.get_context().pe.
"""
import bisect
import os
import re
import struct
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])
EXE = Path(os.environ.get("GRUNTZ_EXE") or REPO / "build/exe/GRUNTZ.EXE")
IMAGEBASE = 0x400000
# The incremental-link (ILT) jmp-thunk band: the leading jump table the retail linker
# emits (a run of 5-byte `E9 rel32` forwarders). Real callers `call <ILT entry>`; the
# entry `jmp`s to the body - pass-through, not code.
ILT_LO, ILT_HI = 0x1000, 0x7c20


def load():
    """(data, secs) from the process-wide cached PE (the ex-xref._load shape)."""
    from gruntz.core import get_context
    pe = get_context().pe
    return pe.data, pe.secs


def text(secs):
    """The .text row (name, va, vsz, rp, rsz) of a `secs` list (ex xref._text)."""
    return next(s for s in secs if s[0] == ".text")


class PE:
    def __init__(self, path=None):
        self.path = Path(path) if path else EXE
        self.data = self.path.read_bytes()
        d = self.data
        if len(d) < 0x40 or d[:2] != b"MZ":
            raise ValueError(f"not a PE file: {self.path}")
        e = struct.unpack_from("<I", d, 0x3c)[0]
        if d[e:e + 4] != b"PE\0\0":
            raise ValueError(f"missing PE signature: {self.path}")
        nsec = struct.unpack_from("<H", d, e + 6)[0]
        self._optsz = struct.unpack_from("<H", d, e + 20)[0]
        self._opt = e + 24
        if struct.unpack_from("<H", d, self._opt)[0] != 0x10B:
            raise ValueError(f"expected PE32 optional header: {self.path}")
        self.image_base = struct.unpack_from("<I", d, self._opt + 28)[0]
        # SizeOfRawData is rounded UP to FileAlignment - a section's last
        # <FileAlignment bytes may be padding, not emitted content (data_audit).
        self.file_alignment = struct.unpack_from("<I", d, self._opt + 36)[0]
        self.sections = []                 # [{name, rva, virtual_size, raw_size,
        #                                     raw_offset, characteristics}]
        for i in range(nsec):
            o = self._opt + self._optsz + i * 40
            name = d[o:o + 8].rstrip(b"\0").decode("latin1")
            vsz, va, rsz, rp = struct.unpack_from("<IIII", d, o + 8)
            self.sections.append({
                "name": name, "rva": va, "virtual_size": vsz,
                "raw_size": rsz, "raw_offset": rp,
                "characteristics": struct.unpack_from("<I", d, o + 36)[0],
            })
        # legacy 5-tuple shape (name, va, vsz, rp, rsz) - most scan code uses this
        self.secs = [(s["name"], s["rva"], s["virtual_size"],
                      s["raw_offset"], s["raw_size"]) for s in self.sections]
        self.exec_ranges = [(s["rva"], s["rva"] + max(s["virtual_size"], s["raw_size"]))
                            for s in self.sections if s["characteristics"] & 0x20000000]
        self._reloc_sites = None
        self._call_index = None
        self._strings_at = None

    # --- sections ----------------------------------------------------------
    @property
    def text(self):
        """The .text row: (name, va, vsz, rp, rsz)."""
        return next(s for s in self.secs if s[0] == ".text")

    def off(self, rva):
        """File offset of `rva`, or None if unmapped OR past the section's raw
        data (a virtual-only tail has no file bytes - reading through would
        return the NEXT section's content)."""
        for name, va, vsz, rp, rsz in self.secs:
            if va <= rva < va + max(vsz, rsz):
                o = rva - va + rp
                return o if o < rp + rsz else None
        return None

    def sec_of(self, rva):
        for s in self.secs:
            if s[1] <= rva < s[1] + max(s[2], s[4]):
                return s
        return None

    def sec_name(self, rva):
        s = self.sec_of(rva)
        return s[0] if s else None

    def is_exec(self, rva):
        return any(lo <= rva < hi for lo, hi in self.exec_ranges)

    def u32(self, rva):
        o = self.off(rva)
        return struct.unpack_from("<I", self.data, o)[0] if o is not None else None

    def cstr(self, rva, n=512):
        """NUL-terminated latin1 string at `rva` (bounded), or None."""
        o = self.off(rva)
        if o is None:
            return None
        end = self.data.find(b"\0", o, o + n)
        return self.data[o:end].decode("latin1") if end != -1 else None

    # --- base relocations (the real DIR32 address-operand sites) -----------
    @property
    def reloc_sites(self):
        """Sorted RVAs of every IMAGE_REL_BASED_HIGHLOW site (data dir 5)."""
        if self._reloc_sites is None:
            d = self.data
            rva = struct.unpack_from("<I", d, self._opt + 96 + 5 * 8)[0]
            sz = struct.unpack_from("<I", d, self._opt + 96 + 5 * 8 + 4)[0]
            sites = []
            if rva:
                base = self.off(rva)
                p, end = base, base + sz
                while p < end:
                    page, blk = struct.unpack_from("<II", d, p)
                    if blk == 0:
                        break
                    for i in range((blk - 8) // 2):
                        ent = struct.unpack_from("<H", d, p + 8 + i * 2)[0]
                        if ent >> 12 == 3:
                            sites.append(page + (ent & 0xfff))
                    p += blk
            sites.sort()
            self._reloc_sites = sites
        return self._reloc_sites

    def relocs_in(self, lo, hi):
        """[(site_rva, stored_va)] for every HIGHLOW site in [lo, hi)."""
        out = []
        sites = self.reloc_sites
        i = bisect.bisect_left(sites, lo)
        while i < len(sites) and sites[i] < hi:
            o = self.off(sites[i])
            if o is not None:
                out.append((sites[i], struct.unpack_from("<I", self.data, o)[0]))
            i += 1
        return out

    # --- .text call graph ---------------------------------------------------
    @property
    def call_index(self):
        """{target_rva: [(site_rva, opcode)]} for every E8/E9 rel32 whose target
        lands in .text - ONE scan, shared by callers/tree/thunk queries."""
        if self._call_index is None:
            _n, tva, tvsz, trp, trsz = self.text
            tb = self.data[trp:trp + trsz]
            idx = {}
            n = len(tb) - 4                # an E8/E9 in the last 5 bytes counts
            i = 0
            while i < n:
                op = tb[i]
                if op == 0xE8 or op == 0xE9:
                    rel = struct.unpack_from("<i", tb, i + 1)[0]
                    tgt = tva + i + 5 + rel
                    if tva <= tgt < tva + tvsz:
                        idx.setdefault(tgt, []).append((tva + i, op))
                i += 1
            self._call_index = idx
        return self._call_index

    def thunks_to(self, target):
        """ILT-band entry RVAs whose `E9 rel32` jumps to `target` (vtables and the
        game's command tables store these, not the body)."""
        return [site for site, op in self.call_index.get(target, [])
                if op == 0xE9 and ILT_LO <= site < ILT_HI]

    def ilt_target(self, rva):
        """The body an ILT-band `E9` entry at `rva` forwards to, or None."""
        if not (ILT_LO <= rva < ILT_HI):
            return None
        o = self.off(rva)
        if o is None or self.data[o] != 0xE9:
            return None
        rel = struct.unpack_from("<i", self.data, o + 1)[0]
        return rva + 5 + rel

    # --- strings ------------------------------------------------------------
    @property
    def strings_at(self):
        """{start_VA: text} for every printable run (>=4) in the non-.text sections."""
        if self._strings_at is None:
            pat = re.compile(rb"[\x20-\x7e]{4,}")
            out = {}
            for name, va, vsz, rp, rsz in self.secs:
                if name == ".text":
                    continue
                blob = self.data[rp:rp + rsz]
                for m in pat.finditer(blob):
                    out[IMAGEBASE + va + m.start()] = m.group().decode("latin1")
            self._strings_at = out
        return self._strings_at
