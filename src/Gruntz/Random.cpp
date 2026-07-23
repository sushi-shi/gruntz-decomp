#include <Gruntz/Random.h>
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>

#include <Mfc.h> // superset of Win32.h; GameRegistry.h pulls afx via SoundCue.h
#include <rva.h>
#include <Gruntz/GameRegistry.h> // g_gameReg canonical view (0x24556c)

DATA(0x0024c22c)
char g_coinRolled; // bit0 set once this frame's coin was rolled
DATA(0x0024c26c)
i32 g_coinValue; // the cached 0/1 result
DATA(0x002c127d)
u8 g_randSeeded; // 0x6c127d bit0 set once seeded
DATA(0x002c1288)
i32 g_randSeed; // 0x6c1288 32-bit LCG state
DATA(0x002c278c)
char g_rng2Seeded; // bit0 set once seeded
DATA(0x002c2798)
i32 g_rng2State; // 32-bit LCG state

// Manager-owned primary LCG range helper. Retail callers pass g_gameReg in ecx;
// the body does not otherwise access the receiver.
// @early-stop
// Scheduling wall: cl hoists g_randSeeded and uses inc for the span where retail
// loads the flag later and materializes the span with lea.
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

namespace Rng {
    // __cdecl rand(): lazily seed from timeGetTime, then advance the MS-CRT LCG.
    // @interleaver Rng - own-namespace helper COMDAT scattered at 0x15cbe0; RVA-placement.
    RVA(0x0015cbe0, 0x46)
    i32 Next2() {
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
} // namespace Rng
