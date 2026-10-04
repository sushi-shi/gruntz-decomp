#include <StdAfx.h>

#include <rva.h>

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
#include <Gruntz/CoordPool.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntCoordRecycleMacros.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntMovementInline.h>
#include <Gruntz/GruntPickupInline.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MapMgr.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/Play.h>
#include <Gruntz/ScanGridMacros.h>
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
#include <Wwd/WwdFile.h>
#include <ZTools/BitVec.h>

#include <limits.h>
#include <math.h>
#include <new>
#include <stdlib.h>
#include <string.h>

DATA(0x0022b7ec)
i32 g_battlezRoutePassableMask;

RVA(0x00031610, 0x501)
i32 CBattlezMapConfig::Step(CGrunt* g) {
    if (g->CoordsEmpty()) {
        if (g->GetAiState() == AISTATE_ATTACK) {
            goto inflight;
        }

        i32 W = m_board->GetWidth();
        i32 H = m_board->GetHeight();
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
            if (g->MoveToTile(c1.m_x, c1.m_y, 0xd87, 0, 1, 0) == 0) {
                return 1;
            }
            g->SetAiAttackTarget(nb);
            AcceptAlways(g);
            return 1;
        }

        if (static_cast<u32>(g->GetDwell()) > static_cast<u32>(m_idleRerouteDelay)) {
            Coord here;
            g->GetScreenTile(&here);
            RerouteIdleUnit(g, here.m_x, here.m_y, m_idleBurnRandX, m_idleBurnRandY, -1);
            if (g->CoordCount() > m_idleRouteLimitY + m_idleRouteLimitX && !g->CoordsEmpty()) {
                g->RecycleCoords();
            }
            g->ResetDwell();
        }
        return 1;
    }

    if (g->GetAiState() != AISTATE_ATTACK) {
        return 1;
    }
inflight: {

    CGrunt* cur = m_triggerMgr->UnitAt(g->ArrivalCell().m_x, g->ArrivalCell().m_y);
    i32 W = m_board->GetWidth();
    i32 H = m_board->GetHeight();
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
        g->RecycleCoords();
        g->SetAiAttackTarget(nb);
        {
            if (g->MoveToTile(nb->GetScreenTileX(), nb->GetScreenTileY(), 0, 0xd87, 0, 0) == 0) {
                return 1;
            }
        }
        cur = nb;
    }

    if (cur != NULL) {
        {
            CGameObject* s = cur->m_object;
            if (g->IsWithinReach(s->m_screenX, s->m_screenY) != 0) {

                g->RecycleCoords();
                UNSET_COORD(g->m_arrivalCell);
                HandleUnitContact(g, cur);
                g->SetAiState(AISTATE_SEEK);
                return 1;
            }
        }

        if (static_cast<u32>(g->GetDwell()) <= static_cast<u32>(m_reserveBudget)) {
            return 1;
        }
        {
            Coord here;
            g->GetScreenTile(&here);
            i32 x5 = here.m_x;
            i32 y5 = here.m_y;
            Coord nbpos;
            nbpos = cur->GetTilePos();
            i32 dx = nbpos.m_x - x5;
            i32 dy = nbpos.m_y - y5;
            i32 adx = abs(dx);
            i32 ady = abs(dy);
            i32 dist = static_cast<i32>(sqrt(static_cast<double>(SquaredDistance(adx, ady))));
            if (dist > m_assignedTargetMaxDistance) {
                g->RecycleCoords();
                goto L_clearAt;
            }
            g->RecycleCoords();
            if (g->MoveToTile(cur->GetScreenTileX(), cur->GetScreenTileY(), 0, 0xd87, 0, 0) != 0) {
                goto L_done;
            }
        }
    L_clearAt:
        g->ResetToSeek();
    L_done:
        g->ResetDwell();
        return 1;
    }

L_clear: {
    g->m_arrivalCell.m_x = -1;
    g->SetAiState(AISTATE_SEEK);
    g->m_arrivalCell.m_y = -1;
    return 1;
}
}
}
#undef MOVE_RECYCLE

RVA(0x00031c70, 0x1d)
Coord CGrunt::GetTilePos() {
    Coord out;
    CWwdSpriteObject* object = m_object;
    out.Set(object->m_screenX, object->m_screenY);
    ScreenTile(&out);
    return out;
}

RVA(0x00031ca0, 0x2f2)
i32 CBattlezMapConfig::TrackAssignedEnemy(CGrunt* unit) {
    if (unit->ArrivalCell().m_x != -1 && unit->ArrivalCell().m_y != -1) {
        CGrunt* target = m_triggerMgr->UnitAt(unit->ArrivalCell().m_x, unit->ArrivalCell().m_y);
        if (target != NULL) {
            CGameObject* lvl = target->m_object;
            if ((static_cast<CGrunt*>(unit))->IsWithinReach(lvl->m_screenX, lvl->m_screenY) != 0) {
                unit->RecycleCoords();
                UNSET_COORD(unit->m_arrivalCell);
                HandleUnitContact(unit, target);
                return 1;
            }

            CMapMgr* board = m_board;
            board->Clip(NULL);
            if (static_cast<u32>(unit->GetDwell()) > DWELL_REPATH_MS && unit->CoordsEmpty()) {
                i32 flags = unit->GetRouteBlockedMask();
                unit->SetRoutePassableMask(BATTLEZ_ROUTE_ALL_TOOLS_TRIGGER);
                CGameObject* tl = target->m_object;
                unit->MoveToTile(
                    tl->m_screenX >> TILE_SHIFT_PX,
                    tl->m_screenY >> TILE_SHIFT_PX,
                    0,
                    flags,
                    0,
                    BATTLEZ_ROUTE_ALL_TOOLS_TRIGGER
                );
                unit->ResetDwell();
            }
            return 1;
        }

        UNSET_COORD(unit->m_arrivalCell);
        UNSET_COORD(unit->m_defenderPx);
        unit->SetAiState(AISTATE_SEEK);
        unit->SetBattlezTask(BZTASK_ADVANCE);
        unit->RecycleCoords();
        return 1;
    }

    UNSET_COORD(unit->m_arrivalCell);
    UNSET_COORD(unit->m_defenderPx);
    unit->SetAiState(AISTATE_SEEK);
    unit->SetBattlezTask(BZTASK_ADVANCE);
    unit->RecycleCoords();
    return 1;
}

RVA(0x00032060, 0x7bd)
i32 CBattlezMapConfig::AdvanceToEnemyBase(CGrunt* unit) {
    i32 aiState = unit->GetAiState();
    if (aiState == AISTATE_RETURN) {
        return 1;
    }
    i32 targetPlayerIndex = unit->GetBattlezTargetPlayerIndex();
    if (targetPlayerIndex == -1) {
        targetPlayerIndex = GetRandom(3);
        if (targetPlayerIndex == m_playerIndex) {
            targetPlayerIndex++;
        }
        targetPlayerIndex = targetPlayerIndex % 4;
        GruntzPlayer* slot = &m_ctx->GetPlayer(targetPlayerIndex);
        if (slot->IsEliminated() != false) {
            return 1;
        }
        if (slot->IsActive() == false) {
            return 1;
        }
        unit->SetBattlezTargetPlayerIndex(targetPlayerIndex);
        UNSET_COORD(unit->m_defenderPx);
    } else {
        GruntzPlayer* slot = &m_ctx->GetPlayer(targetPlayerIndex);
        if (slot->IsEliminated() != false || slot->IsActive() == false) {

            unit->RecycleCoords();
            UNSET_COORD(unit->m_arrivalCell);
            UNSET_COORD(unit->m_defenderPx);
            unit->SetBattlezTargetPlayerIndex(-1);
            unit->SetAiState(AISTATE_SEEK);
            unit->SetRouteBlockedMask(g_battlezRouteBlockedMask);
            unit->SetRoutePassableMask(g_battlezRoutePassableMask);
            return 1;
        }
    }
    targetPlayerIndex = unit->GetBattlezTargetPlayerIndex();
    CBattlezMapConfig* bundle = m_ctx->GetPlayer(targetPlayerIndex).GetBattlezConfig();
    Coord marker = bundle->GetBaseTile();
    if (unit->CoordsEmpty()) {
        switch (unit->GetAiState()) {
            case AISTATE_SEEK: {
                unit->SetRouteBlockedMask(g_battlezRouteBlockedMask);
                unit->SetRoutePassableMask(g_battlezRoutePassableMask);
                Coord goal = marker;
                Coord currentScreenPos = unit->DefenderPosition();
                i32 gx = currentScreenPos.m_x;
                if (gx == -1) {
                    if (bundle->GetAttackWaypointCount() != 0) {
                        Coord out;
                        goal = *PickSpawnCoord(&out, unit, targetPlayerIndex);
                    }
                    unit->m_defenderPx = goal;
                    unit->SetAiState(AISTATE_BATTLEZ_ROUTE_TARGET);
                    return 1;
                }
                goal = unit->DefenderPosition();
                unit->GetScreenPos(&currentScreenPos);
                i32 currentDx = abs(marker.m_x - (currentScreenPos.m_x >> TILE_SHIFT_PX));
                unit->GetScreenPos(&currentScreenPos);
                i32 currentDy = abs(marker.m_y - (currentScreenPos.m_y >> TILE_SHIFT_PX));
                i32 currentDistanceSquared = SquaredDistance(currentDx, currentDy);
                i32 goalDx = abs(marker.m_x - goal.m_x);
                i32 goalDy = abs(marker.m_y - goal.m_y);
                i32 goalDistanceSquared = SquaredDistance(goalDx, goalDy);
                if (currentDistanceSquared > goalDistanceSquared) {
                    unit->SetAiState(AISTATE_BATTLEZ_ROUTE_TARGET);
                } else {
                    unit->SetAiState(AISTATE_BATTLEZ_FINAL_ROUTE);
                    unit->SetRouteBlockedMask(g_battlezRouteBlockedMask);
                    unit->SetRoutePassableMask(BATTLEZ_ROUTE_WINGZ_SHOVEL_EXPANDED);
                }
                return 1;
            }
            case AISTATE_BATTLEZ_ROUTE_TARGET: {
                if (static_cast<u32>(unit->GetDwell()) <= static_cast<u32>(m_moveBudget)) {
                    return 1;
                }
                Coord defender = unit->DefenderPosition();
                i32 gx = defender.m_x;
                i32 gy = defender.m_y;
                if (gx == -1 || gy == -1) {

                    unit->SetAiState(AISTATE_SEEK);
                    unit->RecycleCoords();
                    UNSET_COORD(unit->m_defenderPx);
                    return 1;
                }
                i32 dx = abs(gx - unit->GetScreenTileX());
                i32 dy = abs(gy - unit->GetScreenTileY());
                if (SquaredDistance(dx, dy) > 0x10) {
                    i32 cfg = unit->GetRouteBlockedMask();
                    i32 flags = unit->GetRoutePassableMask();
                    ADD_BATTLEZ_TRAVERSAL_FLAGS(unit, flags);
                    Coord routeTarget = unit->DefenderPosition();
                    if (unit->MoveToTile(routeTarget.m_x, routeTarget.m_y, 0, cfg, 0, flags) != 0) {
                        goto routeSuccess;
                    }
                    i32 st = unit->GetRoutePassableMask();
                    if (st == g_battlezRoutePassableMask) {
                        unit->SetRoutePassableMask(BATTLEZ_ROUTE_WINGZ_SHOVEL);
                    } else if (st == BATTLEZ_ROUTE_WINGZ_SHOVEL) {
                        unit->SetRoutePassableMask(BATTLEZ_ROUTE_WINGZ_SHOVEL_EXPANDED);
                    } else if (st == BATTLEZ_ROUTE_WINGZ_SHOVEL_EXPANDED) {
                        unit->SetRoutePassableMask(BATTLEZ_ROUTE_OTHER_TOOLS);
                    } else if (st == BATTLEZ_ROUTE_OTHER_TOOLS) {
                        unit->SetRoutePassableMask(BATTLEZ_ROUTE_OTHER_TOOLS_EXPANDED);
                    } else if (st == BATTLEZ_ROUTE_OTHER_TOOLS_EXPANDED) {
                        unit->SetRoutePassableMask(BATTLEZ_ROUTE_ALL_TOOLS_EXPANDED);
                    } else if (st == BATTLEZ_ROUTE_ALL_TOOLS_EXPANDED) {
                        unit->SetRoutePassableMask(BATTLEZ_ROUTE_ALL_TOOLS_TRIGGER);
                    }
                    unit->ResetDwell();
                    return 1;
                }
                unit->SetAiState(AISTATE_BATTLEZ_FINAL_ROUTE);
                unit->SetRouteBlockedMask(g_battlezRouteBlockedMask);
                unit->SetRoutePassableMask(BATTLEZ_ROUTE_WINGZ_SHOVEL_EXPANDED);
                return 1;
            }
            case AISTATE_BATTLEZ_FINAL_ROUTE: {
                CMapMgr* board = m_board;
                board->Clip(NULL);
                i32 flags = unit->GetRoutePassableMask();
                ADD_BATTLEZ_TRAVERSAL_FLAGS(unit, flags);
                if (unit->MoveToTile(marker.m_x, marker.m_y, 0, 0x987, 1, flags) != 0) {
                    goto routeSuccess;
                }
                unit->ResetDwell();
                unit->SetRoutePassableMask(BATTLEZ_ROUTE_ALL_TOOLS_TRIGGER);
                return 1;
            }
        }
        return 1;
    routeSuccess:
        unit->SetRouteBlockedMask(g_battlezRouteBlockedMask);
        unit->SetRoutePassableMask(g_battlezRoutePassableMask);
        unit->ResetDwell();
        return 1;
    }
    if (unit->GetAiState() == AISTATE_SEEK) {
        return 1;
    }
    if (unit->GetAiState() != AISTATE_BATTLEZ_ROUTE_TARGET) {
        return 1;
    }
    Coord defender = unit->DefenderPosition();
    i32 gx = defender.m_x;
    i32 gy = defender.m_y;
    if (gx == -1 || gy == -1) {

        unit->SetAiState(AISTATE_SEEK);
        unit->RecycleCoords();
        UNSET_COORD(unit->m_defenderPx);
        return 1;
    }
    i32 dx = abs(gx - unit->GetScreenTileX());
    i32 dy = abs(gy - unit->GetScreenTileY());
    if (SquaredDistance(dx, dy) > 0x10) {
        return 1;
    }
    unit->RecycleCoords();
    unit->SetAiState(AISTATE_BATTLEZ_FINAL_ROUTE);
    unit->SetRouteBlockedMask(g_battlezRouteBlockedMask);
    unit->SetRoutePassableMask(BATTLEZ_ROUTE_WINGZ_SHOVEL_EXPANDED);
    return 1;
}
