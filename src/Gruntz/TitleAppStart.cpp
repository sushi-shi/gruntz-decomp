#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/AssetRoot.h>
#include <Gruntz/SplashState.h>
#include <Gruntz/State.h>

RVA(0x000f9880, 0x43)
i32 CSplashState::EnterState(i32) {
    int(WINAPI * sc)(BOOL) = ShowCursor;
    while (sc(0) >= 0) {
    }
    RunTitleSeq(static_cast<const char*>(CAssetRootStorage::s_value), 1, 1, 1, 0);
    m_splashCountdownMs = 0xea60;
    return 1;
}
RVA(0x000f98f0, 0x16)
i32 CSplashState::LeaveState(i32) {
    m_world->m_drawTarget->ClearAllPages(0);
    return 1;
}
