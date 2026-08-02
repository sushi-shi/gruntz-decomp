#include <rva.h>

#include <Gruntz/Random.h>

#include <Mfc.h>

#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>

DATA(0x0024c22c)
char g_coinRolled;
DATA(0x0024c26c)
i32 g_coinValue;
DATA(0x002c127d)
u8 g_randSeeded;
DATA(0x002c1288)
i32 g_randSeed;
DATA(0x002c278c)
char g_rng2Seeded;
DATA(0x002c2798)
i32 g_rng2State;

// @early-stop
RVA(0x00019f50, 0xb2)
i32 CGruntzMgr::RandRange(i32 lo, i32 hi) {
    i32 span = hi - lo + 1;
    i32 seed;
    if (span == 0) {
        if (!(g_randSeeded & 1)) {
            g_randSeeded |= 1;
            seed = timeGetTime();
        } else {
            seed = g_randSeed;
        }
        g_randSeed = seed * 214013 + 2531011;
        if (g_randSeed & 0x10000) {
            return lo;
        }
        return hi;
    }
    if (!(g_randSeeded & 1)) {
        g_randSeeded |= 1;
        seed = timeGetTime();
    } else {
        seed = g_randSeed;
    }
    g_randSeed = seed * 214013 + 2531011;
    return lo + ((g_randSeed >> 0x10) & 0x7fff) % span;
}

// @interleaver Rng emitted in a first-use COMDAT pool.
RVA(0x0015cbe0, 0x46)
i32 Rng2Next() {
    i32 seed;
    if (!(g_rng2Seeded & 1)) {
        g_rng2Seeded |= 1;
        seed = timeGetTime();
    } else {
        seed = g_rng2State;
    }
    g_rng2State = seed * 214013 + 2531011;
    return (g_rng2State >> 0x10) & 0x7fff;
}
