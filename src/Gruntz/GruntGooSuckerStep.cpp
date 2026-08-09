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
// The six ret blocks now sit at retail's six positions and the four Clip sites
// have their proven expansions.  Residue: the frame is 0x90 against retail's
// 0x88 (two surplus dwords in the rect group), and cl holds `this` in edi where
// retail takes esi - `push esi; mov esi,ecx; push edi` vs base's four pushes
// then `mov edi,ecx`.  (The three `Coord x[2]` locals were frame padding, not a
// buffer requirement - CUserLogic::GetScreenPos writes exactly two dwords - and
// removing them took the frame 0xa4 -> 0x90.)
RVA(0x000f0e20, 0x928)
i32 CGrunt::StepGooSuckerBehavior() {
    bool eqI = (strcmp(*g_typeColl.GetNameRecord(m_objAux->m_actKey), "I") == 0);
    if (eqI) {
        return 1;
    }
    m_defenderPx.m_x = m_lastTilePx.m_x;
    m_defenderPx.m_y = m_lastTilePx.m_y;
    CMapMgr* grid = g_gameReg->m_tileGrid;
    GRID_CLIP_NULL(grid);

    Coord c1;
    GetScreenPos(&c1);
    i32 cx = c1.m_x >> TILE_SHIFT_PX;
    Coord c2;
    GetScreenPos(&c2);
    i32 cy = c2.m_y >> TILE_SHIFT_PX;

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
        if (m_neighborValid == 0) {
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
            m_entranceActive = 0;
            m_combatActive = 0;
            m_neighborValid = 0;
            m_poweredUp = 0;
            ResetEntranceAnimation(1, 0, 0);
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
    } else {
        m_blockedVoicePending = 0;
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
        Coord cc;
        g->GetScreenPos(&cc);
        if (TileSwitch(cc.m_x >> TILE_SHIFT_PX, cc.m_y >> TILE_SHIFT_PX, 0, m_arrivalFlags, 1, 0)
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
    if (CoordCount() == 0) {
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
        GRID_CLIP_INL(grid, &isect);

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
                    GRID_CLIP_INL(grid, NULL);
                    return 1;
                }
                i32 dx = gx - (m_object->m_screenX >> TILE_SHIFT_PX);
                dx = abs(dx);
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
            dx = abs(dx);
            i32 dy = bestY - cy;
            dy = abs(dy);
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
    } else {
        Coord* coord = static_cast<Coord*>(m_coordList.GetHead());
        i32 col = coord->m_x;
        i32 row = coord->m_y;
        if (CellTargetable(col, row) == 0) {
            return 1;
        }
        m_tileMgr->ApplyTriggerA(
            m_tileOwnerHi,
            m_tileOwnerLo,
            (col << TILE_SHIFT_PX) + TILE_HALF_PX,
            (row << TILE_SHIFT_PX) + TILE_HALF_PX
        );
        SetEntrancePos(1, 1);
    }
    m_dwell = 0;
    return 1;
}
