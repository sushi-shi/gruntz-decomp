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
#include <Gruntz/RandomExtentPoint.h>
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
#include <Wap32/ZVec.h>

#include <limits.h>
#include <new>
#include <stdlib.h>
#include <string.h>

// @early-stop
RVA(0x000ed9f0, 0x900)
i32 CGrunt::StepHitAndRunnerBehavior() {
    m_defenderPx = m_lastTilePx;

    i32 flag = 0;
    FIND_NEAREST_ENEMY_AT_TARGET_WITH_FLAG(g, flag)

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
                } else {
                    if (flag != 0) {
                        goto retreat;
                    }
                    if (m_poweredUp == false || m_neighborValid != false) {
                        goto retreat;
                    }
                }
                RESET_GRUNT_POWERED_STATE(this)
            }
        } else {
            m_neighborValid = false;
        }
    retreat:
        m_defenderState = AISTATE_RETREAT;
    }

    switch (m_defenderState) {
        case AISTATE_SEEK:
            if (g != NULL) {
                if (m_poweredUp == false && m_stamina >= STAMINA_FULL
                    && GRUNT_AT_SAVED_SCREEN_POS(g)
                    && RectContains(
                           g->m_object->m_screenPosition.m_x,
                           g->m_object->m_screenPosition.m_y
                       ) != 0) {
                    COMMIT_GRUNT_NEIGHBOR(g);
                    m_neighborScanEnabled = false;
                    RecycleGruntCoords(this);
                    m_defenderState = AISTATE_RETREAT;
                    return 1;
                }
                if (static_cast<u32>(m_dwell) > DWELL_SEEK_PATH_MS) {
                    if (GruntInRadius(g->m_playerIndex, g->m_unitIndex) != 0) {
                        Coord targetTile;
                        g->GetScreenTile(&targetTile);
                        if (TileSwitch(targetTile.m_x, targetTile.m_y, 0, m_arrivalFlags, 1, 0)
                            != 0) {
                            SET_GRUNT_ARRIVAL_TARGET(g);
                            m_defenderState = AISTATE_CHASE;
                            CGruntzMgr* reg = g_gameReg;
                            if (CGameLevel::PointInBounds(
                                    &reg->m_world->m_level->m_mainPlane->m_planeViewRect,
                                    m_object->m_screenPosition.m_x,
                                    m_object->m_screenPosition.m_y
                                )
                                != 0) {
                                reg->m_voiceManager->PlayVoice(this, 0x366, -1, 0, -1, -1);
                            }
                        }
                    }
                    m_dwell = 0;
                    return 1;
                }
            }
            goto timeout;

        case AISTATE_CHASE: {
            CGrunt* slot =
                m_triggerMgr->m_units[m_arrivalCell.m_x * TM_UNITS_PER_PLAYER + m_arrivalCell.m_y];
            CGrunt* active = m_triggerMgr->FindNearestEnemy(this);
            if (active != NULL && active != slot) {
                m_arrivalCell.Set(-1, -1);
                m_defenderState = AISTATE_SEEK;
                return 1;
            }
            if (slot == NULL || slot->m_entranceCommitted == false
                || GruntInRadius(slot->m_playerIndex, slot->m_unitIndex) == 0) {
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
            if (m_poweredUp != false) {
                return 1;
            }
            if (m_stamina < STAMINA_FULL) {
                return 1;
            }
            if (RectContains(
                    slot->m_object->m_screenPosition.m_x,
                    slot->m_object->m_screenPosition.m_y
                )
                == 0) {
                return 1;
            }
            if (slot->m_object->ScreenPos() != slot->m_lastTilePx) {
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
            CGrunt* slot =
                m_triggerMgr->m_units[m_arrivalCell.m_x * TM_UNITS_PER_PLAYER + m_arrivalCell.m_y];
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
            if (RectContains(
                    slot->m_object->m_screenPosition.m_x,
                    slot->m_object->m_screenPosition.m_y
                )
                == 0) {
                goto ph1;
            }
            if (slot->m_object->ScreenPos() != slot->m_lastTilePx) {
                goto ph1;
            }
            COMMIT_GRUNT_NEIGHBOR(slot);
            m_neighborScanEnabled = false;
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
            i32 clip = 1;
            Coord baseTile;
            GetScreenTile(&baseTile);
            Coord destination(rand() % 4 + baseTile.m_x - 2, rand() % 4 + baseTile.m_y - 2);
            if (static_cast<u32>(m_arrivalCell.m_x) < 4
                && static_cast<u32>(m_arrivalCell.m_y) < 0xf) {
                CGrunt* entry =
                    g_gameReg->m_triggerMgr
                        ->m_units[m_arrivalCell.m_x * TM_UNITS_PER_PLAYER + m_arrivalCell.m_y];
                if (entry != NULL) {
                    Coord entryTile;
                    entry->GetScreenTile(&entryTile);
                    CRect rc(
                        entryTile.m_x - 2,
                        entryTile.m_y - 2,
                        entryTile.m_x + 3,
                        entryTile.m_y + 3
                    );
                    if (::PtInRect(&rc, destination.m_x, destination.m_y)) {
                        clip = 0;
                    }
                }
            }
            if (clip == 0) {
                return 1;
            }
            CMapMgr* grid = g_gameReg->m_tileGrid;
            if (static_cast<u32>(destination.m_x) >= static_cast<u32>(grid->m_width)) {
                return 1;
            }
            if (static_cast<u32>(destination.m_y) >= static_cast<u32>(grid->m_height)) {
                return 1;
            }
            TileSwitch(destination.m_x, destination.m_y, 0, m_arrivalFlags, 1, 0);
            return 1;
        }

        default:
            return 1;
    }

timeout:
    if (m_resetApplied == false && m_hasExtent != false
        && static_cast<u32>(m_dwell) > DWELL_STUCK_RESET_MS) {
        if (IsArrivalRerollPending() != 0) {
            CWwdSpriteObject* base = m_object;
            Coord point;
            Coord span;
            SelectRandomExtentPoint(base, &point, &span);
            if (static_cast<u32>(point.m_x) < g_gameReg->m_tileGrid->m_width
                && static_cast<u32>(point.m_y) < g_gameReg->m_tileGrid->m_height) {
                TileSwitch(point.m_x, point.m_y, 0, m_arrivalFlags, 1, 0);
            }
            if (CoordCount() != 0) {
                if (CoordCount() > Max(span.m_x, span.m_y)) {
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
