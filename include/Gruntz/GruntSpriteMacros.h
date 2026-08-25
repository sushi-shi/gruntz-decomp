#ifndef GRUNTZ_GRUNTSPRITEMACROS_H
#define GRUNTZ_GRUNTSPRITEMACROS_H

#include <Wwd/WwdGameObjectFlags.h>

#define HIDE_AND_CLEAR_GRUNT_SPRITE(sprite)                                                        \
    if (sprite) {                                                                                  \
        sprite->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);                               \
        sprite = NULL;                                                                             \
    }

#endif // GRUNTZ_GRUNTSPRITEMACROS_H
