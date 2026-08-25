#ifndef GRUNTZ_BIGANIMATIONMACROS_H
#define GRUNTZ_BIGANIMATIONMACROS_H

#include <DDrawMgr/LogicRecordFlags.h>
#include <Wwd/WwdGameObjectFlags.h>

// The receiver-direct twin, which re-read m_object->m_layer instead of caching
// it, was folded onto this form 2026-08-22 by passing m_object->m_layer as the
// height layer - byte-neutral (0 rows moved).
#define NORMALIZE_BIG_ANIMATION_WITH_AUX(heightLayer)                                              \
    CImage* aux = m_object->m_layer;                                                               \
    if (aux != NULL) {                                                                             \
        i32 bigW = aux->m_width;                                                                   \
        i32 bigH;                                                                                  \
        if (bigW >= g_buteMgr.GetInt("World", "BigActHeight")                                      \
            || (bigH = heightLayer->m_height) >= g_buteMgr.GetInt("World", "BigActHeight")) {      \
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
