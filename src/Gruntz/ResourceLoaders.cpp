// ResourceLoaders.cpp - Win32 resource (RT_BITMAP / PALETTE) loaders re-homed
// out of the src/Stub/ApiCallers.cpp winapi grab-bag. Each host struct is a
// local view onto its (not-yet-recovered) owning class; offsets + emitted
// bytes are load-bearing. The WAVE pair (0x136a30/0x136ce0) moved onward to
// src/Dsndmgr/DirectSoundMgr.cpp (wave1-B) and the GDI counter-draw pair
// (0x164380/0x164420) to src/DDrawMgr/DDrawSurfacePair.cpp (wave1-C) - retail
// birth-positions both inside those TUs' blocks.
#include <Win32.h>
#include <ddraw.h> // IDirectDrawSurface (the palette path's surface source)
#include <rva.h>
#include <string.h>
#include <stdio.h> // sprintf (0x11f890)

// The app HINSTANCE used as the resource module (DAT_00683ee0).
DATA(0x00283ee0)
extern HINSTANCE g_resModule;
// The resource-module handle for palette lookups (DAT_006bf6e0).
DATA(0x002bf6e0)
extern HINSTANCE g_palModule_6bf6e0;
// Shared sprintf-into-buffer helper (also referenced from ApiCallers.cpp).

namespace ResLoaders {
    struct AppModule_136a30 {
        char m_pad0[8];
        HINSTANCE m_8; // +0x08 = the resource module handle
    };
    AppModule_136a30* AppModule_1d3631(); // RVA 0x1d3631 (global accessor)

    // The two WAVE-resource loaders (0x136a30 / 0x136ce0, the former
    // WaveHost_136a30/WaveHost2_136ce0 views) re-homed to src/Dsndmgr/
    // DirectSoundMgr.cpp as SoundDevice::AcquireResource / ReloadResource (the
    // DSNDMGR.CPP obj, per docs/exe-map/interval-dossiers.md: the +0x78 gate IS
    // SoundDevice::m_initialized, the callees ARE Acquire/ReloadRiff).

    // The former ResLoad_144270 view (RVA 0x144270) was RVA-proven to be
    // CDDSurface::Load (its Init@0x13e0a0 / Parse@0x13ece0 ARE CDDSurface::Init1 /
    // BlitDirect) and re-homed to src/Image/FileImage.cpp (the file-codec obj,
    // wave4-K, dossier #14I).

    // The former PalLoad_1479e0 view (RVA 0x1479e0) was RVA-proven to be
    // CDDPalette::LoadDefault (its Apply@0x147390 IS CDDPalette::Create) and
    // re-homed to src/DDrawMgr/DirPal.cpp (the DIRPAL.CPP obj, wave3-J).

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

    struct PalCache_17cd90 {
        char m_pad0[0x108];
        PALETTEENTRY m_108[0x100]; // +0x108
        void Snapshot(HWND hWnd);
    };
    // __thiscall(hWnd): read the system palette, then blank every entry to a
    // reserved black so a remap can be rebuilt against it.
    RVA(0x0017cd90, 0x58)
    void PalCache_17cd90::Snapshot(HWND hWnd) {
        HDC hdc = GetDC(hWnd);
        GetSystemPaletteEntries(hdc, 0, 0x100, m_108);
        for (i32 i = 0; i < 0x100; i++) {
            m_108[i].peRed = 0;
            m_108[i].peBlue = 0;
            m_108[i].peGreen = 0;
            m_108[i].peFlags = 4;
        }
        ReleaseDC(hWnd, hdc);
    }
} // namespace ResLoaders
