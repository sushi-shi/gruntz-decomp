#ifndef GRUNTZ_GRUNTSPRITEMACROS_H
#define GRUNTZ_GRUNTSPRITEMACROS_H

#define HIDE_AND_CLEAR_GRUNT_SPRITE(sprite)                                                        \
    if (sprite) {                                                                                  \
        sprite->m_flags |= 0x10000;                                                                \
        sprite = NULL;                                                                             \
    }

#endif // GRUNTZ_GRUNTSPRITEMACROS_H
