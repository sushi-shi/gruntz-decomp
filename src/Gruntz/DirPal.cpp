// DirPal.cpp - C:\Proj\DDrawMgr\DIRPAL.CPP. The DirectDraw palette helper that
// snapshots the Windows system-reserved palette entries (the low + high halves
// GDI keeps for the shell) into the object's working palette, then installs it.
// Migrated out of src/Stub/ApiCallers.cpp (RVA 0x1485b0). Placeholder
// m_<hexoffset> field names; only OFFSETS + code bytes are load-bearing. GDI
// imports come from <Win32.h> (call [__imp_*]); the install/assert helpers are
// modeled with NO body so their rel32 references reloc-mask.
#include <Win32.h>

#include <rva.h>

// The install worker (0x147aa0) + the DIRPAL.CPP assert-logger (0x141400).
extern "C" void ErrLog_141400(const char* file, i32 line, i32 code);

// A stack LOGPALETTE with a full 256-entry table (the 0x410-byte frame).
struct LogPal256 {
    u16 palVersion;               // +0x00
    u16 palNumEntries;            // +0x02
    PALETTEENTRY palPalEntry[256]; // +0x04
};

struct DirPal {
    char m_pad0[0xc];
    PALETTEENTRY* m_c; // +0x0c working palette (256 entries)
    i32 Install(i32 start, i32 count, PALETTEENTRY* pal, i32 flag); // 0x147aa0 __thiscall
    i32 CaptureSystemPalette(); // 0x1485b0
};

// @early-stop
// reloc-typing scoring artifact (~68%, was a 1.6% `return 0` stub). The CODE
// BYTES match retail instruction-for-instruction (verified base-vs-target with
// llvm-objdump -dr: prologue, both GDI-reserved copy loops - cl's strength-
// reduced single-induction/base-biased pointer walk - and the install/error
// tail are all byte-identical). The residual is only that retail reaches GDI
// through its own fn-ptr table (PTR_CreateDCA_006c3e20 etc.) while our base
// emits `ff 15 [__imp_*]`: same `ff 15` DIR32 form, differently-classed reloc
// target, so objdiff scores the ~9 relocated call sites as fuzzy. Routing
// through the named game globals instead scored LOWER (65%), confirming this is
// reloc-typing, not a codegen miss.
RVA(0x001485b0, 0x162)
i32 DirPal::CaptureSystemPalette() {
    HDC hdc = CreateDCA("DISPLAY", 0, 0, 0);
    if (!hdc) {
        return 0;
    }
    i32 sizePal = GetDeviceCaps(hdc, SIZEPALETTE);
    i32 half = GetDeviceCaps(hdc, NUMRESERVED) / 2;
    LogPal256 lp;
    lp.palVersion = 0x300;
    lp.palNumEntries = 0x100;
    if (!GetSystemPaletteEntries(hdc, 0, half, lp.palPalEntry)) {
        return 0;
    }
    if (!GetSystemPaletteEntries(hdc, sizePal - half, half,
                                 &lp.palPalEntry[lp.palNumEntries - half])) {
        return 0;
    }
    DeleteDC(hdc);
    PALETTEENTRY* dest = m_c;
    if (!dest) {
        return 0;
    }
    i32 i;
    for (i = 0; i < half; i++) {
        dest[i].peRed = lp.palPalEntry[i].peRed;
        dest[i].peGreen = lp.palPalEntry[i].peGreen;
        dest[i].peBlue = lp.palPalEntry[i].peBlue;
    }
    for (i = sizePal - half; i < sizePal; i++) {
        dest[i].peRed = lp.palPalEntry[i].peRed;
        dest[i].peGreen = lp.palPalEntry[i].peGreen;
        dest[i].peBlue = lp.palPalEntry[i].peBlue;
    }
    i32 rc = Install(0, 0x100, dest, 0);
    if (rc != 0) {
        ErrLog_141400("C:\\Proj\\DDrawMgr\\DIRPAL.CPP", 0x495, rc);
        return 0;
    }
    return 1;
}
