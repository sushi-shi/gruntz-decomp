#ifndef GRUNTZ_GRUNTPICKUPSTATS_H
#define GRUNTZ_GRUNTPICKUPSTATS_H

#include <Ints.h>
#include <rva.h>

// The MEGAPHONE announce path resolves a unit-type count through g_gameReg->m_2c (the
// current game state), its +0x2dc sub-object, and that object's 0x10bbe0 count getter.
// @identity-TODO: the +0x2dc sub-object's class is unrecovered (BoundaryLowerMethodsViews.h:
// reg->m_2c's +0x2dc member, the +0x528 active-cell table getter).
SIZE_UNKNOWN(MegaCounter);
struct MegaCounter {
    i32 M(); // 0x10bbe0  the unit-type count getter
};
SIZE_UNKNOWN(MegaHolder);
struct MegaHolder {
    char m_pad0[0x2dc];
    MegaCounter* m_2dc; // +0x2dc
};

#endif // GRUNTZ_GRUNTPICKUPSTATS_H
