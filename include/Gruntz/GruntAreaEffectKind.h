#ifndef GRUNTZ_GRUNTZ_GRUNTAREAEFFECTKIND_H
#define GRUNTZ_GRUNTZ_GRUNTAREAEFFECTKIND_H

#include <Enums.h>

// What CTriggerMgr::ApplyGruntAreaEffect does to every grunt in its radius. Each value is
// named by its own arm, with no inference:
//
//   1  StartUnitDeath(..., DEATH_DROP)     unless the grunt is invulnerable
//   2  TryTeleportToCell at a random offset, then a GAME_LIGHTING_FLASH
//   3  m_health = 100 - healed to full
//   4  StepArrivalCommit - freezes the affected Grunt in place
//   5  gives a random toy: rand() % 9 + PICKUP_TOYZ_FIRST
//   6  StartUnitDeath(..., DEATH_EXPLODE)
//   7  StartUnitDeath(..., DEATH_SQUASH)
//
// The three DEATH_* tiers all guard on m_gruntKind != GRUNT_INVULNERABLE; the
// beneficial/status ones do not, which is the shape that tells the two halves
// of this domain apart.
GZ_ENUM_BEGIN(GruntAreaEffectKind)
    GRUNT_AREA_EFFECT_DROP = 1,
    GRUNT_AREA_EFFECT_TELEPORT = 2,
    GRUNT_AREA_EFFECT_HEAL = 3,
    GRUNT_AREA_EFFECT_FREEZE = 4,
    GRUNT_AREA_EFFECT_GIVE_TOY = 5,
    GRUNT_AREA_EFFECT_EXPLODE = 6,
    GRUNT_AREA_EFFECT_SQUASH = 7
GZ_ENUM_END(GruntAreaEffectKind)

#endif // GRUNTZ_GRUNTZ_GRUNTAREAEFFECTKIND_H
