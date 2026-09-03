#ifndef GRUNTZ_BIGANIMATIONMACROS_H
#define GRUNTZ_BIGANIMATIONMACROS_H

#include <Mfc.h>

#include <DDrawMgr/LogicRecordFlags.h>
#include <Wwd/WwdGameObjectFlags.h>

#define NORMALIZE_BIG_ANIMATION_WITH_AUX(heightLayer)                                              \
    CImage* aux = m_object->m_frameImage;                                                          \
    if (aux != NULL) {                                                                             \
        CSize bigSize;                                                                             \
        bigSize.cx = aux->m_width;                                                                 \
        if (bigSize.cx >= g_buteMgr.GetInt("World", "BigActHeight")                                \
            || (bigSize.cy = heightLayer->m_height)                                                \
                   >= g_buteMgr.GetInt("World", "BigActHeight")) {                                 \
            if (m_object->m_logicRecord != NULL) {                                                 \
                m_object->m_logicRecord->m_flags &=                                                \
                    ~(IDX(LOGIC_RECORD_FLAG_SMALL_ACTIVE_REGION)                                   \
                      | IDX(LOGIC_RECORD_FLAG_KEEP_ACTIVE));                                       \
                m_object->m_logicRecord->m_flags |= IDX(LOGIC_RECORD_FLAG_LARGE_ACTIVE_REGION);    \
                m_wwdObject->m_flags &=                                                            \
                    ~(IDX(WWD_GAME_OBJECT_FLAG_SMALL_ACTIVE_REGION)                                \
                      | IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));                                    \
                m_wwdObject->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_LARGE_ACTIVE_REGION);             \
            }                                                                                      \
        }                                                                                          \
    }

#endif // GRUNTZ_BIGANIMATIONMACROS_H
