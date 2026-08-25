#ifndef GRUNTZ_GRUNTAISTATE_H
#define GRUNTZ_GRUNTAISTATE_H

#include <Enums.h>

// The m_dwell thresholds the behaviours below wait on. m_dwell accumulates
// g_frameDelta (CGrunt::Step), so these are milliseconds, and each is named for
// the transition it gates - the prose below already described all three, in hex:
//
//   500   the CHASE re-path interval, and the value ATTACK reseeds m_dwell to
//         when it falls back. Also what nine sites assign as the seed.
//   1000  how long SEEK waits before TileSwitch paths it to a target
//   3000  how long a stuck grunt waits before ResetEntranceAnimation
//   8000  the only exit from COOLDOWN
GZ_ENUM_CONST_BEGIN(GruntDwellMs)
    DWELL_REPATH_MS = 0x1f4,
    DWELL_SEEK_PATH_MS = 0x3e8,
    DWELL_STUCK_RESET_MS = 0xbb8,
    DWELL_COOLDOWN_MS = 0x1f40
GZ_ENUM_CONST_END(GruntDwellMs)

// The engagement sub-state an enemy grunt's behaviour step runs in, held in
// CGrunt::m_defenderState. Orthogonal to EnemyAiType: the type picks WHICH step
// method runs each frame, this says where in its fight/return cycle the grunt is.
// Every behaviour in GruntArrivalScan.cpp / GruntTargetScan.cpp switches on it,
// and CGameStats drives the same field from the Battlez side.
//
// Each name is what its own arm does, not a guess at intent:
//
//   SEEK    FindNearestEnemy each tick. If that grunt is standing still on its
//           tile and inside m_reachRect, CommitNeighbor swings at it; otherwise,
//           once m_dwell passes 0x3e8, TileSwitch paths to it, m_arrivalCell
//           latches its grid slot and the state goes to CHASE (with the 0x366
//           voice cue when the grunt is on screen).
//   CHASE   Re-reads m_triggerMgr->m_units[m_arrivalCell]. FindNearestEnemy returning
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
//           CGameStats scans its row for RETURN grunts by
//           m_defenderQueuePosition and pushes the front one to SEEK or RETREAT.
//   COOLDOWN Parked after acting: the only exit is m_dwell passing 0x1f40, then
//           SEEK. The Time Bomber enters it after walking clear of its bomb and
//           the Tool Thief after stripping a tool.
//   RETREAT The Hit-And-Runner's back-off (CGrunt::StepHitAndRunnerBehavior sets it whenever
//           m_poweredUp or a CommitNeighbor lands). Holds while m_combatActive,
//           returns to SEEK the moment m_stamina is back to 0x64, and otherwise
//           walks to a random tile within two of where it stands.
//
// Battlez adds a two-stage route to an enemy base. ROUTE_TARGET paths to the
// selected waypoint (or the base when there are no waypoints); FINAL_ROUTE then
// paths to the base marker itself. StepTimeBomberBehavior adds two one-tick transitions for
// a path cell carrying the phase-trigger bit. Both mirror the grunt across that
// cell, then differ only in the state they enter afterwards.
GZ_ENUM_BEGIN(GruntAiState)
    AISTATE_SEEK = 0,
    AISTATE_CHASE = 1,
    AISTATE_ATTACK = 2,
    AISTATE_RETURN = 3,
    AISTATE_COOLDOWN = 4,
    AISTATE_RETREAT = 5,
    AISTATE_BATTLEZ_ROUTE_TARGET = 6,
    AISTATE_BATTLEZ_FINAL_ROUTE = 7,
    AISTATE_PHASE_MIRROR_THEN_COOLDOWN = 0x19,
    AISTATE_PHASE_MIRROR_THEN_SEEK = 0x1a
GZ_ENUM_END(GruntAiState)

#endif // GRUNTZ_GRUNTAISTATE_H
