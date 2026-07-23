// TitleAppStart.cpp - OnStart (0xf9880, __thiscall ret 4): hide the cursor (spin
// ShowCursor until the count goes negative), kick off the title sequence, arm a 60s
// (0xea60) timer at +0x1b8, return 1. Split out of the AppHelpers.cpp holding TU.
//
// @identity-TODO (owner deferred): OnStart calls RunTitleSeq on `this`, and retail
// 0xfa350 = ?RunTitleSeq@CAttract@@QAEHPBDHHHH@Z [attract] - so `this` is a CAttract*.
// But (1) OnStart @0xf9880 sits far from the CAttract obj block (which starts at
// FadeInTitle @0xfa1f0, amid the splashstate/gamemode objs) and (2) it writes an int
// timer (0xea60) to +0x1b8, which Attract.h models as a CAttractHost* sound/host
// sub-object - a real layout conflict. So the CAttract binding is NOT forced here: the
// CTitleApp placeholder host keeps the RVA (RunTitleSeq declared-only, reloc-masked to
// the real CAttract::RunTitleSeq at 0xfa350). Resolve the +0x1b8 conflict + the obj
// placement in a dedicated CAttract pass before binding.
#include <rva.h>
#include <Mfc.h>          // afx-first (unified CObject; superset of Win32.h) - ::ShowCursor fn-ptr
#include <Gruntz/State.h> // the CState base this title state derives (RunTitleSeq @0xfa350)

#include <Gruntz/AssetRoot.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>  // m_world->m_drawTarget (the splash page clear)
#include <DDrawMgr/DDrawSubMgrPages.h> // ClearAllPages @0x158d50
#include <Gruntz/TitleApp.h>           // canonical CTitleApp (CState leaf; identity @identity-TODO)
#include <Gruntz/SplashState.h> // CSplashState: its slot-9 override is DEFINED here (retail TU placement)

RVA(0x000f9880, 0x43)
i32 CSplashState::Vslot09(i32) {
    int(WINAPI * sc)(BOOL) = ::ShowCursor;
    while (sc(0) >= 0) {
    }
    RunTitleSeq(
        static_cast<const char*>(CAssetRootStorage::s_value),
        1,
        1,
        1,
        0
    ); // CState::RunTitleSeq @0xfa350
    m_1b8 = 0xea60;
    return 1;
}
RVA(0x000f98f0, 0x16)
i32 CSplashState::FrameSlot28(i32) {
    m_world->m_drawTarget->ClearAllPages(0);
    return 1;
}
