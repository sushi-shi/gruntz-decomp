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
#include <Gruntz/GruntCoordRecycleMacros.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntMovementInline.h>
#include <Gruntz/GruntMovementMacros.h>
#include <Gruntz/GruntPoweredStateMacros.h>
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

// @early-stop
RVA(0x000ecc90, 0x86a)
i32 CGrunt::StepBrickLayerBehavior() {
    bool eqI = IsAnimationAct("I");
    if (eqI) {
        return 1;
    }
    m_defenderPx = m_lastTilePx;
    CMapMgr* grid = g_gameReg->m_tileGrid;
    GRID_CLIP_NULL(grid);

    Coord c1;
    GetScreenPos(&c1);
    c1.m_x >>= TILE_SHIFT_PX;
    Coord c2;
    GetScreenPos(&c2);
    c2.m_y >>= TILE_SHIFT_PX;

    FIND_NEAREST_ENEMY_AT_TARGET(g, atTarget, x)

    b32 powered = m_poweredUp;
    if (powered != false) {
        if (m_neighborValid == false) {
            if (m_combatActive != false) {
                goto L_powered_yes;
            }
            if (m_stamina >= STAMINA_FULL) {
                if (FindGridNeighbor(1) != NULL) {
                    goto L_powered_yes;
                }
                if (atTarget && g == NULL) {
                    goto L_powered_yes;
                }
                if (m_poweredUp == false) {
                    goto L_powered_yes;
                }
            } else {
                if (atTarget) {
                    goto L_powered_yes;
                }
                if (m_poweredUp == false) {
                    goto L_powered_yes;
                }
            }
            if (m_neighborValid != false) {
                goto L_powered_yes;
            }
            RESET_GRUNT_POWERED_STATE(this)
        } else {
            m_neighborValid = false;
        }
    L_powered_yes:
        return 1;
    }

    if (g != NULL) {
        if (m_neighborValid != false) {
            return 1;
        }
        if (m_combatActive == false && m_stamina >= STAMINA_FULL) {
            if (atTarget) {
                COMMIT_GRUNT_NEIGHBOR(g);
                RecycleGruntCoords(this);
                return 1;
            }
        } else {
            if (atTarget) {
                RecycleGruntCoords(this);
                return 1;
            }
        }
    } else {
        m_blockedVoicePending = false;
    }

    if (g == NULL || static_cast<u32>(m_dwell) <= DWELL_REPATH_MS
        || GruntInRadius(g->m_playerIndex, g->m_unitIndex) == 0) {
        m_blockedVoicePending = false;
        goto L_ed153;
    }
    if (m_poweredUp != false) {
        goto L_ed153;
    }
    if (m_stamina >= STAMINA_FULL && IsGruntAtSavedScreenPos(g)
        && RectContains(g->m_object->m_screenX, g->m_object->m_screenY) != 0) {
        COMMIT_GRUNT_NEIGHBOR(g);
        m_dwell = 0;
        return 1;
    }
    if (m_poweredUp != false) {
        goto L_ed153;
    }
    if (TileSwitch(
            g->m_object->m_screenX >> TILE_SHIFT_PX,
            g->m_object->m_screenY >> TILE_SHIFT_PX,
            0,
            m_arrivalFlags,
            1,
            0
        )
        == 0) {
        goto L_ed153;
    }
    if (m_blockedVoicePending != false) {
        PLAY_VOICE_IN_VIEW(0x366);
        m_blockedVoicePending = false;
    }
    m_dwell = 0;

L_ed153:
    if (CoordCount() == 0) {
        if (static_cast<u32>(m_dwell) <= DWELL_SEEK_PATH_MS) {
            return 1;
        }

        i32 r = m_defenderRadius;
        RECT box;
        box.left = c1.m_x - r;
        box.right = c1.m_x + r;
        box.top = c2.m_y - r;
        box.bottom = c2.m_y + r;
        RECT gb;
        SET_RECT_COMPONENTS(gb, 0, 0, grid->m_width, grid->m_height);
        RECT isect;
        if (!IntersectRect(&isect, &box, &gb)) {
            isect = box;
        }

        i32 best = INT_MAX;
        i32 bestCol = -1;
        i32 bestRow = -1;
        GRID_CLIP_INL_FIELDS(grid, &isect);
        for (i32 row = isect.top; row < isect.bottom; row++) {
            BrickzCell* cell = &grid->m_rows[row][isect.left];
            for (i32 col = isect.left; col < isect.right; col++) {
                if ((cell->m_flags & IDX(CELL_FLAG_HIDDEN_POWERUP)) != 0
                    || cell->m_typeCode == TILEKIND_GAUNTLET_BRICK_A
                    || cell->m_typeCode == TILEKIND_GAUNTLET_BRICK_B) {
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
        GRID_RECT_INLINE(grid);
        m_dwell = 0;
        return 1;
    }
    {
        Coord* coord = GetHeadCoord();
        i32 col = coord->m_x;
        i32 row = coord->m_y;
        BrickzCell* cell = &grid->m_rows[row][col];
        if ((cell->m_flags & IDX(CELL_FLAG_HIDDEN_POWERUP)) != 0
            || cell->m_typeCode == TILEKIND_GAUNTLET_BRICK_A
            || cell->m_typeCode == TILEKIND_GAUNTLET_BRICK_B) {
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
