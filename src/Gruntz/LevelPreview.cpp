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
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/PreviewState.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/State.h>
#include <Image/ImageFormatTag.h>
#include <Rez/FrameClock.h>
#include <Rez/RezSync.h>
#include <Wap32/GameApp.h>
#include <Wap32/Wap32.h>

#include <ddraw.h>
#include <stdio.h>

DATA(0x0024c69c)
i32 g_flag64c69c = 0;

RVA(0x000de030, 0xc2)

i32 CPreviewState::Enter(CGruntzMgr* mgr, i32 areaArg, i32 a2) {

    if (CState::LoadGameAssetNamespaces(mgr, areaArg, a2) == 0) {
        return 0;
    }
    while (ShowCursor(FALSE) >= 0) {
    }
    m_stateBank = static_cast<CSymTab*>(m_symParser->ResolvePath("STATEZ_PREVIEW"));
    if (m_stateBank == 0) {
        return 0;
    }
    if (g_disableAudio == 0 && g_disableSound == 0) {
        void* set = SymTab2c()->FindSub("SOUNDZ");
        if (set != 0) {
            m_world->m_soundRegistry->ScanTree(static_cast<CSymTab*>(set), "PREVIEW", "_");
        }
    }
    m_previewName = "PREVIEW0";
    m_previewIndex = 0;
    m_mgr->m_gameWnd->PumpMessages(0x100, 0x40);
    return 1;
}

RVA(0x000de140, 0x33)
void CPreviewState::ResetPreview() {
    CDDrawSubMgrLeafScan* reg = m_world->m_soundRegistry;
    if (reg->m_soundStream != 0) {
        reg->m_soundStream->Stop();
    }
    m_world->m_soundRegistry->RemoveKeysEqual("PREVIEW", "_");
    CState::ReleaseResources();
}

RVA(0x000de190, 0x35)
i32 CPreviewState::NextScreenCmd(i32 param) {
    while (ShowCursor(FALSE) >= 0) {
    }
    LoadLevelPreviewScreen();
    m_previewCountdownMs = 60000;
    return 1;
}

RVA(0x000de200, 0x85)
i32 CPreviewState::Tick() {
    IDirectDrawSurface* surf = m_world->m_drawTarget->m_frontPair->m_surface->m_ddSurface;
    if (surf == 0 || surf->IsLost() != 0) {
        if (InputVirtual() == 0) {
            m_mgr->ReportError(0x8006, 0xfa0);
            return 0;
        }
    }
    CDDrawSubMgrLeafScan* reg = m_world->m_soundRegistry;
    if (reg->m_soundStream != 0) {
        reg->m_soundStream->PurgeVoiceList(-1);
    }
    if (static_cast<u32>(g_wap32FrameDelta) >= m_previewCountdownMs) {
        m_previewCountdownMs = 0;
    } else {
        m_previewCountdownMs = m_previewCountdownMs - g_wap32FrameDelta;
    }
    return 1;
}

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

RVA(0x000de3c0, 0x2d)
i32 CPreviewState::OnKey(i32 key, i32 param) {
    if (key == 0x1b) {
        Cancel();
    }
    if (key == 0x20 || key == 0xd) {
        LoadLevelPreviewScreen();
    }
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
            void* p_ob = 0;
            h->m_cues.Lookup("GAME_TELEPORTEROPEN", p_ob);
            LeafCue* p = static_cast<LeafCue*>(p_ob);
            if (p != 0) {
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

// @early-stop
RVA(0x000de590, 0x2e)
void CPreviewState::Cancel() {
    if (g_flag64c69c) {
        m_mgr->DelayedQuit();
        return;
    }
    PostMessageA(static_cast<HWND>((m_mgr->m_gameWnd->m_hwnd)), 0x111, 0x8027, 0);
}
RVA(0x000fab90, 0xaa)
i32 CPreviewState::LoadScreen(char* name, i32 doFlip, i32 unused3, i32 unused4) {
    if (m_world == 0) {
        return 0;
    }
    if (m_symParser == 0) {
        return 0;
    }
    if (m_stateBank == 0) {
        return 0;
    }
    char buf[64];
    sprintf(buf, "\\SCREENZ\\%s", name);
    CParseSource* sym = SymTab2c()->ResolveQualified(buf, IMGTAG_XCP);
    if (sym == 0) {
        return 0;
    }
    if (m_world->m_drawTarget->LoadPageImage(sym, 1) == 0) {
        return 0;
    }
    if (doFlip != 0) {
        m_world->m_drawTarget->m_frontPair->m_surface->Flip(0);
    }
    return 1;
}
