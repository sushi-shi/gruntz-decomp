#include <rva.h>

#include <Bute/SymParser.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDSurface.h>
#include <DinMgr2/DirectInputMgr2.h>
#include <EmptyString.h>
#include <Enums.h>
#include <Gruntz/Attract.h>
#include <Gruntz/FixedPtrArray32.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GameStateId.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Random.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/String.h>
#include <Rez/FrameClock.h>

#include <ddraw.h>
#include <stddef.h>

VTBL(CAttract, 0x001ea194);
VTBL(CState, 0x001ea21c);
DATA(0x0020b5bc)
char s_dat60b5bc[] = "2";

RVA(0x00013fb0, 0xd5)
i32 CAttract::LoadGameAssetNamespaces(CGruntzMgr* a, i32 b, i32 mode) {

    if (CState::LoadGameAssetNamespaces(a, b, mode) == 0) {
        return 0;
    }

    if (ShowCursor(0) >= 0) {
        do {
        } while (ShowCursor(0) >= 0);
    }

    owner()->RestoreVideoMode(0);

    CSymTab* state = static_cast<CSymTab*>(stateMgr()->ResolvePath("STATEZ_ATTRACT"));
    m_stateBank = (state);
    if (state == NULL) {
        return 0;
    }

    void* sound = state->FindSub("SOUNDZ");
    if (sound == NULL) {
        return 0;
    }

    menuRoot()->m_soundRegistry->ScanTree(static_cast<CSymTab*>(sound), "ATTRACT", "_");

    if (ShowCursor(0) >= 0) {
        do {
        } while (ShowCursor(0) >= 0);
    }

    if (static_cast<GameStateId>(mode) == GAMESTATE_PLAY) {
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
        reg->m_soundStream->Stop();
    }
    menuRoot()->m_soundRegistry->RemoveKeysEqual("ATTRACT", "_");

    CState::ReleaseResources();
}

// @early-stop
RVA(0x00014120, 0x1a9)
i32 CAttract::EnterState(GameStateId arg) {

    if (ShowCursor(0) >= 0) {
        do {
        } while (ShowCursor(0) >= 0);
    }
    i32 idx = g_gameReg->m_numRuns % g_attractStateCount + 1;
    CString s;
    s.Format("TITLE%d", idx);
    RunTitleSeq(s, 0, 0, 1, 0);
    CDDrawSubMgrPages* page = menuRoot()->m_drawTarget;
    page->BlitPage(page->m_backPair);

    i32 seed;
    if (!(g_randSeeded & 1)) {
        g_randSeeded |= 1;
        seed = timeGetTime();
    } else {
        seed = g_randSeed;
    }
    g_randSeed = seed * 214013 + 2531011;
    i32 r = (g_randSeed >> 0x10) & 0x7fff;
    const char* pick = (r % 2) ? s_dat60b5bc : g_emptyString;

    char buf[0x40];
    wsprintfA(buf, "ATTRACT_TITLE%s", pick);

    void* found = 0;
    menuRoot()->m_soundRegistry->m_cues.Lookup(buf, found);
    m_host = static_cast<LeafCue*>(found);
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
i32 CAttract::LeaveState(GameStateId arg) {
    if (m_host == NULL) {
        return 1;
    }
    if (!m_host->m_sound->IsPlaying()) {
        return 1;
    }
    m_host->m_sound->CloneAndPlay(0, 0x1f4, 1);
    if (!m_host->m_sound->IsPlaying()) {
        return 1;
    }
    do {
        CDDrawSubMgrLeafScan* reg = menuRoot()->m_soundRegistry;
        if (reg->m_soundStream) {
            reg->m_soundStream->PurgeVoiceList(-1);
        }
    } while (m_host->m_sound->IsPlaying());
    return 1;
}

RVA(0x000143e0, 0xfb)
i32 CAttract::Render() {
    IDirectDrawSurface* busy = menuRoot()->m_drawTarget->m_frontPair->m_surface->m_ddSurface;
    if (busy == NULL || busy->IsLost() != 0) {
        if (InputVirtual() == 0) {
            owner()->ReportError(IDX(CMD_RETURN_TO_MENU), 0x3e8);
            return 0;
        }
    }

    CDDrawSubMgrLeafScan* reg = menuRoot()->m_soundRegistry;
    if (reg->m_soundStream) {
        reg->m_soundStream->PurgeVoiceList(-1);
    }

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
    return RunTitleSeq(s, 0, 0, 1, 0);
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
    return RunTitleSeq(s, 0, 0, 1, 0);
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
    menuRoot()->m_drawTarget->m_frontPair->m_surface->Flip(0);
    menuRoot()->m_drawTarget->BlitPage(menuRoot()->m_drawTarget->m_backPair);
    return 1;
}

// @interleaver Update - fixed-size generated body (6 B, byte-identical across
// 11 classes), so every TU emits one and the linker folds them to first use.
RVA(0x0008cd40, 0x6)
GameStateId CAttract::Update() {
    return GAMESTATE_ATTRACT;
}

RVA_COMPGEN(0x0008cd60, 0x1e, ??_GCAttract@@UAEPAXI@Z)
RVA(0x0008cd90, 0x55)
CAttract::~CAttract() {
    ReleaseResources();
}
