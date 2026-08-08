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
// Reloc sequence is identical to retail's (29/29, in order) and the instruction
// count is +4; the residue is that cl spills `this` to [esp+0x44] where retail
// keeps it in ebp, so the frame is wider and every [esp+N] shifts.
RVA(0x000f36a0, 0x78e)
i32 CGrunt::StepDiggerBehavior() {
    if (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "I") == 0) {
        return 1;
    }
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

    m_defenderPx.m_x = m_lastTilePx.m_x;
    m_defenderPx.m_y = m_lastTilePx.m_y;

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

    if (g == NULL || GruntInRadius(g->m_tileOwnerHi, g->m_tileOwnerLo) == 0) {
        m_blockedVoicePending = 0;
        goto L_tailc;
    }
    if (m_poweredUp != 0) {
        goto L_tailc;
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
        if (m_poweredUp == 0 && static_cast<u32>(m_dwell) > DWELL_SEEK_PATH_MS) {
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
            GRID_CLIP_INL(grid, &isect);
            i32 best = INT_MAX;
            i32 bestCol = -1;
            i32 bestRow = -1;
            for (i32 row = isect.top; row < isect.bottom; row++) {
                BrickzCell* cell = &grid->m_rows[row][isect.left];
                for (i32 col = isect.left; col < isect.right; col++) {
                    if ((cell->m_flags & 0x10000) != 0) {
                        i32 dr = row - cy;
                        dr = abs(dr);
                        i32 dc = col - cx;
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
                i32 dc = bestCol - cx;
                dc = abs(dc);
                i32 dr = bestRow - cy;
                dr = abs(dr);
                if (dc <= 1 && dr <= 1) {
                    m_tileMgr->ApplyTriggerA(
                        m_tileOwnerHi,
                        m_tileOwnerLo,
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
                m_tileOwnerHi,
                m_tileOwnerLo,
                (col << TILE_SHIFT_PX) + TILE_HALF_PX,
                (row << TILE_SHIFT_PX) + TILE_HALF_PX
            );
            SetEntrancePos(1, 1);
            m_dwell = 0;
        }
    }
    return 1;
}
