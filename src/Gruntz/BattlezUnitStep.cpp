#include <Gruntz/EnemyAiType.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/Play.h>
#include <Gruntz/TileCollisionKind.h>
#include <Gruntz/TileTriggerLogic.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Io/FileMem.h>
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Gruntz/TileActionEvent.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/BattlezDifficulty.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Wwd/WwdFile.h>
#include <Gruntz/GameLevel.h>
#include <rva.h>

#include <Gruntz/CoordNode.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/BattlezRouteMaskPreset.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/MapMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Wap32/zBitVec.h>
#include <Gruntz/ActReg.h>
#include <Bute/ButeMgr.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/SerialArchive.h>

#include <stdlib.h>
#include <math.h>
#pragma intrinsic(sqrt)
#include <string.h>
#include <new>
#include <Mfc.h>
#include <MfcNoInline.h>
#include <MfcWin.h>
#include <Gruntz/TileTriggerContainer.h>

#include <Gruntz/FreeNodePool.h>
#include <Wap32/TileGeometry.h>
#include <Gruntz/BattlezTask.h>
#include <Gruntz/BrickTileId.h>
#include <Gruntz/StaminaPct.h>
#include <limits.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/GruntDirStatics.h>

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
                        void* d = g->CoordListOps()->NextData(pos);
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

    i32 col = g->m_arrivalCell.m_x;
    i32 row = g->m_arrivalCell.m_y;
    CGrunt* cur = m_triggerMgr->m_grid[15 * col + row];
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

    if (cur == NULL) {
        goto L_clear;
    }
    {
        CGameObject* s = cur->m_object;
        if (g->RectContains(s->m_screenX, s->m_screenY) != 0) {

            if (g->CoordCount() != 0) {
                MOVE_RECYCLE(g);
            }
            g->m_arrivalCell.m_x = -1;
            g->m_arrivalCell.m_y = -1;
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
        i32 adx = dx < 0 ? -dx : dx;
        i32 ady = dy < 0 ? -dy : dy;
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
            g->m_dwell = 0;
            return 1;
        }
    }
L_clearAt:
    g->m_arrivalCell.m_x = -1;
    g->m_arrivalCell.m_y = -1;
    g->m_defenderState = AISTATE_SEEK;
    g->m_dwell = 0;
    return 1;

L_clear:
    g->m_arrivalCell.m_x = -1;
    g->m_defenderState = AISTATE_SEEK;
    g->m_arrivalCell.m_y = -1;
    return 1;
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
                        void* coord = unit->CoordListOps()->NextData(pos);
                        if (coord != NULL) {
                            g_coordPool.Push(coord);
                        }
                    }
                    unit->m_coordList.RemoveAll();
                }
                unit->m_arrivalCell.m_x = -1;
                unit->m_arrivalCell.m_y = -1;
                HandleUnitContact(unit, target);
                return 1;
            }

            CMapMgr* board = m_board;
            RECT r1;
            static_cast<RECT*>(new (&r1) CRect(0, 0, board->m_width, board->m_height));
            RECT r2;
            RECT* boardRect =
                static_cast<RECT*>(new (&r2) CRect(0, 0, board->m_width, board->m_height));
            RECT rc;
            rc.left = boardRect->left;
            rc.top = boardRect->top;
            rc.right = boardRect->right;
            rc.bottom = boardRect->bottom;
            if (!IntersectRect(&board->m_bounds, &rc, &r1)) {
                board->m_bounds = rc;
            }
            board->m_gridW = board->m_bounds.right - board->m_bounds.left;
            board->m_gridH = board->m_bounds.bottom - board->m_bounds.top;
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

        unit->m_arrivalCell.m_x = -1;
        unit->m_arrivalCell.m_y = -1;
        unit->m_defenderPx.m_x = -1;
        unit->m_defenderState = AISTATE_SEEK;
        unit->m_battleState = BZTASK_ADVANCE;
        unit->m_defenderPx.m_y = -1;
        if (unit->CoordCount() != 0) {
            CoordNode* n = unit->CoordHead();
            if (n != NULL) {
                do {
                    CoordNode* cur = n;
                    n = n->m_next;
                    void* coord = cur->m_coord;
                    if (coord != NULL) {
                        CoordPoolNode* slot = g_coordPool.NodeOf(coord);
                        slot->m_next = g_coordPool.m_freeHead;
                        g_coordPool.m_freeHead = slot;
                    }
                } while (n != NULL);
            }
            unit->m_coordList.RemoveAll();
        }
        return 1;
    }

    unit->m_arrivalCell.m_x = -1;
    unit->m_arrivalCell.m_y = -1;
    unit->m_defenderPx.m_x = -1;
    unit->m_defenderState = AISTATE_SEEK;
    unit->m_battleState = BZTASK_ADVANCE;
    unit->m_defenderPx.m_y = -1;
    if (unit->CoordCount() != 0) {
        POSITION pos = unit->m_coordList.GetHeadPosition();
        while (pos != NULL) {
            void* coord = unit->CoordListOps()->NextData(pos);
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
        if (m_ctx->m_options[band].m_clearedRound != 0) {
            return 1;
        }
        if (m_ctx->m_options[band].m_liveGate == 0) {
            return 1;
        }
        unit->m_targetTeam = band;
        unit->m_defenderPx.m_x = -1;
        unit->m_defenderPx.m_y = -1;
    } else {
        if (m_ctx->m_options[band].m_clearedRound != 0 || m_ctx->m_options[band].m_liveGate == 0) {

            if (unit->CoordCount() != 0) {
                POSITION pos = unit->m_coordList.GetHeadPosition();
                if (pos != NULL) {
                    do {
                        void* coord = unit->CoordListOps()->NextData(pos);
                        if (coord != NULL) {
                            g_coordPool.Push(coord);
                        }
                    } while (pos != NULL);
                }
                unit->m_coordList.RemoveAll();
            }
            unit->m_arrivalCell.m_x = -1;
            unit->m_arrivalCell.m_y = -1;
            unit->m_defenderPx.m_x = -1;
            unit->m_targetTeam = -1;
            unit->m_defenderPx.m_y = -1;
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
    if (unit->CoordCount() != 0) {
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
                        CoordPoolNode* node = g_coordPool.NodeOf(cur->m_coord);
                        node->m_next = g_coordPool.m_freeHead;
                        g_coordPool.m_freeHead = node;
                    }
                }
                unit->m_coordList.RemoveAll();
            }
            unit->m_defenderPx.m_x = -1;
            unit->m_defenderPx.m_y = -1;
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
                CoordPoolNode* node = g_coordPool.NodeOf(cur->m_coord);
                node->m_next = g_coordPool.m_freeHead;
                g_coordPool.m_freeHead = node;
            }
        }
        unit->m_coordList.RemoveAll();
        unit->m_defenderState = AISTATE_BATTLEZ_FINAL_ROUTE;
        unit->m_routeMaskA = g_spawnCfg;
        unit->m_routeMaskC = BATTLEZ_ROUTE_WINGZ_SHOVEL_EXPANDED;
        return 1;
    }
    if (unit->m_defenderState == AISTATE_SEEK) {
        unit->m_routeMaskA = g_spawnCfg;
        unit->m_routeMaskC = g_spawnState;
        i32 gx = unit->m_defenderPx.m_x;
        if (gx == -1) {
            i32 x, y;

            if (bundle->m_attackWaypoints.GetSize() != 0) {
                Coord out;
                Coord* r = static_cast<Coord*>(PickSpawnCoord(&out, unit, band));
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
        Coord c2;
        (static_cast<CUserLogic*>(unit))->GetScreenPos((&c2));
        i32 dyA = abs(ry - (c2.m_y >> TILE_SHIFT_PX));
        i32 distA = dxA * dxA + dyA * dyA;
        i32 dxB = abs(rx - gx);
        i32 dyB = abs(ry - gy);
        i32 distB = dxB * dxB + dyB * dyB;
        if (distA > distB) {
            unit->m_defenderState = AISTATE_BATTLEZ_ROUTE_TARGET;
        }
        return 1;
    }
    if (unit->m_defenderState == AISTATE_BATTLEZ_ROUTE_TARGET) {
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
            unit->m_defenderPx.m_x = -1;
            unit->m_defenderPx.m_y = -1;
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
    if (unit->m_defenderState != AISTATE_BATTLEZ_FINAL_ROUTE) {
        return 1;
    }
    CMapMgr* board = m_board;
    RECT box2;
    box2.left = 0;
    box2.top = 0;
    RECT bounds;
    RECT* bp = static_cast<RECT*>(new (&bounds) CRect(0, 0, board->m_width, board->m_height));
    box2.right = board->m_width;
    box2.bottom = board->m_height;
    RECT rc;
    rc.left = bp->left;
    rc.top = bp->top;
    rc.right = bp->right;
    rc.bottom = bp->bottom;
    if (!IntersectRect(&board->m_bounds, &rc, &box2)) {
        board->m_bounds = rc;
    }
    board->m_gridW = board->m_bounds.right - board->m_bounds.left;
    board->m_gridH = board->m_bounds.bottom - board->m_bounds.top;
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
