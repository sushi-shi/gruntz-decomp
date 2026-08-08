#ifndef GRUNTZ_GAMERAND_H
#define GRUNTZ_GAMERAND_H

#include <Mfc.h>

#include <Ints.h>

#include <stdlib.h>

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
    static long holdrand = timeGetTime();
    return (((holdrand = holdrand * 214013L + 2531011L) >> 16) & 0x7fff);
}

// A uniform draw from the CLOSED range [lo, hi], inlined at 26 retail sites.
// The degenerate arm (hi == lo - 1, so the span is 0) costs a SECOND `rand()`
// call, which is what identifies the helper in the disassembly: every one of
// those sites is a `test n,n / jne` around two distinct `call _rand`s, one
// feeding `cdq; idiv`, the other a branchless coin-flip select between the two
// endpoints. See docs/patterns/rand-modulo-peel.md.
__inline i32 GetRandom(i32 lo, i32 hi) {
    i32 n = hi - lo + 1;
    if (n == 0) {
        return (rand() & 1) ? lo : hi;
    }
    return lo + rand() % n;
}

#endif // GRUNTZ_GAMERAND_H
