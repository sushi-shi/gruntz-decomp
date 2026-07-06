// ResourceLoaders.cpp - Win32 resource (WAVE / RT_BITMAP / PALETTE) loaders and
// GDI counter-draw helpers re-homed out of the src/Stub/ApiCallers.cpp winapi
// grab-bag. Each host struct is a local view onto its (not-yet-recovered) owning
// class; offsets + emitted bytes are load-bearing. The moves are byte-neutral
// (only FindResource/LoadResource/GDI + sprintf).
#include <Win32.h>
#include <ddraw.h> // IDirectDrawSurface (the counter window's GetDC/ReleaseDC source)
#include <Dsndmgr/DirectSoundMgr.h> // real DirectSoundMgr (the WAVE-load looping gate)
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

    struct WaveHost_136a30 {
        char m_pad0[0x78];
        i32 m_78; // +0x78
        i32 LoadWave(const char* name, i32 a, i32 b);
        i32 Use136910(void* data, i32 a, i32 b); // thiscall, RVA 0x136910
    };
    // __thiscall(name, a, b): find/load/lock a WAVE resource, hand it to Use136910.
    RVA(0x00136a30, 0x76)
    i32 WaveHost_136a30::LoadWave(const char* name, i32 a, i32 b) {
        if (!m_78) {
            return 0;
        }
        HINSTANCE mod1 = AppModule_1d3631()->m_8;
        HRSRC hRsrc = FindResourceA(mod1, name, "WAVE");
        if (!hRsrc) {
            return 0;
        }
        HINSTANCE mod2 = AppModule_1d3631()->m_8;
        HGLOBAL hRes = LoadResource(mod2, hRsrc);
        if (!hRes) {
            return 0;
        }
        void* data = LockResource(hRes);
        if (!data) {
            return 0;
        }
        return Use136910(data, a, b);
    }

    struct WaveHost2_136ce0 {
        char m_pad0[0x78];
        i32 m_78; // +0x78
        i32 LoadWave(DirectSoundMgr* probe, const char* name, i32 arg);
        i32 Use136bd0(DirectSoundMgr* probe, void* data, i32 arg); // thiscall, RVA 0x136bd0
    };
    // __thiscall(probe, name, arg): if the probe (a DS buffer) is looping, find/load/lock the WAVE.
    RVA(0x00136ce0, 0x92)
    i32 WaveHost2_136ce0::LoadWave(DirectSoundMgr* probe, const char* name, i32 arg) {
        if (!m_78) {
            return 0;
        }
        if (probe->IsLooping() == 0) {
            return 1;
        }
        HINSTANCE mod1 = AppModule_1d3631()->m_8;
        HRSRC hRsrc = FindResourceA(mod1, name, "WAVE");
        if (!hRsrc) {
            return 0;
        }
        HINSTANCE mod2 = AppModule_1d3631()->m_8;
        HGLOBAL hRes = LoadResource(mod2, hRsrc);
        if (!hRes) {
            return 0;
        }
        void* data = LockResource(hRes);
        if (!data) {
            return 0;
        }
        return Use136bd0(probe, data, arg);
    }

    // The header of the locked RT_BITMAP resource (its +0xe must be 8).
    struct ResHdr_144270 {
        i32 m_0; // +0x00 (payload size; data follows at +m_0+0x400)
        i32 m_4; // +0x04
        i32 m_8; // +0x08
        char m_padc[0xe - 0xc];
        i16 m_e; // +0x0e (must be 8)
    };
    struct ResLoad_144270 {
        char m_pad0[0x10];
        i32 m_10; // +0x10 (set to 0x6c after a 0x6c-byte zero-fill)
        i32 m_14; // +0x14
        i32 m_18; // +0x18
        i32 m_1c; // +0x1c
        char m_pad20[0x78 - 0x20];
        i32 m_78;                        // +0x78
        i32 Init(i32 saved);             // thiscall, RVA 0x13e0a0
        void Parse(char* data, i32 two); // thiscall, RVA 0x13ece0
        i32 Load(i32 a, char* name, i32 c);
    };
    // __thiscall(a, name, c): find/load/lock the named RT_BITMAP, validate its
    // header (+0xe==8), zero a 0x6c-byte block, seed the loader fields, init it,
    // then parse the payload that follows the 0x400-byte header.
    RVA(0x00144270, 0xd2)
    i32 ResLoad_144270::Load(i32 a, char* name, i32 c) {
        HRSRC hr = FindResourceA(g_resModule, name, (LPCSTR)2);
        if (!hr) {
            return 0;
        }
        HGLOBAL hg = LoadResource(g_resModule, hr);
        if (!hg) {
            return 0;
        }
        ResHdr_144270* p = (ResHdr_144270*)LockResource(hg);
        if (!p) {
            return 0;
        }
        i32 saved = p->m_8;
        if (p->m_e != 8) {
            return 0;
        }
        memset(&m_10, 0, 0x6c);
        m_10 = 0x6c;
        m_78 = c | 0x40;
        m_14 = 7;
        m_1c = p->m_4;
        m_18 = c;
        if (!Init(saved)) {
            return 0;
        }
        Parse((char*)p + p->m_0 + 0x400, 2);
        return 1;
    }

    struct PalLoad_1479e0 {
        i32 Apply(i32 a, PALETTEENTRY* pal, i32 c); // thiscall, RVA 0x147390
        i32 Load(i32 a, char* name, i32 c);
    };
    // __thiscall(a, name, c): load the named PALETTE resource as 256 RGB triples,
    // expand to PALETTEENTRY[256] (flags 0), and apply it.
    RVA(0x001479e0, 0xbb)
    i32 PalLoad_1479e0::Load(i32 a, char* name, i32 c) {
        PALETTEENTRY pal[256];
        HRSRC hr = FindResourceA(g_resModule, name, "PALETTE");
        if (!hr) {
            return 0;
        }
        HGLOBAL hg = LoadResource(g_resModule, hr);
        if (!hg) {
            return 0;
        }
        char* src = (char*)LockResource(hg);
        if (!src) {
            return 0;
        }
        for (i32 i = 0; i < 256; i++) {
            pal[i].peRed = src[0];
            pal[i].peGreen = src[1];
            pal[i].peBlue = src[2];
            pal[i].peFlags = 0;
            src += 3;
        }
        return Apply(a, pal, c);
    }

    // The counter window's DC source at +0x08 is a real IDirectDrawSurface COM
    // interface (<ddraw.h>): GetDC (slot 17, +0x44) and ReleaseDC (slot 26, +0x68)
    // are the standard __stdcall COM slots (self pushed on the stack), so the calls
    // fold to `push arg; push surface; call [vtbl+slot]` cast-free.
    struct CounterWnd_164380 {
        char m_pad0[8];
        IDirectDrawSurface* m_8; // +0x08  DC-capable DirectDraw surface
    };
    struct DrawHost_164380 {
        char m_pad0[0x2c];
        CounterWnd_164380* m_2c; // +0x2c
        void DrawCount(RECT* rc, i32 n);
    };
    // __thiscall(rc, n): print n centred into rc using the counter window's DC.
    RVA(0x00164380, 0x98)
    void DrawHost_164380::DrawCount(RECT* rc, i32 n) {
        char buf[0x20];
        sprintf(buf, "%i", n);
        CounterWnd_164380* w = m_2c;
        if (!w) {
            return;
        }
        HDC hdc = 0;
        w->m_8->GetDC(&hdc);
        if (!hdc) {
            return;
        }
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, 0xffffff);
        DrawTextA(hdc, buf, strlen(buf), rc, 0x25);
        w->m_8->ReleaseDC(hdc);
    }

    struct DrawHost2_164420 {
        char m_pad0[0x2c];
        CounterWnd_164380* m_2c; // +0x2c
        void DrawLabel(RECT* rc, char* text);
    };
    // __thiscall(rc, text): print text centred into rc using the counter window's DC.
    RVA(0x00164420, 0x79)
    void DrawHost2_164420::DrawLabel(RECT* rc, char* text) {
        CounterWnd_164380* w = m_2c;
        if (!w) {
            return;
        }
        HDC hdc = 0;
        w->m_8->GetDC(&hdc);
        if (!hdc) {
            return;
        }
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, 0xffffff);
        DrawTextA(hdc, text, strlen(text), rc, 0x25);
        w->m_8->ReleaseDC(hdc);
    }

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
