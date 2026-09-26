#ifndef GRUNTZ_GRUNTZ_PLAYINLINE_H
#define GRUNTZ_GRUNTZ_PLAYINLINE_H

#include <Dsndmgr/MidiManager.h>
#include <Gruntz/CoordPool.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/Play.h>
#include <Rez/FrameClock.h>

#include <string.h>

inline void CPlay::ResetAssetLoadState(GruntzPlayer* player) {
    player->m_active = true;
    player->m_humanControlled = true;
    m_region0Gate = false;
    m_region1Gate = false;
    m_region2Gate = false;
    m_region3Gate = false;
    m_viewportResizeMode = VIEW_RESIZE_IDLE;
    m_hudSuppressed = true;
    m_cameraBookmarkIndex = -1;
    m_defeatCountdownActive = false;
    m_scrollEdgeActive = 0;
    m_scrollEdgeLock = 0;
    m_levelTimer = NULL;
}

inline void CPlay::FreeStartMarkers() {
    for (i32 i = 0; i < StartMarkerCount(); i++) {
        Coord* node = StartMarkerAt(i);
        if (node != NULL) {
            g_coordPool.Push(node);
        }
    }
    m_startMarkers.SetSize(0, -1);
}

inline void CPlay::FreePlacedObjectCells(i32 group) {
    for (i32 i = 0; i < PlacedObjectCellCount(group); i++) {
        Coord* node = PlacedObjectCellAt(group, i);
        if (node != NULL) {
            g_coordPool.Push(node);
        }
    }
    m_placedObjectCells[group].SetSize(0, -1);
}

inline void CPlay::UpdateAmbientMusic() {
    if (m_ambientInitDone == false) {
        if (static_cast<i64>(g_frameTime) - m_ambientTiming.m_start.m_v
            >= m_ambientTiming.m_interval.m_v) {
            char sequenceName[0x40];
            wsprintfA(sequenceName, "AMBIENT%d", GetAmbientId());
            if (g_gameReg->m_musicEnabled != false) {
                m_mgr->m_midi->PlaySequence(sequenceName, true);
            } else {
                m_mgr->m_midi->SelectSequence(sequenceName);
                m_mgr->m_midi->SetCurrentLooping(true);
            }
            m_ambientInitDone = true;
        }
    }
}

inline void CPlay::SetInitialFramePending(b32 pending) {
    m_initialFramePending = pending;
}

inline void CPlay::SetNotifyLatch(b32 notify) {
    m_notifyLatch = notify;
}

inline void CPlay::SetCompletedFinalLevel(b32 completed) {
    m_completedFinalLevel = completed;
}

inline void CPlay::ClearSaveSlot() {
    memset(&m_saveSlot, 0, sizeof(m_saveSlot));
}

inline void CPlay::SetSavedClock(u32 clock) {
    m_savedClock = clock;
}

#endif // GRUNTZ_GRUNTZ_PLAYINLINE_H
