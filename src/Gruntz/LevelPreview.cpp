#include <StdAfx.h>

#include <Ints.h>

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

b32 g_previewCancelQuits = false;

i32 CPreviewState::Enter(CGruntzMgr* mgr, i32 areaArg, i32 prevStateId) {

    if (CState::LoadGameAssetNamespaces(mgr, areaArg, prevStateId) == 0) {
        return 0;
    }
    while (ShowCursor(false) >= 0) {
    }
    m_stateResources = m_resourceArchive->GetDirFromPath("STATEZ_PREVIEW");
    if (m_stateResources == NULL) {
        return 0;
    }
    if (g_disableAudio == false && g_disableSound == false) {
        CRezDir* set = StateResources()->GetDir("SOUNDZ");
        if (set != NULL) {
            m_world->SoundRegistry()->LoadFromTree(static_cast<CRezDir*>(set), "PREVIEW", "_");
        }
    }
    m_previewName = "PREVIEW0";
    m_previewIndex = 0;
    m_mgr->m_gameWnd->PumpMessages(WM_KEYDOWN, 0x40);
    return 1;
}

void CPreviewState::ResetPreview() {
    SoundCueRegistry* reg = m_world->SoundRegistry();
    if (reg->m_soundStream != NULL) {
        reg->m_soundStream->StopAllStreams();
    }
    m_world->SoundRegistry()->RemoveWithPrefix("PREVIEW", "_");
    CState::ReleaseResources();
}

i32 CPreviewState::NextScreenCmd(i32 unused) {
    while (ShowCursor(false) >= 0) {
    }
    LoadLevelPreviewScreen();
    return 1;
}

i32 CPreviewState::AcceptPreviewCommand(i32 unused) {
    return 1;
}

i32 CPreviewState::Tick() {
    if (IsSceneFading()) return AdvanceSceneFade(m_mgr->Timing().deltaMs()) >= 0;
    IDirectDrawSurface* surf =
        m_world->GetDrawTarget()->GetFrontSurface()->GetSurface()->GetDirectDrawSurface();
    if (surf == NULL || surf->IsLost() != 0) {
        if (InputVirtual() == 0) {
            m_mgr->ReportError(IDX(IDS_RESTORE_GAME), 0xfa0);
            return 0;
        }
    }
    m_world->SoundRegistry()->TickVolumeRamps();
    if (m_mgr->Timing().deltaMs() >= m_previewCountdownMs) {
        m_previewCountdownMs = 0;
    } else {
        m_previewCountdownMs = m_previewCountdownMs - m_mgr->Timing().deltaMs();
    }
    return 1;
}

i32 CPreviewState::Refade() {
    CancelSceneFade();
    if (m_world->GetDrawTarget()->PagesReady() == 0) {
        return 0;
    }
    while (ShowCursor(false) >= 0) {
    }
    i32 r =
        LoadTitlePage((m_previewName), 0, 0, 0, 0, true);
    return r && BeginSceneFade(0x50, 0x3e8, 0, true);
}

i32 CPreviewState::RefadeVirtual() {
    return IsActive() && Refade();
}

i32 CPreviewState::RestoreAfterSceneFade() {
    if (!m_world->GetDrawTarget()->PagesReady()) return 0;
    if (!LoadAndPresentTitlePage(m_previewName, 0, 0, 0, 0)) return 0;
    OnSceneFadeComplete();
    return 1;
}

i32 CPreviewState::OnKey(i32 key, i32 unused) {
    if (key == VK_ESCAPE) {
        Cancel();
    }
    if (key == VK_SPACE || key == VK_RETURN) {
        LoadLevelPreviewScreen();
    }
    return 1;
}

i32 CPreviewState::OnLButtonDown(i32, i32, i32) {
    LoadLevelPreviewScreen();
    return 1;
}

void CPreviewState::LoadLevelPreviewScreen() {
    if (IsSceneFading()) return;
    m_resetTimerAfterFade = true;
    i32 idx = m_previewIndex;
    m_previewIndex = idx + 1;
    m_previewName = formatText("PREVIEW%i", idx);
    const std::string path = "\\SCREENZ\\" + m_previewName;
    StateResources()->GetRezFromPath(path.c_str(), IMGTAG_XCP);
    b32 failed = false;
    if (LoadTitlePage((m_previewName), 0, 0, 0, 0, true)
        == 0) {
        failed = true;
    } else {
        PlayRegistryCueIfElapsed(m_world->SoundRegistry(), "GAME_TELEPORTEROPEN");
        if (!BeginSceneFade(0x50, 0x3e8, 0, true)) failed = true;
    }
    if (failed) {
        Cancel();
    }
}

void CPreviewState::Cancel() {
    CancelSceneFade();
    m_resetTimerAfterFade = false;
    if (g_previewCancelQuits) {
        m_mgr->DelayedQuit();
        return;
    }
    PostMessageA(static_cast<HWND>((m_mgr->m_gameWnd->GetHwnd())), WM_COMMAND, 0x8027, 0);
}

void CPreviewState::OnSceneFadeComplete() {
    if (m_resetTimerAfterFade) {
        m_previewCountdownMs = 60000;
        m_resetTimerAfterFade = false;
    }
}
