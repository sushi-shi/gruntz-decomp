#include <rva.h>

#include <Mfc.h>
#include <MfcWin.h>

#include <Enums.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/FreeNodePoolInline.h>
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
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GruntzMgr.h>
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
RVA(0x000ee800, 0x971)
i32 CGrunt::StepDefenderBehavior() {
    i32 defTX = m_defenderPx.m_x >> TILE_SHIFT_PX;
    i32 defTY = m_defenderPx.m_y >> TILE_SHIFT_PX;

    i32 scanRadius = m_defenderRadius + m_reachRect.right - 1;
    i32 trimRadius = m_defenderRadius - 1;
    RECT scanBounds;
    scanBounds.left = defTX - scanRadius;
    scanBounds.top = defTY - scanRadius;
    scanBounds.right = defTX + scanRadius + 1;
    scanBounds.bottom = defTY + scanRadius + 1;

    Coord pt;
    GetScreenPos(&pt);
    i32 dTX = abs((pt.m_x >> TILE_SHIFT_PX) - (m_defenderPx.m_x >> TILE_SHIFT_PX));
    GetScreenPos(&pt);
    i32 dTY = abs((pt.m_y >> TILE_SHIFT_PX) - (m_defenderPx.m_y >> TILE_SHIFT_PX));
    i32 dist = dTX > dTY ? dTX : dTY;
    if (dist > m_defenderRadius) {
        m_defenderPx = m_lastTilePx;
        return 1;
    }

    CGrunt* occ = m_triggerMgr->FindNearestEnemy(this);
    i32 occOnTile = 0;
    if (occ) {
        CGameObject* oo = occ->m_object;
        if (IsGruntAtSavedScreenPos(occ) != 0) {
            if (RectContains(oo->m_screenX, oo->m_screenY)) {
                occOnTile = 1;
            }
        }
    }

    if (m_poweredUp) {
        if (m_neighborValid == 0) {
            if (m_combatActive) {
                return 1;
            }
            if (m_stamina >= STAMINA_FULL) {
                if (FindGridNeighbor(1)) {
                    return 1;
                }
                if (occOnTile && occ == NULL) {
                    return 1;
                }
                if (m_poweredUp == 0) {
                    return 1;
                }
            } else {
                if (occOnTile) {
                    return 1;
                }
            }
            if (m_neighborValid) {
                return 1;
            }
            RESET_GRUNT_POWERED_STATE(this)
        } else {
            m_neighborValid = 0;
        }
        return 1;
    }

    if (occ != NULL) {
        if (m_neighborValid) {
            return 1;
        }
        if (m_combatActive == 0 && m_stamina >= STAMINA_FULL && occOnTile) {
            COMMIT_GRUNT_NEIGHBOR(occ);
            RecycleGruntCoords(this);
            return 1;
        }
        if (occOnTile) {
            RecycleGruntCoords(this);
            return 1;
        }
    } else {
        m_blockedVoicePending = 0;
    }

    if (occ != NULL && static_cast<u32>(m_dwell) > DWELL_REPATH_MS) {
        i32 occTX = occ->m_object->m_screenX >> TILE_SHIFT_PX;
        i32 occTY = occ->m_object->m_screenY >> TILE_SHIFT_PX;
        i32 dx = abs(occTX - defTX);
        i32 dy = abs(occTY - defTY);
        i32 radius = dx > dy ? dx : dy;

        if (radius < m_defenderRadius + m_reachRect.right) {
            if (m_blockedVoicePending != 0) {
                const RECT* view = &g_gameReg->m_world->m_level->m_mainPlane->m_planeViewRect;
                if (CGameLevel::PointInBounds(view, m_object->m_screenX, m_object->m_screenY)
                    != 0) {
                    g_gameReg->m_voiceManager->PlayVoice(this, 0x366, -1, 0, -1, -1);
                }
                m_blockedVoicePending = 0;
            }

            POINT target;
            target.x = occTX;
            target.y = occTY;
            if (PtInRect(&scanBounds, target) != 0 && m_defenderRadius > 1) {
                RECT oldBounds = g_gameReg->m_tileGrid->m_bounds;
                CDWordArray saved;
                for (i32 y = oldBounds.top; y < oldBounds.bottom + 1; y++) {
                    for (i32 x = oldBounds.left; x < oldBounds.right + 1; x++) {
                        if (static_cast<u32>(x) < g_gameReg->m_tileGrid->m_width
                            && static_cast<u32>(y) < g_gameReg->m_tileGrid->m_height) {
                            saved.SetAtGrow(
                                saved.GetSize(),
                                static_cast<DWORD>(g_gameReg->m_tileGrid->CellFlagsAt(x, y))
                            );
                        }
                    }
                }

                i32 cx = m_defenderPx.m_x >> TILE_SHIFT_PX;
                i32 cy = m_defenderPx.m_y >> TILE_SHIFT_PX;
                for (i32 borderX = cx - m_defenderRadius; borderX < cx + m_defenderRadius + 1;
                     borderX++) {
                    i32 top = cy - m_defenderRadius;
                    i32 bottom = cy + m_defenderRadius;
                    if (static_cast<u32>(borderX) < g_gameReg->m_tileGrid->m_width
                        && static_cast<u32>(top) < g_gameReg->m_tileGrid->m_height
                        && (borderX != occTX || top != occTY)) {
                        g_gameReg->m_tileGrid->m_rows[top][borderX].m_flags = 1;
                    }
                    if (static_cast<u32>(borderX) < g_gameReg->m_tileGrid->m_width
                        && static_cast<u32>(bottom) < g_gameReg->m_tileGrid->m_height
                        && (borderX != occTX || bottom != occTY)) {
                        g_gameReg->m_tileGrid->m_rows[bottom][borderX].m_flags = 1;
                    }
                }
                for (i32 borderY = cy - m_defenderRadius; borderY < cy + m_defenderRadius + 1;
                     borderY++) {
                    i32 left = cx - m_defenderRadius;
                    i32 right = cx + m_defenderRadius;
                    if (static_cast<u32>(left) < g_gameReg->m_tileGrid->m_width
                        && static_cast<u32>(borderY) < g_gameReg->m_tileGrid->m_height
                        && (left != occTX || borderY != occTY)) {
                        g_gameReg->m_tileGrid->m_rows[borderY][left].m_flags = 1;
                    }
                    if (static_cast<u32>(right) < g_gameReg->m_tileGrid->m_width
                        && static_cast<u32>(borderY) < g_gameReg->m_tileGrid->m_height
                        && (right != occTX || borderY != occTY)) {
                        g_gameReg->m_tileGrid->m_rows[borderY][right].m_flags = 1;
                    }
                }

                TileSwitch(occTX, occTY, 0, m_arrivalFlags, 1, 0);

                i32 savedIndex = 0;
                for (i32 restoreY = oldBounds.top; restoreY < oldBounds.bottom + 1; restoreY++) {
                    for (i32 restoreX = oldBounds.left; restoreX < oldBounds.right + 1;
                         restoreX++) {
                        if (static_cast<u32>(restoreX) < g_gameReg->m_tileGrid->m_width
                            && static_cast<u32>(restoreY) < g_gameReg->m_tileGrid->m_height) {
                            g_gameReg->m_tileGrid->m_rows[restoreY][restoreX].m_flags =
                                saved.GetAt(savedIndex++);
                        }
                    }
                }

                saved.RemoveAll();

                if (CoordCount() != 0) {
                    Coord* previous = NULL;
                    POSITION pos = m_coordList.GetHeadPosition();
                    while (pos != NULL) {
                        POSITION trimPos = pos;
                        Coord* trimCoord = static_cast<Coord*>(m_coordList.GetNext(pos));
                        i32 pathDx = abs(trimCoord->m_x - defTX);
                        i32 pathDy = abs(trimCoord->m_y - defTY);
                        i32 pathDist = pathDx > pathDy ? pathDx : pathDy;
                        if (pathDist > trimRadius) {
                            if (previous != NULL) {
                                i32 backDx = abs(previous->m_x - occTX);
                                i32 backDy = abs(previous->m_y - occTY);
                                i32 backDist = backDx > backDy ? backDx : backDy;
                                if (backDist <= m_reachRect.right) {
                                    PushFreeNode(&g_coordPool, trimCoord);
                                    m_coordList.RemoveAt(trimPos);
                                    while (pos != NULL) {
                                        POSITION nextPos = pos;
                                        Coord* coord =
                                            static_cast<Coord*>(m_coordList.GetNext(pos));
                                        if (coord != NULL) {
                                            PushFreeNode(&g_coordPool, coord);
                                        }
                                        m_coordList.RemoveAt(nextPos);
                                    }
                                } else {
                                    SetEntrancePos(1, 1);
                                    if (CoordCount() != 0) {
                                        RECYCLE_GRUNT_COORDS_EXPANDED(this)
                                    }
                                }
                            } else {
                                SetEntrancePos(1, 1);
                                if (CoordCount() != 0) {
                                    RECYCLE_GRUNT_COORDS_EXPANDED(this)
                                }
                            }
                            return 1;
                        }
                        previous = trimCoord;
                    }
                } else if ((m_object->m_screenX >> TILE_SHIFT_PX) != defTX
                           || (m_object->m_screenY >> TILE_SHIFT_PX) != defTY) {
                    TileSwitch(defTX, defTY, 0, m_arrivalFlags, 1, 0);
                    m_dwell = 0;
                }
            }
        } else if ((m_object->m_screenX >> TILE_SHIFT_PX) != defTX
                   || (m_object->m_screenY >> TILE_SHIFT_PX) != defTY) {
            TileSwitch(defTX, defTY, 0, m_arrivalFlags, 1, 0);
        }
        m_dwell = 0;
    } else if (occ == NULL && static_cast<u32>(m_dwell) > DWELL_REPATH_MS
               && ((m_object->m_screenX >> TILE_SHIFT_PX) != defTX
                   || (m_object->m_screenY >> TILE_SHIFT_PX) != defTY)) {
        TileSwitch(defTX, defTY, 0, m_arrivalFlags, 1, 0);
    }

    GRID_RECT_INLINE_LOCAL(g_gameReg->m_tileGrid);

    return 1;
}
