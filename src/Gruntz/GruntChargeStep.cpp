#include <rva.h>

#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/TriggerMgr.h>
#include <Ints.h>

#include <stdlib.h>

// @early-stop
RVA(0x000ef6b0, 0x61d)
i32 CGrunt::ChargeStep() {
    m_defenderPx.m_x = m_lastTilePx.m_x;
    m_defenderPx.m_y = m_lastTilePx.m_y;
    CGrunt* g = m_tileMgr->FindNearestEnemy(this);
    i32 hitGate = 0;
    if (g != 0) {
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
        if (m_stamina >= 100) {
            if (FindGridNeighbor(1) != 0) {
                return 1;
            }
            if (hitGate != 0 && g == 0) {
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
        case 0: {

            if (g != 0) {
                if (hitGate != 0 && m_stamina >= 100) {
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
                if (m_dwell > 500) {
                    if (GruntInRadius(g->m_tileOwnerHi, g->m_tileOwnerLo) == 0) {
                        return 1;
                    }
                    if (TileSwitch(
                            g->m_object->m_screenX >> 5,
                            g->m_object->m_screenY >> 5,
                            0,
                            m_arrivalFlags,
                            1,
                            0
                        )
                        != 0) {
                        SetEntrancePos(1, 1);
                        m_arrivalCell.m_x = g->m_tileOwnerHi;
                        m_arrivalCell.m_y = g->m_tileOwnerLo;
                        m_defenderState = 1;
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
            if (m_resetApplied == 0 && m_hasExtent != 0 && m_dwell > 3000) {
                CWwdGameObjectA* mp = m_object;
                i32 baseX = mp->m_extent.left;
                i32 spanX = mp->m_extent.right - baseX;
                spanX = spanX < 0 ? -spanX : spanX;
                i32 baseY = mp->m_extent.top;
                i32 spanY = mp->m_extent.bottom - baseY;
                spanY = spanY < 0 ? -spanY : spanY;
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
                    if (spanX < m_coordList.GetCount()) {
                        SetEntrancePos(1, 1);
                    }
                }
                m_dwell = 0;
            }
            break;
        }
        case 1: {

            CGrunt* t = m_tileMgr->m_grid[m_arrivalCell.m_y + m_arrivalCell.m_x * TM_GRID_COLS];
            CGrunt* cur = m_tileMgr->FindNearestEnemy(this);
            if (cur != 0 && cur != t) {
                m_arrivalCell.m_x = -1;
                m_defenderState = 0;
                m_arrivalCell.m_y = -1;
                return 1;
            }
            if (t == 0 || t->m_entranceCommitted == 0
                || GruntInRadius(t->m_tileOwnerHi, t->m_tileOwnerLo) == 0) {
                m_defenderState = 0;
                return 1;
            }
            if (static_cast<u32>(m_dwell) > 500) {
                StepArrivalDrop(t->m_lastTilePx.m_x, t->m_lastTilePx.m_y, 0, m_arrivalFlags, 1, 0);
                m_dwell = 0;
            }
            if (m_poweredUp == 0 && m_stamina >= 100
                && RectContains(t->m_object->m_screenX, t->m_object->m_screenY) != 0
                && t->m_object->m_screenX == t->m_lastTilePx.m_x
                && t->m_object->m_screenY == t->m_lastTilePx.m_y) {
                CommitNeighbor(
                    t->m_tileOwnerHi,
                    t->m_tileOwnerLo,
                    t->m_lastTilePx.m_x,
                    t->m_lastTilePx.m_y
                );
                m_defenderState = 2;
                return 1;
            }
            break;
        }
        case 2: {

            if (m_poweredUp != 0) {
                CGrunt* t = m_tileMgr->m_grid[m_arrivalCell.m_y + m_arrivalCell.m_x * TM_GRID_COLS];
                if (t == 0 || GruntInRadius(t->m_tileOwnerHi, t->m_tileOwnerLo) == 0
                    || t->m_entranceCommitted == 0) {
                    m_defenderState = 1;
                    m_dwell = 0x1f4;
                    return 1;
                }
                if (m_neighborValid != 0 || m_combatActive != 0 || m_stamina < 100) {
                    return 1;
                }
                if (RectContains(t->m_object->m_screenX, t->m_object->m_screenY) == 0
                    || t->m_object->m_screenX != t->m_lastTilePx.m_x
                    || t->m_object->m_screenY != t->m_lastTilePx.m_y) {
                    m_defenderState = 1;
                    m_dwell = 0x1f4;
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
            m_defenderState = 1;
            m_dwell = 0x1f4;
            return 1;
        }
    }
    return 1;
}
