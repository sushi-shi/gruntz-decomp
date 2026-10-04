#ifndef GRUNTZ_GRUNTZ_PLAYINLINE_H
#define GRUNTZ_GRUNTZ_PLAYINLINE_H

#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerList.h>
#include <DinMgr2/DirectInputMgr2.h>
#include <Dsndmgr/MidiManager.h>
#include <Gruntz/CoordPool.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/InputState.h>
#include <Gruntz/Play.h>
#include <Gruntz/Timer.h>
#include <Gruntz/TriggerMgr.h>
#include <Rez/FrameClock.h>

#include <string.h>

inline void CPlay::ResetAssetLoadState(GruntzPlayer* player) {
    player->m_active = true;
    player->SetHumanControlled(true);
    m_tinyViewportCurseActive = false;
    m_darknessCurseActive = false;
    m_monitorCurseActive = false;
    m_randomColorsCurseActive = false;
    m_viewportResizeMode = VIEW_RESIZE_IDLE;
    m_inputBlocked = true;
    m_cameraBookmarkIndex = -1;
    m_defeatCountdownActive = false;
    m_scrollEdgeActive = 0;
    m_scrollEdgeLock = 0;
    m_levelTimer = NULL;
}

inline void CPlay::HandleSelectionGroupKey(i32 slot) {
    if (g_gameplayInput->GetHeldButtons() & IDX(INPUT_BUTTON5)) {
        g_gameReg->GetTriggerMgr()->SaveSelectionGroup(slot);
    } else {
        g_gameReg->GetTriggerMgr()->RecallSelectionGroup(slot);
    }
}

inline void CPlay::FreeLevelTimer() {
    CLevelTimer* timer = m_levelTimer;
    if (timer != NULL) {
        timer->Reset();
        delete timer;
        m_levelTimer = NULL;
    }
}

inline void CPlay::FreeStartMarkers() {
    for (i32 i = 0; i < StartMarkerCount(); i++) {
        Coord* node = StartMarkerAt(i);
        if (node != NULL) {
            g_coordPool.Push(node);
        }
    }
    m_startMarkers.RemoveAll();
}

inline void CPlay::FreePlacedObjectCells(i32 group) {
    for (i32 i = 0; i < PlacedObjectCellCount(group); i++) {
        Coord* node = PlacedObjectCellAt(group, i);
        if (node != NULL) {
            g_coordPool.Push(node);
        }
    }
    m_placedObjectCells[group].RemoveAll();
}

inline void CPlay::UpdateAmbientMusic() {
    if (m_introMusicComplete == false) {
        if (m_introMusicTimer.Expired()) {
            char sequenceName[0x40];
            wsprintfA(sequenceName, "AMBIENT%d", GetMusicVariant());
            if (g_gameReg->m_musicEnabled != false) {
                m_mgr->m_midi->PlaySequence(sequenceName, true);
            } else {
                m_mgr->m_midi->SelectSequence(sequenceName);
                m_mgr->m_midi->SetCurrentLooping(true);
            }
            m_introMusicComplete = true;
        }
    }
}

inline void CPlay::DrawVisibleWorld() {
    m_world->m_level->VisitVisible(
        m_world->GetDisplayBuffers()->GetBackBuffer(),
        m_world->ChildGroup()
    );
    m_world->m_transientDrawList->RenderAndPrune(
        m_world->GetDisplayBuffers()->GetBackBuffer(),
        m_world->GetDisplayBuffers()->GetOverlayBuffer()
    );
}

inline void CPlay::DrawWorldView() {
    if (m_darknessCurseActive != false) {
        DrawDarknessView();
    } else {
        DrawVisibleWorld();
    }
}

inline void CPlay::SetInitialFramePending(b32 pending) {
    m_initialFramePending = pending;
}

inline void CPlay::SetReturningToMenu(b32 returning) {
    m_returningToMenu = returning;
}

inline void CPlay::SetCompletedFinalLevel(b32 completed) {
    m_completedFinalLevel = completed;
}

inline void CPlay::ClearSaveSlot() {
    memset(&m_saveSlot, 0, sizeof(m_saveSlot));
}

inline void CPlay::SetSavedGameTimeMs(u32 timeMs) {
    m_savedGameTimeMs = timeMs;
}

#endif // GRUNTZ_GRUNTZ_PLAYINLINE_H
