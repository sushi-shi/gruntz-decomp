#ifndef GRUNTZ_GAMERAND_H
#define GRUNTZ_GAMERAND_H

#include <Mfc.h>

#include <Ints.h>

#include <stdlib.h>

__inline i32 GetRandomNumber() {
    static long s_holdrand = timeGetTime();
    return (((s_holdrand = s_holdrand * 214013L + 2531011L) >> 16) & 0x7fff);
}

__inline i32 GetRandomNumber(i32 lo, i32 hi) {
    return lo + GetRandomNumber() % (hi - lo + 1);
}

__inline i32 GetRandom(i32 lo, i32 hi) {
    if ((hi - lo + 1) == 0) {
        if (rand() & 1) {
            return lo;
        } else {
            return hi;
        }
    }
    return (rand() % (hi - lo + 1)) + lo;
}

#endif // GRUNTZ_GAMERAND_H
