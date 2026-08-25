#ifndef GRUNTZ_GAMERAND_H
#define GRUNTZ_GAMERAND_H

#include <Mfc.h>

#include <Ints.h>

#include <stdlib.h>

__inline i32 GetRandomNumber() {
    static long holdrand = timeGetTime();
    return (((holdrand = holdrand * 214013L + 2531011L) >> 16) & 0x7fff);
}

__inline i32 GetRandom(i32 lo, i32 hi) {
    i32 n = hi - lo + 1;
    if (n == 0) {
        return (rand() & 1) ? lo : hi;
    }
    return lo + rand() % n;
}

#endif // GRUNTZ_GAMERAND_H
