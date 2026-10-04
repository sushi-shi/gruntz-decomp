#ifndef GRUNTZ_GRUNTSPRITEMACROS_H
#define GRUNTZ_GRUNTSPRITEMACROS_H

#include <Wwd/WwdGameObjectFlags.h>

#define HIDE_AND_CLEAR_GRUNT_SPRITE(sprite)                                                        \
    if (sprite) {                                                                                  \
        sprite->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);                               \
        sprite = NULL;                                                                             \
    }

#define LOAD_POSE(dst, sfx)                                                                        \
    ((dst) = MapFind<CAnimationSequence>(                                                          \
         m_wwdObject->OwnerMgr()->GetAnimationRegistry()->m_animations,                            \
         "GRUNTZ_" + m_animSetName + (sfx)                                                         \
     ))

#define DEATH_FRAME()                                                                              \
    (m_wwdObject->m_animationCursor.GetAnimation()->RecordAt(0)->GetFrameParameter())

// *_IF_VISIBLE calls the out-of-line CGameLevel::PointInBounds; *_IN_VIEW inlines ::PtInRect.
#define PLAY_VOICE_IF_VISIBLE(tag)                                                                 \
    do {                                                                                           \
        CGruntzMgr* _g = g_gameReg;                                                                \
        if (CGameLevel::PointInBounds(                                                             \
                _g->m_world->m_level->m_mainPlane->GetPlaneViewRect(),                             \
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
                _g->World()->m_level->m_mainPlane->GetPlaneViewRect(),                             \
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
                _g->World()->m_level->m_mainPlane->GetPlaneViewRect(),                             \
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
                _g->m_world->m_level->m_mainPlane->GetPlaneViewRect(),                             \
                m_object->m_screenX,                                                               \
                m_object->m_screenY                                                                \
            )) {                                                                                   \
            _g->VoiceMgr()->PlayGruntVoiceCue(this, (cue), -1, -1, -1);                            \
        }                                                                                          \
    } while (0)

#define PICKUP(key, idv)                                                                           \
    do {                                                                                           \
        CAnimationSequence* pickupAnimation = MapFind<CAnimationSequence>(                         \
            m_wwdObject->OwnerMgr()->GetAnimationRegistry()->m_animations,                         \
            (key)                                                                                  \
        );                                                                                         \
        m_pickupAnimation = pickupAnimation;                                                       \
        id = (idv);                                                                                \
    } while (0)

#endif // GRUNTZ_GRUNTSPRITEMACROS_H
