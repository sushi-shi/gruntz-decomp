#ifndef GRUNTZ_RANDOM_H
#define GRUNTZ_RANDOM_H

// The MS-CRT-style LCG random helpers Gruntz reaches through ILT jmp-thunks.
// Two independent generators share the same 214013/2531011 recurrence: the
// primary state (g_randSeeded/g_randSeed, canonical DATA in src/Globals.cpp) and
// a second state (g_rng2*) used by the 0x15cbe0 helper.
#include <Ints.h>

// The primary generator's state (DEFINED in src/Gruntz/Random.cpp): the seeded flag and
// the 32-bit LCG value. Declared here (the random subsystem's owner header) so consumers
// stop re-`extern`-ing them per-TU.
extern u8 g_randSeeded; // 0x6c127d  bit0 set once seeded
extern i32 g_randSeed;  // 0x6c1288  32-bit LCG state

namespace Rng {
    // 0xcd00: lazily seed the primary generator from timeGetTime, advance it, and
    // return the top 15 bits (the classic MS rand()).
    i32 Next();
    // 0x15cbe0: same recurrence over the second generator (g_rng2*).
    i32 Next2();
    // 0x19f50: a lazily-seeded value in [lo,hi]; coin-flips the endpoints when the
    // span collapses (hi == lo - 1).
    i32 __stdcall RangeStd(i32 lo, i32 hi);

    // 0xda200: a deterministic-in-replay coin flip cached once per frame; m_1c is
    // the object's replay-seed index.
    struct CoinFlip {
        char m_pad0[0x1c];
        i32 m_1c; // +0x1c
        i32 Flip();
    };

    // 0xcd70: a range roller that caches its result + parameters in the host record.
    struct RangeBox {
        char m_pad0[0x40];
        i32 m_40; // +0x40 lo
        i32 m_44; // +0x44 hi
        i32 m_48; // +0x48
        i32 m_4c; // +0x4c
        i32 m_50; // +0x50 rolled value
        i32 m_54; // +0x54 (1 once rolled)
        void Roll(i32 lo, i32 hi, i32 a3, i32 a4);
    };
} // namespace Rng

#endif // GRUNTZ_RANDOM_H
