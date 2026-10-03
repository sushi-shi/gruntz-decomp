#ifndef GRUNTZ_TRIGGERMGRCAMERAINLINE_H
#define GRUNTZ_TRIGGERMGRCAMERAINLINE_H

#include <Gruntz/TriggerMgr.h>
#include <Wwd/WwdGameObjectFamily.h>

inline void CTriggerMgr::ClearCameraSprite() {
    if (m_goal != NULL) {
        m_goal->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
        m_goal = NULL;
    }
}

inline void CTriggerMgr::StopCameraTracking() {
    ClearCameraSprite();
    m_armed = false;
}

#endif
