#ifndef GRUNTZ_GRUNTZ_BOOTYSTATSINLINE_H
#define GRUNTZ_GRUNTZ_BOOTYSTATSINLINE_H

#include <Ints.h>

static __inline i32 maxRunIndex(const i32* values, i32 count) {
    i32 best = -1;
    i32 bestIndex = 0;
    for (i32 i = 0; i < count; i++) {
        if (values[i] > best) {
            best = values[i];
            bestIndex = i;
        }
    }
    return bestIndex;
}

static __inline i32 sumRun(i32* p, i32 n) {
    i32 s = 0;
    i32 k;
    for (k = 0; k < n; k++) {
        s += p[k];
    }
    return s;
}

#endif // GRUNTZ_GRUNTZ_BOOTYSTATSINLINE_H
