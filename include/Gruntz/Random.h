#ifndef GRUNTZ_RANDOM_H
#define GRUNTZ_RANDOM_H

#include <Ints.h>

extern u8 g_randSeeded;   // 0x6c127d  bit0 set once seeded
extern i32 g_randSeed;    // 0x6c1288  32-bit LCG state
extern char g_coinRolled; // 0x64c22c  bit0 set once this frame's ambient coin was rolled
extern i32 g_coinValue;   // 0x64c26c  the cached 0/1 result (CPlay::GetAmbientId)

namespace Rng {
    // 0x15cbe0: same recurrence over the second generator (g_rng2*).
    i32 Next2();
} // namespace Rng

#endif // GRUNTZ_RANDOM_H
