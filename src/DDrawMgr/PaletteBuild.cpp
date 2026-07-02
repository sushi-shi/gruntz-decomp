#include <Win32.h>

#include <rva.h>

// System-palette builder / tuner (RVA 0x176df0..0x1775f0), re-homed out of the
// artificial src/Stub/ApiCallers.cpp aggregate. GAME code: builds a 256-entry
// LOGPALETTE, marks the animatable interior PC_RESERVED, and loads PALETTE resources.

namespace ApiCallerStubs {
    i32 winapi_1770a0_CreateICA_DeleteDC_GetDeviceCaps(); // RVA 0x1770a0
    // __thiscall(flags, src): build a 256-entry LOGPALETTE from src and realize it.
    struct PalBuilder_176df0 {
        HPALETTE m_0;     // +0x00
        LOGPALETTE m_pal; // +0x04 (palVersion/palNumEntries/palPalEntry[1])
        char m_pad_entries[0x408 - (4 + 4 + 4)];
        i32 m_408; // +0x408
        i32 m_40c; // +0x40c
        i32 Build(PALETTEENTRY* src, i32 flags);
        void Tune1770e0(); // RVA 0x1770e0
    };
    RVA(0x00176df0, 0x71)
    i32 PalBuilder_176df0::Build(PALETTEENTRY* src, i32 flags) {
        m_408 = flags;
        m_pal.palNumEntries = 0x100;
        m_pal.palVersion = 0x300;
        DWORD* s = (DWORD*)src;
        PALETTEENTRY* d = m_pal.palPalEntry;
        i32 i = 0x100;
        do {
            *(DWORD*)d = *s++;
            d->peFlags = 0;
            d++;
        } while (--i);
        if (winapi_1770a0_CreateICA_DeleteDC_GetDeviceCaps() && !(flags & 1)) {
            Tune1770e0();
            m_40c = 1;
        }
        m_0 = CreatePalette(&m_pal);
        return m_0 != 0;
    }

    // __thiscall: delete the owned GDI object, then clear a far flag.
    struct DeleteObjHost_177070 {
        HGDIOBJ m_obj; // +0x00
        char m_pad[0x408 - 4];
        i32 m_408; // +0x408
        void Run();
    };
    RVA(0x00177070, 0x22)
    void DeleteObjHost_177070::Run() {
        if (m_obj) {
            DeleteObject(m_obj);
            m_obj = 0;
        }
        m_408 = 0;
    }

    // __cdecl(): does the display device support a palette? (RC_PALETTE bit)
    RVA(0x001770a0, 0x3a)
    i32 winapi_1770a0_CreateICA_DeleteDC_GetDeviceCaps() {
        HDC ic = CreateICA("DISPLAY", 0, 0, 0);
        if (ic) {
            i32 caps = GetDeviceCaps(ic, RASTERCAPS) & RC_PALETTE;
            DeleteDC(ic);
            return caps;
        }
        return 0;
    }

    void winapi_177160_CreatePalette_DeleteObject_GetDC_RealizePalette_ReleaseD(); // RVA 0x177160
    // __thiscall(): snapshot the reserved system-palette entries, marking the
    // interior animatable range PC_RESERVED (peFlags=1).
    RVA(0x001770e0, 0x7c)
    void PalBuilder_176df0::Tune1770e0() {
        winapi_177160_CreatePalette_DeleteObject_GetDC_RealizePalette_ReleaseD();
        HDC dc = CreateDCA("DISPLAY", 0, 0, 0);
        i32 sizePal = GetDeviceCaps(dc, SIZEPALETTE);
        i32 numReserved = GetDeviceCaps(dc, NUMRESERVED);
        i32 half = numReserved / 2;
        GetSystemPaletteEntries(dc, 0, half, m_pal.palPalEntry);
        GetSystemPaletteEntries(
            dc,
            sizePal - half,
            half,
            &m_pal.palPalEntry[m_pal.palNumEntries - half]
        );
        for (i32 i = half; i < sizePal - half; i++) {
            m_pal.palPalEntry[i].peFlags = 1;
        }
        DeleteDC(dc);
    }

    // __cdecl: realize an all-black 256-entry palette on the screen DC to reset it.
    RVA(0x00177160, 0x81)
    void winapi_177160_CreatePalette_DeleteObject_GetDC_RealizePalette_ReleaseD() {
        char buf[4 + 256 * sizeof(PALETTEENTRY)];
        LOGPALETTE* lp = (LOGPALETTE*)buf;
        HDC hdc = GetDC(0);
        lp->palVersion = 0x300;
        lp->palNumEntries = 256;
        for (i32 i = 0; i < 256; i++) {
            lp->palPalEntry[i].peRed = 0;
            lp->palPalEntry[i].peGreen = 0;
            lp->palPalEntry[i].peBlue = 0;
            lp->palPalEntry[i].peFlags = 4;
        }
        HPALETTE hpal = CreatePalette(lp);
        if (hpal) {
            HPALETTE old = SelectPalette(hdc, hpal, FALSE);
            RealizePalette(hdc);
            DeleteObject(SelectPalette(hdc, old, FALSE));
        }
        ReleaseDC(0, hdc);
    }

    // The resource-module handle for palette lookups (DAT_006bf6e0).
    DATA(0x006bf6e0)
    extern HINSTANCE g_palModule_6bf6e0;
    struct PalHost_1775f0 {
        i32 Apply(const char* name, i32 arg);
        i32 Use176e70(void* data, i32 arg); // thiscall, RVA 0x176e70
    };
    // __thiscall(name, arg): find/load/lock a PALETTE resource, hand it on.
    RVA(0x001775f0, 0x62)
    i32 PalHost_1775f0::Apply(const char* name, i32 arg) {
        HINSTANCE mod = g_palModule_6bf6e0;
        if (!mod) {
            return 0;
        }
        HRSRC hRsrc = FindResourceA(mod, name, "PALETTE");
        if (!hRsrc) {
            return 0;
        }
        HGLOBAL hRes = LoadResource(mod, hRsrc);
        if (!hRes) {
            return 0;
        }
        void* data = LockResource(hRes);
        if (!data) {
            return 0;
        }
        return Use176e70(data, arg);
    }

} // namespace ApiCallerStubs
