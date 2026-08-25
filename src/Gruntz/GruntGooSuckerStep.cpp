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
#include <Gruntz/GruntCoordRecycleMacros.h>
#include <Gruntz/GruntDirStatics.h>
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
RVA(0x000f0db0, 0x48)

i32 CellTargetable(i32 tileX, i32 tileY) {
    CPtrList& list = g_gameReg->m_triggerMgr->m_baseList;
    POSITION pos = list.GetHeadPosition();

    if (pos != NULL) {
        do {
            CGruntPuddle* p = static_cast<CGruntPuddle*>(list.GetNext(pos));
            if (p->m_pending == 0) {
                i32 puddleX = p->m_tileX;
                i32 puddleY = p->m_tileY;
                if (puddleX == tileX && puddleY == tileY) {
                    return 1;
                }
            }
        } while (pos != NULL);
    }
    return 0;
}

RVA(0x000f0e20, 0x928)
i32 CGrunt::StepGooSuckerBehavior() {
    bool eqI = ANIMATION_ACT_EQUALS("I");
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

    i32 powered = m_poweredUp;
    if (powered != 0) {
        i32 neighborValid = m_neighborValid;
        if (neighborValid == 0) {
            if (m_combatActive != 0) {
                goto L_yes;
            }
            if (m_stamina >= STAMINA_FULL) {
                if (FindGridNeighbor(1) != NULL) {
                    goto L_yes;
                }
                if (atTarget && g == NULL) {
                    goto L_yes;
                }
                if (m_poweredUp == 0) {
                    goto L_yes;
                }
            } else {
                if (atTarget) {
                    goto L_yes;
                }
                if (m_poweredUp == 0) {
                    goto L_yes;
                }
            }
            if (m_neighborValid != 0) {
                goto L_yes;
            }
            RESET_GRUNT_POWERED_STATE(this)
        } else {
            m_neighborValid = 0;
        }
    L_yes:
        return 1;
    }

    if (g != NULL) {
        if (m_neighborValid != 0) {
            return 1;
        }
        if (m_combatActive == 0 && m_stamina >= STAMINA_FULL) {
            if (atTarget) {
                COMMIT_GRUNT_NEIGHBOR(g);
                if (CoordCount() != 0) {
                    RECYCLE_GRUNT_COORDS(this)
                }
                return 1;
            }
        } else {
            if (atTarget) {
                if (CoordCount() != 0) {
                    RECYCLE_GRUNT_COORDS(this)
                }
                return 1;
            }
        }
    } else {
        m_blockedVoicePending = 0;
    }

L_ed006b:
    if (g == NULL || GruntInRadius(g->m_playerIndex, g->m_unitIndex) == 0) {
        m_blockedVoicePending = 0;
        goto L_scanb;
    }
    if (m_poweredUp != 0) {
        goto L_scanb;
    }
    if (m_stamina >= STAMINA_FULL && GRUNT_AT_SAVED_SCREEN_POS(g)
        && RectContains(g->m_object->m_screenX, g->m_object->m_screenY) != 0) {
        COMMIT_GRUNT_NEIGHBOR(g);
    }
    if (m_poweredUp != 0) {
        goto L_scanb;
    }
    if (static_cast<u32>(m_dwell) <= DWELL_REPATH_MS) {
        goto L_scanb;
    }
    {
        Coord cc;
        g->GetScreenPos(&cc);
        if (TileSwitch(cc.m_x >> TILE_SHIFT_PX, cc.m_y >> TILE_SHIFT_PX, 0, m_arrivalFlags, 1, 0)
            != 0) {
            if (m_blockedVoicePending != 0) {
                i32 x = m_object->m_screenX;
                i32 y = m_object->m_screenY;
                CGruntzMgr* game = g_gameReg;
                if (CGameLevel::PointInBounds(
                        &game->m_world->m_level->m_mainPlane->m_viewRect,
                        x,
                        y
                    )
                    != 0) {
                    game->m_voiceManager->PlayVoice(this, 0x366, -1, 0, -1, -1);
                }
                m_blockedVoicePending = 0;
            }
            m_dwell = 0;
        }
    }

L_scanb:
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
        i32 bestX = 0;
        i32 bestY = 0;

        POSITION pos = m_triggerMgr->m_baseList.GetHeadPosition();
        while (pos != NULL) {
            CGruntPuddle* gg = static_cast<CGruntPuddle*>(m_triggerMgr->m_baseList.GetNext(pos));
            if (gg->m_pending == 0) {
                i32 gx = gg->m_tileX;
                i32 gy = gg->m_tileY;
                if (RectContains(
                        (gx << TILE_SHIFT_PX) + TILE_HALF_PX,
                        (gy << TILE_SHIFT_PX) + TILE_HALF_PX
                    )
                    != 0) {
                    m_triggerMgr->ApplyTriggerA(
                        m_playerIndex,
                        m_unitIndex,
                        (gx << TILE_SHIFT_PX) + TILE_HALF_PX,
                        (gy << TILE_SHIFT_PX) + TILE_HALF_PX
                    );
                    GRID_CLIP_INL(grid, NULL);
                    return 1;
                }
                i32 dx = gx - (m_object->m_screenX >> TILE_SHIFT_PX);
                i32 dy = gy - (m_object->m_screenY >> TILE_SHIFT_PX);
                i32 dist = abs(dx) + abs(dy);
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
            i32 dx = bestX - c1.m_x;
            dx = abs(dx);
            i32 dy = bestY - c2.m_y;
            dy = abs(dy);
            if (dx <= 1 && dy <= 1) {
                m_triggerMgr->ApplyTriggerA(
                    m_playerIndex,
                    m_unitIndex,
                    (bestX << TILE_SHIFT_PX) + TILE_HALF_PX,
                    (bestY << TILE_SHIFT_PX) + TILE_HALF_PX
                );
                SetEntrancePos(1, 1);
            } else {
                TileSwitch(bestX, bestY, 0, m_arrivalFlags, 1, 0);
            }
        }
        GRID_RECT_INLINE(grid);
    } else {
        Coord* coord = static_cast<Coord*>(m_coordList.GetHead());
        i32 col = coord->m_x;
        i32 row = coord->m_y;
        if (CellTargetable(col, row) == 0) {
            return 1;
        }
        m_triggerMgr->ApplyTriggerA(
            m_playerIndex,
            m_unitIndex,
            (col << TILE_SHIFT_PX) + TILE_HALF_PX,
            (row << TILE_SHIFT_PX) + TILE_HALF_PX
        );
        SetEntrancePos(1, 1);
    }
    m_dwell = 0;
    return 1;
}
