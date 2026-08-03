#ifndef GRUNTZ_GRUNTAISTATE_H
#define GRUNTZ_GRUNTAISTATE_H

#include <Enums.h>

// The engagement sub-state an enemy grunt's behaviour step runs in, held in
// CGrunt::m_defenderState. Orthogonal to EnemyAiType: the type picks WHICH step
// method runs each frame, this says where in its fight/return cycle the grunt is.
// Every behaviour in GruntArrivalScan.cpp / GruntTargetScan.cpp switches on it,
// and CBattlezData drives the same field from the Battlez side.
//
// Each name is what its own arm does, not a guess at intent:
//
//   SEEK    FindNearestEnemy each tick. If that grunt is standing still on its
//           tile and inside m_reachRect, CommitNeighbor swings at it; otherwise,
//           once m_dwell passes 0x3e8, TileSwitch paths to it, m_arrivalCell
//           latches its grid slot and the state goes to CHASE (with the 0x366
//           voice cue when the grunt is on screen).
//   CHASE   Re-reads m_tileMgr->m_grid[m_arrivalCell]. FindNearestEnemy returning
//           anyone else, or the slot emptying / leaving GruntInRadius, drops back
//           to SEEK; otherwise StepArrivalDrop re-paths every 0x1f4 of m_dwell
//           and reaching the target runs CommitNeighbor.
//   ATTACK  The swing is live: the arm only progresses while m_poweredUp, and
//           every failing guard falls back to CHASE with m_dwell reseeded to
//           0x1f4. CommitNeighbor is what enters it in the guard behaviours.
//   RETURN  The `resetState:` label. StepArrivalDrop walks back to
//           m_defenderPx - 0x20 - the post CGrunt::Place seeds for AI_POSTGUARD,
//           AI_OBJECTGUARD, AI_DEFENDER and AI_BOMBER - and arriving flips to
//           SEEK. Battlez reads the same value as "idle at post, take an order":
//           CBattlezData scans its row for RETURN grunts by
//           m_defenderQueuePosition and pushes the front one to SEEK or RETREAT.
//   COOLDOWN Parked after acting: the only exit is m_dwell passing 0x1f40, then
//           SEEK. The Time Bomber enters it after walking clear of its bomb and
//           the Tool Thief after stripping a tool.
//   RETREAT The Hit-And-Runner's back-off (CGrunt::WanderStep sets it whenever
//           m_poweredUp or a CommitNeighbor lands). Holds while m_combatActive,
//           returns to SEEK the moment m_stamina is back to 0x64, and otherwise
//           walks to a random tile within two of where it stands.
//
// NOT declared: 6, 0x19 and 0x1a. 0x19/0x1a live only inside CGrunt::PhaseStep
// (the Time Bomber), which sets them when a coord on the path lands on a cell
// carrying flag 0x20 and then mirrors the grunt to the far side; 6 has exactly
// one read, in CBattlezData, paired with AI_DEFENDER and PICKUP_BRICK. None of
// them appears in a switch and none has a second site to cross-check, so they
// stay literals - and m_defenderState stays i32 so that they still assign.
GZ_ENUM_BEGIN(GruntAiState)
    AISTATE_SEEK = 0,
    AISTATE_CHASE = 1,
    AISTATE_ATTACK = 2,
    AISTATE_RETURN = 3,
    AISTATE_COOLDOWN = 4,
    AISTATE_RETREAT = 5
GZ_ENUM_END(GruntAiState)

// The field this domain was recovered from, CGrunt::m_defenderState, also
// carries values OUTSIDE it, which is why it stays i32 and only its 0..5 sites
// are named. 6 and 7 are Battlez-only path states set by CBattlezMapConfig -
// 7 when the unit has a coord, 6 from the attack-waypoint and distance tests -
// and they gate m_dwell against m_moveBudget rather than the SEEK/CHASE/ATTACK
// ladder. 0x19 and 0x1a have one read each in GruntArrivalScan. None of the
// four has enough evidence to name, so all four stay literal.

#endif // GRUNTZ_GRUNTAISTATE_H
