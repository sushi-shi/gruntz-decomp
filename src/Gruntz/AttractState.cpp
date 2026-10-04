#include <StdAfx.h>

#include <rva.h>

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

RVA(0x00013fb0, 0xd5)
i32 CAttract::LoadGameAssetNamespaces(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId) {

    if (CState::LoadGameAssetNamespaces(mgr, areaArg, prevStateId) == 0) {
        return 0;
    }

    if (ShowCursor(false) >= 0) {
        do {
        } while (ShowCursor(false) >= 0);
    }

    owner()->EnsureStandardVideoMode(false);

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

RVA(0x000140d0, 0x33)
void CAttract::ReleaseResources() {
    SoundCueRegistry* reg = menuRoot()->SoundRegistry();
    if (reg->m_soundStream) {
        reg->m_soundStream->StopAllStreams();
    }
    menuRoot()->SoundRegistry()->RemoveWithPrefix("ATTRACT", "_");

    CState::ReleaseResources();
}

RVA(0x00014120, 0x1a9)
i32 CAttract::EnterState(GameStateId previousState) {

    if (ShowCursor(false) >= 0) {
        do {
        } while (ShowCursor(false) >= 0);
    }
    i32 idx = g_gameReg->m_numRuns % g_attractStateCount + 1;
    CString s;
    s.Format("TITLE%d", idx);
    LoadAndPresentTitlePage(s, 0, 0, 1, 0);
    CDDrawSubMgrPages* page = menuRoot()->GetDrawTarget();
    page->CopyFrontToSurface(page->GetBackPair());

    i32 r = GetRandomNumber();
    const char* pick = (r % 2) ? DATA_COMPGEN(0x0020b5bc, "2") : "";

    char buf[0x40];
    wsprintfA(buf, "ATTRACT_TITLE%s", pick);

    SoundCue* found = menuRoot()->SoundRegistry()->FindCue(buf);
    m_titleCue = found;
    if (found != NULL && m_titleCueEnabled != false) {
        if (g_soundEnabled) {
            m_titleCue->GetSound()->ApplyAndPlay(0x64, 0, 0, false);
        }
        m_titleCountdownMs = m_titleCue->GetSound()->GetDurationMs() + 0x2710;
    } else {
        m_titleCountdownMs = 0x1f40;
    }

    CInputDeviceGroup* list = g_actorList;
    for (i32 i = 0; i < list->m_count; i++) {
        list->m_items[i]->ResetState();
    }
    return 1;
}

RVA(0x00014340, 0x71)
i32 CAttract::LeaveState(GameStateId nextState) {
    if (m_titleCue == NULL) {
        return 1;
    }
    if (!m_titleCue->IsPlaying()) {
        return 1;
    }
    m_titleCue->GetSound()->RampVolumeTo(0, 0x1f4, true);
    if (!m_titleCue->GetSound()->IsPlaying()) {
        return 1;
    }
    do {
        (menuRoot()->SoundRegistry())->TickVolumeRamps();
    } while (m_titleCue->IsPlaying());
    return 1;
}

RVA(0x000143e0, 0xfb)
i32 CAttract::Render() {
    IDirectDrawSurface* busy =
        menuRoot()->GetDrawTarget()->GetFrontSurface()->GetSurface()->GetDirectDrawSurface();
    if (busy == NULL || busy->IsLost() != 0) {
        if (RestoreGraphics() == 0) {
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

    i32 n = g_actorList->GetCount();
    for (i = 0; i < n; i++) {
        if (g_actorList->GetAt(i)->GetPressedButtons() & IDX(INPUT_BUTTON8)) {
            PostMessageA(owner()->GetGameWindow()->GetHwnd(), WM_COMMAND, IDX(CMD_MAIN_MENU), 0);
            return 1;
        }
    }
    return 1;
}

RVA(0x00014520, 0xc3)
i32 CAttract::RestoreGraphics() {

    if (menuRoot()->GetDrawTarget()->RestoreLostSurfaces() == 0) {
        return 0;
    }

    if (ShowCursor(false) >= 0) {
        do {
        } while (ShowCursor(false) >= 0);
    }
    i32 idx = g_gameReg->m_numRuns % g_attractStateCount + 1;
    CString s;
    s.Format("TITLE%d", idx);
    return LoadAndPresentTitlePage(s, 0, 0, 1, 0);
}

RVA(0x00014630, 0xbd)
i32 CAttract::RestoreDisplay() {
    if (IsActive() == 0) {
        return 0;
    }

    if (ShowCursor(false) >= 0) {
        do {
        } while (ShowCursor(false) >= 0);
    }
    i32 idx = g_gameReg->m_numRuns % g_attractStateCount + 1;
    CString s;
    s.Format("TITLE%d", idx);
    return LoadAndPresentTitlePage(s, 0, 0, 1, 0);
}

RVA(0x00014720, 0x37)
i32 CAttract::OnKeyDown(i32 code, i32 unused) {
    if (code == VK_SPACE || code == VK_RETURN || code == VK_ESCAPE) {
        PostMessageA(owner()->GetGameWindow()->GetHwnd(), WM_COMMAND, IDX(CMD_MAIN_MENU), 0);
    }
    return 1;
}

RVA(0x00014770, 0x24)
i32 CAttract::OnLButtonDown(i32, i32, i32) {
    PostMessageA(owner()->GetGameWindow()->GetHwnd(), WM_COMMAND, IDX(CMD_MAIN_MENU), 0);
    return 1;
}

// @early-stop
RVA(0x000147b0, 0x6a)
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
    menuRoot()->GetDrawTarget()->CopyFrontToSurface(menuRoot()->GetDrawTarget()->GetBackPair());
    return 1;
}

RVA_COMPGEN(0x0008cd60, 0x1e, ??_GCAttract@@UAEPAXI@Z)
RVA(0x0008cd90, 0x55)
CAttract::~CAttract() {
    ReleaseResources();
}
