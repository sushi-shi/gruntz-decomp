#include <Gruntz/GruntzMapMgr.h> // the real +0x70 board class (ex GruntBoard view)
#include <Ints.h>
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <rva.h>

#include <stdlib.h> // rand (0x11fee0)

#include <Gruntz/GameLevel.h>        // CGameLevel::PointInBounds (the on-screen voice gate)
#include <Gruntz/Grunt.h>            // canonical CGrunt + CGruntHud + the WwdGameReg facets
#include <Gruntz/GruntSpawnConfig.h> // the m_cueSink voice driver (SpawnVoiceDriver)
#include <Gruntz/TriggerMgr.h>       // canonical CTriggerMgr (the +0x260 board)


// ---------------------------------------------------------------------------
// @early-stop
// CRACKED 21%->57.6% (2026-07-05). The state machine is a `switch (m_defenderState)`
// (NOT if/else) - that alone produced retail's `sub eax; je state0; dec; je state1; dec;
// jne return; [state2 fall-through]` dispatch and the state2/state1/state0 reverse layout
// (+30% in one build). Also fixed: the powered-up (m_poweredUp) block flattened to retail's
// guard-gotos with the >=100 stamina path as fall-through; a real bug - the state 0/1/2
// RectContains args were reversed (must be (m_5c, m_60) like the top hitGate); state1's
// dwell compare is unsigned (jbe). Residual ~42% is three genuine walls (final sweep):
//   1. State-2 powered-up recheck (0x198, 57 insns) is DEAD in-source (the switch is only
//      reached with m_poweredUp==0, verified: 0x157 has one pred = the m_poweredUp==0
//      guard) so THIS cl dead-code-eliminates it; retail's cl emits it. Pure DCE-behavior
//      artifact - no clean C spelling forces the dead block without a goto/opaque hack.
//   2. Zero-register swap cascade: retail pins the ubiquitous 0 constant in ebp and hitGate
//      in ebx; this cl swaps them (ebx=0, ebp=hitGate), flipping every `cmp <zero>,x`.
//   3. Shared CommitNeighbor(0x5b050) tail-merge (states 0 and 2 share one
//      CommitNeighbor;return 1 tail via `sub esp,0xc` coord spills) + the surrounding
//      regalloc permutations.
RVA(0x000ef6b0, 0x61d)
i32 CGrunt::ChargeStep() {
    m_defenderX = m_lastTilePxX;
    m_defenderY = m_lastTilePxY;
    CTmCell* g = m_tileMgr->FindNearestEnemy(this);
    i32 hitGate = 0;
    if (g != 0) {
        CGameObject* gp = g->m_object;
        if (gp->m_screenX == g->m_lastTilePxX && gp->m_screenY == g->m_lastTilePxY
            && RectContains(gp->m_screenX, gp->m_screenY)) {
            hitGate = 1;
        }
    }

    if (m_poweredUp != 0) {
        if (m_neighborValid != 0) {
            m_neighborValid = 0;
            return 1;
        }
        if (m_combatActive != 0) {
            return 1;
        }
        if (m_stamina >= 100) {
            if (FindGridNeighbor(1) != 0) {
                return 1;
            }
            if (hitGate != 0 && g == 0) {
                return 1;
            }
            if (m_poweredUp == 0) {
                return 1;
            }
            if (m_neighborValid != 0) {
                return 1;
            }
            m_entranceActive = 0;
            m_combatActive = 0;
            m_neighborValid = 0;
            m_poweredUp = 0;
            ResetEntranceAnimation(1, 0, 0);
            return 1;
        }
        if (hitGate != 0) {
            return 1;
        }
        if (m_poweredUp == 0) {
            return 1;
        }
        if (m_neighborValid != 0) {
            return 1;
        }
        m_entranceActive = 0;
        m_combatActive = 0;
        m_neighborValid = 0;
        m_poweredUp = 0;
        ResetEntranceAnimation(1, 0, 0);
        return 1;
    }

    // ---- m_poweredUp == 0: the charge state machine (switch -> sub/dec/dec dispatch;
    // states 0 and 2 both end with CommitNeighbor(...);return 1 and MSVC tail-merges
    // them). ----
    switch (m_defenderState) {
        case 0: {
            // scan for a target on the wander tile
            if (g != 0) {
                if (hitGate != 0 && m_stamina >= 100) {
                    CGameObject* gp = g->m_object;
                    if (gp->m_screenX == g->m_lastTilePxX && gp->m_screenY == g->m_lastTilePxY
                        && RectContains(gp->m_screenX, gp->m_screenY)) {
                        CommitNeighbor(
                            g->m_tileOwnerHi,
                            g->m_tileOwnerLo,
                            g->m_lastTilePxX,
                            g->m_lastTilePxY
                        );
                        return 1;
                    }
                }
                if (m_dwell > 500) {
                    if (GruntInRadius(g->m_tileOwnerHi, g->m_tileOwnerLo) == 0) {
                        return 1;
                    }
                    if (TileSwitch(
                            g->m_object->m_screenX >> 5,
                            g->m_object->m_screenY >> 5,
                            0,
                            m_arrivalFlags,
                            1,
                            0
                        )
                        != 0) {
                        SetEntrancePos(1, 1);
                        m_arrivalCol = g->m_tileOwnerHi;
                        m_arrivalRow = g->m_tileOwnerLo;
                        m_defenderState = 1;
                        CWwdGameObjectA* mp = m_object;
                        CGruntzMgr* mgr = g_gameReg;
                        // the visible-rect gate: play the "engaged" voice only when this
                        // grunt is on screen (the rect sits 0x40 into the viewport object)
                        i32 los = CGameLevel::PointInBounds(
                            reinterpret_cast<const LevelCoordRect*>(&mgr->m_world->m_level->m_mainPlane->m_originX),
                            mp->m_screenX,
                            mp->m_screenY
                        );
                        if (los != 0) {
                            mgr->m_cueSink
                                ->SpawnVoiceDriver(reinterpret_cast<i32>(this), 0x366, -1, 0, -1, -1);
                        }
                    }
                    m_dwell = 0;
                    return 1;
                }
            }
            if (m_resetApplied == 0 && m_318 != 0 && m_dwell > 3000) {
                CWwdGameObjectA* mp = m_object;
                i32 baseX = mp->m_extent.left;
                i32 spanX = mp->m_extent.right - baseX;
                spanX = spanX < 0 ? -spanX : spanX;
                i32 baseY = mp->m_extent.top;
                i32 spanY = mp->m_extent.bottom - baseY;
                spanY = spanY < 0 ? -spanY : spanY;
                if (spanX != 0) {
                    baseX += rand() % spanX;
                }
                if (spanY != 0) {
                    baseY += rand() % spanY;
                }
                CGruntzMgr* mgr = g_gameReg;
                if (static_cast<u32>(baseX) < static_cast<u32>(mgr->m_tileGrid->m_width)
                    && static_cast<u32>(baseY) < static_cast<u32>(mgr->m_tileGrid->m_height)) {
                    TileSwitch(baseX, baseY, 0, m_arrivalFlags, 1, 0);
                }
                if (m_31c.GetCount() != 0) {
                    if (spanX <= spanY) {
                        spanX = spanY;
                    }
                    if (spanX < m_31c.GetCount()) {
                        SetEntrancePos(1, 1);
                    }
                }
                m_dwell = 0;
            }
            break;
        }
        case 1: {
            // moving to the arrival tile
            CTmCell* t = m_tileMgr->m_grid[m_arrivalRow + m_arrivalCol * TM_GRID_COLS];
            CTmCell* cur = m_tileMgr->FindNearestEnemy(this);
            if (cur != 0 && cur != t) {
                m_arrivalCol = -1;
                m_defenderState = 0;
                m_arrivalRow = -1;
                return 1;
            }
            if (t == 0 || t->m_entranceCommitted == 0
                || GruntInRadius(t->m_tileOwnerHi, t->m_tileOwnerLo) == 0) {
                m_defenderState = 0;
                return 1;
            }
            if (static_cast<u32>(m_dwell) > 500) {
                StepArrivalDrop(t->m_lastTilePxX, t->m_lastTilePxY, 0, m_arrivalFlags, 1, 0);
                m_dwell = 0;
            }
            if (m_poweredUp == 0 && m_stamina >= 100
                && RectContains(t->m_object->m_screenX, t->m_object->m_screenY) != 0
                && t->m_object->m_screenX == t->m_lastTilePxX
                && t->m_object->m_screenY == t->m_lastTilePxY) {
                CommitNeighbor(
                    t->m_tileOwnerHi,
                    t->m_tileOwnerLo,
                    t->m_lastTilePxX,
                    t->m_lastTilePxY
                );
                m_defenderState = 2;
                return 1;
            }
            break;
        }
        case 2: {
            // arrived: re-check target then hold
            if (m_poweredUp != 0) {
                CTmCell* t = m_tileMgr->m_grid[m_arrivalRow + m_arrivalCol * TM_GRID_COLS];
                if (t == 0 || GruntInRadius(t->m_tileOwnerHi, t->m_tileOwnerLo) == 0
                    || t->m_entranceCommitted == 0) {
                    m_defenderState = 1;
                    m_dwell = 0x1f4;
                    return 1;
                }
                if (m_neighborValid != 0 || m_combatActive != 0 || m_stamina < 100) {
                    return 1;
                }
                if (RectContains(t->m_object->m_screenX, t->m_object->m_screenY) == 0
                    || t->m_object->m_screenX != t->m_lastTilePxX
                    || t->m_object->m_screenY != t->m_lastTilePxY) {
                    m_defenderState = 1;
                    m_dwell = 0x1f4;
                    return 1;
                }
                CommitNeighbor(
                    t->m_tileOwnerHi,
                    t->m_tileOwnerLo,
                    t->m_lastTilePxX,
                    t->m_lastTilePxY
                );
                return 1;
            }
            m_defenderState = 1;
            m_dwell = 0x1f4;
            return 1;
        }
    }
    return 1;
}
