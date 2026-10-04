#ifndef GRUNTZ_GRUNTZ_BOOTYSTATSINLINE_H
#define GRUNTZ_GRUNTZ_BOOTYSTATSINLINE_H

#include <Ints.h>

static __inline i32 sumRun(const i32* p, i32 n) {
    i32 s = 0;
    i32 k;
    for (k = 0; k < n; k++) {
        s += p[k];
    }
    return s;
}

#endif // GRUNTZ_GRUNTZ_BOOTYSTATSINLINE_H
