#include <Enums.h>
#include <Gruntz/PickupType.h>
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
#include <Gruntz/ScanGridMacros.h>

#pragma intrinsic(strcmp)

#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/TriggerMgrRecords.h>
#include <Wap32/TileGeometry.h>
#include <Gruntz/StaminaPct.h>
#include <limits.h>

// @early-stop
// Referent set is exact (52/52 in order) since the CRT-rand fix; the residue is
// block placement inside the reroll arm.
RVA(0x000ed9f0, 0x9a0)
i32 CGrunt::WanderStep() {
    m_defenderPx.m_x = m_lastTilePx.m_x;
    m_defenderPx.m_y = m_lastTilePx.m_y;

    i32 flag = 0;
    CGrunt* g = m_tileMgr->FindNearestEnemy(this);
    if (g != NULL) {
        i32 gx = g->m_object->m_screenX;
        if (gx == g->m_lastTilePx.m_x && g->m_object->m_screenY == g->m_lastTilePx.m_y
            && RectContains(gx, g->m_object->m_screenY) != 0) {
            flag = 1;
        }
    }

    if (m_poweredUp != 0) {
        if (m_neighborValid == 0) {
            if (m_combatActive == 0) {
                if (m_stamina >= STAMINA_FULL) {
                    if (FindGridNeighbor(1) != NULL) {
                        m_defenderState = AISTATE_RETREAT;
                        return 1;
                    }
                    if (flag != 0 && g == NULL) {
                        goto retreat;
                    }
                    if (m_poweredUp == 0 || m_neighborValid != 0) {
                        goto retreat;
                    }
                } else {
                    if (flag != 0) {
                        goto retreat;
                    }
                    if (m_poweredUp == 0 || m_neighborValid != 0) {
                        goto retreat;
                    }
                }
                m_entranceActive = 0;
                m_combatActive = 0;
                m_neighborValid = 0;
                m_poweredUp = 0;
                ResetEntranceAnimation(1, 0, 0);
            }
        } else {
            m_neighborValid = 0;
        }
    retreat:
        m_defenderState = AISTATE_RETREAT;
    }

    switch (m_defenderState) {
        case AISTATE_SEEK:
            if (g != NULL) {
                if (m_poweredUp == 0 && m_stamina >= STAMINA_FULL
                    && g->m_object->m_screenX == g->m_lastTilePx.m_x
                    && g->m_object->m_screenY == g->m_lastTilePx.m_y
                    && RectContains(g->m_object->m_screenX, g->m_object->m_screenY) != 0) {
                    Coord cp = g->m_lastTilePx;
                    CommitNeighbor(g->m_tileOwnerHi, g->m_tileOwnerLo, cp.m_x, cp.m_y);
                    m_neighborScanEnabled = 0;
                    if (CoordCount() != 0) {
                        void* node = m_coordList.GetHeadPosition();
                        if (node != NULL) {
                            do {
                                CoordNode* cur = static_cast<CoordNode*>(node);
                                node = *static_cast<void**>(node);
                                Coord* data = cur->m_coord;
                                if (data != NULL) {
                                    g_coordPool.Push(data);
                                }
                            } while (node != NULL);
                        }
                        m_coordList.RemoveAll();
                    }
                    m_defenderState = AISTATE_RETREAT;
                    return 1;
                }
                if (static_cast<u32>(m_dwell) > DWELL_SEEK_PATH_MS) {
                    if (GruntInRadius(g->m_tileOwnerHi, g->m_tileOwnerLo) != 0) {
                        Coord c[2];
                        g->GetScreenPos(c);
                        if (TileSwitch(
                                c[0].m_x >> TILE_SHIFT_PX,
                                c[0].m_y >> TILE_SHIFT_PX,
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
                            CGruntzMgr* reg = g_gameReg;
                            if (CGameLevel::PointInBounds(
                                    &reg->m_world->m_level->m_mainPlane->m_viewRect,
                                    m_object->m_screenX,
                                    m_object->m_screenY
                                )
                                != 0) {
                                reg->m_cueSink->SpawnVoiceDriver(this, 0x366, -1, 0, -1, -1);
                            }
                        }
                    }
                    m_dwell = 0;
                    return 1;
                }
            }
            goto timeout;

        case AISTATE_CHASE: {
            CGrunt* slot = m_tileMgr->m_grid[m_arrivalCell.m_x * TM_GRID_COLS + m_arrivalCell.m_y];
            CGrunt* active = m_tileMgr->FindNearestEnemy(this);
            if (active != NULL && active != slot) {
                m_arrivalCell.m_x = -1;
                m_defenderState = AISTATE_SEEK;
                m_arrivalCell.m_y = -1;
                return 1;
            }
            if (slot == NULL || slot->m_entranceCommitted == 0
                || GruntInRadius(slot->m_tileOwnerHi, slot->m_tileOwnerLo) == 0) {
                m_defenderState = AISTATE_SEEK;
                return 1;
            }
            if (static_cast<u32>(m_dwell) > DWELL_REPATH_MS) {
                StepArrivalDrop(
                    slot->m_lastTilePx.m_x,
                    slot->m_lastTilePx.m_y,
                    0,
                    m_arrivalFlags,
                    1,
                    0
                );
                m_dwell = 0;
            }
            if (m_poweredUp != 0) {
                return 1;
            }
            if (m_stamina < STAMINA_FULL) {
                return 1;
            }
            if (RectContains(slot->m_object->m_screenX, slot->m_object->m_screenY) == 0) {
                return 1;
            }
            if (slot->m_object->m_screenX != slot->m_lastTilePx.m_x) {
                return 1;
            }
            if (slot->m_object->m_screenY != slot->m_lastTilePx.m_y) {
                return 1;
            }
            Coord cp = slot->m_lastTilePx;
            CommitNeighbor(slot->m_tileOwnerHi, slot->m_tileOwnerLo, cp.m_x, cp.m_y);
            m_neighborScanEnabled = 0;
            if (CoordCount() != 0) {

                POSITION pos = m_coordList.GetHeadPosition();
                while (pos != NULL) {
                    void* data = m_coordList.GetNext(pos);
                    if (data != NULL) {
                        g_coordPool.Push(data);
                    }
                }
                m_coordList.RemoveAll();
            }
            m_defenderState = AISTATE_RETREAT;
            return 1;
        }

        case AISTATE_ATTACK: {
            if (m_poweredUp == 0) {
                m_defenderState = AISTATE_SEEK;
                return 1;
            }
            CGrunt* slot = m_tileMgr->m_grid[m_arrivalCell.m_x * TM_GRID_COLS + m_arrivalCell.m_y];
            if (slot == NULL || GruntInRadius(slot->m_tileOwnerHi, slot->m_tileOwnerLo) == 0
                || slot->m_entranceCommitted == 0) {
                goto ph1;
            }
            if (m_neighborValid != 0) {
                return 1;
            }
            if (m_combatActive != 0) {
                return 1;
            }
            if (m_stamina < STAMINA_FULL) {
                return 1;
            }
            if (RectContains(slot->m_object->m_screenX, slot->m_object->m_screenY) == 0) {
                goto ph1;
            }
            if (slot->m_object->m_screenX != slot->m_lastTilePx.m_x) {
                goto ph1;
            }
            if (slot->m_object->m_screenY != slot->m_lastTilePx.m_y) {
                goto ph1;
            }
            Coord cp = slot->m_lastTilePx;
            CommitNeighbor(slot->m_tileOwnerHi, slot->m_tileOwnerLo, cp.m_x, cp.m_y);
            m_neighborScanEnabled = 0;
            if (CoordCount() != 0) {
                POSITION pos = m_coordList.GetHeadPosition();
                while (pos != NULL) {
                    void* data = m_coordList.GetNext(pos);
                    if (data != NULL) {

                        CoordPoolNode* fslot = g_coordPool.NodeOf(data);
                        fslot->m_next = g_coordPool.m_freeHead;
                        g_coordPool.m_freeHead = fslot;
                    }
                }
                m_coordList.RemoveAll();
            }
            m_defenderState = AISTATE_RETREAT;
            m_dwell = DWELL_REPATH_MS;
            return 1;
        ph1:
            m_defenderState = AISTATE_CHASE;
            m_dwell = DWELL_REPATH_MS;
            return 1;
        }

        case AISTATE_RETREAT: {
            if (m_combatActive != 0) {
                return 1;
            }
            if (m_stamina >= STAMINA_FULL) {
                m_defenderState = AISTATE_SEEK;
                return 1;
            }
            if (CoordCount() != 0) {
                return 1;
            }
            CWwdGameObjectA* base = m_object;
            i32 clip = 1;
            i32 py = rand() % 4 + (base->m_screenY >> TILE_SHIFT_PX) - 2;
            i32 px = rand() % 4 + (base->m_screenX >> TILE_SHIFT_PX) - 2;
            if (static_cast<u32>(m_arrivalCell.m_x) < 4
                && static_cast<u32>(m_arrivalCell.m_y) < 0xf) {
                CGrunt* entry = g_gameReg->m_cmdGrid
                                    ->m_grid[m_arrivalCell.m_x * TM_GRID_COLS + m_arrivalCell.m_y];
                if (entry != NULL) {
                    CGameObject* e10 = entry->m_object;
                    RECT rc;
                    rc.left = (e10->m_screenX >> TILE_SHIFT_PX) - 2;
                    rc.top = (e10->m_screenY >> TILE_SHIFT_PX) - 2;
                    rc.right = (e10->m_screenX >> TILE_SHIFT_PX) + 3;
                    rc.bottom = (e10->m_screenY >> TILE_SHIFT_PX) + 3;
                    POINT pt;
                    pt.x = px;
                    pt.y = py;
                    if (PtInRect(&rc, pt)) {
                        clip = 0;
                    }
                }
            }
            if (clip == 0) {
                return 1;
            }
            CMapMgr* grid = g_gameReg->m_tileGrid;
            if (static_cast<u32>(px) >= static_cast<u32>(grid->m_width)) {
                return 1;
            }
            if (static_cast<u32>(py) >= static_cast<u32>(grid->m_height)) {
                return 1;
            }
            TileSwitch(px, py, 0, m_arrivalFlags, 1, 0);
            return 1;
        }

        default:
            return 1;
    }

timeout:
    if (m_resetApplied == 0 && m_hasExtent != 0
        && static_cast<u32>(m_dwell) > DWELL_STUCK_RESET_MS) {
        if (static_cast<i64>(g_frameTime) - m_arrivalReroll64 < m_arrivalRerollWindow64) {
            CWwdGameObjectA* base = m_object;
            u32 lx = static_cast<u32>(base->m_extent.left);
            i32 ax = abs(base->m_extent.right - static_cast<i32>(lx));
            u32 ly = static_cast<u32>(base->m_extent.top);
            i32 ay = abs(base->m_extent.bottom - static_cast<i32>(ly));
            if (ax != 0) {
                lx += rand() % ax;
            }
            if (ay != 0) {
                ly += rand() % ay;
            }
            if (lx < g_gameReg->m_tileGrid->m_width && ly < g_gameReg->m_tileGrid->m_height) {
                TileSwitch(static_cast<i32>(lx), static_cast<i32>(ly), 0, m_arrivalFlags, 1, 0);
            }
            if (CoordCount() != 0) {
                if (ax <= ay) {
                    ax = ay;
                }
                if (CoordCount() > ax) {
                    SetEntrancePos(1, 1);
                    m_dwell = 0;
                    return 1;
                }
            }
        } else {
            ResetEntranceAnimation(1, 1, 0);
            m_arrivalRerollLo = 0;
            m_arrivalRerollWindowLo = 0;
            m_arrivalRerollHi = 0;
            m_arrivalRerollWindowHi = 0;
            m_arrivalRerollWindowLo = rand() % 30000 + 30000;
            m_arrivalRerollWindowHi = 0;
            m_arrivalRerollLo = static_cast<i32>(g_frameTime);
            m_arrivalRerollHi = 0;
        }
        m_dwell = 0;
    }
    return 1;
}
