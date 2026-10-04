#include <StdAfx.h>

#include <rva.h>

#include <Gruntz/BattlezGruntInline.h>
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CoordPool.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntCoordRecycleMacros.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntMovementInline.h>
#include <Gruntz/ScanGridMacros.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeColl.h>
#include <Gruntz/TypeKeyColl.h>
#include <Ints.h>
#include <Lith/BDefs.h>
#include <Wap32/TileGeometry.h>
#include <ZTools/ZDArray.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

RVA(0x00033520, 0xbc3)
i32 CBattlezMapConfig::StepDefenderUnit(CGrunt* defender) {
    GruntAiState state = defender->GetDefenderState();
    if (state == AISTATE_RETURN) {
        return 1;
    }
    if (state != AISTATE_ATTACK) {

        CGrunt* target;
        {
            Coord searchTile;
            defender->GetScreenTile(&searchTile);
            target = FindIdleGruntInBox(
                searchTile.m_x,
                searchTile.m_y,
                m_defenderSearchRadiusX,
                m_defenderSearchRadiusY
            );
        }
        if (target != NULL) {
            defender->RecycleCoords();

            i32 arrivalMask = 0xdc7;
            i32 manhattanDistance;
            {
                Coord targetXTile;
                target->GetScreenTile(&targetXTile);
                Coord defenderXTile;
                defender->GetScreenTile(&defenderXTile);
                Coord targetYTile;
                target->GetScreenTile(&targetYTile);
                Coord defenderYTile;
                defender->GetScreenTile(&defenderYTile);
                manhattanDistance = abs(targetYTile.m_y - defenderYTile.m_y)
                                    + abs(targetXTile.m_x - defenderXTile.m_x);
            }
            if (manhattanDistance <= 0xa) {

                CRect searchBounds(
                    defender->ScanCell().m_x - 5,
                    defender->ScanCell().m_y - 5,
                    defender->ScanCell().m_x + 5,
                    defender->ScanCell().m_y + 5
                );
                CMapMgr* grid = m_board;
                arrivalMask = 0x20000dc7;
                grid->Clip(&searchBounds);
            }
            {
                Coord targetTile;
                target->GetScreenTile(&targetTile);
                if (defender->TileSwitch(targetTile.m_x, targetTile.m_y, 0, arrivalMask, 0, 0)) {
                    defender->SetDefenderState(AISTATE_ATTACK);
                    defender->m_arrivalCell.Set(target->GetPlayerIndex(), target->GetUnitIndex());
                    defender->m_dwell = 0;
                }
            }
            if (manhattanDistance <= 0xa) {
                m_board->Clip(NULL);
            }
        }
        goto checkIdleWander;
    }

    {
        CGrunt* target =
            m_triggerMgr->UnitAt(defender->ArrivalCell().m_x, defender->ArrivalCell().m_y);
        if (target != NULL) {
            CGameObject* targetSprite = target->m_object;
            if (defender->RectContains(targetSprite->m_screenX, targetSprite->m_screenY) != 0) {

                defender->RecycleCoords();
                UNSET_COORD(defender->m_arrivalCell);
                if (defender != NULL && defender->IsAtSavedScreenPos()
                    && defender->m_entranceCommitted != false
                    && defender->IsDeathAnimationStarted() == false
                    && defender->m_entranceActive == false && defender->m_poweredUp == false
                    && BattlezActDiffersFromIGLPJCR(defender)) {
                    HandleUnitContact(defender, target);
                }
                defender->SetDefenderState(AISTATE_SEEK);
                goto checkIdleWander;
            }

            i32 targetDistance;
            {
                Coord defenderTile = defender->GetTilePos();
                Coord targetTile = target->GetTilePos();
                i32 dx = targetTile.m_x - defenderTile.m_x;
                i32 dy = targetTile.m_y - defenderTile.m_y;
                targetDistance =
                    static_cast<i32>(sqrt(static_cast<double>((SQR(abs(dx)) + SQR(abs(dy))))));
            }
            if (targetDistance > m_defenderTargetMaxDistance) {
                if (GetAttackWaypointCount() != 0) {
                    Coord* attackWaypoint = CoordAt(rand() % GetAttackWaypointCount());
                    defender->TileSwitch(attackWaypoint->m_x, attackWaypoint->m_y, 0, 0x983, 0, 0);
                }
                UNSET_COORD(defender->m_arrivalCell);
                defender->m_dwell = 0;
                defender->SetDefenderState(AISTATE_SEEK);
                defender->RecycleCoords();
                defender->m_dwell = 0;
                goto checkIdleWander;
            }

            defender->RecycleCoords();
            i32 arrivalMask = 0xdc7;
            i32 manhattanDistance;
            {
                Coord targetXTile;
                target->GetScreenTile(&targetXTile);
                Coord defenderXTile;
                defender->GetScreenTile(&defenderXTile);
                Coord targetYTile;
                target->GetScreenTile(&targetYTile);
                Coord defenderYTile;
                defender->GetScreenTile(&defenderYTile);
                manhattanDistance = abs(targetXTile.m_x - defenderXTile.m_x)
                                    + abs(targetYTile.m_y - defenderYTile.m_y);
            }
            if (manhattanDistance <= 0xa) {
                CRect searchBounds(
                    defender->ScanCell().m_x - 5,
                    defender->ScanCell().m_y - 5,
                    defender->ScanCell().m_x + 5,
                    defender->ScanCell().m_y + 5
                );
                CMapMgr* grid = m_board;
                arrivalMask = 0x20000dc7;
                grid->Clip(&searchBounds);
            }
            {
                Coord targetTile;
                target->GetScreenTile(&targetTile);
                if (!defender->TileSwitch(targetTile.m_x, targetTile.m_y, 0, arrivalMask, 0, 0)) {
                    ResetToSeek(defender);
                }
            }
            if (manhattanDistance <= 0xa) {
                m_board->Clip(NULL);
            }
            defender->m_dwell = 0;
            goto checkIdleWander;
        }
        ResetToSeek(defender);
        defender->RecycleCoords();
    }

checkIdleWander:
    if (CanPlaySpecialAnim(defender)) {
        if (defender->CoordsEmpty()
            && static_cast<u32>(defender->m_dwell) > static_cast<u32>(m_idleAttackWaypointDelay)
            && GetAttackWaypointCount() != 0) {
            Coord* attackWaypoint = CoordAt(rand() % GetAttackWaypointCount());
            defender->TileSwitch(attackWaypoint->m_x, attackWaypoint->m_y, 0, 0x983, 0, 0);
            defender->m_dwell = 0;
        }
    }
    return 1;
}
