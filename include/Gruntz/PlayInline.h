#ifndef GRUNTZ_GRUNTZ_PLAYINLINE_H
#define GRUNTZ_GRUNTZ_PLAYINLINE_H

#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/Play.h>

#include <string.h>

static inline void ResetAssetLoadState(CPlay* play, GruntzPlayer* player) {
    player->m_active = true;
    player->m_humanControlled = true;
    play->m_region0Gate = false;
    play->m_region1Gate = false;
    play->m_region2Gate = false;
    play->m_region3Gate = false;
    play->m_viewportResizeMode = VIEW_RESIZE_IDLE;
    play->m_hudSuppressed = true;
    play->m_cameraBookmarkIndex = -1;
    play->m_defeatCountdownActive = false;
    play->m_scrollEdgeActive = 0;
    play->m_scrollEdgeLock = 0;
    play->m_levelTimer = NULL;
}

static inline void SetInitialFramePending(CPlay* play, b32 pending) {
    play->m_initialFramePending = pending;
}

static inline void SetNotifyLatch(CPlay* play, b32 notify) {
    play->m_notifyLatch = notify;
}

static inline void SetCompletedFinalLevel(CPlay* play, b32 completed) {
    play->m_completedFinalLevel = completed;
}

static inline void ClearSaveSlot(CPlay* play) {
    memset(&play->m_saveSlot, 0, sizeof(play->m_saveSlot));
}

static inline void SetSavedClock(CPlay* play, u32 clock) {
    play->m_savedClock = clock;
}

#endif // GRUNTZ_GRUNTZ_PLAYINLINE_H
