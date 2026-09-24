#ifndef GRUNTZ_GRUNTSPRITEMACROS_H
#define GRUNTZ_GRUNTSPRITEMACROS_H

#include <Wwd/WwdGameObjectFlags.h>

#define HIDE_AND_CLEAR_GRUNT_SPRITE(sprite)                                                        \
    if (sprite) {                                                                                  \
        sprite->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);                               \
        sprite = NULL;                                                                             \
    }

#define LOAD_POSE(dst, sfx)                                                                        \
    ((dst) = MapFind<CAniElement>(                                                                 \
         m_wwdObject->OwnerMgr()->m_animRegistry->m_animations,                                    \
         "GRUNTZ_" + m_animSetName + (sfx)                                                         \
     ))

#define DEATH_FRAME()                                                                              \
    (static_cast<CAniRecordView*>(                                                                 \
         m_wwdObject->m_animationCursor.m_animation->m_records.GetSize() > 0                       \
             ? m_wwdObject->m_animationCursor.m_animation->m_records.GetAt(0)                      \
             : NULL                                                                                \
    )                                                                                              \
         ->m_param)

#define DEATH_CUE(tag)                                                                             \
    do {                                                                                           \
        CGruntzMgr* _g = g_gameReg;                                                                \
        if (CGameLevel::PointInBounds(                                                             \
                &_g->m_world->m_level->m_mainPlane->m_planeViewRect,                               \
                m_object->m_screenPosition.m_x,                                                    \
                m_object->m_screenPosition.m_y                                                     \
            )) {                                                                                   \
            _g->m_voiceManager->PlayVoice(this, (tag), -1, 0, -1, -1);                             \
        }                                                                                          \
    } while (0)

#define PICKUP(key, idv)                                                                           \
    do {                                                                                           \
        CAniElement* geo = NULL;                                                                   \
        MapLookup(m_wwdObject->OwnerMgr()->m_animRegistry->m_animations, (key), geo);              \
        m_pickupGeoSrc = geo;                                                                      \
        id = (idv);                                                                                \
    } while (0)

#endif // GRUNTZ_GRUNTSPRITEMACROS_H
