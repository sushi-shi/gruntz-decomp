#include <rva.h>

#include <Gruntz/LevelPreview.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDSurface.h>
#include <Dsndmgr/SoundBuffer.h>
#include <Dsndmgr/SoundStream.h>
#include <Enums.h>
#include <Gruntz/ErrorStringId.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/PreviewState.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SoundCueRegistryInline.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/State.h>
#include <Rez/FrameClock.h>
#include <Rez/RezArchive.h>
#include <Rez/RezArchiveDir.h>
#include <Rez/RezSync.h>
#include <Rez/RezTypeTag.h>
#include <Utils/MapTyped.h>
#include <Wap32/GameApp.h>
#include <Wap32/Wap32.h>

#include <ddraw.h>
#include <stdio.h>

DATA(0x0024d5f4)
b32 g_previewCancelQuits = false;

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000de060, 0xc2)

i32 CPreviewState::Enter(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId) {

    if (CState::LoadGameAssetNamespaces(mgr, areaArg, prevStateId) == 0) {
        return 0;
    }
    while (ShowCursor(false) >= 0) {
    }
    m_stateResources = m_resourceArchive->FindDirectoryByPath("STATEZ_PREVIEW");
    if (m_stateResources == NULL) {
        return 0;
    }
    if (g_disableAudio == false && g_disableSound == false) {
        CRezArchiveDir* set = StateResources()->FindSubdirectory("SOUNDZ");
        if (set != NULL) {
            m_world->m_soundRegistry
                ->LoadFromTree(static_cast<CRezArchiveDir*>(set), "PREVIEW", "_");
        }
    }
    m_previewName = "PREVIEW0";
    m_previewIndex = 0;
    m_mgr->m_gameWnd->PumpMessages(0x100, 0x40);
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000de170, 0x33)
void CPreviewState::ResetPreview() {
    SoundCueRegistry* reg = m_world->m_soundRegistry;
    if (reg->m_soundStream != NULL) {
        reg->m_soundStream->StopAllStreams();
    }
    m_world->m_soundRegistry->RemoveWithPrefix("PREVIEW", "_");
    CState::ReleaseResources();
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000de1c0, 0x35)
i32 CPreviewState::NextScreenCmd(i32 unused) {
    while (ShowCursor(false) >= 0) {
    }
    LoadLevelPreviewScreen();
    m_previewCountdownMs = 60000;
    return 1;
}

// @identity-TODO: owner, ABI, and true result are proven; the command identity is not.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000de210, 0x8)
i32 CPreviewState::AcceptPreviewCommand(i32 unused) {
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000de230, 0x85)
i32 CPreviewState::Tick() {
    IDirectDrawSurface* surf = m_world->m_drawTarget->m_frontSurface->m_surface->m_ddSurface;
    if (surf == NULL || surf->IsLost() != 0) {
        if (InputVirtual() == 0) {
            m_mgr->ReportError(IDX(IDS_RESTORE_GAME), 0xfa0);
            return 0;
        }
    }
    TickSoundVolumeRamps(m_world->m_soundRegistry);
    if (static_cast<u32>(g_gameAppFrameDeltaMs) >= m_previewCountdownMs) {
        m_previewCountdownMs = 0;
    } else {
        m_previewCountdownMs = m_previewCountdownMs - g_gameAppFrameDeltaMs;
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000de2f0, 0x5c)
i32 CPreviewState::Refade() {
    if (m_world->m_drawTarget->PagesReady() == 0) {
        return 0;
    }
    while (ShowCursor(false) >= 0) {
    }
    i32 r =
        LoadTitlePage(const_cast<char*>(static_cast<const char*>(m_previewName)), 0, 0, 0, 0, true);
    RetireScene(0x50, 0x3e8, 0, true);
    return r;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000de370, 0x56)
i32 CPreviewState::RefadeVirtual() {
    if (IsActive() == 0) {
        return 0;
    }
    while (ShowCursor(false) >= 0) {
    }
    i32 r =
        LoadTitlePage(const_cast<char*>(static_cast<const char*>(m_previewName)), 0, 0, 0, 0, true);
    RetireScene(0x50, 0x3e8, 0, true);
    return r;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000de3f0, 0x2d)
i32 CPreviewState::OnKey(i32 key, i32 unused) {
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
RVA(0x000de430, 0xd)
i32 CPreviewState::OnLButtonDown(i32, i32, i32) {
    LoadLevelPreviewScreen();
    return 1;
}

RVA(0x000de450, 0x115)
void CPreviewState::LoadLevelPreviewScreen() {
    char buf[64];
    i32 idx = m_previewIndex;
    m_previewIndex = idx + 1;
    sprintf(buf, "PREVIEW%i", idx);
    m_previewName = buf;
    sprintf(buf, "\\SCREENZ\\%s", static_cast<const char*>(m_previewName));
    StateResources()->FindEntryByPath(buf, IMGTAG_XCP);
    b32 failed = false;
    if (LoadTitlePage(const_cast<char*>(static_cast<const char*>(m_previewName)), 0, 0, 0, 0, true)
        == 0) {
        failed = true;
    } else {
        SoundCueRegistry* h = m_world->m_soundRegistry;
        if (h->m_silentMode == false) {
            SoundCue* found = NULL;
            MapLookup(h->m_cues, "GAME_TELEPORTEROPEN", found);
            SoundCue* p = found;
            if (p != NULL) {
                i32 volumePercent = g_soundVolumePercent;
                if (g_soundEnabled != false
                    && static_cast<u32>((g_soundCueTimeMs - p->m_lastPlayTimeMs))
                           >= static_cast<u32>(p->m_replayDelayMs)) {
                    p->m_lastPlayTimeMs = g_soundCueTimeMs;
                    p->m_sound->AcquireAndPlay(volumePercent, 0, 0, false);
                }
            }
        }
        RetireScene(0x50, 0x3e8, 0, true);
    }
    m_previewCountdownMs = 60000;
    if (failed) {
        Cancel();
    }
}

RVA(0x000de5c0, 0x2e)
void CPreviewState::Cancel() {
    if (g_previewCancelQuits) {
        m_mgr->DelayedQuit();
        return;
    }
    PostMessageA(static_cast<HWND>((m_mgr->m_gameWnd->m_hwnd)), 0x111, 0x8027, 0);
}
