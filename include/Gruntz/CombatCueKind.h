#ifndef GRUNTZ_GRUNTZ_COMBATCUEKIND_H
#define GRUNTZ_GRUNTZ_COMBATCUEKIND_H

#include <Enums.h>

// What CTriggerMgr::CombatCue does to every grunt in its radius. Each value is
// named by its own arm, with no inference:
//
//   1  CellDispatch(..., DEATH_DROP)     unless the grunt is invulnerable
//   2  TryTeleportToCell at a random offset, then a GAME_LIGHTING_FLASH
//   3  m_health = 100 - healed to full
//   5  gives a random toy: rand() % 9 + PICKUP_TOYZ_FIRST
//   6  CellDispatch(..., DEATH_EXPLODE)
//   7  CellDispatch(..., DEATH_SQUASH)
//
// 4 has no arm. The three DEATH_* tiers all guard on m_gruntKind !=
// GRUNT_INVULNERABLE; the three beneficial ones do not, which is the shape that
// tells the two halves of this domain apart.
GZ_ENUM_BEGIN(CombatCueKind)
    CUE_DROP = 1,
    CUE_TELEPORT = 2,
    CUE_HEAL = 3,
    CUE_GIVE_TOY = 5,
    CUE_EXPLODE = 6,
    CUE_SQUASH = 7
GZ_ENUM_END(CombatCueKind)

#endif // GRUNTZ_GRUNTZ_COMBATCUEKIND_H
