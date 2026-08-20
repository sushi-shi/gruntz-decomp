#include <rva.h>

#include <Gruntz/LevelPreview.h>

#include <Mfc.h>

#include <Bute/SymParser.h>
#include <Bute/SymTab.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDSurface.h>
#include <Dsndmgr/DirectSoundMgr.h>
#include <Dsndmgr/SoundStream.h>
#include <Enums.h>
#include <Gruntz/ErrorStringId.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/PreviewState.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/State.h>
#include <Rez/FrameClock.h>
#include <Rez/RezSync.h>
#include <Rez/RezTypeTag.h>
#include <Utils/MapTyped.h>
#include <Wap32/GameApp.h>
#include <Wap32/Wap32.h>

#include <ddraw.h>
#include <stdio.h>

DATA(0x0024c69c)
i32 g_previewCancelQuits = 0;

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000de030, 0xc2)

i32 CPreviewState::Enter(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId) {

    if (CState::LoadGameAssetNamespaces(mgr, areaArg, prevStateId) == 0) {
        return 0;
    }
    while (ShowCursor(FALSE) >= 0) {
    }
    m_stateBank = m_symParser->ResolvePath("STATEZ_PREVIEW");
    if (m_stateBank == NULL) {
        return 0;
    }
    if (g_disableAudio == 0 && g_disableSound == 0) {
        CSymTab* set = SymTab2c()->FindSub("SOUNDZ");
        if (set != NULL) {
            m_world->m_soundRegistry->ScanTree(static_cast<CSymTab*>(set), "PREVIEW", "_");
        }
    }
    m_previewName = "PREVIEW0";
    m_previewIndex = 0;
    m_mgr->m_gameWnd->PumpMessages(0x100, 0x40);
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000de140, 0x33)
void CPreviewState::ResetPreview() {
    CDDrawSubMgrLeafScan* reg = m_world->m_soundRegistry;
    if (reg->m_soundStream != NULL) {
        reg->m_soundStream->Stop();
    }
    m_world->m_soundRegistry->RemoveKeysEqual("PREVIEW", "_");
    CState::ReleaseResources();
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000de190, 0x35)
i32 CPreviewState::NextScreenCmd(i32 param) {
    while (ShowCursor(FALSE) >= 0) {
    }
    LoadLevelPreviewScreen();
    m_previewCountdownMs = 60000;
    return 1;
}

// @identity-TODO: owner, ABI, and true result are proven; the command identity is not.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000de1e0, 0x8)
i32 CPreviewState::AcceptPreviewCommand(i32 param) {
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000de200, 0x85)
i32 CPreviewState::Tick() {
    IDirectDrawSurface* surf = m_world->m_drawTarget->m_frontPair->m_surface->m_ddSurface;
    if (surf == NULL || surf->IsLost() != 0) {
        if (InputVirtual() == 0) {
            m_mgr->ReportError(IDX(IDS_RESTORE_GAME), 0xfa0);
            return 0;
        }
    }
    CDDrawSubMgrLeafScan* reg = m_world->m_soundRegistry;
    if (reg->m_soundStream != NULL) {
        reg->m_soundStream->PurgeVoiceList(-1);
    }
    if (static_cast<u32>(g_wap32FrameDelta) >= m_previewCountdownMs) {
        m_previewCountdownMs = 0;
    } else {
        m_previewCountdownMs = m_previewCountdownMs - g_wap32FrameDelta;
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000de2c0, 0x5c)
i32 CPreviewState::Refade() {
    if (m_world->m_drawTarget->PagesReady() == 0) {
        return 0;
    }
    while (ShowCursor(FALSE) >= 0) {
    }
    i32 r = FadeInTitle(const_cast<char*>(static_cast<const char*>(m_previewName)), 0, 0, 0, 0, 1);
    RetireScene(0x50, 0x3e8, 0, 1);
    return r;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000de340, 0x56)
i32 CPreviewState::RefadeVirtual() {
    if (IsActive() == 0) {
        return 0;
    }
    while (ShowCursor(FALSE) >= 0) {
    }
    i32 r = FadeInTitle(const_cast<char*>(static_cast<const char*>(m_previewName)), 0, 0, 0, 0, 1);
    RetireScene(0x50, 0x3e8, 0, 1);
    return r;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000de3c0, 0x2d)
i32 CPreviewState::OnKey(i32 key, i32 param) {
    if (key == VK_ESCAPE) {
        Cancel();
    }
    if (key == VK_SPACE || key == VK_RETURN) {
        LoadLevelPreviewScreen();
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000de400, 0xd)
i32 CPreviewState::OnLButtonDown(i32, i32, i32) {
    LoadLevelPreviewScreen();
    return 1;
}

RVA(0x000de420, 0x115)
void CPreviewState::LoadLevelPreviewScreen() {
    char buf[64];
    i32 idx = m_previewIndex;
    m_previewIndex = idx + 1;
    sprintf(buf, "PREVIEW%i", idx);
    m_previewName = buf;
    sprintf(buf, "\\SCREENZ\\%s", static_cast<const char*>(m_previewName));
    SymTab2c()->ResolveQualified(buf, IMGTAG_XCP);
    i32 failed = 0;
    if (FadeInTitle(const_cast<char*>(static_cast<const char*>(m_previewName)), 0, 0, 0, 0, 1)
        == 0) {
        failed = 1;
    } else {
        CDDrawSubMgrLeafScan* h = m_world->m_soundRegistry;
        if (h->m_emitGate == 0) {
            LeafCue* found = NULL;
            MapLookup(h->m_cues, "GAME_TELEPORTEROPEN", found);
            // LeafCue::PlayIfElapsed inlined: the call's `this` copy holds the cue
            // in a register across the m_lastPlayTime store.
            LeafCue* p = found;
            if (p != NULL) {
                i32 tag = g_sndCueTag;
                if (g_sndEnabled != 0
                    && static_cast<u32>((g_killCueClock - p->m_lastPlayTime))
                           >= static_cast<u32>(p->m_replayDelay)) {
                    p->m_lastPlayTime = g_killCueClock;
                    p->m_sound->ConfigureItem(tag, 0, 0, 0);
                }
            }
        }
        RetireScene(0x50, 0x3e8, 0, 1);
    }
    m_previewCountdownMs = 60000;
    if (failed) {
        Cancel();
    }
}

RVA(0x000de590, 0x2e)
void CPreviewState::Cancel() {
    if (g_previewCancelQuits) {
        m_mgr->DelayedQuit();
        return;
    }
    PostMessageA(static_cast<HWND>((m_mgr->m_gameWnd->m_hwnd)), 0x111, 0x8027, 0);
}
