#ifndef GRUNTZ_GAMERAND_H
#define GRUNTZ_GAMERAND_H

#include <Mfc.h>

#include <Ints.h>

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
// The body itself, written ONCE. Retail needs three separate COMMON slots (one
// per module), which requires three distinct class scopes - but not three copies
// of the source text.
#define GZ_GET_RANDOM_NUMBER_BODY                                                                  \
    static long holdrand = timeGetTime();                                                          \
    return (((holdrand = holdrand * 214013L + 2531011L) >> 16) & 0x7fff);

__inline i32 GetRandomNumber() {
    GZ_GET_RANDOM_NUMBER_BODY
}

#endif // GRUNTZ_GAMERAND_H
