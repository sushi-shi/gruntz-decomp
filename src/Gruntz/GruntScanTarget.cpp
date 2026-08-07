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
#include <Gruntz/ScanGridMacros.h>

#pragma intrinsic(strcmp)

#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/TriggerMgrRecords.h>
#include <Wap32/TileGeometry.h>
#include <Gruntz/StaminaPct.h>
#include <limits.h>

// @early-stop

// @early-stop
RVA(0x000f42f0, 0x15c0)
i32 CGrunt::ScanNearestTarget() {
    i32 ownerHi = m_tileOwnerHi;
    m_defenderPx.m_x = m_lastTilePx.m_x;
    m_defenderPx.m_y = m_lastTilePx.m_y;
    i32 cx = m_lastTilePx.m_x >> TILE_SHIFT_PX;
    i32 cy = m_lastTilePx.m_y >> TILE_SHIFT_PX;

    CGrunt* best = 0;
    i32 bestDist = INT_MAX;
    for (i32 row = 0; row < 4; row++) {
        if (row == ownerHi) {
            continue;
        }
        CTriggerMgr* board = g_gameReg->m_cmdGrid;
        for (i32 col = 0; col < 15; col++) {
            CGrunt* cand = board->m_grid[row * TM_GRID_COLS + col];
            if (cand != NULL && cand->m_entranceCommitted != 0
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

    i32 halfBox = m_defenderRadius + m_reachRect.right + 1;
    Coord pt;
    GetScreenPos(&pt);
    i32 by = pt.m_y >> TILE_SHIFT_PX;
    GetScreenPos(&pt);
    i32 bx = pt.m_x >> TILE_SHIFT_PX;
    GetScreenPos(&pt);
    i32 t3y = pt.m_y >> TILE_SHIFT_PX;
    GetScreenPos(&pt);
    i32 t4x = pt.m_x >> TILE_SHIFT_PX;
    RECT box;
    box.left = t4x - halfBox;
    box.top = t3y - halfBox;
    box.right = bx + halfBox + 1;
    box.bottom = by + halfBox + 1;
    if (best != NULL) {
        POINT pt;
        pt.x = best->m_lastTilePx.m_x >> TILE_SHIFT_PX;
        pt.y = best->m_lastTilePx.m_y >> TILE_SHIFT_PX;
        if (!PtInRect(&box, pt)) {
            best = NULL;
        }
    }

    i32 atTarget = 0;
    if (best != NULL) {
        i32 x = best->m_object->m_screenX;
        if (x == best->m_lastTilePx.m_x && best->m_object->m_screenY == best->m_lastTilePx.m_y
            && this->RectContains(x, best->m_object->m_screenY) != 0) {
            atTarget = 1;
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
            if (atTarget && best == NULL) {
                return 1;
            }
            if (m_poweredUp == 0) {
                return 1;
            }
        } else {
            if (atTarget) {
                return 1;
            }
            if (m_poweredUp == 0) {
                return 1;
            }
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

            if (best == NULL) {
                goto L_wander;
            }
            if (m_poweredUp == 0 && m_stamina >= STAMINA_FULL
                && best->m_object->m_screenX == best->m_lastTilePx.m_x
                && best->m_object->m_screenY == best->m_lastTilePx.m_y) {
                i32 pa;
                PRIO(pa, m_entranceReason);
                i32 pb;
                PRIO(pb, best->m_entranceReason);
                if (pa <= pb
                    && this->RectContains(best->m_object->m_screenX, best->m_object->m_screenY)
                           != 0) {
                    CommitNeighbor(
                        best->m_tileOwnerHi,
                        best->m_tileOwnerLo,
                        best->m_lastTilePx.m_x,
                        best->m_lastTilePx.m_y
                    );
                    return 1;
                }
            }

            if (best == NULL) {
                goto L_wander;
            }
            {
                i32 pa;
                PRIO(pa, m_entranceReason);
                i32 pb;
                PRIO(pb, best->m_entranceReason);
                if (pa > pb) {
                    goto L_wander;
                }
            }
            if (static_cast<u32>(m_dwell) <= DWELL_SEEK_PATH_MS) {
                goto L_wander;
            }
            m_defenderPx.m_x = m_lastTilePx.m_x;
            m_defenderPx.m_y = m_lastTilePx.m_y;
            {
                i32 pa;
                PRIO(pa, m_entranceReason);
                i32 pb;
                PRIO(pb, best->m_entranceReason);
                if (pa > pb) {
                    goto L_scanDone;
                }
            }
            if (this->GruntInRadius(best->m_tileOwnerHi, best->m_tileOwnerLo) == 0) {
                goto L_scanDone;
            }
            {
                Coord cc[2];
                best->GetScreenPos(cc);
                if (this->TileSwitch(
                        cc[0].m_x >> TILE_SHIFT_PX,
                        cc[0].m_y >> TILE_SHIFT_PX,
                        0,
                        m_arrivalFlags,
                        1,
                        0
                    )
                    == 0) {
                    goto L_scanDone;
                }
            }
            SetEntrancePos(1, 1);
            m_arrivalCell.m_x = best->m_tileOwnerHi;
            m_arrivalCell.m_y = best->m_tileOwnerLo;
            m_defenderState = AISTATE_CHASE;
            {
                if (CGameLevel::PointInBounds(
                        &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect,
                        m_object->m_screenX,
                        m_object->m_screenY
                    )
                    != 0) {
                    g_gameReg->m_cueSink->SpawnVoiceDriver(this, 0x366, -1, 0, -1, -1);
                }
            }
        L_scanDone:
            m_dwell = 0;
            return 1;

        L_wander:
            if (m_resetApplied != 0 || m_hasExtent == 0
                || static_cast<u32>(m_dwell) <= DWELL_STUCK_RESET_MS) {
                return 1;
            }

            {
                i32 lo = static_cast<i32>(g_frameTime) - m_arrivalRerollLo;
                i32 hi = 0 - m_arrivalRerollHi
                         - (g_frameTime < static_cast<u32>(m_arrivalRerollLo) ? 1 : 0);
                i32 winHi = m_arrivalRerollWindowHi;
                if (hi > winHi
                    || (hi == winHi
                        && static_cast<u32>(lo) >= static_cast<u32>(m_arrivalRerollWindowLo))) {

                    ResetEntranceAnimation(1, 1, 0);
                    m_arrivalRerollLo = 0;
                    m_arrivalRerollWindowLo = 0;
                    m_arrivalRerollHi = 0;
                    m_arrivalRerollWindowHi = 0;
                    m_arrivalRerollWindowLo = rand() % 0x7530 + 0x7530;
                    m_arrivalRerollWindowHi = 0;
                    m_arrivalRerollLo = static_cast<i32>(g_frameTime);
                    m_arrivalRerollHi = 0;
                } else {

                    CWwdGameObjectA* hud = m_object;
                    i32 baseCol = hud->m_extent.left;
                    i32 spanX = hud->m_extent.right - baseCol;
                    i32 baseRow = hud->m_extent.top;
                    spanX = (spanX ^ (spanX >> 31)) - (spanX >> 31);
                    i32 spanY = hud->m_extent.bottom - baseRow;
                    spanY = (spanY ^ (spanY >> 31)) - (spanY >> 31);
                    if (spanX != 0) {
                        baseCol += rand() % spanX;
                    }
                    if (spanY != 0) {
                        baseRow += rand() % spanY;
                    }
                    CMapMgr* grid = g_gameReg->m_tileGrid;
                    if (static_cast<u32>(baseCol) < static_cast<u32>(grid->m_width)
                        && static_cast<u32>(baseRow) < static_cast<u32>(grid->m_height)) {
                        this->TileSwitch(baseCol, baseRow, 0, m_arrivalFlags, 1, 0);
                    }
                    if (CoordCount() != 0) {
                        if (spanX > spanY) {
                            spanX = spanY;
                        }
                        if (CoordCount() > spanX) {
                            SetEntrancePos(1, 1);
                        }
                    }
                }
            }
            m_dwell = 0;
            return 1;
        }
        case AISTATE_CHASE: {
            CGrunt* sg = m_tileMgr->m_grid[m_arrivalCell.m_x * TM_GRID_COLS + m_arrivalCell.m_y];
            if (best != NULL && best != sg) {
                m_arrivalCell.m_x = -1;
                m_defenderState = AISTATE_SEEK;
                m_arrivalCell.m_y = -1;
                return 1;
            }
            if (sg == NULL) {
                goto L_clearMode;
            }
            i32 pa;
            PRIO(pa, m_entranceReason);
            i32 pb;
            PRIO(pb, sg->m_entranceReason);
            if (pa > pb) {
                goto L_clearMode;
            }
            if (sg->m_entranceCommitted == 0) {
                goto L_clearMode;
            }
            if (this->GruntInRadius(sg->m_tileOwnerHi, sg->m_tileOwnerLo) == 0) {
                goto L_clearMode;
            }
            if (static_cast<u32>(m_dwell) > DWELL_REPATH_MS) {
                StepArrivalDrop(
                    sg->m_lastTilePx.m_x,
                    sg->m_lastTilePx.m_y,
                    m_arrivalFlags,
                    0,
                    1,
                    0
                );
                m_dwell = 0;
            }
            if (m_poweredUp != 0 || m_stamina < STAMINA_FULL) {
                return 1;
            }
            if (this->RectContains(sg->m_object->m_screenX, sg->m_object->m_screenY) == 0) {
                return 1;
            }
            if (sg->m_object->m_screenX != sg->m_lastTilePx.m_x
                || sg->m_object->m_screenY != sg->m_lastTilePx.m_y) {
                return 1;
            }
            CommitNeighbor(
                sg->m_tileOwnerHi,
                sg->m_tileOwnerLo,
                sg->m_lastTilePx.m_x,
                sg->m_lastTilePx.m_y
            );
            m_defenderState = AISTATE_ATTACK;
            return 1;
        L_clearMode:
            m_defenderState = AISTATE_SEEK;
            return 1;
        }
        case AISTATE_ATTACK: {
            if (m_poweredUp != 0) {
                CGrunt* sg =
                    m_tileMgr->m_grid[m_arrivalCell.m_x * TM_GRID_COLS + m_arrivalCell.m_y];
                if (sg == NULL) {
                    goto L_setLock;
                }
                i32 pa;
                PRIO(pa, m_entranceReason);
                i32 pb;
                PRIO(pb, sg->m_entranceReason);
                if (pa > pb) {
                    goto L_setLock;
                }
                if (this->GruntInRadius(sg->m_tileOwnerHi, sg->m_tileOwnerLo) == 0) {
                    goto L_setLock;
                }
                if (sg->m_entranceCommitted == 0) {
                    goto L_setLock;
                }
                if (m_neighborValid != 0 || m_combatActive != 0 || m_stamina < STAMINA_FULL) {
                    return 1;
                }
                if (this->RectContains(sg->m_object->m_screenX, sg->m_object->m_screenY) == 0) {
                    goto L_setLock;
                }
                if (sg->m_object->m_screenX != sg->m_lastTilePx.m_x
                    || sg->m_object->m_screenY != sg->m_lastTilePx.m_y) {
                    goto L_setLock;
                }
                CommitNeighbor(
                    sg->m_tileOwnerHi,
                    sg->m_tileOwnerLo,
                    sg->m_lastTilePx.m_x,
                    sg->m_lastTilePx.m_y
                );
                m_defenderState = AISTATE_ATTACK;
                return 1;
            L_setLock:
                m_defenderState = AISTATE_CHASE;
                m_dwell = DWELL_REPATH_MS;
                return 1;
            }
            m_defenderState = AISTATE_CHASE;
            m_dwell = DWELL_REPATH_MS;
            return 1;
        }
    }
    return 1;
}
