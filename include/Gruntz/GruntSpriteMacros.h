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

#define DEATH_FRAME() (m_wwdObject->m_animationCursor.m_animation->RecordAt(0)->m_param)

// *_IF_VISIBLE calls the out-of-line CGameLevel::PointInBounds; *_IN_VIEW inlines ::PtInRect.
#define PLAY_VOICE_IF_VISIBLE(tag)                                                                 \
    do {                                                                                           \
        CGruntzMgr* _g = g_gameReg;                                                                \
        if (CGameLevel::PointInBounds(                                                             \
                &_g->m_world->m_level->m_mainPlane->m_planeViewRect,                               \
                m_object->m_screenX,                                                               \
                m_object->m_screenY                                                                \
            )) {                                                                                   \
            _g->VoiceMgr()->PlayVoice(this, (tag), -1, 0, -1, -1);                                 \
        }                                                                                          \
    } while (0)

#define PLAY_GRUNT_CUE_IF_VISIBLE(cue)                                                             \
    do {                                                                                           \
        CGruntzMgr* _g = g_gameReg;                                                                \
        if (CGameLevel::PointInBounds(                                                             \
                &_g->m_world->m_level->m_mainPlane->m_planeViewRect,                               \
                m_object->m_screenX,                                                               \
                m_object->m_screenY                                                                \
            )) {                                                                                   \
            _g->VoiceMgr()->PlayGruntVoiceCue(this, (cue), -1, -1, -1);                            \
        }                                                                                          \
    } while (0)

#define PLAY_VOICE_IN_VIEW(tag)                                                                    \
    do {                                                                                           \
        CGruntzMgr* _g = g_gameReg;                                                                \
        if (::PtInRect(                                                                            \
                &_g->m_world->m_level->m_mainPlane->m_planeViewRect,                               \
                m_object->m_screenX,                                                               \
                m_object->m_screenY                                                                \
            )) {                                                                                   \
            _g->VoiceMgr()->PlayVoice(this, (tag), -1, 0, -1, -1);                                 \
        }                                                                                          \
    } while (0)

#define PLAY_GRUNT_CUE_IN_VIEW(cue)                                                                \
    do {                                                                                           \
        CGruntzMgr* _g = g_gameReg;                                                                \
        if (::PtInRect(                                                                            \
                &_g->m_world->m_level->m_mainPlane->m_planeViewRect,                               \
                m_object->m_screenX,                                                               \
                m_object->m_screenY                                                                \
            )) {                                                                                   \
            _g->VoiceMgr()->PlayGruntVoiceCue(this, (cue), -1, -1, -1);                            \
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
