#ifndef GRUNTZ_BIGANIMATIONMACROS_H
#define GRUNTZ_BIGANIMATIONMACROS_H

#define NORMALIZE_BIG_ANIMATION_WITH_AUX(heightLayer)                                              \
    CImage* aux = m_object->m_layer;                                                               \
    if (aux != NULL) {                                                                             \
        i32 bigW = aux->m_width;                                                                   \
        i32 bigH;                                                                                  \
        if (bigW >= g_buteMgr.GetInt("World", "BigActHeight")                                      \
            || (bigH = heightLayer->m_height) >= g_buteMgr.GetInt("World", "BigActHeight")) {      \
            if (m_object->m_animWorker != NULL) {                                                  \
                m_object->m_animWorker->m_flags &= ~6;                                             \
                m_object->m_animWorker->m_flags |= 1;                                              \
                m_wwdObject->m_flags &= ~0x1000002;                                                \
                m_wwdObject->m_flags |= 0x800000;                                                  \
            }                                                                                      \
        }                                                                                          \
    }

#define NORMALIZE_BIG_ANIMATION_DIRECT                                                             \
    if (m_object->m_layer != NULL) {                                                               \
        i32 bigW = m_object->m_layer->m_width;                                                     \
        i32 bigH;                                                                                  \
        if (bigW >= g_buteMgr.GetInt("World", "BigActHeight")                                      \
            || (bigH = m_object->m_layer->m_height)                                                \
                   >= g_buteMgr.GetInt("World", "BigActHeight")) {                                 \
            if (m_object->m_animWorker != NULL) {                                                  \
                m_object->m_animWorker->m_flags &= ~6;                                             \
                m_object->m_animWorker->m_flags |= 1;                                              \
                m_wwdObject->m_flags &= ~0x1000002;                                                \
                m_wwdObject->m_flags |= 0x800000;                                                  \
            }                                                                                      \
        }                                                                                          \
    }

#endif // GRUNTZ_BIGANIMATIONMACROS_H
