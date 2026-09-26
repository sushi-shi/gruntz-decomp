#include <StdAfx.h>

#include <rva.h>

#include <Enums.h>
#include <Globals.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/CoordPool.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntAiState.h>
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
#include <Gruntz/RandomExtentPoint.h>
#include <Gruntz/ScanGridMacros.h>
#include <Gruntz/StaminaPct.h>
#include <Gruntz/TileCollisionKind.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TriggerMgrRecords.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/VoiceManager.h>
#include <Ints.h>
#include <MakeRect.h>
#include <Wap32/TileGeometry.h>
#include <ZTools/ZDArray.h>

#include <limits.h>
#include <new>
#include <stdlib.h>
#include <string.h>

RVA(0x000f42f0, 0x15c0)
i32 CGrunt::StepSmartChaserBehavior() {
    i32 playerIndex = m_playerIndex;
    COPY_LAST_TILE_TO_DEFENDER
    i32 cx = m_lastTilePx.m_x >> TILE_SHIFT_PX;
    i32 cy = m_lastTilePx.m_y >> TILE_SHIFT_PX;

    CGrunt* best = NULL;
    i32 bestDist = INT_MAX;
    for (i32 candidatePlayerIndex = 0; candidatePlayerIndex < PLAYER_SLOT_COUNT;
         candidatePlayerIndex++) {
        if (candidatePlayerIndex == playerIndex) {
            continue;
        }
        for (i32 candidateUnitIndex = 0; candidateUnitIndex < TM_UNITS_PER_PLAYER;
             candidateUnitIndex++) {
            CGrunt* cand =
                g_gameReg->m_triggerMgr->UnitAt(candidatePlayerIndex, candidateUnitIndex);
            if (cand != NULL && cand->m_entranceCommitted != false
                && cand->m_gruntKind != GRUNT_GHOST) {
                i32 pa;
                PRIO(pa, m_entranceReason);
                i32 pb;
                PRIO(pb, cand->m_entranceReason);
                if (pa <= pb) {
                    i32 dx = cand->GetScreenTileX() - cx;
                    i32 dy = cand->GetScreenTileY() - cy;
                    i32 d = SquaredDistance(dx, dy);
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
        SET_RECT_COMPONENTS(box, t4x - halfBox, t3y - halfBox, bx + halfBox + 1, by + halfBox + 1);
    }
    if (best != NULL) {
        Coord bp;
        best->GetScreenPos(&bp);
        POINT pt;
        SET_POINT_COMPONENTS(pt, bp.m_x >> TILE_SHIFT_PX, bp.m_y >> TILE_SHIFT_PX);
        if (!PtInRect(&box, pt)) {
            best = NULL;
        }
    }

    i32 atTarget = 0;
    if (best != NULL) {
        i32 x = best->m_object->m_screenPosition.m_x;
        if (GRUNT_X_AT_SAVED_POS(x, best) && GRUNT_SCREEN_Y_AT_SAVED_POS(best->m_object, best)
            && this->RectContains(x, best->m_object->m_screenPosition.m_y) != 0) {
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
                    && IsGruntAtSavedScreenPos(best)) {
                    i32 pa;
                    PRIO(pa, m_entranceReason);
                    i32 pb;
                    PRIO(pb, best->m_entranceReason);
                    if (pa <= pb
                        && this->RectContains(
                               best->m_object->m_screenPosition.m_x,
                               best->m_object->m_screenPosition.m_y
                           ) != 0) {
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
                            PLAY_VOICE_IF_VISIBLE(0x366);
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
                        i32 maxSpan = Max(spanX, spanY);
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
            CGrunt* sg = m_triggerMgr->UnitAt(m_arrivalCell.m_x, m_arrivalCell.m_y);
            if (best != NULL && best != sg) {
                ResetToSeek(this);
                return 1;
            }
            if (sg != NULL) {
                i32 pa;
                PRIO(pa, m_entranceReason);
                i32 pb;
                PRIO(pb, sg->m_entranceReason);
                if (pa <= pb && sg->m_entranceCommitted != false
                    && this->GruntInRadius(sg->m_playerIndex, sg->m_unitIndex) != 0) {
                    RepathToward(this, sg);
                    if (m_poweredUp != false || m_stamina < STAMINA_FULL) {
                        return 1;
                    }
                    if (this->RectContains(
                            sg->m_object->m_screenPosition.m_x,
                            sg->m_object->m_screenPosition.m_y
                        )
                        == 0) {
                        return 1;
                    }
                    if (!IsGruntAtSavedScreenPos(sg)) {
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
                CGrunt* sg = m_triggerMgr->UnitAt(m_arrivalCell.m_x, m_arrivalCell.m_y);
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
                        if (this->RectContains(
                                sg->m_object->m_screenPosition.m_x,
                                sg->m_object->m_screenPosition.m_y
                            ) != 0
                            && IsGruntAtSavedScreenPos(sg)) {
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
