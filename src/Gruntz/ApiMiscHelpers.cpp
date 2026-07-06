// ApiMiscHelpers.cpp - small self-contained geometry / window / timer / object-
// lifetime helpers re-homed out of the src/Stub/ApiCallers.cpp winapi grab-bag.
// Each host struct is a local view onto its (not-yet-recovered) owning class;
// offsets + emitted bytes are load-bearing, field names are placeholders. These
// functions touch only Win32 + timeGetTime, so the move is byte-neutral.
#include <Win32.h>
#include <Dsndmgr/DirectSoundMgr.h> // real DirectSoundMgr (the DS play-cursor timing source)
#include <rva.h>

namespace ApiMisc {
    // __thiscall(src): clip the (0,0,m_c,m_10) box against the optional src rect
    // (right/bottom inclusive->+1), store the clipped rect at +0x60 and its w/h.
    struct ClipHost_02b340 {
        char m_pad0[0xc];
        i32 m_c;  // +0x0c
        i32 m_10; // +0x10
        char m_pad14[0x60 - 0x14];
        RECT m_rc60; // +0x60
        i32 m_w70;   // +0x70
        i32 m_h74;   // +0x74
        void Clip(const RECT* src);
    };
    RVA(0x0002b340, 0xaa)
    void ClipHost_02b340::Clip(const RECT* src) {
        RECT a, b;
        b.left = 0;
        b.top = 0;
        b.right = m_c;
        b.bottom = m_10;
        if (src) {
            a.left = src->left;
            a.top = src->top;
            a.right = src->right + 1;
            a.bottom = src->bottom + 1;
        } else {
            a.left = 0;
            a.top = 0;
            a.right = m_c;
            a.bottom = m_10;
        }
        if (!IntersectRect(&m_rc60, &a, &b)) {
            m_rc60 = a;
        }
        m_w70 = m_rc60.right - m_rc60.left;
        m_h74 = m_rc60.bottom - m_rc60.top;
    }

    // __thiscall(l, t, r, b): the object IS the RECT being initialised.
    struct RectHost_08c380 {
        RECT m_rc;
        void Set(i32 l, i32 t, i32 r, i32 b);
    };
    RVA(0x0008c380, 0x1e)
    void RectHost_08c380::Set(i32 l, i32 t, i32 r, i32 b) {
        SetRect(&m_rc, l, t, r, b);
    }

    // __thiscall(src): copy src into this RECT; return this.
    struct RectHost_115b30 {
        RECT m_rc;
        RectHost_115b30* Copy(const RECT* src);
    };
    RVA(0x00115b30, 0x15)
    RectHost_115b30* RectHost_115b30::Copy(const RECT* src) {
        CopyRect(&m_rc, src);
        return this;
    }

    // __cdecl: activate + focus the same window.
    RVA(0x00118930, 0x15)
    void winapi_118930_SetActiveWindow_SetFocus(HWND hWnd) {
        SetActiveWindow(hWnd);
        SetFocus(hWnd);
    }

    // __cdecl(hWnd, msg, wParam): block screen-saver / monitor-power while not iconic.
    RVA(0x001192d0, 0x39)
    i32 winapi_1192d0_IsIconic(HWND hWnd, i32 msg, i32 wParam) {
        if (msg == 0x112) {
            i32 sc = wParam & 0xfff0;
            if (sc == 0xf140 || sc == 0xf170) {
                if (!IsIconic(hWnd)) {
                    return 1;
                }
            }
        }
        return 0;
    }

    // __thiscall(timestamp): throttle to >0x64 ms since the last tick, query the
    // source (m_8), wrap localC against m_c into the window, then run the work pass.
    struct Throttle_137e30 {
        char m_pad0[8];
        DirectSoundMgr* m_8; // +0x08  DS play-cursor timing source
        i32 m_c;             // +0x0c
        i32 m_10;            // +0x10
        i32 m_14;            // +0x14
        char m_pad18[0x1c - 0x18];
        i32 m_1c; // +0x1c
        char m_pad20[0x28 - 0x20];
        i32 m_28; // +0x28
        i32 Tick(i32 timestamp);
        i32 Work(i32 a, i32 b); // RVA 0x137f30
    };
    RVA(0x00137e30, 0x98)
    i32 Throttle_137e30::Tick(i32 timestamp) {
        if (!m_1c) {
            return 1;
        }
        i32 t = (timestamp == -1) ? (i32)timeGetTime() : timestamp;
        if ((u32)t <= (u32)(m_28 + 0x64)) {
            return 1;
        }
        m_28 = t;
        u32 hi, lo;
        if (!m_8->GetCurrentPosition(&hi, &lo)) {
            return 0;
        }
        i32 v;
        if ((u32)hi >= (u32)m_c) {
            if (hi == m_c) {
                v = m_10;
            } else {
                v = hi - m_c;
            }
        } else {
            v = m_10 + hi - m_c;
        }
        if ((u32)v < (u32)m_14) {
            return 1;
        }
        return Work(m_c, v) != 0;
    }

    // __thiscall(timestamp) host: timestamp -1 means "now"; prep the device, then
    // run the work pass (RVA 0x137f30) over m_c/m_10. Returns whether it ran.
    struct Timer_1380d0 {
        char m_pad0[0x8];
        DirectSoundMgr* m_8; // +0x08  DS buffer (SetCurrentPosition(0))
        i32 m_c;             // +0x0c
        i32 m_10;            // +0x10
        char m_pad14[0x20 - 0x14];
        i32 m_20; // +0x20
        char m_pad24[0x28 - 0x24];
        i32 m_28; // +0x28
        i32 Tick(i32 timestamp);
        i32 Work(i32 a, i32 b); // RVA 0x137f30
    };
    RVA(0x001380d0, 0x4e)
    i32 Timer_1380d0::Tick(i32 timestamp) {
        i32 t = (timestamp == -1) ? (i32)timeGetTime() : timestamp;
        m_28 = t;
        m_c = 0;
        if (!m_8->SetCurrentPosition(0)) {
            return 0;
        }
        m_20 = 0;
        return Work(m_c, m_10) != 0;
    }

    // __thiscall(RECT*): cache the bounds rect + derived size/center, then recompute.
    struct GeoHost_161e80 {
        char m_pad0[0x50];
        RECT m_50; // +0x50 bounds
        char m_pad60[0x70 - 0x60];
        i32 m_70; // +0x70 width
        i32 m_74; // +0x74 height
        i32 m_78; // +0x78 half-width
        i32 m_7c; // +0x7c half-height
        void Build(RECT* pRect);
        void Recompute(); // RVA 0x161c90
    };
    RVA(0x00161e80, 0x79)
    void GeoHost_161e80::Build(RECT* pRect) {
        if (pRect->left != (LONG)0x80000000) {
            RECT local;
            CopyRect(&local, pRect);
            m_50 = local;
            i32 width = m_50.right - m_50.left + 1;
            i32 height = m_50.bottom - m_50.top + 1;
            m_70 = width;
            m_74 = height;
            m_78 = width / 2;
            m_7c = height / 2;
            Recompute();
        }
    }

    // __thiscall: free the owned module handle if present.
    struct LibHost_1bf577 {
        HMODULE m_0; // +0x00
        void Run();
    };
    RVA(0x001bf577, 0xe)
    void LibHost_1bf577::Run() {
        if (m_0) {
            FreeLibrary(m_0);
        }
    }

    // __thiscall(): free the owned global handle at +0x00 if present.
    struct GlobalOwner_1c09de {
        HGLOBAL m_0; // +0x00
        void Free();
    };
    RVA(0x001c09de, 0xe)
    void GlobalOwner_1c09de::Free() {
        if (m_0) {
            GlobalFree(m_0);
        }
    }
} // namespace ApiMisc
