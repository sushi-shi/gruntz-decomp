#ifndef GRUNTZ_TRIGGERMGRCAMERAINLINE_H
#define GRUNTZ_TRIGGERMGRCAMERAINLINE_H

#include <Gruntz/TriggerMgr.h>
#include <Wwd/WwdGameObjectFamily.h>

inline void CTriggerMgr::ClearCameraSprite() {
    if (m_cameraSprite != NULL) {
        m_cameraSprite->AddFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
        m_cameraSprite = NULL;
    }
}

inline void CTriggerMgr::StopCameraTracking() {
    ClearCameraSprite();
    m_cameraTrackingActive = false;
}

#endif
