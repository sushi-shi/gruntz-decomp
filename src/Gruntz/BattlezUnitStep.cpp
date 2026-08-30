#include <rva.h>

#include <Mfc.h>
#include <MfcNoInline.h>
#include <MfcWin.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/BattlezDifficulty.h>
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/BattlezRouteMaskPreset.h>
#include <Gruntz/BattlezTask.h>
#include <Gruntz/BrickTileId.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/FreeNodePoolInline.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntCoordRecycleMacros.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntPickupInline.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MapMgr.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/Play.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/StaminaPct.h>
#include <Gruntz/TileActionEvent.h>
#include <Gruntz/TileCollisionKind.h>
#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/TileTriggerLogic.h>
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/VoiceManager.h>
#include <Io/FileMem.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/zBitVec.h>
#include <Wwd/WwdFile.h>

#include <limits.h>
#include <math.h>
#include <new>
#include <stdlib.h>
#include <string.h>

#define MOVE_RECYCLE(g)                                                                            \
    {                                                                                              \
        CoordNode* nd = (g)->CoordHead();                                                          \
        while (nd != 0) {                                                                          \
            CoordNode* cur = nd;                                                                   \
            nd = nd->m_next;                                                                       \
            if (cur->m_coord != 0) {                                                               \
                g_coordPool.Push(cur->m_coord);                                                    \
            }                                                                                      \
        }                                                                                          \
        (g)->m_coordList.RemoveAll();                                                              \
    }

static inline i32 DistSq(i32 dx, i32 dy) {
    return dx * dx + dy * dy;
}

DATA(0x0022b7ec)
i32 g_battlezRoutePassableMask;

// @early-stop
RVA(0x00031610, 0x501)
i32 CBattlezMapConfig::Step(CGrunt* g) {
    if (g->CoordCount() == 0) {
        if (g->m_defenderState == AISTATE_ATTACK) {
            goto inflight;
        }

        i32 W = m_board->m_width;
        i32 H = m_board->m_height;
        Coord c0;
        g->GetScreenTile((&c0));
        CGrunt* nb = FindIdleGruntInBox(
            c0.m_x,
            c0.m_y,
            static_cast<i32>((static_cast<u32>(W) / 3)),
            static_cast<i32>((static_cast<u32>(H) / 3))
        );
        if (nb != NULL) {
            Coord c1;
            nb->GetScreenTile((&c1));
            if (g->TileSwitch(c1.m_x, c1.m_y, 0xd87, 0, 1, 0) == 0) {
                return 1;
            }
            g->m_arrivalCell.m_x = nb->m_playerIndex;
            g->m_arrivalCell.m_y = nb->m_unitIndex;
            g->m_defenderState = AISTATE_ATTACK;
            g->m_dwell = 0;
            AcceptAlways(g);
            return 1;
        }

        if (static_cast<u32>(g->m_dwell) > static_cast<u32>(m_idleRerouteDelay)) {
            Coord here;
            g->GetScreenPos((&here));
            RerouteIdleUnit(
                g,
                here.m_x >> TILE_SHIFT_PX,
                here.m_y >> TILE_SHIFT_PX,
                m_idleBurnRandX,
                m_idleBurnRandY,
                -1
            );
            if (g->CoordCount() > m_idleRouteLimitY + m_idleRouteLimitX && g->CoordCount() != 0) {
                RECYCLE_GRUNT_COORDS_VIA_NEXTDATA(g)
            }
            g->m_dwell = 0;
        }
        return 1;
    }

    if (g->m_defenderState != AISTATE_ATTACK) {
        return 1;
    }
inflight: {

    CGrunt* cur =
        m_triggerMgr->m_units[TM_UNITS_PER_PLAYER * g->ArrivalCell().m_x + g->ArrivalCell().m_y];
    i32 W = m_board->m_width;
    i32 H = m_board->m_height;
    Coord c0;
    g->GetScreenTile((&c0));
    CGrunt* nb = FindIdleGruntInBox(
        c0.m_x,
        c0.m_y,
        static_cast<i32>((static_cast<u32>(W) / 3)),
        static_cast<i32>((static_cast<u32>(H) / 3))
    );

    if (cur == NULL) {
        goto L_clear;
    }
    if (nb != NULL && cur != nb) {
        if (g->CoordCount() != 0) {
            MOVE_RECYCLE(g);
        }
        g->m_arrivalCell.m_x = nb->m_playerIndex;
        g->m_arrivalCell.m_y = nb->m_unitIndex;
        g->m_defenderState = AISTATE_ATTACK;
        g->m_dwell = 0;
        {
            CGameObject* s = static_cast<CGameObject*>(nb->m_object);
            if (g->TileSwitch(
                    s->m_screenX >> TILE_SHIFT_PX,
                    s->m_screenY >> TILE_SHIFT_PX,
                    0,
                    0xd87,
                    0,
                    0
                )
                == 0) {
                return 1;
            }
        }
        cur = nb;
    }

    if (cur != NULL) {
        {
            CGameObject* s = cur->m_object;
            if (g->RectContains(s->m_screenX, s->m_screenY) != 0) {

                if (g->CoordCount() != 0) {
                    MOVE_RECYCLE(g);
                }
                Coord none;
                g->m_arrivalCell = *none.Set(-1, -1);
                HandleUnitContact(g, cur);
                g->m_defenderState = AISTATE_SEEK;
                return 1;
            }
        }

        if (static_cast<u32>(g->m_dwell) <= static_cast<u32>(m_reserveBudget)) {
            return 1;
        }
        {
            Coord here;
            g->GetScreenPos((&here));
            i32 x5 = here.m_x >> TILE_SHIFT_PX;
            i32 y5 = here.m_y >> TILE_SHIFT_PX;
            Coord nbpos;
            nbpos = cur->GetTilePos();
            i32 dx = nbpos.m_x - x5;
            i32 dy = nbpos.m_y - y5;
            i32 adx = abs(dx);
            i32 ady = abs(dy);
            i32 dist = static_cast<i32>(sqrt(static_cast<double>(DistSq(adx, ady))));
            if (dist > m_assignedTargetMaxDistance) {
                if (g->CoordCount() != 0) {
                    MOVE_RECYCLE(g);
                }
                goto L_clearAt;
            }
            if (g->CoordCount() != 0) {
                MOVE_RECYCLE(g);
            }
            CGameObject* s = cur->m_object;
            if (g->TileSwitch(
                    s->m_screenX >> TILE_SHIFT_PX,
                    s->m_screenY >> TILE_SHIFT_PX,
                    0,
                    0xd87,
                    0,
                    0
                )
                != 0) {
                goto L_done;
            }
        }
    L_clearAt: {
        Coord none;
        g->m_arrivalCell = *none.Set(-1, -1);
        g->m_defenderState = AISTATE_SEEK;
    }
    L_done:
        g->m_dwell = 0;
        return 1;
    }

L_clear: {
    Coord none;
    g->m_arrivalCell = *none.Set(-1, -1);
    g->m_defenderState = AISTATE_SEEK;
    return 1;
}
}
}
#undef MOVE_RECYCLE

// @early-stop
RVA(0x00031c70, 0x1d)
Coord CGrunt::GetTilePos() {
    Coord out;
    CWwdSpriteObject* h = m_object;
    out.m_x = h->m_screenX >> TILE_SHIFT_PX;
    out.m_y = h->m_screenY >> TILE_SHIFT_PX;
    return out;
}

static inline i32 AddBattlezTraversalFlags(CGrunt* unit, i32 flags) {
    PickupType prim = unit->m_entranceReason;
    PickupType t = ArrivalPickupOf(unit, prim);
    if (t == PICKUP_TOOB) {
        flags |= BATTLEZ_ROUTE_TOOB_TRAVERSAL;
    } else {
        t = prim;
        if (prim > PICKUP_EQUIPPABLE_LAST) {
            t = unit->m_toolId;
        }
        if (t == PICKUP_SPRING) {
            flags |= BATTLEZ_ROUTE_SPRING_TRAVERSAL;
        } else {
            if (prim > PICKUP_EQUIPPABLE_LAST) {
                prim = unit->m_toolId;
            }
            if (prim == PICKUP_WINGZ) {
                flags |= BATTLEZ_ROUTE_WINGZ_TRAVERSAL;
            }
        }
    }
    return flags;
}

// @early-stop
RVA(0x00031ca0, 0x2f2)
i32 CBattlezMapConfig::TrackAssignedEnemy(CGrunt* unit) {
    if (unit->ArrivalCell().m_x != -1 && unit->ArrivalCell().m_y != -1) {
        CGrunt* target =
            m_triggerMgr
                ->m_units[unit->ArrivalCell().m_x * TM_UNITS_PER_PLAYER + unit->ArrivalCell().m_y];
        if (target != NULL) {
            CGameObject* lvl = target->m_object;
            if ((static_cast<CGrunt*>(unit))->RectContains(lvl->m_screenX, lvl->m_screenY) != 0) {
                if (unit->CoordCount() != 0) {
                    RECYCLE_GRUNT_COORDS_VIA_NEXTDATA(unit)
                }
                Coord none;
                unit->m_arrivalCell = *none.Set(-1, -1);
                HandleUnitContact(unit, target);
                return 1;
            }

            CMapMgr* board = m_board;
            CRect r1(0, 0, board->m_width, board->m_height);
            RECT rc;
            rc = CRect(0, 0, board->m_width, board->m_height);
            RECT* rcDst = &board->m_bounds;
            if (!IntersectRect(rcDst, &rc, &r1)) {
                *rcDst = rc;
            }
            board->m_gridW = rcDst->right - rcDst->left;
            board->m_gridH = rcDst->bottom - rcDst->top;
            if (static_cast<u32>(unit->m_dwell) > DWELL_REPATH_MS && unit->CoordCount() == 0) {
                i32 flags = unit->m_routeBlockedMask;
                unit->m_routePassableMask = BATTLEZ_ROUTE_ALL_TOOLS_TRIGGER;
                CGameObject* tl = target->m_object;
                unit->TileSwitch(
                    tl->m_screenX >> TILE_SHIFT_PX,
                    tl->m_screenY >> TILE_SHIFT_PX,
                    0,
                    flags,
                    0,
                    BATTLEZ_ROUTE_ALL_TOOLS_TRIGGER
                );
                unit->m_dwell = 0;
            }
            return 1;
        }

        Coord noCell;
        unit->m_arrivalCell = *noCell.Set(-1, -1);
        Coord noPx;
        unit->m_defenderPx = *noPx.Set(-1, -1);
        unit->m_defenderState = AISTATE_SEEK;
        unit->m_battleState = BZTASK_ADVANCE;
        if (unit->CoordCount() != 0) {
            RECYCLE_GRUNT_COORDS_EXPANDED(unit)
        }
        return 1;
    }

    Coord noCell;
    unit->m_arrivalCell = *noCell.Set(-1, -1);
    Coord noPx;
    unit->m_defenderPx = *noPx.Set(-1, -1);
    unit->m_defenderState = AISTATE_SEEK;
    unit->m_battleState = BZTASK_ADVANCE;
    if (unit->CoordCount() != 0) {
        RECYCLE_GRUNT_COORDS_VIA_NEXTDATA(unit)
    }
    return 1;
}

// @early-stop
RVA(0x00032060, 0x7bd)
i32 CBattlezMapConfig::AdvanceToEnemyBase(CGrunt* unit) {
    i32 defenderState = unit->m_defenderState;
    if (defenderState == AISTATE_RETURN) {
        return 1;
    }
    i32 band = unit->m_targetTeam;
    if (band == -1) {
        band = rand() % 4;
        if (band == m_playerIndex) {
            band++;
        }
        band = band % 4;
        GruntzPlayer* slot = &m_ctx->m_players[band];
        if (slot->m_clearedRound != false) {
            return 1;
        }
        if (slot->m_active == false) {
            return 1;
        }
        unit->m_targetTeam = band;
        Coord noPx;
        unit->m_defenderPx = *noPx.Set(-1, -1);
    } else {
        GruntzPlayer* slot = &m_ctx->m_players[band];
        if (slot->m_clearedRound != false || slot->m_active == false) {

            if (unit->CoordCount() != 0) {
                RECYCLE_GRUNT_COORDS_VIA_NEXTDATA(unit)
            }
            Coord noCell;
            unit->m_arrivalCell = *noCell.Set(-1, -1);
            Coord noPx;
            unit->m_defenderPx = *noPx.Set(-1, -1);
            unit->m_targetTeam = -1;
            unit->m_defenderState = AISTATE_SEEK;
            unit->m_routeBlockedMask = g_battlezRouteBlockedMask;
            unit->m_routePassableMask = g_battlezRoutePassableMask;
            return 1;
        }
    }
    band = unit->m_targetTeam;
    CBattlezMapConfig* bundle = &m_ctx->m_players[band].m_battlezConfig;
    Coord marker = bundle->m_marker;
    if (unit->CoordCount() == 0) {
        switch (unit->m_defenderState) {
            case AISTATE_SEEK: {
                unit->m_routeBlockedMask = g_battlezRouteBlockedMask;
                unit->m_routePassableMask = g_battlezRoutePassableMask;
                Coord goal = marker;
                Coord currentScreenPos = unit->m_defenderPx;
                i32 gx = currentScreenPos.m_x;
                if (gx == -1) {
                    if (bundle->m_attackWaypoints.GetSize() != 0) {
                        Coord out;
                        goal = *PickSpawnCoord(&out, unit, band);
                    }
                    unit->m_defenderPx = goal;
                    unit->m_defenderState = AISTATE_BATTLEZ_ROUTE_TARGET;
                    return 1;
                }
                goal = unit->m_defenderPx;
                (static_cast<CUserLogic*>(unit))->GetScreenPos((&currentScreenPos));
                i32 currentDx = abs(marker.m_x - (currentScreenPos.m_x >> TILE_SHIFT_PX));
                (static_cast<CUserLogic*>(unit))->GetScreenPos((&currentScreenPos));
                i32 currentDy = abs(marker.m_y - (currentScreenPos.m_y >> TILE_SHIFT_PX));
                i32 currentDistanceSquared = DistSq(currentDx, currentDy);
                i32 goalDx = abs(marker.m_x - goal.m_x);
                i32 goalDy = abs(marker.m_y - goal.m_y);
                i32 goalDistanceSquared = DistSq(goalDx, goalDy);
                if (currentDistanceSquared > goalDistanceSquared) {
                    unit->m_defenderState = AISTATE_BATTLEZ_ROUTE_TARGET;
                } else {
                    unit->m_defenderState = AISTATE_BATTLEZ_FINAL_ROUTE;
                    unit->m_routeBlockedMask = g_battlezRouteBlockedMask;
                    unit->m_routePassableMask = BATTLEZ_ROUTE_WINGZ_SHOVEL_EXPANDED;
                }
                return 1;
            }
            case AISTATE_BATTLEZ_ROUTE_TARGET: {
                if (static_cast<u32>(unit->m_dwell) <= static_cast<u32>(m_moveBudget)) {
                    return 1;
                }
                i32 gx = unit->m_defenderPx.m_x;
                i32 gy = unit->m_defenderPx.m_y;
                if (gx == -1 || gy == -1) {

                    unit->m_defenderState = AISTATE_SEEK;
                    if (unit->CoordCount() != 0) {
                        RECYCLE_GRUNT_COORDS(unit)
                    }
                    Coord noPx;
                    unit->m_defenderPx = *noPx.Set(-1, -1);
                    return 1;
                }
                CGameObject* lvl = unit->m_object;
                i32 dx = abs(gx - (lvl->m_screenX >> TILE_SHIFT_PX));
                i32 dy = abs(gy - (lvl->m_screenY >> TILE_SHIFT_PX));
                if (DistSq(dx, dy) > 0x10) {
                    i32 cfg = unit->m_routeBlockedMask;
                    i32 flags = AddBattlezTraversalFlags(unit, unit->m_routePassableMask);
                    Coord routeTarget = unit->m_defenderPx;
                    if (unit->TileSwitch(routeTarget.m_x, routeTarget.m_y, 0, cfg, 0, flags) != 0) {
                        goto routeSuccess;
                    }
                    i32 st = unit->m_routePassableMask;
                    if (st == g_battlezRoutePassableMask) {
                        unit->m_routePassableMask = BATTLEZ_ROUTE_WINGZ_SHOVEL;
                    } else if (st == BATTLEZ_ROUTE_WINGZ_SHOVEL) {
                        unit->m_routePassableMask = BATTLEZ_ROUTE_WINGZ_SHOVEL_EXPANDED;
                    } else if (st == BATTLEZ_ROUTE_WINGZ_SHOVEL_EXPANDED) {
                        unit->m_routePassableMask = BATTLEZ_ROUTE_OTHER_TOOLS;
                    } else if (st == BATTLEZ_ROUTE_OTHER_TOOLS) {
                        unit->m_routePassableMask = BATTLEZ_ROUTE_OTHER_TOOLS_EXPANDED;
                    } else if (st == BATTLEZ_ROUTE_OTHER_TOOLS_EXPANDED) {
                        unit->m_routePassableMask = BATTLEZ_ROUTE_ALL_TOOLS_EXPANDED;
                    } else if (st == BATTLEZ_ROUTE_ALL_TOOLS_EXPANDED) {
                        unit->m_routePassableMask = BATTLEZ_ROUTE_ALL_TOOLS_TRIGGER;
                    }
                    unit->m_dwell = 0;
                    return 1;
                }
                unit->m_defenderState = AISTATE_BATTLEZ_FINAL_ROUTE;
                unit->m_routeBlockedMask = g_battlezRouteBlockedMask;
                unit->m_routePassableMask = BATTLEZ_ROUTE_WINGZ_SHOVEL_EXPANDED;
                return 1;
            }
            case AISTATE_BATTLEZ_FINAL_ROUTE: {
                CMapMgr* board = m_board;
                RECT box2;
                box2.left = 0;
                box2.top = 0;
                i32 h = board->m_height;
                i32 w = board->m_width;
                box2.right = w;
                box2.bottom = h;
                RECT rc = CRect(0, 0, w, h);
                RECT* rcDst = &board->m_bounds;
                if (!IntersectRect(rcDst, &rc, &box2)) {
                    *rcDst = rc;
                }
                board->m_gridW = rcDst->right - rcDst->left;
                board->m_gridH = rcDst->bottom - rcDst->top;
                i32 flags = AddBattlezTraversalFlags(unit, unit->m_routePassableMask);
                if (unit->TileSwitch(marker.m_x, marker.m_y, 0, 0x987, 1, flags) != 0) {
                    goto routeSuccess;
                }
                unit->m_dwell = 0;
                unit->m_routePassableMask = BATTLEZ_ROUTE_ALL_TOOLS_TRIGGER;
                return 1;
            }
        }
        return 1;
    routeSuccess:
        unit->m_routeBlockedMask = g_battlezRouteBlockedMask;
        unit->m_routePassableMask = g_battlezRoutePassableMask;
        unit->m_dwell = 0;
        return 1;
    }
    if (unit->m_defenderState == AISTATE_SEEK) {
        return 1;
    }
    if (unit->m_defenderState != AISTATE_BATTLEZ_ROUTE_TARGET) {
        return 1;
    }
    i32 gx = unit->m_defenderPx.m_x;
    i32 gy = unit->m_defenderPx.m_y;
    if (gx == -1 || gy == -1) {

        unit->m_defenderState = AISTATE_SEEK;
        if (unit->CoordCount() != 0) {
            RECYCLE_GRUNT_COORDS_EXPANDED(unit)
        }
        Coord noPx;
        unit->m_defenderPx = *noPx.Set(-1, -1);
        return 1;
    }
    CGameObject* lvl = unit->m_object;
    i32 dx = abs(gx - (lvl->m_screenX >> TILE_SHIFT_PX));
    i32 dy = abs(gy - (lvl->m_screenY >> TILE_SHIFT_PX));
    if (DistSq(dx, dy) > 0x10) {
        return 1;
    }
    RECYCLE_GRUNT_COORDS_EXPANDED(unit)
    unit->m_defenderState = AISTATE_BATTLEZ_FINAL_ROUTE;
    unit->m_routeBlockedMask = g_battlezRouteBlockedMask;
    unit->m_routePassableMask = BATTLEZ_ROUTE_WINGZ_SHOVEL_EXPANDED;
    return 1;
}
