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
    // @identity-TODO: this is NOT the CRect engine-rect class (that was recovered for
    // 0x8c380/0x115b30 -> src/Wap32/Rect.cpp). `this` is a bigger clip-CONTEXT object
    // (source dims m_c/m_10 @+0xc/+0x10, clip rect @+0x60, clipped dims @+0x70/+0x74).
    // It is called directly (not a vtable slot) on divergent `this` from map/movement
    // methods (CBattlezMapConfig / CGruntMover::Step / CStepMgr::Step / CGrunt::PathScan,
    // xref) with no ctor/new/RTTI trace, so the owning class identity is unrecovered.
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

    // (0x8c380 CRect::SetRect + 0x115b30 CRect::operator= re-homed to the real
    // WAP32 engine rectangle class CRect in src/Wap32/Rect.cpp - `this` IS the
    // tagRECT; Ghidra/FID attests ??0CRect@@QAE@HHHH@Z for the sibling direct-store
    // ctor @0x29ac0. The old RectHost_08c380/RectHost_115b30 per-fn views were CRect.)

    // (0x118930/0x118960/0x118990 SetActiveWindow_SetFocus / Set/ClearTopmostExStyle
    // re-homed to Utils::WinAPI in src/Utils/WinAPI.cpp.)

    // (0x1192d0 winapi_1192d0_IsIconic re-homed to BlockScreenSaver in
    // src/Utils/TimeSplit.cpp (RVA-adjacent free helper).)

    // (0x137e30 Throttle_137e30::Tick + 0x1380d0 Timer_1380d0::Tick re-homed to
    // StreamFeeder::Tick / StreamFeeder::TickPump in src/Dsndmgr/StreamFeeder.cpp -
    // their placeholder fields were a documented 1:1 map onto StreamFeeder.)

    // (GeoHost_161e80::Build @0x161e80 re-homed to src/Gruntz/GameLevel.cpp as
    // CLevelPlane::Build - the local view IS CLevelPlane (a CGameLevel per-plane
    // object, m_planes[i]); xref-proven (CGameLevel::SetExtentsAndBuildAll /
    // BuildAllPlanes call it on esi->m_38[i]). Build was already declared @0x161e80
    // in GameLevel.h with the sibling RecomputePlaneCoords @0x161c90 matched there.)

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
