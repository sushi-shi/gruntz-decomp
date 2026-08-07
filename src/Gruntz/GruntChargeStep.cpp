#include <Enums.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/GameRand.h>
#include <Mfc.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/TileCollisionKind.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GameLevel.h>
#include <Wap32/ZVec.h>
#include <Ints.h>
#include <string.h>
#include <stdlib.h>
#include <Gruntz/FreeNodePool.h>
#include <MfcWin.h>
#include <new>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/EnemyAiType.h>
#include <rva.h>
#include <Gruntz/GruntDirStatics.h>

#pragma intrinsic(strcmp)

#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/TriggerMgrRecords.h>
#include <Wap32/TileGeometry.h>
#include <Gruntz/StaminaPct.h>
#include <Gruntz/ScanGridMacros.h>
#include <limits.h>

// @early-stop
// Frame is 8 bytes short of retail (push ecx = 4 vs sub esp,0xc): two spill slots we do
// not model, which is the whole `add esp,0xc`-vs-`pop ecx` epilogue delta. Retail also
// keeps two exits we do not - cl cross-jumps the m_poweredUp block's two
// ResetEntranceAnimation tails into one and folds one guarded GruntInRadius/RectContains
// pair - so retail has 61 branches / 11 rets against our 47 / 9.
RVA(0x000ef6b0, 0x61d)
i32 CGrunt::ChargeStep() {
    m_defenderPx = m_lastTilePx;
    CGrunt* g = m_tileMgr->FindNearestEnemy(this);
    i32 hitGate = 0;
    if (g != NULL) {
        CGameObject* gp = g->m_object;
        if (gp->m_screenX == g->m_lastTilePx.m_x && gp->m_screenY == g->m_lastTilePx.m_y
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
        if (m_stamina >= STAMINA_FULL) {
            if (FindGridNeighbor(1) != NULL) {
                return 1;
            }
            if (hitGate != 0 && g == NULL) {
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

    switch (m_defenderState) {
        case AISTATE_SEEK: {

            if (g != NULL) {
                if (hitGate != 0 && m_stamina >= STAMINA_FULL) {
                    CGameObject* gp = g->m_object;
                    if (gp->m_screenX == g->m_lastTilePx.m_x && gp->m_screenY == g->m_lastTilePx.m_y
                        && RectContains(gp->m_screenX, gp->m_screenY)) {
                        CommitNeighbor(
                            g->m_tileOwnerHi,
                            g->m_tileOwnerLo,
                            g->m_lastTilePx.m_x,
                            g->m_lastTilePx.m_y
                        );
                        return 1;
                    }
                }
                if (static_cast<u32>(m_dwell) > 500) {
                    if (GruntInRadius(g->m_tileOwnerHi, g->m_tileOwnerLo) == 0) {
                        return 1;
                    }
                    if (TileSwitch(
                            g->m_object->m_screenX >> TILE_SHIFT_PX,
                            g->m_object->m_screenY >> TILE_SHIFT_PX,
                            0,
                            m_arrivalFlags,
                            1,
                            0
                        )
                        != 0) {
                        SetEntrancePos(1, 1);
                        m_arrivalCell.m_x = g->m_tileOwnerHi;
                        m_arrivalCell.m_y = g->m_tileOwnerLo;
                        m_defenderState = AISTATE_CHASE;
                        CWwdGameObjectA* mp = m_object;
                        CGruntzMgr* mgr = g_gameReg;

                        i32 los = CGameLevel::PointInBounds(
                            &mgr->m_world->m_level->m_mainPlane->m_viewRect,
                            mp->m_screenX,
                            mp->m_screenY
                        );
                        if (los != 0) {
                            mgr->m_cueSink->SpawnVoiceDriver(this, 0x366, -1, 0, -1, -1);
                        }
                    }
                    m_dwell = 0;
                    return 1;
                }
            }
            if (m_resetApplied == 0 && m_hasExtent != 0 && static_cast<u32>(m_dwell) > 3000) {
                CWwdGameObjectA* mp = m_object;
                i32 baseX = mp->m_extent.left;
                i32 spanX = abs(mp->m_extent.right - baseX);
                i32 baseY = mp->m_extent.top;
                i32 spanY = abs(mp->m_extent.bottom - baseY);
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
                if (m_coordList.GetCount() != 0) {
                    if (spanX <= spanY) {
                        spanX = spanY;
                    }
                    if (m_coordList.GetCount() > spanX) {
                        SetEntrancePos(1, 1);
                    }
                }
                m_dwell = 0;
            }
            break;
        }
        case AISTATE_CHASE: {

            CGrunt* t = m_tileMgr->m_grid[m_arrivalCell.m_y + m_arrivalCell.m_x * TM_GRID_COLS];
            CGrunt* cur = m_tileMgr->FindNearestEnemy(this);
            if (cur != NULL && cur != t) {
                m_arrivalCell.m_x = -1;
                m_defenderState = AISTATE_SEEK;
                m_arrivalCell.m_y = -1;
                return 1;
            }
            if (t == NULL || t->m_entranceCommitted == 0
                || GruntInRadius(t->m_tileOwnerHi, t->m_tileOwnerLo) == 0) {
                m_defenderState = AISTATE_SEEK;
                return 1;
            }
            if (static_cast<u32>(m_dwell) > 500) {
                StepArrivalDrop(t->m_lastTilePx.m_x, t->m_lastTilePx.m_y, 0, m_arrivalFlags, 1, 0);
                m_dwell = 0;
            }
            if (m_poweredUp == 0 && m_stamina >= STAMINA_FULL
                && RectContains(t->m_object->m_screenX, t->m_object->m_screenY) != 0
                && t->m_object->m_screenX == t->m_lastTilePx.m_x
                && t->m_object->m_screenY == t->m_lastTilePx.m_y) {
                CommitNeighbor(
                    t->m_tileOwnerHi,
                    t->m_tileOwnerLo,
                    t->m_lastTilePx.m_x,
                    t->m_lastTilePx.m_y
                );
                m_defenderState = AISTATE_ATTACK;
                return 1;
            }
            break;
        }
        case AISTATE_ATTACK: {

            if (m_poweredUp != 0) {
                CGrunt* t = m_tileMgr->m_grid[m_arrivalCell.m_y + m_arrivalCell.m_x * TM_GRID_COLS];
                if (t == NULL || GruntInRadius(t->m_tileOwnerHi, t->m_tileOwnerLo) == 0
                    || t->m_entranceCommitted == 0) {
                    m_defenderState = AISTATE_CHASE;
                    m_dwell = DWELL_REPATH_MS;
                    return 1;
                }
                if (m_neighborValid != 0 || m_combatActive != 0 || m_stamina < STAMINA_FULL) {
                    return 1;
                }
                if (RectContains(t->m_object->m_screenX, t->m_object->m_screenY) == 0
                    || t->m_object->m_screenX != t->m_lastTilePx.m_x
                    || t->m_object->m_screenY != t->m_lastTilePx.m_y) {
                    m_defenderState = AISTATE_CHASE;
                    m_dwell = DWELL_REPATH_MS;
                    return 1;
                }
                CommitNeighbor(
                    t->m_tileOwnerHi,
                    t->m_tileOwnerLo,
                    t->m_lastTilePx.m_x,
                    t->m_lastTilePx.m_y
                );
                return 1;
            }
            m_defenderState = AISTATE_CHASE;
            m_dwell = DWELL_REPATH_MS;
            return 1;
        }
    }
    return 1;
}
