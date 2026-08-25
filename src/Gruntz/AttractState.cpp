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
#include <Gruntz/FixedPtrArray32.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GameStateId.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SoundCueRegistryInline.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/String.h>
#include <Rez/FrameClock.h>
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

    if (ShowCursor(0) >= 0) {
        do {
        } while (ShowCursor(0) >= 0);
    }

    owner()->RestoreVideoMode(0);

    CRezArchiveDir* state = ResourceArchive()->FindDirectoryByPath("STATEZ_ATTRACT");
    m_stateResources = (state);
    if (state == NULL) {
        return 0;
    }

    CRezArchiveDir* sound = state->FindSubdirectory("SOUNDZ");
    if (sound == NULL) {
        return 0;
    }

    menuRoot()->m_soundRegistry->LoadFromTree(static_cast<CRezArchiveDir*>(sound), "ATTRACT", "_");

    if (ShowCursor(0) >= 0) {
        do {
        } while (ShowCursor(0) >= 0);
    }

    if (static_cast<GameStateId>(prevStateId) == GAMESTATE_PLAY) {
        m_titleCueEnabled = 0;
        m_titleCue = NULL;
    } else {
        m_titleCueEnabled = 1;
        m_titleCue = NULL;
    }
    return 1;
}

RVA(0x000140d0, 0x33)
void CAttract::ReleaseResources() {
    SoundCueRegistry* reg = menuRoot()->m_soundRegistry;
    if (reg->m_soundStream) {
        reg->m_soundStream->StopAllStreams();
    }
    menuRoot()->m_soundRegistry->RemoveWithPrefix("ATTRACT", "_");

    CState::ReleaseResources();
}

// @early-stop
RVA(0x00014120, 0x1a9)
i32 CAttract::EnterState(GameStateId previousState) {

    if (ShowCursor(0) >= 0) {
        do {
        } while (ShowCursor(0) >= 0);
    }
    i32 idx = g_gameReg->m_numRuns % g_attractStateCount + 1;
    CString s;
    s.Format("TITLE%d", idx);
    LoadAndPresentTitlePage(s, 0, 0, 1, 0);
    CDDrawSubMgrPages* page = menuRoot()->m_drawTarget;
    page->BlitPage(page->m_backPair);

    i32 r = GetRandomNumber();
    const char* pick = (r % 2) ? DATA_COMPGEN(0x0020b5bc, "2") : "";

    char buf[0x40];
    wsprintfA(buf, "ATTRACT_TITLE%s", pick);

    SoundCue* found = NULL;
    MapLookup(menuRoot()->m_soundRegistry->m_cues, buf, found);
    m_titleCue = found;
    if (found != NULL && m_titleCueEnabled != 0) {
        if (g_soundEnabled) {
            m_titleCue->m_sound->ApplyAndPlay(0x64, 0, 0, 0);
        }
        m_titleCountdownMs = m_titleCue->m_sound->m_durationMs + 0x2710;
    } else {
        m_titleCountdownMs = 0x1f40;
    }

    CFixedPtrArray32* list = g_actorList;
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
    if (!m_titleCue->m_sound->IsPlaying()) {
        return 1;
    }
    m_titleCue->m_sound->RampVolumeTo(0, 0x1f4, 1);
    if (!m_titleCue->m_sound->IsPlaying()) {
        return 1;
    }
    do {
        TickSoundVolumeRamps(menuRoot()->m_soundRegistry);
    } while (m_titleCue->m_sound->IsPlaying());
    return 1;
}

RVA(0x000143e0, 0xfb)
i32 CAttract::Render() {
    IDirectDrawSurface* busy = menuRoot()->m_drawTarget->m_frontSurface->m_surface->m_ddSurface;
    if (busy == NULL || busy->IsLost() != 0) {
        if (InputVirtual() == 0) {
            owner()->ReportError(IDX(IDS_RESTORE_GAME), 0x3e8);
            return 0;
        }
    }

    TickSoundVolumeRamps(menuRoot()->m_soundRegistry);

    if (g_frameDelta >= m_titleCountdownMs) {
        m_titleCountdownMs = 0;
    } else {
        m_titleCountdownMs -= g_frameDelta;
    }

    CFixedPtrArray32* list = g_actorList;
    i32 i;
    for (i = 0; i < list->m_count; i++) {
        list->m_items[i]->Poll();
    }

    i32 n = g_actorList->m_count;
    for (i = 0; i < n; i++) {
        if (g_actorList->m_items[i]->m_pressedButtons & IDX(INPUT_BUTTON8)) {
            PostMessageA(owner()->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_MAIN_MENU), 0);
            return 1;
        }
    }
    return 1;
}

RVA(0x00014520, 0xc3)
i32 CAttract::InputVirtual() {

    if (menuRoot()->m_drawTarget->PagesReady() == 0) {
        return 0;
    }

    if (ShowCursor(0) >= 0) {
        do {
        } while (ShowCursor(0) >= 0);
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

    if (ShowCursor(0) >= 0) {
        do {
        } while (ShowCursor(0) >= 0);
    }
    i32 idx = g_gameReg->m_numRuns % g_attractStateCount + 1;
    CString s;
    s.Format("TITLE%d", idx);
    return LoadAndPresentTitlePage(s, 0, 0, 1, 0);
}

RVA(0x00014720, 0x37)
i32 CAttract::OnKeyDown(i32 code, i32 unused) {
    if (code == VK_SPACE || code == VK_RETURN || code == VK_ESCAPE) {
        PostMessageA(owner()->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_MAIN_MENU), 0);
    }
    return 1;
}

RVA(0x00014770, 0x24)
i32 CAttract::OnLButtonDown(i32, i32, i32) {
    PostMessageA(owner()->m_gameWnd->m_hwnd, WM_COMMAND, IDX(CMD_MAIN_MENU), 0);
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

    if (ShowCursor(0) >= 0) {
        do {
        } while (ShowCursor(0) >= 0);
    }
    menuRoot()->m_drawTarget->m_frontSurface->m_surface->Flip(NULL);
    menuRoot()->m_drawTarget->BlitPage(menuRoot()->m_drawTarget->m_backPair);
    return 1;
}

RVA_COMPGEN(0x0008cd60, 0x1e, ??_GCAttract@@UAEPAXI@Z)
RVA(0x0008cd90, 0x55)
CAttract::~CAttract() {
    ReleaseResources();
}
