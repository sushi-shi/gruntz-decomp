#include <rva.h>

#include <Mfc.h>
#include <MfcWin.h>

#include <Enums.h>
#include <Globals.h>
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
#include <MakeRect.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/ZVec.h>

#include <limits.h>
#include <new>
#include <stdlib.h>
#include <string.h>

// @early-stop
RVA(0x000ee800, 0x971)
i32 CGrunt::StepDefenderBehavior() {
    Coord defenderTile = m_defenderPx;
    ScreenTile(&defenderTile);
    Coord currentTile;
    GetScreenTile(&currentTile);

    i32 scanRadius = m_defenderRadius + m_reachRect.right - 1;
    i32 trimRadius = m_defenderRadius - 1;
    CRect scanBounds(
        defenderTile.m_x - scanRadius,
        defenderTile.m_y - scanRadius,
        defenderTile.m_x + scanRadius + 1,
        defenderTile.m_y + scanRadius + 1
    );

    {
        Coord selfTile;
        GetScreenTile(&selfTile);
        Coord distance = (selfTile - defenderTile).GetAbs();
        i32 dist = Max(distance.m_x, distance.m_y);
        if (dist > m_defenderRadius) {
            m_defenderPx = m_lastTilePx;
            return 1;
        }
    }

    FIND_NEAREST_ENEMY_AT_TARGET(occ, occOnTile)

    b32 powered = m_poweredUp;
    if (powered != false) {
        b32 neighborValid = m_neighborValid;
        if (neighborValid == false) {
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
                if (m_poweredUp == false) {
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
            m_neighborValid = false;
        }
        return 1;
    }

    if (occ != NULL) {
        if (m_neighborValid) {
            return 1;
        }
        if (m_combatActive == false && m_stamina >= STAMINA_FULL && occOnTile) {
            COMMIT_GRUNT_NEIGHBOR(occ);
            RecycleGruntCoords(this);
            return 1;
        }
        if (occOnTile) {
            RecycleGruntCoords(this);
            return 1;
        }
    } else {
        m_blockedVoicePending = false;
    }

    if (occ != NULL && static_cast<u32>(m_dwell) > DWELL_REPATH_MS) {
        Coord occupantTile;
        occ->GetScreenTile(&occupantTile);
        Coord distance = (occupantTile - defenderTile).GetAbs();
        i32 radius = Max(distance.m_x, distance.m_y);

        if (radius < m_defenderRadius + m_reachRect.right) {
            if (m_blockedVoicePending != false) {
                CGruntzMgr* gameMgr = g_gameReg;
                const RECT* view = &gameMgr->m_world->m_level->m_mainPlane->m_planeViewRect;
                if (CGameLevel::PointInBounds(
                        view,
                        m_object->m_screenPosition.m_x,
                        m_object->m_screenPosition.m_y
                    )
                    != 0) {
                    gameMgr->m_voiceManager->PlayVoice(this, 0x366, -1, 0, -1, -1);
                }
                m_blockedVoicePending = false;
            }

            CPoint target(occupantTile.m_x, occupantTile.m_y);
            if (scanBounds.PtInRect(target) != false && m_defenderRadius > 1) {
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

                Coord center = m_defenderPx;
                ScreenTile(&center);
                for (i32 borderX = center.m_x - m_defenderRadius;
                     borderX < center.m_x + m_defenderRadius + 1;
                     borderX++) {
                    i32 top = center.m_y - m_defenderRadius;
                    i32 bottom = center.m_y + m_defenderRadius;
                    if (static_cast<u32>(borderX) < g_gameReg->m_tileGrid->m_width
                        && static_cast<u32>(top) < g_gameReg->m_tileGrid->m_height
                        && (borderX != occupantTile.m_x || top != occupantTile.m_y)) {
                        g_gameReg->m_tileGrid->m_rows[top][borderX].m_flags = IDX(CELL_FLAG_SOLID);
                    }
                    if (static_cast<u32>(borderX) < g_gameReg->m_tileGrid->m_width
                        && static_cast<u32>(bottom) < g_gameReg->m_tileGrid->m_height
                        && (borderX != occupantTile.m_x || bottom != occupantTile.m_y)) {
                        g_gameReg->m_tileGrid->m_rows[bottom][borderX].m_flags =
                            IDX(CELL_FLAG_SOLID);
                    }
                }
                for (i32 borderY = center.m_y - m_defenderRadius;
                     borderY < center.m_y + m_defenderRadius + 1;
                     borderY++) {
                    i32 left = center.m_x - m_defenderRadius;
                    i32 right = center.m_x + m_defenderRadius;
                    if (static_cast<u32>(left) < g_gameReg->m_tileGrid->m_width
                        && static_cast<u32>(borderY) < g_gameReg->m_tileGrid->m_height
                        && (left != occupantTile.m_x || borderY != occupantTile.m_y)) {
                        g_gameReg->m_tileGrid->m_rows[borderY][left].m_flags = IDX(CELL_FLAG_SOLID);
                    }
                    if (static_cast<u32>(right) < g_gameReg->m_tileGrid->m_width
                        && static_cast<u32>(borderY) < g_gameReg->m_tileGrid->m_height
                        && (right != occupantTile.m_x || borderY != occupantTile.m_y)) {
                        g_gameReg->m_tileGrid->m_rows[borderY][right].m_flags =
                            IDX(CELL_FLAG_SOLID);
                    }
                }

                TileSwitch(occupantTile.m_x, occupantTile.m_y, 0, m_arrivalFlags, 1, 0);

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
                        Coord pathDistance = (*trimCoord - defenderTile).GetAbs();
                        i32 pathDist = Max(pathDistance.m_x, pathDistance.m_y);
                        if (pathDist > trimRadius) {
                            if (previous != NULL) {
                                Coord backDistance = (*previous - occupantTile).GetAbs();
                                i32 backDist = Max(backDistance.m_x, backDistance.m_y);
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
                } else if (currentTile != defenderTile) {
                    TileSwitch(defenderTile.m_x, defenderTile.m_y, 0, m_arrivalFlags, 1, 0);
                    m_dwell = 0;
                }
            }
        } else if (currentTile != defenderTile) {
            TileSwitch(defenderTile.m_x, defenderTile.m_y, 0, m_arrivalFlags, 1, 0);
        }
        m_dwell = 0;
    } else if (occ == NULL && static_cast<u32>(m_dwell) > DWELL_REPATH_MS
               && currentTile != defenderTile) {
        TileSwitch(defenderTile.m_x, defenderTile.m_y, 0, m_arrivalFlags, 1, 0);
    }

    CMapMgr* grid = g_gameReg->m_tileGrid;
    grid->Clip(NULL);

    return 1;
}
