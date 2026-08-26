#include <rva.h>

#include <Mfc.h>
#include <MfcWin.h>

#include <Enums.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntMovementMacros.h>
#include <Gruntz/GruntPoweredStateMacros.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntRandomPointMacros.h>
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
RVA(0x000f42f0, 0x15c0)
i32 CGrunt::StepSmartChaserBehavior() {
    i32 playerIndex = m_playerIndex;
    COPY_LAST_TILE_TO_DEFENDER
    i32 cx = m_lastTilePx.m_x >> TILE_SHIFT_PX;
    i32 cy = m_lastTilePx.m_y >> TILE_SHIFT_PX;

    CGrunt* best = NULL;
    i32 bestDist = INT_MAX;
    for (i32 candidatePlayerIndex = 0; candidatePlayerIndex < TM_PLAYER_COUNT;
         candidatePlayerIndex++) {
        if (candidatePlayerIndex == playerIndex) {
            continue;
        }
        for (i32 candidateUnitIndex = 0; candidateUnitIndex < TM_UNITS_PER_PLAYER;
             candidateUnitIndex++) {
            CGrunt* cand =
                g_gameReg->m_triggerMgr
                    ->m_units[candidatePlayerIndex * TM_UNITS_PER_PLAYER + candidateUnitIndex];
            if (cand != NULL && cand->m_entranceCommitted != false
                && cand->m_gruntKind != GRUNT_GHOST) {
                i32 pa;
                PRIO(pa, m_entranceReason);
                i32 pb;
                PRIO(pb, cand->m_entranceReason);
                if (pa <= pb) {
                    i32 dx = (cand->m_object->m_screenX >> TILE_SHIFT_PX) - cx;
                    i32 dy = (cand->m_object->m_screenY >> TILE_SHIFT_PX) - cy;
                    i32 d = dx * dx + dy * dy;
                    if (d < bestDist) {
                        best = cand;
                        bestDist = d;
                    }
                }
            }
        }
    }

    RECT box;
    {
        i32 halfBox = m_defenderRadius + m_reachRect.right + 1;
        Coord pt1;
        GetScreenTile(&pt1);
        i32 by = pt1.m_y;
        Coord pt2;
        GetScreenTile(&pt2);
        i32 bx = pt2.m_x;
        Coord pt3;
        GetScreenTile(&pt3);
        i32 t3y = pt3.m_y;
        Coord pt4;
        GetScreenPos(&pt4);
        pt4.m_x >>= TILE_SHIFT_PX;
        i32 t4x = pt4.m_x;
        box.left = t4x - halfBox;
        box.top = t3y - halfBox;
        box.right = bx + halfBox + 1;
        box.bottom = by + halfBox + 1;
    }
    if (best != NULL) {
        Coord bp;
        best->GetScreenPos(&bp);
        POINT pt;
        pt.x = bp.m_x >> TILE_SHIFT_PX;
        pt.y = bp.m_y >> TILE_SHIFT_PX;
        if (!PtInRect(&box, pt)) {
            best = NULL;
        }
    }

    i32 atTarget = 0;
    if (best != NULL) {
        i32 x = best->m_object->m_screenX;
        if (GRUNT_X_AT_SAVED_POS(x, best) && best->GRUNT_SCREEN_Y_AT_SAVED_POS(m_object, best)
            && this->RectContains(x, best->m_object->m_screenY) != 0) {
            atTarget = 1;
        }
    }

    b32 powered = m_poweredUp;
    if (powered != false) {
        b32 neighborValid = m_neighborValid;
        if (neighborValid == false) {
            if (m_combatActive != false) {
                return 1;
            }
            if (m_stamina >= STAMINA_FULL) {
                if (FindGridNeighbor(1) != NULL) {
                    return 1;
                }
                if (atTarget && best == NULL) {
                    return 1;
                }
                if (m_poweredUp == false) {
                    return 1;
                }
                if (m_neighborValid != false) {
                    return 1;
                }
                RESET_GRUNT_POWERED_STATE(this)
                return 1;
            }
            if (atTarget) {
                return 1;
            }
            if (m_poweredUp == false) {
                return 1;
            }
            if (m_neighborValid != false) {
                return 1;
            }
            RESET_GRUNT_POWERED_STATE(this)
            return 1;
        }
        m_neighborValid = false;
        return 1;
    }

    switch (m_defenderState) {
        case AISTATE_SEEK: {

            if (best != NULL) {
                if (m_poweredUp == false && m_stamina >= STAMINA_FULL
                    && GRUNT_AT_SAVED_SCREEN_POS(best)) {
                    i32 pa;
                    PRIO(pa, m_entranceReason);
                    i32 pb;
                    PRIO(pb, best->m_entranceReason);
                    if (pa <= pb
                        && this->RectContains(best->m_object->m_screenX, best->m_object->m_screenY)
                               != 0) {
                        COMMIT_GRUNT_NEIGHBOR(best);
                        return 1;
                    }
                }
            }

            if (best != NULL) {
                i32 seekPa;
                PRIO(seekPa, m_entranceReason);
                i32 seekPb;
                PRIO(seekPb, best->m_entranceReason);
                if (seekPa <= seekPb && static_cast<u32>(m_dwell) > DWELL_SEEK_PATH_MS) {
                    COPY_LAST_TILE_TO_DEFENDER
                    i32 pathPa;
                    PRIO(pathPa, m_entranceReason);
                    i32 pathPb;
                    PRIO(pathPb, best->m_entranceReason);
                    if (pathPa <= pathPb
                        && this->GruntInRadius(best->m_playerIndex, best->m_unitIndex) != 0) {
                        Coord cc;
                        best->GetScreenPos(&cc);
                        if (this->TileSwitch(
                                cc.m_x >> TILE_SHIFT_PX,
                                cc.m_y >> TILE_SHIFT_PX,
                                0,
                                m_arrivalFlags,
                                1,
                                0
                            )
                            != 0) {
                            SET_GRUNT_ARRIVAL_TARGET(best);
                            m_defenderState = AISTATE_CHASE;
                            CGruntzMgr* reg = g_gameReg;
                            if (CGameLevel::PointInBounds(
                                    &reg->m_world->m_level->m_mainPlane->m_planeViewRect,
                                    m_object->m_screenX,
                                    m_object->m_screenY
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

            if (m_resetApplied != false || m_hasExtent == false
                || static_cast<u32>(m_dwell) <= DWELL_STUCK_RESET_MS) {
                return 1;
            }

            {
                if (IsArrivalRerollPending() != 0) {

                    CWwdSpriteObject* object = m_object;
                    SELECT_RANDOM_EXTENT_POINT(object, baseCol, spanX, baseRow, spanY)
                    CMapMgr* grid = g_gameReg->m_tileGrid;
                    if (static_cast<u32>(baseCol) < static_cast<u32>(grid->m_width)
                        && static_cast<u32>(baseRow) < static_cast<u32>(grid->m_height)) {
                        this->TileSwitch(baseCol, baseRow, 0, m_arrivalFlags, 1, 0);
                    }
                    i32 steps = CoordCount();
                    if (steps != 0) {
                        i32 maxSpan = spanX > spanY ? spanX : spanY;
                        if (steps > maxSpan) {
                            SetEntrancePos(1, 1);
                        }
                    }
                } else {
                    ResetArrivalReroll();
                }
            }
            m_dwell = 0;
            return 1;
        }
        case AISTATE_CHASE: {
            CGrunt* sg =
                m_triggerMgr->m_units[m_arrivalCell.m_x * TM_UNITS_PER_PLAYER + m_arrivalCell.m_y];
            if (best != NULL && best != sg) {
                Coord none;
                m_arrivalCell = *none.Set(-1, -1);
                m_defenderState = AISTATE_SEEK;
                return 1;
            }
            if (sg != NULL) {
                i32 pa;
                PRIO(pa, m_entranceReason);
                i32 pb;
                PRIO(pb, sg->m_entranceReason);
                if (pa <= pb && sg->m_entranceCommitted != false
                    && this->GruntInRadius(sg->m_playerIndex, sg->m_unitIndex) != 0) {
                    if (static_cast<u32>(m_dwell) > DWELL_REPATH_MS) {
                        StepArrivalDrop(
                            sg->m_lastTilePx.m_x,
                            sg->m_lastTilePx.m_y,
                            0,
                            m_arrivalFlags,
                            1,
                            0
                        );
                        m_dwell = 0;
                    }
                    if (m_poweredUp != false || m_stamina < STAMINA_FULL) {
                        return 1;
                    }
                    if (this->RectContains(sg->m_object->m_screenX, sg->m_object->m_screenY) == 0) {
                        return 1;
                    }
                    if (GRUNT_NOT_AT_SAVED_SCREEN_POS(sg)) {
                        return 1;
                    }
                    COMMIT_GRUNT_NEIGHBOR(sg);
                    m_defenderState = AISTATE_ATTACK;
                    return 1;
                }
            }
            m_defenderState = AISTATE_SEEK;
            return 1;
        }
        case AISTATE_ATTACK: {
            if (m_poweredUp == false) {
                m_defenderState = AISTATE_CHASE;
                m_dwell = DWELL_REPATH_MS;
                return 1;
            }
            {
                CGrunt* sg =
                    m_triggerMgr
                        ->m_units[m_arrivalCell.m_x * TM_UNITS_PER_PLAYER + m_arrivalCell.m_y];
                if (sg != NULL) {
                    i32 pa;
                    PRIO(pa, m_entranceReason);
                    i32 pb;
                    PRIO(pb, sg->m_entranceReason);
                    if (pa <= pb && this->GruntInRadius(sg->m_playerIndex, sg->m_unitIndex) != 0
                        && sg->m_entranceCommitted != false) {
                        if (m_neighborValid != false || m_combatActive != false
                            || m_stamina < STAMINA_FULL) {
                            return 1;
                        }
                        if (this->RectContains(sg->m_object->m_screenX, sg->m_object->m_screenY)
                                != 0
                            && GRUNT_AT_SAVED_SCREEN_POS(sg)) {
                            COMMIT_GRUNT_NEIGHBOR(sg);
                            return 1;
                        }
                    }
                }
                m_defenderState = AISTATE_CHASE;
                m_dwell = DWELL_REPATH_MS;
                return 1;
            }
        }
    }
    return 1;
}
