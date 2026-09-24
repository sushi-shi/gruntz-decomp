#ifndef GRUNTZ_GRUNTZ_PLAYINLINE_H
#define GRUNTZ_GRUNTZ_PLAYINLINE_H

#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/Play.h>

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
