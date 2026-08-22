#include <rva.h>

#include <Mfc.h>
#include <MfcWin.h>

#include <Enums.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/FreeNodePoolInline.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntCoordRecycleMacros.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntMovementInline.h>
#include <Gruntz/GruntMovementMacros.h>
#include <Gruntz/GruntPoweredStateMacros.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntRandomPointMacros.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/ScanGridMacros.h>
#include <Gruntz/StaminaPct.h>
#include <Gruntz/TileCollisionKind.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TriggerMgrRecords.h>
#include <Gruntz/TypeKeyColl.h>
#include <Ints.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/ZVec.h>

#include <limits.h>
#include <new>
#include <stdlib.h>
#include <string.h>

// @early-stop
// Referent set is exact (52/52 in order) since the CRT-rand fix; the residue is
// block placement inside the reroll arm.
RVA(0x000ed9f0, 0x900)
i32 CGrunt::WanderStep() {
    m_defenderPx = m_lastTilePx;

    i32 flag = 0;
    FIND_NEAREST_ENEMY_AT_TARGET_WITH_FLAG(g, flag, gx)

    i32 powered = m_poweredUp;
    if (powered != 0) {
        i32 neighborValid = m_neighborValid;
        if (neighborValid == 0) {
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
                RESET_GRUNT_POWERED_STATE
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
                if (m_poweredUp == 0 && m_stamina >= STAMINA_FULL && GRUNT_AT_SAVED_SCREEN_POS(g)
                    && RectContains(g->m_object->m_screenX, g->m_object->m_screenY) != 0) {
                    COMMIT_GRUNT_NEIGHBOR_COPY(g, cp);
                    m_neighborScanEnabled = 0;
                    RecycleGruntCoords(this);
                    m_defenderState = AISTATE_RETREAT;
                    return 1;
                }
                if (static_cast<u32>(m_dwell) > DWELL_SEEK_PATH_MS) {
                    if (GruntInRadius(g->m_tileOwnerHi, g->m_tileOwnerLo) != 0) {
                        Coord c[2];
                        g->GetScreenTile(c);
                        if (TileSwitch(c[0].m_x, c[0].m_y, 0, m_arrivalFlags, 1, 0) != 0) {
                            SET_GRUNT_ARRIVAL_TARGET(g);
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
                Coord none;
                m_arrivalCell = *none.Set(-1, -1);
                m_defenderState = AISTATE_SEEK;
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
            if (slot->GRUNT_SCREEN_X_NOT_AT_SAVED_POS(m_object, slot)) {
                return 1;
            }
            if (slot->GRUNT_SCREEN_Y_NOT_AT_SAVED_POS(m_object, slot)) {
                return 1;
            }
            COMMIT_GRUNT_NEIGHBOR_COPY(slot, cp);
            m_neighborScanEnabled = 0;
            RecycleGruntCoords(this);
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
            if (slot->GRUNT_SCREEN_X_NOT_AT_SAVED_POS(m_object, slot)) {
                goto ph1;
            }
            if (slot->GRUNT_SCREEN_Y_NOT_AT_SAVED_POS(m_object, slot)) {
                goto ph1;
            }
            COMMIT_GRUNT_NEIGHBOR_COPY(slot, cp);
            m_neighborScanEnabled = 0;
            if (CoordCount() != 0) {
                RECYCLE_GRUNT_COORDS_EXPANDED(this)
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
            i32 baseTileY = base->m_screenY >> TILE_SHIFT_PX;
            i32 baseTileX = base->m_screenX >> TILE_SHIFT_PX;
            i32 py = rand() % 4 + baseTileY - 2;
            i32 px = rand() % 4 + baseTileX - 2;
            if (static_cast<u32>(m_arrivalCell.m_x) < 4
                && static_cast<u32>(m_arrivalCell.m_y) < 0xf) {
                CGrunt* entry = g_gameReg->m_cmdGrid
                                    ->m_grid[m_arrivalCell.m_x * TM_GRID_COLS + m_arrivalCell.m_y];
                if (entry != NULL) {
                    CGameObject* e10 = entry->m_object;
                    CRect rc(
                        (e10->m_screenX >> TILE_SHIFT_PX) - 2,
                        (e10->m_screenY >> TILE_SHIFT_PX) - 2,
                        (e10->m_screenX >> TILE_SHIFT_PX) + 3,
                        (e10->m_screenY >> TILE_SHIFT_PX) + 3
                    );
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
        if (IsArrivalRerollPending() != 0) {
            CWwdGameObjectA* base = m_object;
            SELECT_RANDOM_EXTENT_POINT_UNSIGNED_CAST(base, lx, ax, ly, ay)
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
            ResetArrivalReroll();
        }
        m_dwell = 0;
    }
    return 1;
}
