#ifndef GRUNTZ_GRUNTZ_BATTLEZUNITKIND_H
#define GRUNTZ_GRUNTZ_BATTLEZUNITKIND_H

#include <Enums.h>

// Which unit a Battlez roster slot spawns, as read from the level file's
// GruntStartingPoint "Points" field while m_gameMode is GAMEMODE_QUESTZ.
//
// This is a SECOND domain over the same storage as EnemyAiType, not a subset of
// it. The two disagree at every value: slot 7 grants a Brick where AI_BOMBER
// sits, slot 8 grants Gravity Bootz where AI_BRICKLAYER sits, slot 10 grants a
// Spy where AI_GOOSUCKER sits. So the field means "which AI nature" in a normal
// level and "which starting tool" in Battlez, and the dispatch that reads it is
// gated on the game mode.
//
// Each name is the PickupType its own arm hands the unit.
GZ_ENUM_BEGIN(BattlezUnitKind)
    BZUNIT_BOMB = 1,
    BZUNIT_GUNHAT = 2,
    BZUNIT_GAUNTLETZ = 3,
    BZUNIT_CLUB = 4,
    BZUNIT_SHIELD = 5,
    BZUNIT_GLOVEZ = 6,
    BZUNIT_BRICK = 7,
    BZUNIT_GRAVITYBOOTZ = 8,
    BZUNIT_BOOMERANG = 9,
    BZUNIT_SPY = 10,
    BZUNIT_NERFGUN = 11,
    BZUNIT_ROCK = 12,
    BZUNIT_GOOBER = 13,
    BZUNIT_SWORD = 14,
    BZUNIT_SHOVEL = 15,
    // Also a Shovel, but this slot additionally seeds the unit's vehicle. It is the
    // only pair in the roster that grants the same tool twice.
    BZUNIT_SHOVEL_MOUNTED = 16
GZ_ENUM_END(BattlezUnitKind)

#endif // GRUNTZ_GRUNTZ_BATTLEZUNITKIND_H
