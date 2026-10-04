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
RVA(0x000ee800, 0x971)
i32 CGrunt::StepDefenderBehavior() {
    Coord defenderTile = m_defenderPx;
    ScreenTile(&defenderTile);

    i32 scanRadius = m_defenderRadius + m_reachRect.right - 1;
    i32 trimRadius = m_defenderRadius - 1;
    CRect scanBounds(
        defenderTile.m_x - scanRadius,
        defenderTile.m_y - scanRadius,
        defenderTile.m_x + scanRadius + 1,
        defenderTile.m_y + scanRadius + 1
    );

    {
        Coord pt;
        GetScreenPos(&pt);
        i32 dTX = abs((pt.m_x >> TILE_SHIFT_PX) - (m_defenderPx.m_x >> TILE_SHIFT_PX));
        GetScreenPos(&pt);
        i32 dTY = abs((pt.m_y >> TILE_SHIFT_PX) - (m_defenderPx.m_y >> TILE_SHIFT_PX));
        i32 dist = Max(dTX, dTY);
        if (dist > m_defenderRadius) {
            m_defenderPx = m_lastTilePx;
            return 1;
        }
    }

    FIND_NEAREST_ENEMY_AT_TARGET(occ, occOnTile)

    b32 inCombat = m_inCombat;
    if (inCombat != false) {
        b32 attackQueued = m_attackQueued;
        if (attackQueued == false) {
            if (m_attackWindupActive) {
                return 1;
            }
            if (m_stamina >= STAMINA_FULL) {
                if (FindGridNeighbor(1)) {
                    return 1;
                }
                if (occOnTile && occ == NULL) {
                    return 1;
                }
                if (m_inCombat == false) {
                    return 1;
                }
            } else {
                if (occOnTile) {
                    return 1;
                }
            }
            if (m_attackQueued) {
                return 1;
            }
            RESET_GRUNT_COMBAT_STATE(this)
        } else {
            m_attackQueued = false;
        }
        return 1;
    }

    if (occ != NULL) {
        if (m_attackQueued) {
            return 1;
        }
        if (m_attackWindupActive == false && m_stamina >= STAMINA_FULL && occOnTile) {
            COMMIT_GRUNT_NEIGHBOR(occ);
            this->RecycleCoords();
            return 1;
        }
        if (occOnTile) {
            this->RecycleCoords();
            return 1;
        }
    } else {
        m_blockedVoicePending = false;
    }

    if (occ != NULL && static_cast<u32>(m_dwell) > DWELL_REPATH_MS) {
        i32 occTX = occ->m_object->m_screenX >> TILE_SHIFT_PX;
        i32 occTY = occ->m_object->m_screenY >> TILE_SHIFT_PX;
        i32 dx = abs(occTX - defenderTile.m_x);
        i32 dy = abs(occTY - defenderTile.m_y);
        i32 radius = Max(dx, dy);

        if (radius < m_defenderRadius + m_reachRect.right) {
            if (m_blockedVoicePending != false) {
                PLAY_VOICE_IF_VISIBLE(0x366);
                m_blockedVoicePending = false;
            }

            CPoint target(occTX, occTY);
            if (scanBounds.PtInRect(target) != false && m_defenderRadius > 1) {
                RECT oldBounds = g_gameReg->GetTileGrid()->GetSearchBounds();
                CDWordArray saved;
                for (i32 y = oldBounds.top; y < oldBounds.bottom + 1; y++) {
                    for (i32 x = oldBounds.left; x < oldBounds.right + 1; x++) {
                        if (static_cast<u32>(x) < g_gameReg->GetTileGrid()->GetWidth()
                            && static_cast<u32>(y) < g_gameReg->GetTileGrid()->GetHeight()) {
                            saved.Add(
                                static_cast<DWORD>(g_gameReg->GetTileGrid()->CellFlagsAt(x, y))
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
                    if (static_cast<u32>(borderX) < g_gameReg->GetTileGrid()->GetWidth()
                        && static_cast<u32>(top) < g_gameReg->GetTileGrid()->GetHeight()
                        && (borderX != occTX || top != occTY)) {
                        g_gameReg->GetTileGrid()->CellFlagsAtUnchecked(borderX, top) = 1;
                    }
                    if (static_cast<u32>(borderX) < g_gameReg->GetTileGrid()->GetWidth()
                        && static_cast<u32>(bottom) < g_gameReg->GetTileGrid()->GetHeight()
                        && (borderX != occTX || bottom != occTY)) {
                        g_gameReg->GetTileGrid()->CellFlagsAtUnchecked(borderX, bottom) = 1;
                    }
                }
                for (i32 borderY = cy - m_defenderRadius; borderY < cy + m_defenderRadius + 1;
                     borderY++) {
                    i32 left = cx - m_defenderRadius;
                    i32 right = cx + m_defenderRadius;
                    if (static_cast<u32>(left) < g_gameReg->GetTileGrid()->GetWidth()
                        && static_cast<u32>(borderY) < g_gameReg->GetTileGrid()->GetHeight()
                        && (left != occTX || borderY != occTY)) {
                        g_gameReg->GetTileGrid()->CellFlagsAtUnchecked(left, borderY) = 1;
                    }
                    if (static_cast<u32>(right) < g_gameReg->GetTileGrid()->GetWidth()
                        && static_cast<u32>(borderY) < g_gameReg->GetTileGrid()->GetHeight()
                        && (right != occTX || borderY != occTY)) {
                        g_gameReg->GetTileGrid()->CellFlagsAtUnchecked(right, borderY) = 1;
                    }
                }

                MoveToTile(occTX, occTY, 0, m_arrivalFlags, 1, 0);

                i32 savedIndex = 0;
                for (i32 restoreY = oldBounds.top; restoreY < oldBounds.bottom + 1; restoreY++) {
                    for (i32 restoreX = oldBounds.left; restoreX < oldBounds.right + 1;
                         restoreX++) {
                        if (static_cast<u32>(restoreX) < g_gameReg->GetTileGrid()->GetWidth()
                            && static_cast<u32>(restoreY) < g_gameReg->GetTileGrid()->GetHeight()) {
                            g_gameReg->GetTileGrid()->CellFlagsAtUnchecked(restoreX, restoreY) =
                                saved.GetAt(savedIndex++);
                        }
                    }
                }

                saved.RemoveAll();

                if (!CoordsEmpty()) {
                    Coord* previous = NULL;
                    POSITION pos = m_coordList.GetHeadPosition();
                    while (pos != NULL) {
                        POSITION trimPos = pos;
                        Coord* trimCoord = GetNextCoord(pos);
                        i32 pathDx = abs(trimCoord->m_x - defenderTile.m_x);
                        i32 pathDy = abs(trimCoord->m_y - defenderTile.m_y);
                        i32 pathDist = Max(pathDx, pathDy);
                        if (pathDist > trimRadius) {
                            if (previous != NULL) {
                                i32 backDx = abs(previous->m_x - occTX);
                                i32 backDy = abs(previous->m_y - occTY);
                                i32 backDist = Max(backDx, backDy);
                                if (backDist <= m_reachRect.right) {
                                    g_coordPool.Push(trimCoord);
                                    RemoveCoordAt(trimPos);
                                    while (pos != NULL) {
                                        POSITION nextPos = pos;
                                        Coord* coord = GetNextCoord(pos);
                                        if (coord != NULL) {
                                            g_coordPool.Push(coord);
                                        }
                                        RemoveCoordAt(nextPos);
                                    }
                                } else {
                                    SetEntrancePos(1, 1);
                                    this->RecycleCoords();
                                }
                            } else {
                                SetEntrancePos(1, 1);
                                this->RecycleCoords();
                            }
                            return 1;
                        }
                        previous = trimCoord;
                    }
                } else if ((m_object->m_screenX >> TILE_SHIFT_PX) != defenderTile.m_x
                           || (m_object->m_screenY >> TILE_SHIFT_PX) != defenderTile.m_y) {
                    MoveToTile(defenderTile.m_x, defenderTile.m_y, 0, m_arrivalFlags, 1, 0);
                    m_dwell = 0;
                }
            }
        } else if ((m_object->m_screenX >> TILE_SHIFT_PX) != defenderTile.m_x
                   || (m_object->m_screenY >> TILE_SHIFT_PX) != defenderTile.m_y) {
            MoveToTile(defenderTile.m_x, defenderTile.m_y, 0, m_arrivalFlags, 1, 0);
        }
        m_dwell = 0;
    } else if (occ == NULL && static_cast<u32>(m_dwell) > DWELL_REPATH_MS
               && ((m_object->m_screenX >> TILE_SHIFT_PX) != defenderTile.m_x
                   || (m_object->m_screenY >> TILE_SHIFT_PX) != defenderTile.m_y)) {
        MoveToTile(defenderTile.m_x, defenderTile.m_y, 0, m_arrivalFlags, 1, 0);
    }

    CMapMgr* grid = g_gameReg->GetTileGrid();
    grid->Clip(NULL);

    return 1;
}
