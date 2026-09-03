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
#include <Gruntz/GruntMovementInline.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntPickupInline.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MapCellFlags.h>
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
#include <Lith/BDefs.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/zBitVec.h>
#include <Wwd/WwdFile.h>

#include <limits.h>
#include <math.h>
#include <new>
#include <stdlib.h>
#include <string.h>

DATA(0x0022b7ec)
i32 g_battlezRoutePassableMask;

// @early-stop
RVA(0x00031610, 0x501)
i32 CBattlezMapConfig::Step(CGrunt* g) {
    if (g->CoordCount() == 0) {
        if (g->m_defenderState == AISTATE_ATTACK) {
            goto inflight;
        }

        CSize boardSize(m_board->m_width, m_board->m_height);
        Coord c0;
        g->GetScreenTile((&c0));
        CGrunt* nb = FindIdleGruntInBox(
            c0.m_x,
            c0.m_y,
            static_cast<i32>((static_cast<u32>(boardSize.cx) / 3)),
            static_cast<i32>((static_cast<u32>(boardSize.cy) / 3))
        );
        if (nb != NULL) {
            Coord c1;
            nb->GetScreenTile((&c1));
            if (g->TileSwitch(
                    c1.m_x,
                    c1.m_y,
                    IDX(CELL_FLAG_SOLID | CELL_FLAG_SPECIAL | CELL_FLAG_TRIGGER | CELL_FLAG_ARROW
                        | CELL_FLAG_SPIKES | CELL_FLAG_SINK_HAZARD),
                    0,
                    1,
                    0
                )
                == 0) {
                return 1;
            }
            g->m_arrivalCell.Set(nb->m_playerIndex, nb->m_unitIndex);
            g->m_defenderState = AISTATE_ATTACK;
            g->m_dwell = 0;
            AcceptAlways(g);
            return 1;
        }

        if (static_cast<u32>(g->m_dwell) > static_cast<u32>(m_idleRerouteDelay)) {
            Coord here;
            g->GetScreenTile(&here);
            RerouteIdleUnit(g, here.m_x, here.m_y, m_idleBurnRand.m_x, m_idleBurnRand.m_y, -1);
            if (g->CoordCount() > m_idleRouteLimit.m_y + m_idleRouteLimit.m_x
                && g->CoordCount() != 0) {
                RecycleGruntCoords(g);
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
    CSize boardSize(m_board->m_width, m_board->m_height);
    Coord c0;
    g->GetScreenTile((&c0));
    CGrunt* nb = FindIdleGruntInBox(
        c0.m_x,
        c0.m_y,
        static_cast<i32>((static_cast<u32>(boardSize.cx) / 3)),
        static_cast<i32>((static_cast<u32>(boardSize.cy) / 3))
    );

    if (cur == NULL) {
        goto L_clear;
    }
    if (nb != NULL && cur != nb) {
        if (g->CoordCount() != 0) {
            RecycleGruntCoords(g);
        }
        g->m_arrivalCell.Set(nb->m_playerIndex, nb->m_unitIndex);
        g->m_defenderState = AISTATE_ATTACK;
        g->m_dwell = 0;
        {
            Coord targetTile;
            nb->GetScreenTile(&targetTile);
            if (g->TileSwitch(
                    targetTile.m_x,
                    targetTile.m_y,
                    0,
                    IDX(CELL_FLAG_SOLID | CELL_FLAG_SPECIAL | CELL_FLAG_TRIGGER | CELL_FLAG_ARROW
                        | CELL_FLAG_SPIKES | CELL_FLAG_SINK_HAZARD),
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
            if (g->RectContains(s->m_screenPosition.m_x, s->m_screenPosition.m_y) != 0) {

                if (g->CoordCount() != 0) {
                    RecycleGruntCoords(g);
                }
                g->m_arrivalCell.Set(-1, -1);
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
            g->GetScreenTile(&here);
            Coord nbpos = cur->GetTilePos();
            i32 dist = nbpos.Dist(here);
            if (dist > m_assignedTargetMaxDistance) {
                if (g->CoordCount() != 0) {
                    RecycleGruntCoords(g);
                }
                goto L_clearAt;
            }
            if (g->CoordCount() != 0) {
                RecycleGruntCoords(g);
            }
            Coord targetTile;
            cur->GetScreenTile(&targetTile);
            if (g->TileSwitch(
                    targetTile.m_x,
                    targetTile.m_y,
                    0,
                    IDX(CELL_FLAG_SOLID | CELL_FLAG_SPECIAL | CELL_FLAG_TRIGGER | CELL_FLAG_ARROW
                        | CELL_FLAG_SPIKES | CELL_FLAG_SINK_HAZARD),
                    0,
                    0
                )
                != 0) {
                goto L_done;
            }
        }
    L_clearAt: {
        g->m_arrivalCell.Set(-1, -1);
        g->m_defenderState = AISTATE_SEEK;
    }
    L_done:
        g->m_dwell = 0;
        return 1;
    }

L_clear: {
    g->m_arrivalCell.Set(-1, -1);
    g->m_defenderState = AISTATE_SEEK;
    return 1;
}
}
}

RVA(0x00031c70, 0x1d)
Coord CGrunt::GetTilePos() {
    Coord out;
    GetScreenTile(&out);
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
            if ((static_cast<CGrunt*>(unit))
                    ->RectContains(lvl->m_screenPosition.m_x, lvl->m_screenPosition.m_y)
                != 0) {
                if (unit->CoordCount() != 0) {
                    RecycleGruntCoords(unit);
                }
                unit->m_arrivalCell.Set(-1, -1);
                HandleUnitContact(unit, target);
                return 1;
            }

            CMapMgr* board = m_board;
            board->Clip(NULL);
            if (static_cast<u32>(unit->m_dwell) > DWELL_REPATH_MS && unit->CoordCount() == 0) {
                i32 flags = unit->m_routeBlockedMask;
                unit->m_routePassableMask = BATTLEZ_ROUTE_ALL_TOOLS_TRIGGER;
                Coord targetTile;
                target->GetScreenTile(&targetTile);
                unit->TileSwitch(
                    targetTile.m_x,
                    targetTile.m_y,
                    0,
                    flags,
                    0,
                    BATTLEZ_ROUTE_ALL_TOOLS_TRIGGER
                );
                unit->m_dwell = 0;
            }
            return 1;
        }

        unit->m_arrivalCell.Set(-1, -1);
        unit->m_defenderPx.Set(-1, -1);
        unit->m_defenderState = AISTATE_SEEK;
        unit->m_battleState = BZTASK_ADVANCE;
        if (unit->CoordCount() != 0) {
            RecycleGruntCoords(unit);
        }
        return 1;
    }

    unit->m_arrivalCell.Set(-1, -1);
    unit->m_defenderPx.Set(-1, -1);
    unit->m_defenderState = AISTATE_SEEK;
    unit->m_battleState = BZTASK_ADVANCE;
    if (unit->CoordCount() != 0) {
        RecycleGruntCoords(unit);
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
        unit->m_defenderPx.Set(-1, -1);
    } else {
        GruntzPlayer* slot = &m_ctx->m_players[band];
        if (slot->m_clearedRound != false || slot->m_active == false) {

            if (unit->CoordCount() != 0) {
                RecycleGruntCoords(unit);
            }
            unit->m_arrivalCell.Set(-1, -1);
            unit->m_defenderPx.Set(-1, -1);
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
                        PickSpawnCoord(&goal, unit, band);
                    }
                    unit->m_defenderPx = goal;
                    unit->m_defenderState = AISTATE_BATTLEZ_ROUTE_TARGET;
                    return 1;
                }
                goal = unit->m_defenderPx;
                unit->GetScreenTile(&currentScreenPos);
                i32 currentDistanceSquared = marker.DistSqr(currentScreenPos);
                i32 goalDistanceSquared = marker.DistSqr(goal);
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
                Coord target = unit->m_defenderPx;
                if (target.m_x == -1 || target.m_y == -1) {

                    unit->m_defenderState = AISTATE_SEEK;
                    if (unit->CoordCount() != 0) {
                        RecycleGruntCoords(unit);
                    }
                    unit->m_defenderPx.Set(-1, -1);
                    return 1;
                }
                Coord current;
                unit->GetScreenTile(&current);
                if (target.DistSqr(current) > 0x10) {
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
                board->Clip(NULL);
                i32 flags = AddBattlezTraversalFlags(unit, unit->m_routePassableMask);
                if (unit->TileSwitch(
                        marker.m_x,
                        marker.m_y,
                        0,
                        IDX(CELL_FLAG_SOLID | CELL_FLAG_SPECIAL | CELL_FLAG_TRIGGER
                            | CELL_FLAG_ARROW | CELL_FLAG_WATER | CELL_FLAG_SINK_HAZARD),
                        1,
                        flags
                    )
                    != 0) {
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
    Coord target = unit->m_defenderPx;
    if (target.m_x == -1 || target.m_y == -1) {

        unit->m_defenderState = AISTATE_SEEK;
        if (unit->CoordCount() != 0) {
            RecycleGruntCoords(unit);
        }
        unit->m_defenderPx.Set(-1, -1);
        return 1;
    }
    Coord current;
    unit->GetScreenTile(&current);
    if (target.DistSqr(current) > 0x10) {
        return 1;
    }
    RecycleGruntCoords(unit);
    unit->m_defenderState = AISTATE_BATTLEZ_FINAL_ROUTE;
    unit->m_routeBlockedMask = g_battlezRouteBlockedMask;
    unit->m_routePassableMask = BATTLEZ_ROUTE_WINGZ_SHOVEL_EXPANDED;
    return 1;
}
