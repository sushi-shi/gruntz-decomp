#ifndef GRUNTZ_GAMERAND_H
#define GRUNTZ_GAMERAND_H

#include <Mfc.h>

#include <Gruntz/Random.h>
#include <Ints.h>

static __inline i32 GameRand() {
    i32 seed;
    if (!(g_randSeeded & 1)) {
        g_randSeeded |= 1;
        seed = static_cast<i32>(timeGetTime());
    } else {
        seed = g_randSeed;
    }
    g_randSeed = seed * 214013 + 2531011;
    return (g_randSeed >> 0x10) & 0x7fff;
}

#endif // GRUNTZ_GAMERAND_H
