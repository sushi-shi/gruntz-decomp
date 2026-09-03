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
#include <Gruntz/GruntMovementInline.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntMovementMacros.h>
#include <Gruntz/GruntPoweredStateMacros.h>
#include <Gruntz/GruntPuddle.h>
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
#include <Wap32/TileGeometry.h>
#include <Wap32/ZVec.h>

#include <limits.h>
#include <new>
#include <stdlib.h>
#include <string.h>

// @early-stop
RVA(0x000ecc90, 0x86a)
i32 CGrunt::StepBrickLayerBehavior() {
    bool eqI = ANIMATION_ACT_EQUALS("I");
    if (eqI) {
        return 1;
    }
    m_defenderPx = m_lastTilePx;
    CMapMgr* grid = g_gameReg->m_tileGrid;
    grid->Clip(NULL);

    Coord center;
    GetScreenTile(&center);
    Coord targetTile;

    i32 atTarget;
    CGrunt* g = FindNearestEnemyAtTarget(this, &atTarget);

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
            ResetGruntPoweredState(this);
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
                CommitGruntNeighbor(this, g);
                if (CoordCount() != 0) {
                    RecycleGruntCoords(this);
                }
                return 1;
            }
        } else {
            if (atTarget) {
                if (CoordCount() != 0) {
                    RecycleGruntCoords(this);
                }
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
        && RectContains(g->m_object->m_screenPosition.m_x, g->m_object->m_screenPosition.m_y)
               != 0) {
        CommitGruntNeighbor(this, g);
        m_dwell = 0;
        return 1;
    }
    if (m_poweredUp != false) {
        goto L_ed153;
    }
    g->GetScreenTile(&targetTile);
    if (TileSwitch(targetTile.m_x, targetTile.m_y, 0, m_arrivalFlags, 1, 0) == 0) {
        goto L_ed153;
    }
    if (m_blockedVoicePending != false) {
        CCueRect* board = &g_gameReg->m_world->m_level->m_mainPlane->m_planeViewRect;
        Coord voicePosition = m_object->ScreenPos();
        if (::PtInRect(board, voicePosition.m_x, voicePosition.m_y)) {
            g_gameReg->m_voiceManager->PlayVoice(this, 0x366, -1, 0, -1, -1);
        }
        m_blockedVoicePending = false;
    }
    m_dwell = 0;

L_ed153:
    if (CoordCount() == 0) {
        if (static_cast<u32>(m_dwell) <= DWELL_SEEK_PATH_MS) {
            return 1;
        }

        i32 r = m_defenderRadius;
        CRect box(center.m_x - r, center.m_y - r, center.m_x + r, center.m_y + r);
        CRect gridBounds(0, 0, grid->m_width, grid->m_height);
        CRect isect;
        if (!isect.IntersectRect(&box, &gridBounds)) {
            isect = box;
        }

        i32 best = INT_MAX;
        Coord bestTile(-1, -1);
        grid->Clip(&isect);
        for (i32 row = isect.top; row < isect.bottom; row++) {
            BrickzCell* cell = &grid->m_rows[row][isect.left];
            for (i32 col = isect.left; col < isect.right; col++) {
                if ((cell->m_flags & IDX(CELL_FLAG_HIDDEN_POWERUP)) != 0
                    || cell->m_typeCode == TILEKIND_GAUNTLET_BRICK_A
                    || cell->m_typeCode == TILEKIND_GAUNTLET_BRICK_B) {
                    Coord tile(col, row);
                    Coord delta = tile - center;
                    Coord distance = delta.GetAbs();
                    i32 dist = distance.m_x + distance.m_y;
                    if (dist < best) {
                        best = dist;
                        bestTile = tile;
                    }
                }
                cell++;
            }
        }
        if (best != INT_MAX) {
            Coord delta = bestTile - center;
            Coord distance = delta.GetAbs();
            if (distance.m_x <= 1 && distance.m_y <= 1) {
                Coord targetPosition = bestTile;
                TileCenter(&targetPosition);
                m_triggerMgr->UseEquippedToolAt(
                    m_playerIndex,
                    m_unitIndex,
                    targetPosition.m_x,
                    targetPosition.m_y
                );
                SetEntrancePos(1, 1);
            } else {
                TileSwitch(bestTile.m_x, bestTile.m_y, 0, m_arrivalFlags, 1, 0);
            }
        }
        grid->Clip(NULL);
        m_dwell = 0;
        return 1;
    }
    {
        Coord* coord = static_cast<Coord*>(m_coordList.GetHead());
        Coord targetTile = *coord;
        BrickzCell* cell = &grid->m_rows[targetTile.m_y][targetTile.m_x];
        if ((cell->m_flags & IDX(CELL_FLAG_HIDDEN_POWERUP)) != 0
            || cell->m_typeCode == TILEKIND_GAUNTLET_BRICK_A
            || cell->m_typeCode == TILEKIND_GAUNTLET_BRICK_B) {
            Coord targetPosition = targetTile;
            TileCenter(&targetPosition);
            m_triggerMgr->UseEquippedToolAt(
                m_playerIndex,
                m_unitIndex,
                targetPosition.m_x,
                targetPosition.m_y
            );
            SetEntrancePos(1, 1);
            m_dwell = 0;
        }
    }
    return 1;
}
