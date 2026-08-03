#ifndef GRUNTZ_GRUNTZ_BATTLEZTASK_H
#define GRUNTZ_GRUNTZ_BATTLEZTASK_H

#include <Enums.h>

// What a Battlez AI unit is currently doing, as carried by CGrunt::m_battleState
// and serialized with the rest of the grunt record.
//
// Every value below is named by a guard the code itself writes. Three of them
// are named by the TOOL the unit must be holding for that state to be legal -
// each site reads the unit's tool and, if it does not match, resets the state:
//
//   6   `st != PICKUP_GOOBER && m_battleState == 6` -> reset to ADVANCE
//   9   `st != PICKUP_SPY    && m_battleState == 9` -> reset to ADVANCE
//   0xa `st == PICKUP_BRICK  && m_battleState == 0` -> becomes 0xa
//       and later `prim == PICKUP_BRICK && m_battleState == 0xa` -> ApplyTriggerB
//
// The rest:
//
//   0   the free pool. CGrunt's constructor sets it, and the scheduler counts
//       units in it as `freeCount` before deciding how many more it can task.
//   3   the unit has a specific target. Two readings that do not fully agree,
//       so the name asserts only their overlap: it is set right after
//       LoadPickupSprites(PICKUP_BRICK) and when m_arrivalCell is written, and
//       the cell picker skips any cell a unit in this state already holds -
//       but the per-state dispatch sends it to TrackAssignedEnemy. Whether the
//       target is a cell or an enemy is not settled here.
//   4   advancing. The state a cancelled task falls back to, and the only one
//       that consults m_targetTeam - it measures the distance to that team's
//       m_marker. CGrunt also keeps a unit's path list across a re-tool only
//       when it is in this state (`m_battleState != 4` drops the coord list).
//   0xb heading for a switch. Set where the cell query finds TRIGID_SWITCH_2,
//       alongside m_defenderState = AISTATE_SEEK.
//
// The per-state step dispatch is an independent check on all of these, and it
// confirms 4 outright - that arm calls AdvanceToEnemyBase.
//
// 2 and 7 exist in that dispatch and are DELIBERATELY unnamed: nothing else in
// the tree writes or tests them, so their only evidence is the step function
// each one calls, and those names are reconstructions rather than retail's.
// 1, 5 and 8 do not appear at all.
GZ_ENUM_BEGIN(BattlezTask)
    BZTASK_UNASSIGNED = 0,
    BZTASK_ASSIGNED_TARGET = 3,
    BZTASK_ADVANCE = 4,
    BZTASK_CARRY_GOOBER = 6,
    BZTASK_CARRY_SPY = 9,
    BZTASK_CARRY_BRICK = 0xa,
    BZTASK_SEEK_SWITCH = 0xb
GZ_ENUM_END(BattlezTask)

#endif // GRUNTZ_GRUNTZ_BATTLEZTASK_H
