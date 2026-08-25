#include <rva.h>

#include <Gruntz/SplashState.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDSurface.h>
#include <DinMgr2/DirectInputMgr2.h>
#include <Dsndmgr/SoundStream.h>
#include <Enums.h>
#include <Gruntz/AssetRoot.h>
#include <Gruntz/Attract.h>
#include <Gruntz/BankMgr.h>
#include <Gruntz/ErrorStringId.h>
#include <Gruntz/GameMode.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameStateId.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SoundCueRegistryInline.h>
#include <Gruntz/State.h>
#include <Gruntz/View.h>
#include <Rez/RezArchive.h>
#include <Rez/RezArchiveDir.h>
#include <Wap32/GameApp.h>

#include <ddraw.h>

RVA(0x000f9780, 0x8c)
i32 CSplashState::LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId) {
    if (CAssetRootStorage::s_value.GetLength() == 0) {
        return 0;
    }

    if (!CState::LoadGameAssetNamespaces(mgr, areaArg, prevStateId)) {
        return 0;
    }
    SetCursor(NULL);
    m_mgr->RestoreVideoMode(0);

    m_stateResources = m_resourceArchive->FindDirectoryByPath("STATEZ_SPLASH");
    if (!m_stateResources) {
        return 0;
    }

    CRezArchiveDir* soundz = StateResources()->FindSubdirectory("SOUNDZ");
    if (soundz) {
        m_world->m_soundRegistry->LoadFromTree(static_cast<CRezArchiveDir*>(soundz), "", "_");
    }
    return 1;
}

RVA(0x000f9840, 0x29)
void CSplashState::ReleaseResources() {
    SoundCueRegistry* reg = m_world->m_soundRegistry;
    if (reg->m_soundStream != NULL) {
        reg->m_soundStream->StopAllStreams();
    }
    m_world->m_soundRegistry->ClearCues();
    CState::ReleaseResources();
}

RVA(0x000f9880, 0x43)
i32 CSplashState::EnterState(GameStateId previousState) {
    int(WINAPI * sc)(BOOL) = ShowCursor;
    while (sc(0) >= 0) {
    }
    LoadAndPresentTitlePage(static_cast<const char*>(CAssetRootStorage::s_value), 1, 1, 1, 0);
    m_splashCountdownMs = 0xea60;
    return 1;
}

RVA(0x000f98f0, 0x16)
i32 CSplashState::LeaveState(GameStateId nextState) {
    m_world->m_drawTarget->ClearAllPages(0);
    return 1;
}

RVA(0x000f9920, 0x108)
i32 CSplashState::Render() {
    IDirectDrawSurface* in = m_world->m_drawTarget->m_frontSurface->m_surface->m_ddSurface;
    if (!in || in->IsLost()) {
        if (!InputVirtual()) {
            m_mgr->ReportError(IDX(IDS_RESTORE_GAME), 0x447);
            return 0;
        }
    }

    TickSoundVolumeRamps(m_world->m_soundRegistry);

    if (static_cast<u32>(g_gameAppFrameDeltaMs) >= m_splashCountdownMs) {
        m_splashCountdownMs = 0;
    } else {
        m_splashCountdownMs = m_splashCountdownMs - g_gameAppFrameDeltaMs;
    }

    {
        CFixedPtrArray32* L = g_actorList;
        for (i32 i = 0; i < L->m_count; i++) {
            L->m_items[i]->Poll();
        }
    }

    {
        CFixedPtrArray32* L = g_actorList;
        i32 n = L->m_count;
        i32 j;
        for (j = 0; j < n; j++) {
            if (L->m_items[j]->m_pressedButtons & IDX(INPUT_BUTTON0)) {
                goto post;
            }
        }
        if (m_splashCountdownMs) {
            return 1;
        }
    }
post:
    PostMessageA(m_mgr->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_MAIN_MENU), 0);
    m_mgr->m_owner->m_running = 0;
    return 1;
}

RVA(0x000f9a80, 0x44)
i32 CSplashState::InputVirtual() {
    if (m_world->m_drawTarget->PagesReady() == 0) {
        return 0;
    }
    while (ShowCursor(FALSE) >= 0) {
    }
    return LoadAndPresentTitlePage(
        static_cast<const char*>(CAssetRootStorage::s_value),
        0,
        0,
        1,
        0
    );
}

RVA(0x000f9af0, 0x3e)
i32 CSplashState::RestoreDisplay() {
    if (IsActive() == 0) {
        return 0;
    }
    while (ShowCursor(FALSE) >= 0) {
    }
    return LoadAndPresentTitlePage(
        static_cast<const char*>(CAssetRootStorage::s_value),
        0,
        0,
        1,
        0
    );
}

RVA(0x000f9b40, 0x37)
i32 CSplashState::OnKeyDown(i32 code, i32) {
    if (code == VK_ESCAPE || code == VK_SPACE || code == VK_RETURN) {
        PostMessageA(m_mgr->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_MAIN_MENU), 0);
    }
    return 1;
}

RVA(0x000f9b90, 0x24)
i32 CSplashState::OnLButtonDown(i32, i32, i32) {
    PostMessageA(m_mgr->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_MAIN_MENU), 0);
    return 1;
}
