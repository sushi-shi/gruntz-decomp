#!/usr/bin/env python3
"""msdis_stub.py - make VC5 `link.exe` loadable under wine.

The toolchain's `link.exe` (genuine VC5 linker, version 5.10.7303 - the one that
built retail GRUNTZ.EXE) statically imports **MSDIS100.DLL**, the VC5 disassembler
engine. That DLL is used ONLY by link's `/dump /disasm` path (the `dumpbin
/disasm` feature); plain linking never calls into it. But because the import is
static, the Windows loader resolves all of it at load time, so without the DLL
present wine fails with `status c0000135` (DLL not found) and link.exe won't start
at all.

Toolchain tarballs built before scripts/create-toolchain-release.py learned to
bundle msdis100.dll (from the same VS97 Disc 3 ISO as MSPDB50.DLL) lack it. Two
ways to satisfy the import:

  * REAL dll - if `$MSVC_DIR/bin/MSDIS100.DLL` exists (a tarball built after that
    fix carries it), we use it verbatim.
  * STUB - otherwise synthesize a tiny PE that EXPORTS the 11 symbols link.exe
    imports, each pointing at a `xor eax,eax; ret`. Since linking never calls
    them, the stub bodies are irrelevant; only the export NAMES must resolve. The
    link output (.EXE/.MAP) is byte-for-byte identical either way.

`ensure_msdis(wineprefix)` copies/generates the DLL into the prefix's 32-bit
system dir (syswow64 on a win64/WoW64 prefix, system32 on a pure win32 prefix) so
the 32-bit link.exe finds it. Idempotent: it is a no-op once the DLL is in place.

Run standalone to drop a stub at a path:
    python3 msdis_stub.py <out.dll>
"""

import os
import shutil
import struct
import sys
from pathlib import Path

# The exact 11 mangled C++ names link.exe imports from MSDIS100.DLL (the DIS
# disassembler classes for x86/MIPS/ARM/PPC/SH/AXP). Verified by walking
# link.exe's import directory. Export NAMES must match; bodies are never called.
MSDIS_EXPORTS = sorted([
    "?CchFormatAddr@DIS@@QBEIKPADI@Z",
    "?PfncchfixupSet@DIS@@QAEP6GIPBV1@KIPADIPAK@ZP6GI0KI1I2@Z@Z",
    "??0DISARM@@QAE@W4DIST@DIS@@@Z",
    "?PfncchaddrSet@DIS@@QAEP6GIPBV1@KPADIPAK@ZP6GI0K1I2@Z@Z",
    "?PvClientSet@DIS@@QAEPAXPAX@Z",
    "??0DISX86@@QAE@W4DIST@DIS@@@Z",
    "??0DISMIPS@@QAE@W4DIST@DIS@@@Z",
    "??0DISAXP@@QAE@W4DIST@DIS@@@Z",
    "??0DISSH@@QAE@W4DIST@DIS@@@Z",
    "?CchFormatInstr@DIS@@QBEIPADI@Z",
    "??0DISPPC@@QAE@W4DIST@DIS@@@Z",
])


def build_stub_dll() -> bytes:
    """A minimal i386 PE DLL exporting MSDIS_EXPORTS, all -> a `xor eax,eax; ret`.

    Hand-assembled: DOS+PE headers, one .text section (the 3-byte stub) and one
    .edata section (the export directory). Names are sorted so the loader's binary
    search over AddressOfNames succeeds.
    """
    n = len(MSDIS_EXPORTS)
    IMAGE_BASE, SA, FA = 0x10000000, 0x1000, 0x200
    TEXT_VA, EDATA_VA = 0x1000, 0x2000
    STUB = b"\x31\xc0\xc3"               # xor eax,eax ; ret
    DLLNAME = b"MSDIS100.DLL\x00"

    exp_dir_sz = 40
    eat_off = exp_dir_sz
    ent_off = eat_off + 4 * n
    ord_off = ent_off + 4 * n
    blob = bytearray(b"\x00" * exp_dir_sz)
    for _ in range(n):                   # EAT: every export -> the one stub
        blob += struct.pack("<I", TEXT_VA)
    ent_pos = len(blob)
    blob += b"\x00" * (4 * n)            # ENT (patched below)
    for i in range(n):                   # ordinal table: name i -> EAT[i]
        blob += struct.pack("<H", i)
    dllname_rva = EDATA_VA + len(blob)
    blob += DLLNAME
    name_rvas = []
    for nm in MSDIS_EXPORTS:
        name_rvas.append(EDATA_VA + len(blob))
        blob += nm.encode("ascii") + b"\x00"
    for i, rva in enumerate(name_rvas):
        struct.pack_into("<I", blob, ent_pos + 4 * i, rva)
    struct.pack_into("<IIHHIIIIIII", blob, 0,
                     0, 0, 0, 0, dllname_rva, 1, n, n,
                     EDATA_VA + eat_off, EDATA_VA + ent_off, EDATA_VA + ord_off)
    edata = bytes(blob)
    edata_vsz = len(edata)

    def al(x, a):
        return (x + a - 1) // a * a

    text_raw, edata_raw = al(len(STUB), FA), al(edata_vsz, FA)
    text_ptr = FA
    edata_ptr = text_ptr + text_raw
    size_img = al(EDATA_VA + edata_vsz, SA)

    opt = struct.pack(
        "<HBB III III III HHHHHH IIII HH IIIIII",
        0x10B, 5, 10, text_raw, edata_raw, 0, 0, TEXT_VA, EDATA_VA,
        IMAGE_BASE, SA, FA, 4, 0, 0, 0, 4, 0, 0, size_img, FA, 0,
        2, 0, 0x100000, 0x1000, 0x100000, 0x1000, 0, 16)
    dd = [(0, 0)] * 16
    dd[0] = (EDATA_VA, edata_vsz)        # data dir 0 = export table
    for va, sz in dd:
        opt += struct.pack("<II", va, sz)
    assert len(opt) == 0xE0, len(opt)
    # Characteristics: EXECUTABLE | 32BIT | DLL | RELOCS_STRIPPED
    coff = struct.pack("<HHIIIHH", 0x14C, 2, 0, 0, 0, 0xE0, 0x2103)
    mz = bytearray(0x80)
    mz[0:2] = b"MZ"
    struct.pack_into("<I", mz, 0x3c, 0x80)

    def sect(name, vsz, va, rsz, ptr, chars):
        return struct.pack("<8sIIIIIIHHI", name, vsz, va, rsz, ptr, 0, 0, 0, 0, chars)

    hdr = bytes(mz) + b"PE\x00\x00" + coff + opt
    hdr += sect(b".text", len(STUB), TEXT_VA, text_raw, text_ptr, 0x60000020)
    hdr += sect(b".edata", edata_vsz, EDATA_VA, edata_raw, edata_ptr, 0x40000040)
    out = bytearray(hdr)
    out += b"\x00" * (text_ptr - len(out))
    out += STUB + b"\x00" * (text_raw - len(STUB))
    out += edata + b"\x00" * (edata_raw - len(edata))
    return bytes(out)


def system_dir(wineprefix: Path) -> Path:
    """The 32-bit system dir link.exe loads DLLs from: syswow64 on a win64
    (WoW64) prefix, else system32 on a pure win32 prefix."""
    win = Path(wineprefix) / "drive_c" / "windows"
    wow = win / "syswow64"
    return wow if wow.is_dir() else win / "system32"


def ensure_msdis(wineprefix, msvc_dir=None, verbose=False) -> Path:
    """Make MSDIS100.DLL available to link.exe in `wineprefix`. Idempotent.

    Prefers a real `$MSVC_DIR/bin/MSDIS100.DLL` (case-insensitive) if the
    toolchain has one; otherwise drops a generated stub. Returns the installed
    path. Safe to call before every link.
    """
    dst = system_dir(Path(wineprefix)) / "MSDIS100.DLL"
    if dst.exists():
        return dst
    dst.parent.mkdir(parents=True, exist_ok=True)
    real = None
    msvc_dir = msvc_dir or os.environ.get("MSVC_DIR")
    if msvc_dir:
        binp = Path(msvc_dir) / "bin"
        if binp.is_dir():
            real = next((p for p in binp.iterdir()
                         if p.name.lower() == "msdis100.dll"), None)
    if real is not None:
        shutil.copyfile(real, dst)
        if verbose:
            print(f"[msdis] installed REAL {real} -> {dst}")
    else:
        dst.write_bytes(build_stub_dll())
        if verbose:
            print(f"[msdis] installed STUB -> {dst} "
                  f"({len(MSDIS_EXPORTS)} exports; link /dump /disasm only)")
    return dst


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit("usage: msdis_stub.py <out.dll>")
    Path(sys.argv[1]).write_bytes(build_stub_dll())
    print(f"wrote stub {sys.argv[1]} ({len(MSDIS_EXPORTS)} exports)")
