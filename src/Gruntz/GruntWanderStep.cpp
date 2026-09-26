#include <rva.h>

#include <Mfc.h>
#include <MfcWin.h>

#include <Enums.h>
#include <Globals.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/CoordPool.h>
#include <Gruntz/EnemyAiType.h>
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
#include <Gruntz/GruntSpriteMacros.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/ScanGridMacros.h>
#include <Gruntz/StaminaPct.h>
#include <Gruntz/TileCollisionKind.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TriggerMgrRecords.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/VoiceManager.h>
#include <Ints.h>
#include <Wap32/TileGeometry.h>
#include <ZTools/ZDArray.h>

#include <limits.h>
#include <new>
#include <stdlib.h>
#include <string.h>

RVA(0x000ed9f0, 0x900)
i32 CGrunt::StepHitAndRunnerBehavior() {
    m_defenderPx = m_lastTilePx;

    i32 flag = 0;
    FIND_NEAREST_ENEMY_AT_TARGET_WITH_FLAG(g, flag, gx)

    b32 powered = m_poweredUp;
    if (powered != false) {
        b32 neighborValid = m_neighborValid;
        if (neighborValid == false) {
            if (m_combatActive == false) {
                if (m_stamina >= STAMINA_FULL) {
                    if (FindGridNeighbor(1) != NULL) {
                        m_defenderState = AISTATE_RETREAT;
                        return 1;
                    }
                    if (flag != 0 && g == NULL) {
                        goto retreat;
                    }
                    if (m_poweredUp == false || m_neighborValid != false) {
                        goto retreat;
                    }
                    RESET_GRUNT_POWERED_STATE(this)
                } else {
                    if (flag != 0) {
                        goto retreat;
                    }
                    if (m_poweredUp == false || m_neighborValid != false) {
                        goto retreat;
                    }
                    RESET_GRUNT_POWERED_STATE(this)
                }
            }
        } else {
            m_neighborValid = false;
        }
    retreat:
        m_defenderState = AISTATE_RETREAT;
    }

    switch (m_defenderState) {
        case AISTATE_SEEK: {
            Coord c;
            if (g != NULL && m_poweredUp == false && m_stamina >= STAMINA_FULL
                && GRUNT_AT_SAVED_SCREEN_POS(g)
                && RectContains(g->m_object->m_screenX, g->m_object->m_screenY) != 0) {
                COMMIT_GRUNT_NEIGHBOR(g);
                m_neighborScanEnabled = false;
                RecycleGruntCoords(this);
                m_defenderState = AISTATE_RETREAT;
                return 1;
            }
            if (g != NULL && static_cast<u32>(m_dwell) > DWELL_SEEK_PATH_MS) {
                if (GruntInRadius(g->m_playerIndex, g->m_unitIndex) != 0) {
                    g->GetScreenTile(&c);
                    if (TileSwitch(c.m_x, c.m_y, 0, m_arrivalFlags, 1, 0) != 0) {
                        SET_GRUNT_ARRIVAL_TARGET(g);
                        m_defenderState = AISTATE_CHASE;
                        PLAY_VOICE_IF_VISIBLE(0x366);
                    }
                }
                m_dwell = 0;
                return 1;
            }
            break;
        }

        case AISTATE_CHASE: {
            CGrunt* slot = m_triggerMgr->UnitAt(m_arrivalCell.m_x, m_arrivalCell.m_y);
            CGrunt* active = m_triggerMgr->FindNearestEnemy(this);
            if (active != NULL && active != slot) {
                ResetToSeek(this);
                return 1;
            }
            if (slot == NULL || slot->m_entranceCommitted == false
                || GruntInRadius(slot->m_playerIndex, slot->m_unitIndex) == 0) {
                m_defenderState = AISTATE_SEEK;
                return 1;
            }
            RepathToward(this, slot);
            if (m_poweredUp != false) {
                return 1;
            }
            if (m_stamina < STAMINA_FULL) {
                return 1;
            }
            if (RectContains(slot->m_object->m_screenX, slot->m_object->m_screenY) == 0) {
                return 1;
            }
            if (!(GRUNT_AT_SAVED_SCREEN_POS(slot))) {
                return 1;
            }
            COMMIT_GRUNT_NEIGHBOR(slot);
            m_neighborScanEnabled = false;
            RecycleGruntCoords(this);
            m_defenderState = AISTATE_RETREAT;
            return 1;
        }

        case AISTATE_ATTACK: {
            if (m_poweredUp == false) {
                m_defenderState = AISTATE_SEEK;
                return 1;
            }
            CGrunt* slot = m_triggerMgr->UnitAt(m_arrivalCell.m_x, m_arrivalCell.m_y);
            if (slot == NULL || GruntInRadius(slot->m_playerIndex, slot->m_unitIndex) == 0
                || slot->m_entranceCommitted == false) {
                goto ph1;
            }
            if (m_neighborValid != false) {
                return 1;
            }
            if (m_combatActive != false) {
                return 1;
            }
            if (m_stamina < STAMINA_FULL) {
                return 1;
            }
            if (RectContains(slot->m_object->m_screenX, slot->m_object->m_screenY) == 0) {
                goto ph1;
            }
            if (!(GRUNT_AT_SAVED_SCREEN_POS(slot))) {
                goto ph1;
            }
            COMMIT_GRUNT_NEIGHBOR(slot);
            m_neighborScanEnabled = false;
            if (CoordCount() != 0) {
                RECYCLE_GRUNT_COORDS(this)
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
            if (m_combatActive != false) {
                return 1;
            }
            if (m_stamina >= STAMINA_FULL) {
                m_defenderState = AISTATE_SEEK;
                return 1;
            }
            if (CoordCount() != 0) {
                return 1;
            }
            CWwdSpriteObject* base = m_object;
            i32 clip = 1;
            i32 baseTileY = base->m_screenY >> TILE_SHIFT_PX;
            i32 baseTileX = base->m_screenX >> TILE_SHIFT_PX;
            i32 py = rand() % 4 + baseTileY - 2;
            i32 px = rand() % 4 + baseTileX - 2;
            if (static_cast<u32>(m_arrivalCell.m_x) < 4
                && static_cast<u32>(m_arrivalCell.m_y) < 0xf) {
                CGrunt* entry =
                    g_gameReg->m_triggerMgr->UnitAt(m_arrivalCell.m_x, m_arrivalCell.m_y);
                if (entry != NULL) {
                    CGameObject* candidateObject = entry->m_object;
                    CRect rc(
                        (candidateObject->m_screenX >> TILE_SHIFT_PX) - 2,
                        (candidateObject->m_screenY >> TILE_SHIFT_PX) - 2,
                        (candidateObject->m_screenX >> TILE_SHIFT_PX) + 3,
                        (candidateObject->m_screenY >> TILE_SHIFT_PX) + 3
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

    if (m_resetApplied == false && m_hasExtent != false
        && static_cast<u32>(m_dwell) > DWELL_STUCK_RESET_MS) {
        if (IsArrivalRerollPending() != 0) {
            CWwdSpriteObject* base = m_object;
            SELECT_RANDOM_EXTENT_POINT_UNSIGNED_CAST(base, lx, ax, ly, ay)
            if (lx < g_gameReg->m_tileGrid->m_width && ly < g_gameReg->m_tileGrid->m_height) {
                TileSwitch(static_cast<i32>(lx), static_cast<i32>(ly), 0, m_arrivalFlags, 1, 0);
            }
            if (CoordCount() != 0) {
                ax = Max(ax, ay);
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
