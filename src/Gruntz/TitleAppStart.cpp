#include <rva.h>
#include <Mfc.h>
#include <Gruntz/State.h>

#include <Gruntz/AssetRoot.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <Gruntz/TitleApp.h>
#include <Gruntz/SplashState.h>

RVA(0x000f9880, 0x43)
i32 CSplashState::Vslot09(i32) {
    int(WINAPI * sc)(BOOL) = ::ShowCursor;
    while (sc(0) >= 0) {
    }
    RunTitleSeq(static_cast<const char*>(CAssetRootStorage::s_value), 1, 1, 1, 0);
    m_1b8 = 0xea60;
    return 1;
}
RVA(0x000f98f0, 0x16)
i32 CSplashState::FrameSlot28(i32) {
    m_world->m_drawTarget->ClearAllPages(0);
    return 1;
}
