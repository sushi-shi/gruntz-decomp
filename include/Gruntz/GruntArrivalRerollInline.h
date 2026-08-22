#ifndef GRUNTZ_GRUNTARRIVALREROLLINLINE_H
#define GRUNTZ_GRUNTARRIVALREROLLINLINE_H

#include <Gruntz/Grunt.h>
#include <Rez/FrameClock.h>

#include <stdlib.h>

inline i32 IsGruntArrivalRerollPending(CGrunt* grunt) {
    return static_cast<i64>(g_frameTime) - grunt->m_arrivalReroll64
           < grunt->m_arrivalRerollWindow64;
}

inline i32 IsGruntHoldPending(CGrunt* grunt) {
    return static_cast<i64>(g_frameTime) - grunt->m_holdAnchor64 < grunt->m_holdWindow64;
}

inline void ResetGruntArrivalReroll(CGrunt* grunt) {
    grunt->ResetEntranceAnimation(1, 1, 0);
    grunt->m_arrivalRerollLo = 0;
    grunt->m_arrivalRerollWindowLo = 0;
    grunt->m_arrivalRerollHi = 0;
    grunt->m_arrivalRerollWindowHi = 0;
    grunt->m_arrivalRerollWindowLo = rand() % 30000 + 30000;
    grunt->m_arrivalRerollWindowHi = 0;
    grunt->m_arrivalRerollLo = static_cast<i32>(g_frameTime);
    grunt->m_arrivalRerollHi = 0;
}

#endif // GRUNTZ_GRUNTARRIVALREROLLINLINE_H
