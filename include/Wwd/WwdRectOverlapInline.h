#ifndef GRUNTZ_WWD_WWDRECTOVERLAPINLINE_H
#define GRUNTZ_WWD_WWDRECTOVERLAPINLINE_H

#include <Wwd/WwdFactoryObject.h>

inline i32 CDDrawRectsOverlap(const CDDrawRect* a, const CDDrawRect* b) {
    if (a->m_left > b->m_right) {
        return 0;
    }
    if (a->m_right < b->m_left) {
        return 0;
    }
    if (a->m_top > b->m_bottom) {
        return 0;
    }
    return a->m_bottom >= b->m_top;
}

#endif // GRUNTZ_WWD_WWDRECTOVERLAPINLINE_H
