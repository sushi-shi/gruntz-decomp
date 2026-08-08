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

// @early-stop
// Reloc sequence is 35 vs retail's 34: the AISTATE_ATTACK arm's CommitNeighbor is
// cross-jumped into the AISTATE_SEEK arm's identical tail in retail and kept
// separate here. Everything else matches in order.
RVA(0x000f2b20, 0x6e1)
i32 CGrunt::StepArrivalDefense() {
    m_defenderPx.m_x = m_lastTilePx.m_x;
    m_defenderPx.m_y = m_lastTilePx.m_y;
    CGrunt* occ;
    switch (m_defenderState) {
        case AISTATE_ATTACK:
            if (m_poweredUp == 0) {
                m_defenderState = AISTATE_CHASE;
                return 1;
            }
            occ = m_tileMgr->m_grid[m_arrivalCell.m_x * TM_GRID_COLS + m_arrivalCell.m_y];
            if (occ == NULL) {
                m_defenderState = AISTATE_SEEK;
                return 1;
            }
            // the CHASE-and-shout arm is the FALL-THROUGH: forward gotos would
            // hoist it above the scroll arm, which retail emits first.
            if (GruntInRadius(occ->m_tileOwnerHi, occ->m_tileOwnerLo) != 0
                && occ->m_entranceCommitted != 0) {
                if (m_neighborValid != 0) {
                    return 1;
                }
                if (m_combatActive != 0) {
                    return 1;
                }
                if (m_stamina < STAMINA_FULL) {
                    return 1;
                }
                if (RectContains(occ->m_object->m_screenX, occ->m_object->m_screenY) != 0
                    && occ->m_object->m_screenX == occ->m_lastTilePx.m_x
                    && occ->m_object->m_screenY == occ->m_lastTilePx.m_y) {
                    if (m_vehiclePickupType == PICKUP_SCROLL) {
                        g_gameReg->m_cmdGrid->ApplyTriggerB(
                            m_tileOwnerHi,
                            m_tileOwnerLo,
                            occ->m_object->m_screenX,
                            occ->m_object->m_screenY
                        );
                        return 1;
                    }
                    CommitNeighbor(
                        occ->m_tileOwnerHi,
                        occ->m_tileOwnerLo,
                        occ->m_lastTilePx.m_x,
                        occ->m_lastTilePx.m_y
                    );
                    return 1;
                }
            } else if (occ == NULL) {
                m_defenderState = AISTATE_SEEK;
                return 1;
            }
            m_defenderState = AISTATE_CHASE;
            {
                CWwdGameObjectA* h = m_object;
                i32 vx = h->m_screenX;
                i32 vy = h->m_screenY;
                const RECT* rect = &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect;
                if (CGameLevel::PointInBounds(rect, vx, vy)) {
                    g_gameReg->m_cueSink->SpawnVoiceDriver(this, 0x366, -1, 0, -1, -1);
                }
            }
            return 1;

        case AISTATE_CHASE: {
            occ = m_tileMgr->m_grid[m_arrivalCell.m_x * TM_GRID_COLS + m_arrivalCell.m_y];
            CGrunt* g = m_tileMgr->FindNearestEnemy(this);
            if (g != NULL && g != occ) {
                m_arrivalCell.m_x = -1;
                m_defenderState = AISTATE_SEEK;
                m_arrivalCell.m_y = -1;
                return 1;
            }
            if (occ == NULL) {
                m_defenderState = AISTATE_SEEK;
                return 1;
            }
            if (occ->m_entranceCommitted == 0) {
                m_defenderState = AISTATE_SEEK;
                return 1;
            }
            if (GruntInRadius(occ->m_tileOwnerHi, occ->m_tileOwnerLo) == 0) {
                m_defenderState = AISTATE_SEEK;
                return 1;
            }
            if (static_cast<u32>(m_dwell) > DWELL_REPATH_MS) {
                StepArrivalDrop(
                    occ->m_lastTilePx.m_x,
                    occ->m_lastTilePx.m_y,
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
            if (RectContains(occ->m_object->m_screenX, occ->m_object->m_screenY) == 0) {
                return 1;
            }
            if (m_vehiclePickupType == PICKUP_SCROLL) {
                g_gameReg->m_cmdGrid->ApplyTriggerB(
                    m_tileOwnerHi,
                    m_tileOwnerLo,
                    occ->m_object->m_screenX,
                    occ->m_object->m_screenY
                );
                m_defenderState = AISTATE_ATTACK;
                return 1;
            }
            if (occ->m_object->m_screenX == occ->m_lastTilePx.m_x
                && occ->m_object->m_screenY == occ->m_lastTilePx.m_y) {
                CommitNeighbor(
                    occ->m_tileOwnerHi,
                    occ->m_tileOwnerLo,
                    occ->m_lastTilePx.m_x,
                    occ->m_lastTilePx.m_y
                );
            }
            m_defenderState = AISTATE_ATTACK;
            return 1;
        }

        case AISTATE_SEEK:
            occ = m_tileMgr->FindNearestEnemy(this);
            if (occ == NULL) {
                goto L_f308a;
            }
            if (m_poweredUp == 0 && m_stamina >= STAMINA_FULL
                && occ->m_object->m_screenX == occ->m_lastTilePx.m_x
                && occ->m_object->m_screenY == occ->m_lastTilePx.m_y
                && RectContains(occ->m_object->m_screenX, occ->m_object->m_screenY) != 0) {
                if (m_vehiclePickupType == PICKUP_SCROLL) {
                    g_gameReg->m_cmdGrid->ApplyTriggerB(
                        m_tileOwnerHi,
                        m_tileOwnerLo,
                        occ->m_object->m_screenX,
                        occ->m_object->m_screenY
                    );
                    return 1;
                }
                if (occ->m_object->m_screenX != occ->m_lastTilePx.m_x) {
                    return 1;
                }
                if (occ->m_object->m_screenY != occ->m_lastTilePx.m_y) {
                    return 1;
                }
                CommitNeighbor(
                    occ->m_tileOwnerHi,
                    occ->m_tileOwnerLo,
                    occ->m_lastTilePx.m_x,
                    occ->m_lastTilePx.m_y
                );
                return 1;
            }
            if (occ != NULL && static_cast<u32>(m_dwell) > DWELL_SEEK_PATH_MS) {
                if (GruntInRadius(occ->m_tileOwnerHi, occ->m_tileOwnerLo) == 0) {
                    goto L_f318a;
                }
                {
                    Coord sp;
                    occ->GetScreenPos(&sp);
                    if (TileSwitch(
                            sp.m_x >> TILE_SHIFT_PX,
                            sp.m_y >> TILE_SHIFT_PX,
                            0,
                            m_arrivalFlags,
                            1,
                            0
                        )
                        == 0) {
                        goto L_f318a;
                    }
                    SetEntrancePos(1, 1);
                    m_arrivalCell.m_x = occ->m_tileOwnerHi;
                    m_arrivalCell.m_y = occ->m_tileOwnerLo;
                    m_defenderState = AISTATE_CHASE;
                    CWwdGameObjectA* h = m_object;
                    CGruntzMgr* reg = g_gameReg;
                    const RECT* rect = &reg->m_world->m_level->m_mainPlane->m_viewRect;
                    if (CGameLevel::PointInBounds(rect, h->m_screenX, h->m_screenY) == 0) {
                        goto L_f318a;
                    }
                    reg->m_cueSink->SpawnVoiceDriver(this, 0x366, -1, 0, -1, -1);
                }
            L_f318a:
                m_dwell = 0;
                return 1;
            }
        L_f308a:
            if (m_resetApplied != 0) {
                return 1;
            }
            if (m_hasExtent == 0) {
                return 1;
            }
            if (static_cast<u32>(m_dwell) <= DWELL_STUCK_RESET_MS) {
                return 1;
            }
            // retail lays the reroll arm LAST: the window-still-open arm is the
            // fall-through of the negated test.
            if (static_cast<i64>(g_frameTime) - m_arrivalReroll64 < m_arrivalRerollWindow64) {
                CWwdGameObjectA* h = m_object;
                i32 baseX = h->m_extent.left;
                i32 spanX = abs(h->m_extent.right - baseX);
                i32 baseY = h->m_extent.top;
                i32 spanY = abs(h->m_extent.bottom - baseY);
                i32 outX = baseX;
                if (spanX != 0) {
                    outX += rand() % spanX;
                }
                i32 outY = baseY;
                if (spanY != 0) {
                    outY += rand() % spanY;
                }
                if (outX < g_gameReg->m_tileGrid->m_width
                    && outY < g_gameReg->m_tileGrid->m_height) {
                    TileSwitch(outX, outY, 0, m_arrivalFlags, 1, 0);
                }
                i32 m328 = CoordCount();
                if (m328 != 0) {
                    i32 mx = spanX > spanY ? spanX : spanY;
                    if (m328 > mx) {
                        SetEntrancePos(1, 1);
                    }
                }
                m_dwell = 0;
                return 1;
            }
            ResetEntranceAnimation(1, 1, 0);
            m_arrivalRerollLo = 0;
            m_arrivalRerollWindowLo = 0;
            m_arrivalRerollHi = 0;
            m_arrivalRerollWindowHi = 0;
            m_arrivalRerollWindowLo = rand() % 0x7530 + 0x7530;
            m_arrivalRerollWindowHi = 0;
            m_arrivalRerollLo = static_cast<i32>(g_frameTime);
            m_arrivalRerollHi = 0;
            m_dwell = 0;
            return 1;

        default:
            return 1;
    }
}
