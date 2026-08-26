#ifndef GRUNTZ_WWD_WWDRECTOVERLAPINLINE_H
#define GRUNTZ_WWD_WWDRECTOVERLAPINLINE_H

#include <Wwd/WwdFactoryObject.h>

inline i32 CDDrawRectsOverlap(const CDDrawRect* a, const CDDrawRect* b) {
    if (a->left > b->right) {
        return 0;
    }
    if (a->right < b->left) {
        return 0;
    }
    if (a->top > b->bottom) {
        return 0;
    }
    return a->bottom >= b->top;
}

#endif // GRUNTZ_WWD_WWDRECTOVERLAPINLINE_H
