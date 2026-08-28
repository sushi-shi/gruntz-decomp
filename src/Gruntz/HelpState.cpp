#include <rva.h>

#include <Gruntz/HelpState.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDSurface.h>
#include <DinMgr2/DirectInputMgr2.h>
#include <Enums.h>
#include <Gruntz/Attract.h>
#include <Gruntz/BankMgr.h>
#include <Gruntz/Demo.h>
#include <Gruntz/ErrorStringId.h>
#include <Gruntz/FixedPtrArray32.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameStateId.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SoundCueRegistryInline.h>
#include <Gruntz/SplashState.h>
#include <Rez/RezArchive.h>
#include <Rez/RezArchiveDir.h>

#include <ddraw.h>

DATA(0x002111b0)

char g_titleBuf[] = "HELP";

RVA(0x0008cee0, 0x6)
GameStateId CHelpState::Update() {
    return GAMESTATE_HELP;
}

RVA(0x0008cf30, 0x55)
CHelpState::~CHelpState() {
    CHelpState::ReleaseResources();
}

RVA(0x0008cfb0, 0x6)
GameStateId CSplashState::Update() {
    return GAMESTATE_SPLASH;
}

RVA(0x0008d000, 0x55)
CSplashState::~CSplashState() {
    CSplashState::ReleaseResources();
}

RVA(0x0008d080, 0x6)
GameStateId CDemo::Update() {
    return GAMESTATE_DEMO;
}

RVA(0x00095090, 0x6e)
i32 CHelpState::LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId) {

    if (!CState::LoadGameAssetNamespaces(mgr, areaArg, prevStateId)) {
        return 0;
    }
    while (ShowCursor(false) >= 0)
        ;
    m_stateResources = m_resourceArchive->GetDirFromPath("STATEZ_HELP");
    if (!m_stateResources) {
        return 0;
    }
    m_mgr->m_gameWnd->PumpMessages(0x100, 0x40);
    return 1;
}

RVA(0x00095120, 0x5)
void CHelpState::ReleaseResources() {
    CState::ReleaseResources();
}

RVA(0x00095140, 0x6e)
i32 CHelpState::EnterState(GameStateId previousState) {
    m_mgr->RestoreVideoMode(false);

    if (m_world->m_drawTarget->HasOverlay() == 0
        && m_world->m_drawTarget->CreateOverlay(0, 0x30000) == 0) {
        return 0;
    }
    if (LoadTitlePage(g_titleBuf, 0, 0, 0, 0, true) == 0) {
        return 0;
    }
    RetireScene(0x50, 0x3e8, 0, true);
    return 1;
}

RVA(0x000951d0, 0x8)
i32 CHelpState::LeaveState(GameStateId nextState) {
    return 1;
}

RVA(0x000951f0, 0xeb)
i32 CHelpState::Render() {
    IDirectDrawSurface* busy = m_world->m_drawTarget->m_frontSurface->m_surface->m_ddSurface;
    if (busy == NULL || busy->IsLost() != 0) {
        if (InputVirtual() == 0) {
            m_mgr->ReportError(IDX(IDS_RESTORE_GAME), 0x445);
            return 0;
        }
    }

    TickSoundVolumeRamps(m_world->m_soundRegistry);

    CFixedPtrArray32* list = g_actorList;
    i32 i;
    for (i = 0; i < list->m_count; i++) {
        list->m_items[i]->Poll();
    }

    i32 n = g_actorList->m_count;
    for (i = 0; i < n; i++) {
        if (g_actorList->m_items[i]->m_pressedButtons & IDX(INPUT_BUTTON_MASK)) {
            PostMessageA(m_mgr->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_NEXT_STATE), 0);
            m_mgr->m_owner->m_running = false;
            return 1;
        }
    }
    return 1;
}

RVA(0x00095320, 0x56)
i32 CHelpState::InputVirtual() {
    if (m_world->m_drawTarget->PagesReady() == 0) {
        return 0;
    }
    while (ShowCursor(false) >= 0) {
    }
    i32 r = LoadAndPresentTitlePage(g_titleBuf, 0, 0, 1, 0);
    while (ShowCursor(false) >= 0) {
    }
    return r;
}

RVA(0x000953a0, 0x3c)
i32 CHelpState::RestoreDisplay() {
    if (IsActive() == 0) {
        return 0;
    }
    while (ShowCursor(false) >= 0) {
    }
    return LoadAndPresentTitlePage(g_titleBuf, 0, 0, 1, 0);
}

RVA(0x000953f0, 0x37)
i32 CHelpState::OnKeyDown(i32 code, i32 unused) {
    if (code == VK_ESCAPE || code == VK_SPACE || code == VK_RETURN) {
        PostMessageA(m_mgr->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_NEXT_STATE), 0);
    }
    return 1;
}

RVA(0x00095440, 0x24)
i32 CHelpState::OnLButtonDown(i32, i32, i32) {
    PostMessageA(m_mgr->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_NEXT_STATE), 0);
    return 1;
}
