#include <Enums.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/GameRand.h>
#include <Mfc.h>
#include <MfcNoInline.h>
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
RVA(0x000f0db0, 0x48)

i32 CellTargetable(i32 tileX, i32 tileY) {
    CPtrList& list = g_gameReg->m_cmdGrid->m_baseList;
    POSITION pos = list.GetHeadPosition();

    if (pos != NULL) {
        do {
            CGruntPuddle* p = static_cast<CGruntPuddle*>(list.GetNext(pos));
            if (p->m_pending == 0) {
                i32 v54 = p->m_tileX;
                i32 v58 = p->m_tileY;
                if (v54 == tileX && v58 == tileY) {
                    return 1;
                }
            }
        } while (pos != NULL);
    }
    return 0;
}

// @early-stop
RVA(0x000f0e20, 0x928)
i32 CGrunt::StepGooSuckerBehavior() {
    bool eqI = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "I") == 0);
    if (eqI) {
        return 1;
    }
    m_defenderPx.m_x = m_lastTilePx.m_x;
    m_defenderPx.m_y = m_lastTilePx.m_y;
    CMapMgr* grid = g_gameReg->m_tileGrid;
    GRID_RECT_BOUNDS(grid);

    Coord c1[2];
    GetScreenPos(c1);
    i32 cx = c1[0].m_x >> TILE_SHIFT_PX;
    Coord c2[2];
    GetScreenPos(c2);
    i32 cy = c2[0].m_y >> TILE_SHIFT_PX;

    CGrunt* g = m_tileMgr->FindNearestEnemy(this);
    i32 atTarget = 0;
    if (g != NULL) {
        i32 x = g->m_object->m_screenX;
        if (x == g->m_lastTilePx.m_x && g->m_object->m_screenY == g->m_lastTilePx.m_y
            && RectContains(x, g->m_object->m_screenY) != 0) {
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
            if (atTarget && g == NULL) {
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

    if (g == NULL) {
        m_blockedVoicePending = 0;
        goto L_ed006b;
    }
    if (m_neighborValid != 0) {
        return 1;
    }
    if (m_combatActive == 0 && m_stamina >= STAMINA_FULL) {
        if (atTarget) {
            CommitNeighbor(
                g->m_tileOwnerHi,
                g->m_tileOwnerLo,
                g->m_lastTilePx.m_x,
                g->m_lastTilePx.m_y
            );
            DRAIN_COORDS();
            return 1;
        }
    } else {
        if (atTarget) {
            DRAIN_COORDS();
            return 1;
        }
    }

L_ed006b:
    if (g == NULL || GruntInRadius(g->m_tileOwnerHi, g->m_tileOwnerLo) == 0) {
        m_blockedVoicePending = 0;
        goto L_scanb;
    }
    if (m_poweredUp != 0) {
        goto L_scanb;
    }
    if (m_stamina >= STAMINA_FULL && g->m_object->m_screenX == g->m_lastTilePx.m_x
        && g->m_object->m_screenY == g->m_lastTilePx.m_y
        && RectContains(g->m_object->m_screenX, g->m_object->m_screenY) != 0) {
        CommitNeighbor(
            g->m_tileOwnerHi,
            g->m_tileOwnerLo,
            g->m_lastTilePx.m_x,
            g->m_lastTilePx.m_y
        );
    }
    if (m_poweredUp != 0) {
        goto L_scanb;
    }
    if (static_cast<u32>(m_dwell) <= DWELL_REPATH_MS) {
        goto L_scanb;
    }
    {
        Coord cc[2];
        g->GetScreenPos(cc);
        if (TileSwitch(
                cc[0].m_x >> TILE_SHIFT_PX,
                cc[0].m_y >> TILE_SHIFT_PX,
                0,
                m_arrivalFlags,
                1,
                0
            )
            != 0) {
            if (m_blockedVoicePending != 0) {
                i32 x = m_object->m_screenX;
                i32 y = m_object->m_screenY;
                if (CGameLevel::PointInBounds(
                        &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect,
                        x,
                        y
                    )
                    != 0) {
                    g_gameReg->m_cueSink->SpawnVoiceDriver(this, 0x366, -1, 0, -1, -1);
                }
                m_blockedVoicePending = 0;
            }
            m_dwell = 0;
        }
    }

L_scanb:
    if (CoordCount() != 0) {
        Coord* coord = static_cast<Coord*>(m_coordList.GetHead());
        i32 col = coord->m_x;
        i32 row = coord->m_y;
        if (CellTargetable(col, row) != 0) {
            m_tileMgr->ApplyTriggerA(
                m_tileOwnerHi,
                m_tileOwnerLo,
                (col << TILE_SHIFT_PX) + TILE_HALF_PX,
                (row << TILE_SHIFT_PX) + TILE_HALF_PX
            );
            SetEntrancePos(1, 1);
            m_dwell = 0;
        }
        return 1;
    }
    if (static_cast<u32>(m_dwell) <= DWELL_SEEK_PATH_MS) {
        return 1;
    }

    i32 r = m_defenderRadius;
    RECT box;
    box.left = cx - r;
    box.right = cx + r;
    box.top = cy - r;
    box.bottom = cy + r;
    RECT gb;
    gb.left = 0;
    gb.top = 0;
    gb.right = grid->m_width;
    gb.bottom = grid->m_height;
    RECT isect;
    if (!IntersectRect(&isect, &box, &gb)) {
        isect = box;
    }
    {
        RECT lb;
        lb.left = isect.left;
        lb.top = isect.top;
        lb.right = isect.right + 1;
        lb.bottom = isect.bottom + 1;
        RECT gb2;
        gb2.left = 0;
        gb2.top = 0;
        gb2.right = grid->m_width;
        gb2.bottom = grid->m_height;
        if (!IntersectRect(&grid->m_bounds, &lb, &gb2)) {
            grid->m_bounds = lb;
        }
        grid->m_gridW = grid->m_bounds.right - grid->m_bounds.left;
        grid->m_gridH = grid->m_bounds.bottom - grid->m_bounds.top;
    }

    i32 best = INT_MAX;
    i32 bestX = 0;
    i32 bestY = 0;

    POSITION pos = m_tileMgr->m_baseList.GetHeadPosition();
    while (pos != NULL) {
        CGruntPuddle* gg = static_cast<CGruntPuddle*>(m_tileMgr->m_baseList.GetNext(pos));
        if (gg->m_pending == 0) {
            i32 gx = gg->m_tileX;
            i32 gy = gg->m_tileY;
            if (RectContains(
                    (gx << TILE_SHIFT_PX) + TILE_HALF_PX,
                    (gy << TILE_SHIFT_PX) + TILE_HALF_PX
                )
                != 0) {
                m_tileMgr->ApplyTriggerA(
                    m_tileOwnerHi,
                    m_tileOwnerLo,
                    (gx << TILE_SHIFT_PX) + TILE_HALF_PX,
                    (gy << TILE_SHIFT_PX) + TILE_HALF_PX
                );
                GRID_RECT_BOUNDS(grid);
                return 1;
            }
            i32 dx = gx - (m_object->m_screenX >> TILE_SHIFT_PX);
            IABS(dx);
            i32 dy = gy - (m_object->m_screenY >> TILE_SHIFT_PX);
            i32 dist = ((dy ^ (dy >> 31)) - (dy >> 31)) + dx;
            if (dist < best) {
                POINT pt;
                pt.x = gx;
                pt.y = gy;
                if (PtInRect(&isect, pt)) {
                    best = dist;
                    bestX = gx;
                    bestY = gy;
                }
            }
        }
    }
    if (best != INT_MAX) {
        i32 dx = bestX - cx;
        IABS(dx);
        i32 dy = bestY - cy;
        IABS(dy);
        if (dx <= 1 && dy <= 1) {
            m_tileMgr->ApplyTriggerA(
                m_tileOwnerHi,
                m_tileOwnerLo,
                (bestX << TILE_SHIFT_PX) + TILE_HALF_PX,
                (bestY << TILE_SHIFT_PX) + TILE_HALF_PX
            );
            SetEntrancePos(1, 1);
        } else {
            TileSwitch(bestX, bestY, 0, m_arrivalFlags, 1, 0);
        }
    }
    GRID_RECT_INLINE(grid);
    m_dwell = 0;
    return 1;
}
