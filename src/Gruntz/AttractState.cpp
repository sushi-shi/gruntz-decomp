#include <StdAfx.h>

#include <Ints.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDSurface.h>
#include <DinMgr2/DirectInputMgr2.h>
#include <Dsndmgr/SoundBuffer.h>
#include <Dsndmgr/SoundStream.h>
#include <Enums.h>
#include <Gruntz/Attract.h>
#include <Gruntz/ErrorStringId.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GameStateId.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/InputDeviceGroup.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SoundCueRegistryInline.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/String.h>
#include <Rez/FrameClock.h>
#include <Rez/FrameCountdown.h>
#include <Rez/RezArchive.h>
#include <Rez/RezSync.h>
#include <Utils/MapTyped.h>

#include <ddraw.h>
#include <stddef.h>

i32 CAttract::LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId) {

    if (CState::LoadGameAssetNamespaces(mgr, areaArg, prevStateId) == 0) {
        return 0;
    }

    if (ShowCursor(false) >= 0) {
        do {
        } while (ShowCursor(false) >= 0);
    }

    owner()->RestoreVideoMode(false);

    CRezDir* state = ResourceArchive()->GetDirFromPath("STATEZ_ATTRACT");
    m_stateResources = (state);
    if (state == NULL) {
        return 0;
    }

    CRezDir* sound = state->GetDir("SOUNDZ");
    if (sound == NULL) {
        return 0;
    }

    menuRoot()->SoundRegistry()->LoadFromTree(static_cast<CRezDir*>(sound), "ATTRACT", "_");

    if (ShowCursor(false) >= 0) {
        do {
        } while (ShowCursor(false) >= 0);
    }

    if (static_cast<GameStateId>(prevStateId) == GAMESTATE_PLAY) {
        m_titleCueEnabled = false;
        m_titleCue = NULL;
    } else {
        m_titleCueEnabled = true;
        m_titleCue = NULL;
    }
    return 1;
}

void CAttract::ReleaseResources() {
    SoundCueRegistry* reg = menuRoot()->SoundRegistry();
    if (reg->m_soundStream) {
        reg->m_soundStream->StopAllStreams();
    }
    menuRoot()->SoundRegistry()->RemoveWithPrefix("ATTRACT", "_");

    CState::ReleaseResources();
}

i32 CAttract::EnterState(GameStateId previousState) {

    if (ShowCursor(false) >= 0) {
        do {
        } while (ShowCursor(false) >= 0);
    }
    i32 idx = g_gameReg->m_numRuns % g_attractStateCount + 1;
    std::string s;
    s = formatText("TITLE%d", idx);
    LoadAndPresentTitlePage((s), 0, 0, 1, 0);
    CDDrawSubMgrPages* page = menuRoot()->GetDrawTarget();
    page->BlitPage(page->GetBackPair());

    i32 r = GetRandomNumber();
    const std::string cueName = (r % 2) ? "ATTRACT_TITLE2" : "ATTRACT_TITLE";

    SoundCue* found = menuRoot()->SoundRegistry()->FindCue(cueName);
    m_titleCue = found;
    if (found != NULL && m_titleCueEnabled != false) {
        if (g_soundEnabled) {
            m_titleCue->m_sound->ApplyAndPlay(0x64, 0, 0, false);
        }
        m_titleCountdownMs = m_titleCue->m_sound->m_durationMs + 0x2710;
    } else {
        m_titleCountdownMs = 0x1f40;
    }

    CInputDeviceGroup* list = g_actorList;
    for (i32 i = 0; i < list->m_count; i++) {
        list->m_items[i]->ResetState();
    }
    return 1;
}

i32 CAttract::LeaveState(GameStateId nextState) {
    return BeginAudioDeparture(m_titleCue, 0);
}

i32 CAttract::Render() {
    IDirectDrawSurface* busy =
        menuRoot()->GetDrawTarget()->GetFrontSurface()->GetSurface()->GetDirectDrawSurface();
    if (busy == NULL || busy->IsLost() != 0) {
        if (InputVirtual() == 0) {
            owner()->ReportError(IDX(IDS_RESTORE_GAME), 0x3e8);
            return 0;
        }
    }

    (menuRoot()->SoundRegistry())->TickVolumeRamps();

    CountDown(m_titleCountdownMs, g_frameDelta);

    CInputDeviceGroup* list = g_actorList;
    i32 i;
    for (i = 0; i < list->m_count; i++) {
        list->m_items[i]->Poll();
    }

    i32 n = g_actorList->m_count;
    for (i = 0; i < n; i++) {
        if (g_actorList->m_items[i]->m_pressedButtons & IDX(INPUT_BUTTON8)) {
            PostMessageA(owner()->m_gameWnd->GetHwnd(), WM_COMMAND, IDX(CMD_MAIN_MENU), 0);
            return 1;
        }
    }
    return 1;
}

i32 CAttract::InputVirtual() {

    if (menuRoot()->GetDrawTarget()->PagesReady() == 0) {
        return 0;
    }

    if (ShowCursor(false) >= 0) {
        do {
        } while (ShowCursor(false) >= 0);
    }
    i32 idx = g_gameReg->m_numRuns % g_attractStateCount + 1;
    std::string s;
    s = formatText("TITLE%d", idx);
    return LoadAndPresentTitlePage((s), 0, 0, 1, 0);
}

i32 CAttract::RestoreDisplay() {
    if (IsActive() == 0) {
        return 0;
    }

    if (ShowCursor(false) >= 0) {
        do {
        } while (ShowCursor(false) >= 0);
    }
    i32 idx = g_gameReg->m_numRuns % g_attractStateCount + 1;
    std::string s;
    s = formatText("TITLE%d", idx);
    return LoadAndPresentTitlePage((s), 0, 0, 1, 0);
}

i32 CAttract::OnKeyDown(i32 code, i32 unused) {
    if (code == VK_SPACE || code == VK_RETURN || code == VK_ESCAPE) {
        PostMessageA(owner()->m_gameWnd->GetHwnd(), WM_COMMAND, IDX(CMD_MAIN_MENU), 0);
    }
    return 1;
}

i32 CAttract::OnLButtonDown(i32, i32, i32) {
    PostMessageA(owner()->m_gameWnd->GetHwnd(), WM_COMMAND, IDX(CMD_MAIN_MENU), 0);
    return 1;
}

i32 CAttract::OnPaint() {
    if (!IsActive()) {
        return 0;
    }
    if (!m_world) {
        return 0;
    }
    if (!CState::OnPaint()) {
        return 0;
    }

    if (ShowCursor(false) >= 0) {
        do {
        } while (ShowCursor(false) >= 0);
    }
    menuRoot()->GetDrawTarget()->GetFrontSurface()->GetSurface()->Flip(NULL);
    menuRoot()->GetDrawTarget()->BlitPage(menuRoot()->GetDrawTarget()->GetBackPair());
    return 1;
}

CAttract::~CAttract() {
    ReleaseResources();
}
