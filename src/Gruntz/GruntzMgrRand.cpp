#include <rva.h>

#include <Mfc.h>

#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Random.h>

// @identity-TODO RandRange@CGruntzMgr - thunk oracle: retail gave this an incremental
// thunk, so it was compiled into a LINK-LINE OBJECT, while the rest of this TU
// (1 fns) came from the static library. It belongs to another compiland.
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

// @interleaver Rng2Next - 70 B lone body at 0x15cbe0, between Deserialize
// (wwdfactoryobject) and GetFrame (wwdfactoryobject): a first-use placement.
