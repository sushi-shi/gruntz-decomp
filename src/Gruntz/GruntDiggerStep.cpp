#include <rva.h>

#include <Mfc.h>
#include <MfcNoInline.h>
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

RVA(0x000f36a0, 0x78e)
i32 CGrunt::StepDiggerBehavior() {
    // Retail holds the strcmp result in a `bool` (`sete cl / test cl,cl`).
    bool isI = ANIMATION_ACT_EQUALS("I");
    if (isI) {
        return 1;
    }
    CMapMgr* grid = g_gameReg->m_tileGrid;
    GRID_CLIP_NULL(grid);

    Coord c1;
    GetScreenPos(&c1);
    c1.m_x >>= TILE_SHIFT_PX;
    Coord c2;
    GetScreenPos(&c2);
    c2.m_y >>= TILE_SHIFT_PX;

    FIND_NEAREST_ENEMY_AT_TARGET(g, atTarget, x)

    m_defenderPx = m_lastTilePx;

    i32 powered = m_poweredUp;
    if (powered != 0) {
        i32 neighborValid = m_neighborValid;
        if (neighborValid == 0) {
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
            if (atTarget) {
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
        m_neighborValid = 0;
        return 1;
    }

    if (g == NULL || GruntInRadius(g->m_playerIndex, g->m_unitIndex) == 0) {
        m_blockedVoicePending = 0;
        goto L_tailc;
    }
    if (m_poweredUp != 0) {
        goto L_tailc;
    }
    if (m_stamina >= STAMINA_FULL && g->m_object->m_screenX == g->m_lastTilePx.m_x
        && g->m_object->m_screenY == g->m_lastTilePx.m_y
        && RectContains(g->m_object->m_screenX, g->m_object->m_screenY) != 0) {
        COMMIT_GRUNT_NEIGHBOR(g);
        m_dwell = 0;
        return 1;
    }
    if (m_poweredUp != 0) {
        goto L_tailc;
    }
    if (static_cast<u32>(m_dwell) <= DWELL_REPATH_MS) {
        goto L_tailc;
    }
    if (TileSwitch(
            g->m_object->m_screenX >> TILE_SHIFT_PX,
            g->m_object->m_screenY >> TILE_SHIFT_PX,
            0,
            m_arrivalFlags,
            1,
            0
        )
        != 0) {
        if (m_blockedVoicePending != 0) {
            CCueRect* board = &g_gameReg->m_world->m_level->m_mainPlane->m_viewRect;
            i32 x = m_object->m_screenX;
            i32 y = m_object->m_screenY;
            if (CGameLevel::PointInRect(board, x, y)) {
                g_gameReg->m_cueSink->SpawnVoiceDriver(this, 0x366, -1, 0, -1, -1);
            }
            m_blockedVoicePending = 0;
        }
        m_dwell = 0;
    }

L_tailc:
    if (CoordCount() == 0) {
        if ((m_poweredUp == 0) & (static_cast<u32>(m_dwell) > DWELL_SEEK_PATH_MS)) {
            i32 r = m_defenderRadius;
            RECT box;
            box.left = c1.m_x - r;
            box.right = c1.m_x + r;
            box.top = c2.m_y - r;
            box.bottom = c2.m_y + r;
            RECT gb;
            gb.left = 0;
            gb.top = 0;
            gb.right = grid->m_width;
            gb.bottom = grid->m_height;
            RECT isect;
            if (!IntersectRect(&isect, &box, &gb)) {
                isect = box;
            }
            i32 best = INT_MAX;
            i32 bestCol = -1;
            i32 bestRow = -1;
            GRID_CLIP_INL(grid, &isect);
            for (i32 row = isect.top; row < isect.bottom; row++) {
                BrickzCell* cell = &grid->m_rows[row][isect.left];
                for (i32 col = isect.left; col < isect.right; col++) {
                    if ((cell->m_flags & 0x10000) != 0) {
                        i32 dr = row - c2.m_y;
                        dr = abs(dr);
                        i32 dc = col - c1.m_x;
                        dc = abs(dc);
                        i32 dist = dr + dc;
                        if (dist < best) {
                            best = dist;
                            bestCol = col;
                            bestRow = row;
                        }
                    }
                    cell++;
                }
            }
            if (best != INT_MAX) {
                i32 dc = bestCol - c1.m_x;
                dc = abs(dc);
                i32 dr = bestRow - c2.m_y;
                dr = abs(dr);
                if (dc <= 1 && dr <= 1) {
                    m_tileMgr->ApplyTriggerA(
                        m_playerIndex,
                        m_unitIndex,
                        (bestCol << TILE_SHIFT_PX) + TILE_HALF_PX,
                        (bestRow << TILE_SHIFT_PX) + TILE_HALF_PX
                    );
                    SetEntrancePos(1, 1);
                } else {
                    TileSwitch(bestCol, bestRow, 0, m_arrivalFlags, 1, 0);
                }
            }
            GRID_RECT_INLINE(grid);
            m_dwell = 0;
        }
        return 1;
    }
    {
        Coord* coord = static_cast<Coord*>(m_coordList.GetHead());
        i32 col = coord->m_x;
        i32 row = coord->m_y;
        BrickzCell* cell = &grid->m_rows[row][col];
        if ((cell->m_flags & 0x40) != 0 || (cell->m_flags & 0x10000) != 0) {
            m_tileMgr->ApplyTriggerA(
                m_playerIndex,
                m_unitIndex,
                (col << TILE_SHIFT_PX) + TILE_HALF_PX,
                (row << TILE_SHIFT_PX) + TILE_HALF_PX
            );
            SetEntrancePos(1, 1);
            m_dwell = 0;
        }
    }
    return 1;
}
