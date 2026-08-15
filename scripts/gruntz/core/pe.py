"""gruntz.core.pe - the retail image, parsed once.

The PE section table is the authority for every address-space edge; nothing
in the tree hardcodes an image constant. Parsed lazily and cached per
process (the image never changes).
"""

from __future__ import annotations

import struct
from functools import lru_cache
from pathlib import Path

from gruntz.core.paths import retail_exe


class Pe:
    def __init__(self, path: Path | str | None = None):
        self.path = Path(path or retail_exe())
        self.data = d = self.path.read_bytes()
        pe = struct.unpack_from("<I", d, 0x3C)[0]
        nsec = struct.unpack_from("<H", d, pe + 6)[0]
        optsz = struct.unpack_from("<H", d, pe + 20)[0]
        magic = struct.unpack_from("<H", d, pe + 24)[0]
        if magic != 0x10B:
            raise ValueError(f"{self.path}: not a PE32 image (magic 0x{magic:x})")
        self.image_base = struct.unpack_from("<I", d, pe + 24 + 28)[0]
        self.sections: list[dict] = []
        for i in range(nsec):
            base = pe + 24 + optsz + i * 40
            name = d[base:base + 8].rstrip(b"\0").decode("latin-1")
            vsize, va, rsize, rptr = struct.unpack_from("<IIII", d, base + 8)
            self.sections.append({"name": name, "va": va, "vsize": vsize,
                                  "rsize": rsize, "rptr": rptr})

    def section(self, name: str) -> dict:
        s = next((s for s in self.sections if s["name"] == name), None)
        if s is None:
            raise KeyError(f"{self.path}: no section {name}")
        return s

    def text_span(self) -> tuple[int, int]:
        """[lo, hi) of .text's VIRTUAL extent (what function extents cap at)."""
        t = self.section(".text")
        return t["va"], t["va"] + t["vsize"]

    def data_regions(self) -> dict[str, tuple[int, int]]:
        """The three data regions: .rdata's raw bytes, .data's raw
        (initialized) bytes, and .data's loader-zero virtual tail - this image
        has no separate .bss section header."""
        rd, da = self.section(".rdata"), self.section(".data")
        return {"rdata": (rd["va"], rd["va"] + rd["rsize"]),
                "data": (da["va"], da["va"] + da["rsize"]),
                "bss": (da["va"] + da["rsize"], da["va"] + da["vsize"])}

    def read(self, rva: int, size: int) -> bytes | None:
        for s in self.sections:
            if s["va"] <= rva and rva + size <= s["va"] + max(s["vsize"], s["rsize"]):
                off = s["rptr"] + rva - s["va"]
                return self.data[off:off + size]
        return None


@lru_cache(maxsize=1)
def image() -> Pe:
    return Pe()
