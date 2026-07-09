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

    // (0x118930/0x118960/0x118990 SetActiveWindow_SetFocus / Set/ClearTopmostExStyle
    // re-homed to Utils::WinAPI in src/Utils/WinAPI.cpp.)

    // (0x1192d0 winapi_1192d0_IsIconic re-homed to BlockScreenSaver in
    // src/Utils/TimeSplit.cpp (RVA-adjacent free helper).)

    // (0x137e30 Throttle_137e30::Tick + 0x1380d0 Timer_1380d0::Tick re-homed to
    // StreamFeeder::Tick / StreamFeeder::TickPump in src/Dsndmgr/StreamFeeder.cpp -
    // their placeholder fields were a documented 1:1 map onto StreamFeeder.)

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
