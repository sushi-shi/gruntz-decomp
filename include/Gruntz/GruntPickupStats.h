// GruntPickupStats.h - the pickup/powerup stat + MEGAPHONE-count helper shapes
// CGrunt::LoadPickupSprites reaches through the game-registry singleton. Homed out of
// GruntPickupLoad.cpp (were .cpp-local views). Offsets are load-bearing.
#ifndef GRUNTZ_GRUNTPICKUPSTATS_H
#define GRUNTZ_GRUNTPICKUPSTATS_H

#include <Ints.h>
#include <rva.h>

// The per-owner pickup-stat block hung off WwdGameReg+0x7c (a reused slot).
// @identity-TODO: the exact retail class is unrecovered - it is NOT CBattlezData (whose
// bands sit at +0xd8/+0x238/+0x2d8, while these arrays are at +0xd4/+0x1dc/+0x200/+0x254),
// but one of WwdGameReg::m_7c's other reused-slot casts (WwdGameRegInner / WwdGameRegAux).
// Modeled by its touched fields; the per-owner x per-type stat arrays stay documented
// offset access (their exact 2D dimensions are unproven):
//   +0xd4  [22*owner + type]  weapon stats     +0x1dc [10*owner + type]  toy stats
//   +0x200 [7*owner + type]   powerup stats    +0x254 [4*owner + type]   misc stats
SIZE_UNKNOWN(GruntPickupStats);
struct GruntPickupStats {
    char m_pad00[0x14];
    i32 m_toyzCount;   // +0x14
    i32 m_weaponCount; // +0x18
    char m_pad1c[0x24 - 0x1c];
    i32 m_powerupCount; // +0x24
};

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
