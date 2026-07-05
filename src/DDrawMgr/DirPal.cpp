// DirPal.cpp - C:\Proj\DDrawMgr\DIRPAL.CPP. The DirectDraw palette helper that
// snapshots the Windows system-reserved palette entries (the low + high halves
// GDI keeps for the shell) into the object's working palette, then installs it.
// Migrated out of src/Stub/ApiCallers.cpp (RVA 0x1485b0). Placeholder
// m_<hexoffset> field names; only OFFSETS + code bytes are load-bearing. GDI
// imports come from <Win32.h> (call [__imp_*]); the install/assert helpers are
// modeled with NO body so their rel32 references reloc-mask.
#include <Win32.h>
#include <ddraw.h> // real IDirectDrawPalette (the palette snapshotted via GetEntries slot 4)

#include <rva.h>

// The install worker (0x147aa0) + the DIRPAL.CPP assert-logger (0x141400).
extern "C" void ErrLog_141400(const char* file, i32 line, i32 code);

// A stack LOGPALETTE with a full 256-entry table (the 0x410-byte frame).
struct LogPal256 {
    u16 palVersion;                // +0x00
    u16 palNumEntries;             // +0x02
    PALETTEENTRY palPalEntry[256]; // +0x04
};

// The Rez heap allocator (_RezAlloc), backing the lazily-cloned working copy.
extern "C" void* RezAlloc(u32 size);

// The palette snapshotted into the working buffer is the real IDirectDrawPalette
// (<ddraw.h>): vtable slot 4 (+0x10) is GetEntries(dwFlags, dwBase, dwNumEntries,
// lpEntries) - the call m_4->GetEntries(0, 0, 0x100, entries) reads all 256 entries
// into the caller buffer. Was a hand-rolled PalSurfVtbl PMF view naming only that
// slot; folded to the real SDK interface (the IDirectDrawSurface/IDirectPlay4
// precedent). Byte-neutral COM dispatch (`mov eax,[m_4]; call [eax+0x10]`).

// A second class in DIRPAL.CPP: the DirectDraw palette-fade context. Rebuilds the
// palette snapshot, caches the fade params (3 bytes at +0x1c), timestamps, lazily
// clones the source palette into a working copy, then finalises. Setup6/Setup4
// both reach the same DIRPAL.CPP error logger (0x141400) as DirPal::Capture below.
struct PalCtx {
    char m_pad0[4];
    IDirectDrawPalette* m_4; // +0x04  the DirectDraw palette (GetEntries snapshot)
    char m_pad8[0xc - 8];
    char* m_c; // +0x0c (the 0x400 source palette buffer)
    char m_pad10[0x14 - 0x10];
    i32 m_14;   // +0x14
    char* m_18; // +0x18 (lazily-allocated 0x400 working copy)
    char m_1c;  // +0x1c
    char m_1d;  // +0x1d
    char m_1e;  // +0x1e
    char m_pad1f[0x20 - 0x1f];
    i32 m_20;        // +0x20
    i32 m_24;        // +0x24 (timeGetTime stamp)
    i32 m_28;        // +0x28
    i32 m_2c;        // +0x2c
    i32 m_30;        // +0x30
    i32 m_34;        // +0x34 (1 once set up)
    void Teardown(); // thiscall, RVA 0x148250
    void Finalize(); // thiscall, RVA 0x1480a0
    void Setup6(i32 a, i32 b, char c3, char c4, char c5, i32 a6);
    void Setup4(i32 a, i32 b, i32 a3, i32 a4);
};

// __thiscall(a,b,c3,c4,c5,a6): rebuild the palette snapshot, cache the params
// (3 bytes at +0x1c), timestamp, lazily clone the source palette, then finalize.
RVA(0x00147f30, 0xbe)
void PalCtx::Setup6(i32 a, i32 b, char c3, char c4, char c5, i32 a6) {
    if (m_34) {
        Teardown();
    }
    i32 err = m_4->GetEntries(0, 0, 0x100, (LPPALETTEENTRY)m_c);
    if (err) {
        ErrLog_141400("C:\\Proj\\DDrawMgr\\DIRPAL.CPP", 0x311, err);
    }
    m_2c = a;
    m_30 = b;
    m_20 = a6;
    m_24 = timeGetTime();
    m_28 = -1;
    m_14 = 0;
    m_1c = c3;
    m_1d = c4;
    m_1e = c5;
    if (!m_18) {
        m_18 = (char*)RezAlloc(0x400);
    }
    for (i32 i = 0; i < 0x400; i += 4) {
        *(i32*)(m_18 + i) = *(i32*)(m_c + i);
    }
    m_34 = 1;
    Finalize();
}

// __thiscall(a,b,a3,a4): same as Setup6 but stores a3 at +0x14 and uses the
// 0x34b log line; returns void.
RVA(0x00147ff0, 0xa9)
void PalCtx::Setup4(i32 a, i32 b, i32 a3, i32 a4) {
    if (m_34) {
        Teardown();
    }
    i32 err = m_4->GetEntries(0, 0, 0x100, (LPPALETTEENTRY)m_c);
    if (err) {
        ErrLog_141400("C:\\Proj\\DDrawMgr\\DIRPAL.CPP", 0x34b, err);
    }
    m_2c = a;
    m_30 = b;
    m_20 = a4;
    m_24 = timeGetTime();
    m_14 = a3;
    m_28 = -1;
    if (!m_18) {
        m_18 = (char*)RezAlloc(0x400);
    }
    for (i32 i = 0; i < 0x400; i += 4) {
        *(i32*)(m_18 + i) = *(i32*)(m_c + i);
    }
    m_34 = 1;
    Finalize();
}

struct DirPal {
    char m_pad0[0xc];
    PALETTEENTRY* m_c; // +0x0c working palette (256 entries)
    i32 Install(i32 start, i32 count, PALETTEENTRY* pal, i32 flag); // 0x147aa0 __thiscall
    i32 CaptureSystemPalette();                                     // 0x1485b0
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
    if (!GetSystemPaletteEntries(
            hdc,
            sizePal - half,
            half,
            &lp.palPalEntry[lp.palNumEntries - half]
        )) {
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

SIZE_UNKNOWN(LogPal256);
SIZE_UNKNOWN(DirPal);
SIZE_UNKNOWN(PalCtx);
