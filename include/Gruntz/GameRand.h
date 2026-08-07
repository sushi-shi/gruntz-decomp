#ifndef GRUNTZ_GAMERAND_H
#define GRUNTZ_GAMERAND_H

#include <Mfc.h>

#include <Gruntz/Random.h>
#include <Ints.h>

// Monolith printed a simplified version of this in the game's own CREDITZ
// easter egg ("CODE: int GetRandomNumber() { static long holdrand =
// timeGetTime(); return (((holdrand = holdrand * 214013L + 2531011L) >> 16) &
// 0x7fff); }" with "OUTPUT: 10,10,10,10,..." underneath - they were showing off
// a BUG). The constants, shift and mask confirm this body exactly.
//
// The function-local-static spelling is NOT adoptable here, measured: it is
// byte-neutral (3 inlining units mean 86.5120 either way, so cl5 emits the same
// code) but a `static` inside a header-inline function gets its own copy PER TU
// (`?s_holdrand@?1??GameRand@@YAHXZ@4HA$S20850`, local bss), whereas retail has
// exactly ONE shared guard byte at 0x2c127d immediately followed by one seed at
// 0x2c1288. That adjacency IS the compiler's dynamic-init guard layout, so the
// shipped source held the pair at file scope - which is what this spells.
// Monolith's own source, printed verbatim by the game's CREDITZ easter egg:
//     int GetRandomNumber()
//     {
//         static long holdrand = timeGetTime();
//         return (((holdrand = holdrand * 214013L + 2531011L) >> 16) & 0x7fff);
//     }
// Kept in that exact form. NOT `static __inline`: a non-static inline's local
// static gets external linkage, so every TU shares ONE copy - which is why
// retail has exactly one dynamic-init guard byte at 0x2c127d immediately
// followed by one seed at 0x2c1288, rather than a pair per TU.
__inline i32 GetRandomNumber() {
    static i32 holdrand = static_cast<i32>(timeGetTime());
    return ((holdrand = holdrand * 214013 + 2531011) >> 0x10) & 0x7fff;
}

#endif // GRUNTZ_GAMERAND_H
