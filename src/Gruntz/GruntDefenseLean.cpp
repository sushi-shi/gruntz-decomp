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
#include <Gruntz/GruntPuddle.h>
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
// The two `goto c2_occcheck` / `goto c2_miss` guard chains were the bulk of the
// residue - cl HOISTS a forward goto's target onto the block right after the last
// goto, so both landed inline where retail sinks them past the success return
// (docs/patterns/goto-continuation-label-is-not-a-shared-exit.md).  Positive gates
// took the arm from 63.44 to 74.71.  What is left is the fold family: retail keeps
// the SECOND `occ == NULL` test at the c2_occcheck entry (0xf8433) and the CHASE
// arm's leading one, which cl proves dead against the first null check, and it
// re-loads occ->m_lastTilePx in the CommitNeighbor blocks (22i/12i against our
// 16i/10i) where cl CSEs them with the compares above.
RVA(0x000f8240, 0x5b9)
i32 CGrunt::StepArrivalDefenseLean() {
    m_defenderPx.m_x = m_lastTilePx.m_x;
    m_defenderPx.m_y = m_lastTilePx.m_y;
    bool eqI = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "I") == 0);
    if (eqI) {
        return 1;
    }
    CGrunt* occ;
    switch (m_defenderState) {
        case AISTATE_ATTACK:
            if (m_poweredUp == 0) {
                m_defenderState = AISTATE_CHASE;
                return 1;
            }
            occ = m_tileMgr->m_grid[m_arrivalCell.m_x * TM_GRID_COLS + m_arrivalCell.m_y];
            if (occ == NULL) {
                goto seek;
            }
            if (GruntInRadius(occ->m_tileOwnerHi, occ->m_tileOwnerLo) != 0
                && occ->m_entranceCommitted != 0) {
                if (m_combatActive != 0) {
                    return 1;
                }
                if (m_stamina < STAMINA_FULL) {
                    return 1;
                }
                if (RectContains(occ->m_object->m_screenX, occ->m_object->m_screenY) != 0
                    && occ->m_object->m_screenX == occ->m_lastTilePx.m_x
                    && occ->m_object->m_screenY == occ->m_lastTilePx.m_y) {
                    CommitNeighbor(
                        occ->m_tileOwnerHi,
                        occ->m_tileOwnerLo,
                        occ->m_lastTilePx.m_x,
                        occ->m_lastTilePx.m_y
                    );
                    return 1;
                }
                {
                    CWwdGameObjectA* h = m_object;
                    i32 vx = h->m_screenX;
                    i32 vy = h->m_screenY;
                    const RECT* rect = &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect;
                    if (CGameLevel::PointInRect(rect, vx, vy)) {
                        g_gameReg->m_cueSink->SpawnVoiceDriver(this, 0x366, -1, 0, -1, -1);
                    }
                }
                m_defenderState = AISTATE_CHASE;
                m_dwell = DWELL_REPATH_MS;
                return 1;
            }
            if (occ == NULL) {
                goto seek;
            }
            m_defenderState = AISTATE_CHASE;
            m_dwell = DWELL_REPATH_MS;
            {
                CWwdGameObjectA* h = m_object;
                i32 vx = h->m_screenX;
                i32 vy = h->m_screenY;
                const RECT* rect = &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect;
                if (CGameLevel::PointInRect(rect, vx, vy)) {
                    g_gameReg->m_cueSink->SpawnVoiceDriver(this, 0x366, -1, 0, -1, -1);
                }
            }
            return 1;

        case AISTATE_CHASE: {
            occ = m_tileMgr->m_grid[m_arrivalCell.m_x * TM_GRID_COLS + m_arrivalCell.m_y];
            CGrunt* g = m_tileMgr->FindNearestEnemy(this);
            if (g != NULL && g != occ) {
                Coord none;
                m_arrivalCell = *none.Set(-1, -1);
                m_defenderState = AISTATE_SEEK;
                return 1;
            }
            if (occ == NULL) {
                goto seek;
            }
            if (occ->m_entranceCommitted == 0) {
                goto seek;
            }
            if (GruntInRadius(occ->m_tileOwnerHi, occ->m_tileOwnerLo) == 0) {
                goto seek;
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
            m_defenderState = AISTATE_ATTACK;
            return 1;
        }

        case AISTATE_SEEK:
            occ = m_tileMgr->FindNearestEnemy(this);
            if (rand() % 0x64 == 0 && m_health > 0x1a && occ != NULL && m_stamina >= STAMINA_FULL
                && GruntInRadius(occ->m_tileOwnerHi, occ->m_tileOwnerLo) != 0) {
                m_tileMgr->ApplyTriggerA(
                    m_tileOwnerHi,
                    m_tileOwnerLo,
                    m_lastTilePx.m_x,
                    m_lastTilePx.m_y
                );
                return 1;
            }
            if (m_resetApplied != 0) {
                return 1;
            }
            if (m_hasExtent == 0) {
                return 1;
            }
            if (static_cast<u32>(m_dwell) <= DWELL_STUCK_RESET_MS) {
                return 1;
            }
            if (static_cast<i64>(g_frameTime) - m_arrivalReroll64 >= m_arrivalRerollWindow64) {
                ResetEntranceAnimation(1, 1, 0);
                m_arrivalRerollWindowLo = rand() % 0x7530 + 0x7530;
                m_arrivalRerollWindowHi = 0;
                m_arrivalRerollLo = static_cast<i32>(g_frameTime);
                m_arrivalRerollHi = 0;
                m_dwell = 0;
                return 1;
            }
            {
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
                CMapMgr* bd = g_gameReg->m_tileGrid;
                if (static_cast<u32>(outX) < static_cast<u32>(bd->m_width)
                    && static_cast<u32>(outY) < static_cast<u32>(bd->m_height)) {
                    TileSwitch(outX, outY, 0, m_arrivalFlags, 1, 0);
                }
                i32 m328 = CoordCount();
                if (m328 != 0) {
                    i32 mx = spanX > spanY ? spanX : spanY;
                    if (m328 > mx) {
                        SetEntrancePos(1, 1);
                    }
                }
            }
            m_dwell = 0;
            return 1;

        default:
            return 1;
    }
seek:
    m_defenderState = AISTATE_SEEK;
    return 1;
}
