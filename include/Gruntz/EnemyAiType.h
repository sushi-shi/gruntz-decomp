#ifndef GRUNTZ_GRUNTZ_ENEMYAITYPE_H
#define GRUNTZ_GRUNTZ_ENEMYAITYPE_H

#include <Enums.h>

// An enemy Grunt's "nature", selected by the WWD `Points:` field
// (CGameObject +0x118) and valid only when `Smarts:` (+0x124, the team number)
// is non-zero. Names from docs/domain/enemy-ai.md, distilled from the level
// editor's AppendixC pages (01-DumbChaserz.html ... 16-ScrollGrunt.html).
//
// The 16-way dispatch is the per-frame grunt tick, RVA 0x0005d210 in Grunt.cpp
// (still carrying the placeholder name XferName), switching on
// CGrunt::m_arrivalState. The chain that proves m_arrivalState is this field:
// CLevelTileValidation and CPlay pass the WWD object's m_points to
// CTriggerMgr::PlaceObject as `aiType`, PlaceObject forwards it to CGrunt::Place
// as `kind`, and Place ends with `m_arrivalState = kind`. The same call passes
// m_direction as the sense radius and m_smarts as the team, matching the editor's
// GruntStartingPoint page field for field.
//
// The dispatch corroborates the ORDER independently: three of the step methods
// were named from their bodies before this table was consulted and land on their
// own slot - 8 -> StepBrickLayerBehavior, 10 -> StepGooSuckerBehavior,
// 11 -> StepDiggerBehavior. Two more arms name themselves: 13 is the only value
// for which CGrunt::BuildGruntDeathAnimation runs TryPowerupAtTile() (the Tool
// Thief drops what it stole), and 4 is the only value that seeds
// m_defenderRadius = 1 in the per-tool loaders (the Defender's one-tile post).
//
// NOT declared: 0x11. It has no arm in the dispatch and no documented slot, yet
// ~20 sites in Grunt.cpp branch on it - it drives the m_coordList waypoint path
// and takes m_arrivalFlags = 0x4000983, between AI_NONE's 0x4000901 and every
// other type's 0x1c000d83. The only aiType the engine produces outside the level
// file is CTriggerMgr's Battlez respawn, which reads it from the bute
// ("Grunt" / "RessurectAIType"), and that bute is data we do not have, so pairing
// 0x11 with that key would be a guess. It stays a literal at its use sites.
// (AI_COUNT below is 17 for the 0..16 roster; that it collides with the value is
// a coincidence, not evidence - do not spell 0x11 as AI_COUNT.)
GZ_ENUM_BEGIN(EnemyAiType)
    AI_NONE = 0,
    AI_DUMBCHASER = 1,
    AI_SMARTCHASER = 2,
    AI_HITANDRUNNER = 3,
    AI_DEFENDER = 4,
    AI_POSTGUARD = 5,
    AI_OBJECTGUARD = 6,
    AI_BOMBER = 7,
    AI_BRICKLAYER = 8,
    AI_GAUNTLETZGRUNT = 9,
    AI_GOOSUCKER = 10,
    AI_DIGGER = 11,
    AI_TIMEBOMBER = 12,
    AI_TOOLTHIEF = 13,
    AI_TOYER = 14,
    AI_MAGICWANDGRUNT = 15,
    AI_SCROLLGRUNT = 16,
    AI_COUNT = 17
GZ_ENUM_END(EnemyAiType)

#endif // GRUNTZ_GRUNTZ_ENEMYAITYPE_H
