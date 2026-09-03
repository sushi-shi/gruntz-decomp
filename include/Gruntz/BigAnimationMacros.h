#ifndef GRUNTZ_BIGANIMATIONMACROS_H
#define GRUNTZ_BIGANIMATIONMACROS_H

#include <Mfc.h>
#include <MfcWin.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/LogicRecordFlags.h>
#include <Image/CImage.h>
#include <Wwd/WwdGameObjectFamily.h>
#include <Wwd/WwdGameObjectFlags.h>

inline void
NormalizeBigAnimation(CWwdSpriteObject* object, CWwdSpriteObject* wwdObject, CImage* heightLayer) {
    CImage* image = object->m_frameImage;
    if (image != NULL) {
        CSize bigSize;
        bigSize.cx = image->m_width;
        if (bigSize.cx >= g_buteMgr.GetInt("World", "BigActHeight")
            || (bigSize.cy = heightLayer->m_height) >= g_buteMgr.GetInt("World", "BigActHeight")) {
            if (object->m_logicRecord != NULL) {
                object->m_logicRecord->m_flags &=
                    ~(IDX(LOGIC_RECORD_FLAG_SMALL_ACTIVE_REGION)
                      | IDX(LOGIC_RECORD_FLAG_KEEP_ACTIVE));
                object->m_logicRecord->m_flags |= IDX(LOGIC_RECORD_FLAG_LARGE_ACTIVE_REGION);
                wwdObject->m_flags &=
                    ~(IDX(WWD_GAME_OBJECT_FLAG_SMALL_ACTIVE_REGION)
                      | IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));
                wwdObject->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_LARGE_ACTIVE_REGION);
            }
        }
    }
}

#endif // GRUNTZ_BIGANIMATIONMACROS_H
