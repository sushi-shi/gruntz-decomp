#include <rva.h>

#include <Bute/SymParser.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawSubMgrLeafScanInline.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDSurface.h>
#include <DinMgr2/DirectInputMgr2.h>
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
#include <Gruntz/SoundState.h>
#include <Gruntz/String.h>
#include <Rez/FrameClock.h>
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

    CSymTab* state = stateMgr()->ResolvePath("STATEZ_ATTRACT");
    m_stateBank = (state);
    if (state == NULL) {
        return 0;
    }

    CSymTab* sound = state->FindSub("SOUNDZ");
    if (sound == NULL) {
        return 0;
    }

    menuRoot()->m_soundRegistry->ScanTree(static_cast<CSymTab*>(sound), "ATTRACT", "_");

    if (ShowCursor(0) >= 0) {
        do {
        } while (ShowCursor(0) >= 0);
    }

    if (static_cast<GameStateId>(prevStateId) == GAMESTATE_PLAY) {
        m_activeFlag = 0;
        m_host = NULL;
    } else {
        m_activeFlag = 1;
        m_host = NULL;
    }
    return 1;
}

RVA(0x000140d0, 0x33)
void CAttract::ReleaseResources() {
    CDDrawSubMgrLeafScan* reg = menuRoot()->m_soundRegistry;
    if (reg->m_soundStream) {
        reg->m_soundStream->StopAllStreams();
    }
    menuRoot()->m_soundRegistry->RemoveKeysEqual("ATTRACT", "_");

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

    LeafCue* found = NULL;
    MapLookup(menuRoot()->m_soundRegistry->m_cues, buf, found);
    m_host = found;
    if (found != NULL && m_activeFlag != 0) {
        if (g_sndEnabled) {
            m_host->m_sound->ApplyAndPlay(0x64, 0, 0, 0);
        }
        m_idleTimer = m_host->m_sound->m_durationMs + 0x2710;
    } else {
        m_idleTimer = 0x1f40;
    }

    CFixedPtrArray32* list = g_actorList;
    for (i32 i = 0; i < list->m_count; i++) {
        list->m_items[i]->ResetState();
    }
    return 1;
}

RVA(0x00014340, 0x71)
i32 CAttract::LeaveState(GameStateId nextState) {
    if (m_host == NULL) {
        return 1;
    }
    if (!m_host->m_sound->IsPlaying()) {
        return 1;
    }
    m_host->m_sound->RampVolumeTo(0, 0x1f4, 1);
    if (!m_host->m_sound->IsPlaying()) {
        return 1;
    }
    do {
        PurgeVoices(menuRoot()->m_soundRegistry);
    } while (m_host->m_sound->IsPlaying());
    return 1;
}

RVA(0x000143e0, 0xfb)
i32 CAttract::Render() {
    IDirectDrawSurface* busy = menuRoot()->m_drawTarget->m_frontPair->m_surface->m_ddSurface;
    if (busy == NULL || busy->IsLost() != 0) {
        if (InputVirtual() == 0) {
            owner()->ReportError(IDX(IDS_RESTORE_GAME), 0x3e8);
            return 0;
        }
    }

    PurgeVoices(menuRoot()->m_soundRegistry);

    if (g_frameDelta >= m_idleTimer) {
        m_idleTimer = 0;
    } else {
        m_idleTimer -= g_frameDelta;
    }

    CFixedPtrArray32* list = g_actorList;
    i32 i;
    for (i = 0; i < list->m_count; i++) {
        list->m_items[i]->Poll();
    }

    i32 n = g_actorList->m_count;
    for (i = 0; i < n; i++) {
        if (g_actorList->m_items[i]->m_currentKeys & 0x100) {
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
// Extent, calls, CFG, and ordered referents are exact; only the EAX/ECX/EDX
// rotation in the two page-presentation chains differs. Scoped page-manager
// locals change the lifetime and regress, while 32 mixed TU states are flat.
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
    menuRoot()->m_drawTarget->m_frontPair->m_surface->Flip(NULL);
    menuRoot()->m_drawTarget->BlitPage(menuRoot()->m_drawTarget->m_backPair);
    return 1;
}

RVA_COMPGEN(0x0008cd60, 0x1e, ??_GCAttract@@UAEPAXI@Z)
RVA(0x0008cd90, 0x55)
CAttract::~CAttract() {
    ReleaseResources();
}
