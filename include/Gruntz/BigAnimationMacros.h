#ifndef GRUNTZ_BIGANIMATIONMACROS_H
#define GRUNTZ_BIGANIMATIONMACROS_H

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
            if (m_object->m_animWorker != NULL) {                                                  \
                m_object->m_animWorker->m_flags &= ~6;                                             \
                m_object->m_animWorker->m_flags |= 1;                                              \
                m_wwdObject->m_flags &= ~0x1000002;                                                \
                m_wwdObject->m_flags |= 0x800000;                                                  \
            }                                                                                      \
        }                                                                                          \
    }

#endif // GRUNTZ_BIGANIMATIONMACROS_H
