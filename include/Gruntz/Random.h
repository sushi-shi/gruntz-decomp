#ifndef GRUNTZ_RANDOM_H
#define GRUNTZ_RANDOM_H

#include <Ints.h>

extern u8 g_randSeeded;   // 0x6c127d  bit0 set once seeded
extern i32 g_randSeed;    // 0x6c1288  32-bit LCG state
extern char g_coinRolled; // 0x64c22c  bit0 set once this frame's ambient coin was rolled
extern i32 g_coinValue;   // 0x64c26c  the cached 0/1 result (CPlay::GetAmbientId)

namespace Rng {
    // 0xcd00: lazily seed the primary generator from timeGetTime, advance it, and
    // return the top 15 bits (the classic MS rand()). Defined in WorldSoundSet.cpp
    // (its owner TU - the worldsoundset tail band).
    i32 Next();
    // 0x15cbe0: same recurrence over the second generator (g_rng2*).
    i32 Next2();
    // 0x19f50: a lazily-seeded value in [lo,hi]; coin-flips the endpoints when the
    // span collapses (hi == lo - 1).
    i32 __stdcall RangeStd(i32 lo, i32 hi);

} // namespace Rng

#endif // GRUNTZ_RANDOM_H
