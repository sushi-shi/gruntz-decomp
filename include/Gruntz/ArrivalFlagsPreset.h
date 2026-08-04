#ifndef GRUNTZ_GRUNTZ_ARRIVALFLAGSPRESET_H
#define GRUNTZ_GRUNTZ_ARRIVALFLAGSPRESET_H

#include <Enums.h>

// The four flag words CGrunt::m_arrivalFlags is initialised to.
//
// These are PRESETS, not a bit vocabulary: all 109 sites are plain assignments
// and not one tests the word, so the individual bits have no evidence behind
// them and are not named here.
//
// What IS evidenced is which preset goes with which grunt, because one switch
// picks all four by EnemyAiType:
//
//   AI_NONE           -> PLAYER, or PLAYER_SINGLE when m_gameMode is
//                        GAMEMODE_SINGLE
//   AI_BATTLEZ_PATH   -> BATTLEZ
//   anything else     -> ENEMY
//
// The bit differences line up with that reading rather than cutting across it:
// BATTLEZ is PLAYER plus 0x82, and ENEMY is BATTLEZ plus 0x18000400 - each tier
// adds to the one before it.
//
// NOT included, despite sharing PLAYER_SINGLE's value: CTriggerMgr's
// `(attr & 0x4000911)`, which is a mask over trigger attributes rather than an
// arrival preset. Same number, different question, left alone.
GZ_ENUM_CONST_BEGIN(ArrivalFlagsPreset)
    ARRIVAL_FLAGS_PLAYER = 0x4000901,
    ARRIVAL_FLAGS_PLAYER_SINGLE = 0x4000911,
    ARRIVAL_FLAGS_BATTLEZ = 0x4000983,
    ARRIVAL_FLAGS_ENEMY = 0x1c000d83
GZ_ENUM_CONST_END(ArrivalFlagsPreset)

#endif // GRUNTZ_GRUNTZ_ARRIVALFLAGSPRESET_H
