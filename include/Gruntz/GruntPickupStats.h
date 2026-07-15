// GruntPickupStats.h - the MEGAPHONE-count helper shapes CGrunt::LoadPickupSprites
// reaches through the game-registry singleton. Homed out of GruntPickupLoad.cpp
// (were .cpp-local views). Offsets are load-bearing.
// (The former GruntPickupStats struct is DISSOLVED: the +0x7c object IS CBattlezData
// - its +0xd4/+0x1dc/+0x200/+0x254 "arrays" were the +0xd8/+0x238/+0x2d8/+0x348
// pickup-stat bands with the PickupType id base folded into the displacement, and
// the band sizes 4x{22,10,7,4} end exactly at sizeof == 0x388. See BattlezData.h.)
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
