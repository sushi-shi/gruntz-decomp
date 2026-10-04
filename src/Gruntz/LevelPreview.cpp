#include <StdAfx.h>

#include <rva.h>

#include <Gruntz/LevelPreview.h>

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
#include <Gruntz/SoundCueInline.h>
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

DATA(0x0024c69c)
b32 g_previewCancelQuits = false;

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000de030, 0xc2)

i32 CPreviewState::LoadPreviewAssets(CGruntzMgr* gameManager, i32 levelIndex, i32 previousStateId) {

    if (CState::LoadGameAssetNamespaces(gameManager, levelIndex, previousStateId) == 0) {
        return 0;
    }
    while (ShowCursor(false) >= 0) {
    }
    m_stateResources = m_resourceArchive->GetDirFromPath("STATEZ_PREVIEW");
    if (m_stateResources == NULL) {
        return 0;
    }
    if (g_disableAudio == false && g_disableSound == false) {
        CRezDir* soundResources = StateResources()->GetDir("SOUNDZ");
        if (soundResources != NULL) {
            m_world->SoundRegistry()
                ->LoadFromTree(static_cast<CRezDir*>(soundResources), "PREVIEW", "_");
        }
    }
    m_currentPreviewName = "PREVIEW0";
    m_nextPreviewIndex = 0;
    m_mgr->GetGameWindow()->DiscardMessages(WM_KEYDOWN, 0x40);
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000de140, 0x33)
void CPreviewState::ReleasePreviewAssets() {
    SoundCueRegistry* soundRegistry = m_world->SoundRegistry();
    if (soundRegistry->GetSoundStream() != NULL) {
        soundRegistry->GetSoundStream()->StopAllStreams();
    }
    m_world->SoundRegistry()->RemoveWithPrefix("PREVIEW", "_");
    CState::ReleaseResources();
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000de190, 0x35)
i32 CPreviewState::BeginPreview(i32 unused) {
    while (ShowCursor(false) >= 0) {
    }
    ShowNextPreviewScreen();
    m_previewCountdownMs = 60000;
    return 1;
}

// @identity-TODO: owner, ABI, and true result are proven; the command identity is not.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000de1e0, 0x8)
i32 CPreviewState::AcceptPreviewCommand(i32 unused) {
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000de200, 0x85)
i32 CPreviewState::UpdatePreview() {
    IDirectDrawSurface* frontSurface =
        m_world->GetDisplayBuffers()->GetFrontSurface()->GetSurface()->GetDirectDrawSurface();
    if (frontSurface == NULL || frontSurface->IsLost() != 0) {
        if (RestoreGraphics() == 0) {
            m_mgr->ReportError(IDX(IDS_RESTORE_GAME), 0xfa0);
            return 0;
        }
    }
    m_world->SoundRegistry()->TickVolumeRamps();
    if (static_cast<u32>(g_gameAppFrameDeltaMs) >= m_previewCountdownMs) {
        m_previewCountdownMs = 0;
    } else {
        m_previewCountdownMs = m_previewCountdownMs - g_gameAppFrameDeltaMs;
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000de2c0, 0x5c)
i32 CPreviewState::RestorePreviewGraphics() {
    if (m_world->GetDisplayBuffers()->RestoreLostSurfaces() == 0) {
        return 0;
    }
    while (ShowCursor(false) >= 0) {
    }
    i32 result = LoadTitlePage(
        static_cast<const char*>(m_currentPreviewName),
        0,
        0,
        0,
        0,
        true
    );
    FadeSineToBuffer(0x50, 0x3e8, 0, true);
    return result;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000de340, 0x56)
i32 CPreviewState::RedrawPreview() {
    if (IsActive() == 0) {
        return 0;
    }
    while (ShowCursor(false) >= 0) {
    }
    i32 result = LoadTitlePage(
        static_cast<const char*>(m_currentPreviewName),
        0,
        0,
        0,
        0,
        true
    );
    FadeSineToBuffer(0x50, 0x3e8, 0, true);
    return result;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000de3c0, 0x2d)
i32 CPreviewState::HandlePreviewKey(i32 virtualKey, i32 unused) {
    if (virtualKey == VK_ESCAPE) {
        Cancel();
    }
    if (virtualKey == VK_SPACE || virtualKey == VK_RETURN) {
        ShowNextPreviewScreen();
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000de400, 0xd)
i32 CPreviewState::OnLButtonDown(i32, i32, i32) {
    ShowNextPreviewScreen();
    return 1;
}

RVA(0x000de420, 0x115)
void CPreviewState::ShowNextPreviewScreen() {
    char resourceKey[64];
    i32 previewIndex = m_nextPreviewIndex;
    m_nextPreviewIndex = previewIndex + 1;
    sprintf(resourceKey, "PREVIEW%i", previewIndex);
    m_currentPreviewName = resourceKey;
    sprintf(resourceKey, "\\SCREENZ\\%s", static_cast<const char*>(m_currentPreviewName));
    StateResources()->GetRezFromPath(resourceKey, IMGTAG_XCP);
    b32 failed = false;
    if (LoadTitlePage(
            static_cast<const char*>(m_currentPreviewName),
            0,
            0,
            0,
            0,
            true
        )
        == 0) {
        failed = true;
    } else {
        PlayRegistryCueIfElapsed(m_world->SoundRegistry(), "GAME_TELEPORTEROPEN");
        FadeSineToBuffer(0x50, 0x3e8, 0, true);
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
    PostMessageA(static_cast<HWND>((m_mgr->GetGameWindow()->GetHwnd())), WM_COMMAND, 0x8027, 0);
}
