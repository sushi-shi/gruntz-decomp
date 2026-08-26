#ifndef GRUNTZ_ANIADVANCECURSORINLINE_H
#define GRUNTZ_ANIADVANCECURSORINLINE_H

#include <Gruntz/AniAdvanceCursor.h>

inline i32 IsAniCursorComplete(const CAniAdvanceCursor* cursor) {
    return cursor->m_finished != false && cursor->m_frameTicksLeft == 0;
}

#endif // GRUNTZ_ANIADVANCECURSORINLINE_H
