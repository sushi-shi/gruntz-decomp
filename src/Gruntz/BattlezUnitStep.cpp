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
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntSpawnConfig.h>
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

DATA(0x0022b7ec)
i32 g_spawnState;

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
        g->GetScreenPos((&c0));
        c0.m_x >>= 5;
        c0.m_y >>= 5;
        CGrunt* nb = FindIdleGruntInBox(
            c0.m_x,
            c0.m_y,
            static_cast<i32>((static_cast<u32>(W) / 3)),
            static_cast<i32>((static_cast<u32>(H) / 3))
        );
        if (nb != NULL) {
            Coord c1;
            nb->GetScreenPos((&c1));
            c1.m_x >>= 5;
            c1.m_y >>= 5;
            if (g->TileSwitch(c1.m_x, c1.m_y, 0xd87, 0, 1, 0) == 0) {
                return 1;
            }
            g->m_arrivalCell.m_x = nb->m_tileOwnerHi;
            g->m_arrivalCell.m_y = nb->m_tileOwnerLo;
            g->m_defenderState = AISTATE_ATTACK;
            g->m_dwell = 0;
            AcceptAlways(g);
            return 1;
        }

        if (static_cast<u32>(g->m_dwell) > static_cast<u32>(m_idleRerouteDelay)) {
            Coord here;
            g->GetScreenPos((&here));
            TileSwitch(
                g,
                here.m_x >> TILE_SHIFT_PX,
                here.m_y >> TILE_SHIFT_PX,
                m_idleBurnRandX,
                m_idleBurnRandY,
                -1
            );
            if (g->CoordCount() > m_idleRouteLimitY + m_idleRouteLimitX && g->CoordCount() != 0) {
                POSITION pos = g->m_coordList.GetHeadPosition();
                if (pos != NULL) {
                    do {
                        Coord* d = static_cast<Coord*>(g->CoordListOps()->NextData(pos));
                        if (d != NULL) {
                            g_coordPool.Push(d);
                        }
                    } while (pos != NULL);
                }
                g->m_coordList.RemoveAll();
            }
            g->m_dwell = 0;
        }
        return 1;
    }

    if (g->m_defenderState != AISTATE_ATTACK) {
        return 1;
    }
inflight: {

    Coord arrivalCell = g->m_arrivalCell;
    CGrunt* cur = m_triggerMgr->m_grid[15 * arrivalCell.m_x + arrivalCell.m_y];
    i32 W = m_board->m_width;
    i32 H = m_board->m_height;
    Coord c0;
    g->GetScreenPos((&c0));
    c0.m_x >>= 5;
    c0.m_y >>= 5;
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
        g->m_arrivalCell.m_x = nb->m_tileOwnerHi;
        g->m_arrivalCell.m_y = nb->m_tileOwnerLo;
        g->m_defenderState = AISTATE_ATTACK;
        g->m_dwell = 0;
        {
            CGameObject* s = static_cast<CGameObject*>(nb->m_object);
            if (g->TileSwitch(
                    s->m_screenX >> TILE_SHIFT_PX,
                    s->m_screenY >> TILE_SHIFT_PX,
                    0xd87,
                    0,
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
            i32 dist = static_cast<i32>(sqrt(static_cast<double>((adx * adx + ady * ady))));
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
                    0xd87,
                    0,
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
// cl keeps the hidden return pointer in eax and writes through it; retail parks
// it in edx and copies (`mov eax,edx`) at the end. The member load order
// (m_screenX then m_screenY) only comes out right in the by-value form.
RVA(0x00031c70, 0x1d)
Coord CGrunt::GetTilePos() {
    Coord out;
    CWwdGameObjectA* h = m_object;
    out.m_x = h->m_screenX >> TILE_SHIFT_PX;
    out.m_y = h->m_screenY >> TILE_SHIFT_PX;
    return out;
}

#include <Gruntz/FreeNodePoolInline.h>

// @early-stop
RVA(0x00031ca0, 0x2f2)
i32 CBattlezMapConfig::TrackAssignedEnemy(CGrunt* unit) {
    i32 tx = unit->m_arrivalCell.m_x;
    i32 ty = unit->m_arrivalCell.m_y;
    if (tx != -1 && ty != -1) {
        CGrunt* target = m_triggerMgr->m_grid[tx * 15 + ty];
        if (target != NULL) {
            CGameObject* lvl = target->m_object;
            if ((static_cast<CGrunt*>(unit))->RectContains(lvl->m_screenX, lvl->m_screenY) != 0) {
                if (unit->CoordCount() != 0) {
                    POSITION pos = unit->m_coordList.GetHeadPosition();
                    while (pos != NULL) {
                        Coord* coord = static_cast<Coord*>(unit->CoordListOps()->NextData(pos));
                        if (coord != NULL) {
                            g_coordPool.Push(coord);
                        }
                    }
                    unit->m_coordList.RemoveAll();
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
                i32 flags = unit->m_routeMaskA;
                unit->m_routeMaskC = BATTLEZ_ROUTE_ALL_TOOLS_TRIGGER;
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
            CoordNode* n = unit->CoordHead();
            if (n != NULL) {
                do {
                    CoordNode* cur = n;
                    n = n->m_next;
                    Coord* coord = cur->m_coord;
                    if (coord != NULL) {
                        PushFreeNode(&g_coordPool, coord);
                    }
                } while (n != NULL);
            }
            unit->m_coordList.RemoveAll();
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
        POSITION pos = unit->m_coordList.GetHeadPosition();
        while (pos != NULL) {
            Coord* coord = static_cast<Coord*>(unit->CoordListOps()->NextData(pos));
            if (coord != NULL) {
                g_coordPool.Push(coord);
            }
        }
        unit->m_coordList.RemoveAll();
    }
    return 1;
}

// @early-stop
RVA(0x00032060, 0x7bd)
i32 CBattlezMapConfig::AdvanceToEnemyBase(CGrunt* unit) {
    if (unit->m_defenderState == AISTATE_RETURN) {
        return 1;
    }
    i32 band = unit->m_targetTeam;
    if (band == -1) {
        band = rand() % 4;
        if (band == m_ownerId) {
            band++;
        }
        band = band % 4;
        GruntzPlayer* slot = &m_ctx->m_options[band];
        if (slot->m_clearedRound != 0) {
            return 1;
        }
        if (slot->m_liveGate == 0) {
            return 1;
        }
        unit->m_targetTeam = band;
        Coord noPx;
        unit->m_defenderPx = *noPx.Set(-1, -1);
    } else {
        GruntzPlayer* slot = &m_ctx->m_options[band];
        if (slot->m_clearedRound != 0 || slot->m_liveGate == 0) {

            if (unit->CoordCount() != 0) {
                POSITION pos = unit->m_coordList.GetHeadPosition();
                if (pos != NULL) {
                    do {
                        Coord* coord = static_cast<Coord*>(unit->CoordListOps()->NextData(pos));
                        if (coord != NULL) {
                            g_coordPool.Push(coord);
                        }
                    } while (pos != NULL);
                }
                unit->m_coordList.RemoveAll();
            }
            Coord noCell;
            unit->m_arrivalCell = *noCell.Set(-1, -1);
            Coord noPx;
            unit->m_defenderPx = *noPx.Set(-1, -1);
            unit->m_targetTeam = -1;
            unit->m_defenderState = AISTATE_SEEK;
            unit->m_routeMaskA = g_spawnCfg;
            unit->m_routeMaskC = g_spawnState;
            return 1;
        }
    }
    band = unit->m_targetTeam;
    CBattlezMapConfig* bundle = &m_ctx->m_options[band].m_battlezConfig;
    i32 rx = bundle->m_marker.m_x;
    i32 ry = bundle->m_marker.m_y;
    if (unit->CoordCount() == 0) {
        switch (unit->m_defenderState) {
            case AISTATE_SEEK: {
                unit->m_routeMaskA = g_spawnCfg;
                unit->m_routeMaskC = g_spawnState;
                i32 gx = unit->m_defenderPx.m_x;
                if (gx == -1) {
                    i32 x, y;

                    if (bundle->m_attackWaypoints.GetSize() != 0) {
                        Coord out;
                        Coord* r = PickSpawnCoord(&out, unit, band);
                        x = r->m_x;
                        y = r->m_y;
                    } else {
                        x = rx;
                        y = ry;
                    }
                    unit->m_defenderPx.m_x = x;
                    unit->m_defenderPx.m_y = y;
                    unit->m_defenderState = AISTATE_BATTLEZ_ROUTE_TARGET;
                    return 1;
                }
                i32 gy = unit->m_defenderPx.m_y;
                Coord c1;
                (static_cast<CUserLogic*>(unit))->GetScreenPos((&c1));
                i32 dxA = abs(rx - (c1.m_x >> TILE_SHIFT_PX));
                (static_cast<CUserLogic*>(unit))->GetScreenPos((&c1));
                i32 dyA = abs(ry - (c1.m_y >> TILE_SHIFT_PX));
                i32 distA = dxA * dxA + dyA * dyA;
                i32 dxB = abs(rx - gx);
                i32 dyB = abs(ry - gy);
                i32 distB = dxB * dxB + dyB * dyB;
                if (distA > distB) {
                    unit->m_defenderState = AISTATE_BATTLEZ_ROUTE_TARGET;
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
                        CoordNode* n = unit->CoordHead();
                        while (n != NULL) {
                            CoordNode* cur = n;
                            n = n->m_next;
                            if (cur->m_coord != NULL) {
                                g_coordPool.Push(cur->m_coord);
                            }
                        }
                        unit->m_coordList.RemoveAll();
                    }
                    Coord noPx;
                    unit->m_defenderPx = *noPx.Set(-1, -1);
                    return 1;
                }
                CGameObject* lvl = unit->m_object;
                i32 dx = abs(gx - (lvl->m_screenX >> TILE_SHIFT_PX));
                i32 dy = abs(gy - (lvl->m_screenY >> TILE_SHIFT_PX));
                if (dx * dx + dy * dy <= 0x10) {
                    unit->m_defenderState = AISTATE_BATTLEZ_FINAL_ROUTE;
                    unit->m_routeMaskA = g_spawnCfg;
                    unit->m_routeMaskC = BATTLEZ_ROUTE_WINGZ_SHOVEL_EXPANDED;
                    return 1;
                }
                PickupType prim = unit->m_entranceReason;
                i32 cfg = unit->m_routeMaskA;
                i32 flags = unit->m_routeMaskC;
                PickupType t = prim;
                if (prim > PICKUP_EQUIPPABLE_LAST) {
                    t = unit->m_toolId;
                }
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
                if (unit->TileSwitch(gx, gy, 0, cfg, 0, flags) != 0) {
                    unit->m_routeMaskA = g_spawnCfg;
                    unit->m_routeMaskC = g_spawnState;
                    unit->m_dwell = 0;
                    return 1;
                }
                i32 st = unit->m_routeMaskC;
                if (st == g_spawnState) {
                    unit->m_routeMaskC = BATTLEZ_ROUTE_WINGZ_SHOVEL;
                } else if (st == BATTLEZ_ROUTE_WINGZ_SHOVEL) {
                    unit->m_routeMaskC = BATTLEZ_ROUTE_WINGZ_SHOVEL_EXPANDED;
                } else if (st == BATTLEZ_ROUTE_WINGZ_SHOVEL_EXPANDED) {
                    unit->m_routeMaskC = BATTLEZ_ROUTE_OTHER_TOOLS;
                } else if (st == BATTLEZ_ROUTE_OTHER_TOOLS) {
                    unit->m_routeMaskC = BATTLEZ_ROUTE_OTHER_TOOLS_EXPANDED;
                } else if (st == BATTLEZ_ROUTE_OTHER_TOOLS_EXPANDED) {
                    unit->m_routeMaskC = BATTLEZ_ROUTE_ALL_TOOLS_EXPANDED;
                } else if (st == BATTLEZ_ROUTE_ALL_TOOLS_EXPANDED) {
                    unit->m_routeMaskC = BATTLEZ_ROUTE_ALL_TOOLS_TRIGGER;
                }
                unit->m_dwell = 0;
                return 1;
            }
            case AISTATE_BATTLEZ_FINAL_ROUTE: {
                CMapMgr* board = m_board;
                i32 h = board->m_height;
                i32 w = board->m_width;
                RECT box2;
                box2.left = 0;
                box2.top = 0;
                box2.right = w;
                box2.bottom = h;
                RECT rc = CRect(0, 0, w, h);
                RECT* rcDst = &board->m_bounds;
                if (!IntersectRect(rcDst, &rc, &box2)) {
                    *rcDst = rc;
                }
                board->m_gridW = rcDst->right - rcDst->left;
                board->m_gridH = rcDst->bottom - rcDst->top;
                PickupType prim = unit->m_entranceReason;
                i32 flags = unit->m_routeMaskC;
                PickupType t = prim;
                if (prim > PICKUP_EQUIPPABLE_LAST) {
                    t = unit->m_toolId;
                }
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
                if (unit->TileSwitch(rx, ry, 0, 0x987, 1, flags) != 0) {
                    unit->m_routeMaskA = g_spawnCfg;
                    unit->m_routeMaskC = g_spawnState;
                    unit->m_dwell = 0;
                    return 1;
                }
                unit->m_dwell = 0;
                unit->m_routeMaskC = BATTLEZ_ROUTE_ALL_TOOLS_TRIGGER;
                return 1;
            }
        }
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
            CoordNode* n = unit->CoordHead();
            while (n != NULL) {
                CoordNode* cur = n;
                n = n->m_next;
                if (cur->m_coord != NULL) {
                    PushFreeNode(&g_coordPool, cur->m_coord);
                }
            }
            unit->m_coordList.RemoveAll();
        }
        Coord noPx;
        unit->m_defenderPx = *noPx.Set(-1, -1);
        return 1;
    }
    CGameObject* lvl = unit->m_object;
    i32 dx = abs(gx - (lvl->m_screenX >> TILE_SHIFT_PX));
    i32 dy = abs(gy - (lvl->m_screenY >> TILE_SHIFT_PX));
    if (dx * dx + dy * dy > 0x10) {
        return 1;
    }
    CoordNode* n = unit->CoordHead();
    while (n != NULL) {
        CoordNode* cur = n;
        n = n->m_next;
        if (cur->m_coord != NULL) {
            PushFreeNode(&g_coordPool, cur->m_coord);
        }
    }
    unit->m_coordList.RemoveAll();
    unit->m_defenderState = AISTATE_BATTLEZ_FINAL_ROUTE;
    unit->m_routeMaskA = g_spawnCfg;
    unit->m_routeMaskC = BATTLEZ_ROUTE_WINGZ_SHOVEL_EXPANDED;
    return 1;
    return 1;
}
