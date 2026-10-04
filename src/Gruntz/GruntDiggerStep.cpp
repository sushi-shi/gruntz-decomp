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
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntSpriteMacros.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/MapCellFlags.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/ScanGridMacros.h>
#include <Gruntz/StaminaPct.h>
#include <Gruntz/TileCollisionKind.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TriggerMgrRecords.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/VoiceManager.h>
#include <Ints.h>
#include <RectMacros.h>
#include <Wap32/TileGeometry.h>
#include <ZTools/ZDArray.h>

#include <limits.h>
#include <new>
#include <stdlib.h>
#include <string.h>

RVA(0x000f36a0, 0x78e)
i32 CGrunt::StepDiggerBehavior() {
    bool isI = IsAnimationAct("I");
    if (isI) {
        return 1;
    }
    CMapMgr* grid = g_gameReg->GetTileGrid();
    grid->Clip(NULL);

    i32 tileX = ScanCell().m_x;
    i32 tileY = ScanCell().m_y;

    FIND_NEAREST_ENEMY_AT_TARGET(g, atTarget)

    m_defenderPx = m_lastTilePx;

    b32 inCombat = m_inCombat;
    if (inCombat != false) {
        b32 attackQueued = m_attackQueued;
        if (attackQueued == false) {
            if (m_attackWindupActive != false) {
                return 1;
            }
            if (m_stamina >= STAMINA_FULL) {
                if (FindGridNeighbor(1) != NULL) {
                    return 1;
                }
                if (atTarget && g == NULL) {
                    return 1;
                }
                if (m_inCombat == false) {
                    return 1;
                }
                if (m_attackQueued != false) {
                    return 1;
                }
                m_entranceActive = false;
                m_attackWindupActive = false;
                m_attackQueued = false;
                m_inCombat = false;
                ResetEntranceAnimation(1, 0, 0);
                return 1;
            }
            if (atTarget) {
                return 1;
            }
            if (m_inCombat == false) {
                return 1;
            }
            if (m_attackQueued != false) {
                return 1;
            }
            m_entranceActive = false;
            m_attackWindupActive = false;
            m_attackQueued = false;
            m_inCombat = false;
            ResetEntranceAnimation(1, 0, 0);
            return 1;
        }
        m_attackQueued = false;
        return 1;
    }

    if (g == NULL || GruntInRadius(g->m_playerIndex, g->m_unitIndex) == 0) {
        m_blockedVoicePending = false;
        goto L_tailc;
    }
    if (m_inCombat != false) {
        goto L_tailc;
    }
    if (m_stamina >= STAMINA_FULL && g->m_object->m_screenX == g->m_lastTilePx.m_x
        && g->m_object->m_screenY == g->m_lastTilePx.m_y
        && RectContains(g->m_object->m_screenX, g->m_object->m_screenY) != 0) {
        COMMIT_GRUNT_NEIGHBOR(g);
        m_dwell = 0;
        return 1;
    }
    if (m_inCombat != false) {
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
        if (m_blockedVoicePending != false) {
            PLAY_VOICE_IN_VIEW(0x366);
            m_blockedVoicePending = false;
        }
        m_dwell = 0;
    }

L_tailc:
    if (CoordsEmpty()) {
        if ((m_inCombat == false) & (static_cast<u32>(m_dwell) > DWELL_SEEK_PATH_MS)) {
            i32 r = m_defenderRadius;
            CRect box(tileX - r, tileY - r, tileX + r, tileY + r);
            CRect gb(0, 0, grid->GetWidth(), grid->GetHeight());
            CRect isect;
            if (!isect.IntersectRect(&box, &gb)) {
                isect = box;
            }
            i32 best = INT_MAX;
            i32 bestCol = -1;
            i32 bestRow = -1;
            grid->Clip(&isect);
            for (i32 row = isect.top; row < isect.bottom; row++) {
                BrickzCell* cell = &grid->m_rows[row][isect.left];
                for (i32 col = isect.left; col < isect.right; col++) {
                    if ((cell->m_flags & IDX(CELL_FLAG_COVERED_POWERUP)) != 0) {
                        i32 dr = row - tileY;
                        dr = abs(dr);
                        i32 dc = col - tileX;
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
                i32 dc = bestCol - tileX;
                dc = abs(dc);
                i32 dr = bestRow - tileY;
                dr = abs(dr);
                if (dc <= 1 && dr <= 1) {
                    m_triggerMgr->UseEquippedToolAt(
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
            grid->Clip(NULL);
            m_dwell = 0;
        }
        return 1;
    }
    {
        Coord* coord = GetHeadCoord();
        i32 col = coord->m_x;
        i32 row = coord->m_y;
        BrickzCell* cell = &grid->m_rows[row][col];
        if ((cell->m_flags & IDX(CELL_FLAG_REVEALED_POWERUP)) != 0
            || (cell->m_flags & IDX(CELL_FLAG_COVERED_POWERUP)) != 0) {
            m_triggerMgr->UseEquippedToolAt(
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
