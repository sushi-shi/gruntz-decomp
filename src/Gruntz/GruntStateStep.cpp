#include <StdAfx.h>

#include <Ints.h>

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

i32 CBattlezMapConfig::StepDefenderUnit(CGrunt* g) {
    GruntAiState state = g->m_defenderState;
    if (state == AISTATE_RETURN) {
        return 1;
    }
    if (state != AISTATE_ATTACK) {

        CGrunt* nb;
        {
            Coord tp;
            g->GetScreenTile(&tp);
            nb = FindIdleGruntInBox(
                tp.m_x,
                tp.m_y,
                m_defenderSearchRadiusX,
                m_defenderSearchRadiusY
            );
        }
        if (nb != NULL) {
            g->RecycleCoords();

            i32 arrivalMask = 0xdc7;
            i32 dist;
            {
                Coord np;
                nb->GetScreenTile(&np);
                Coord gp;
                g->GetScreenTile(&gp);
                Coord np2;
                nb->GetScreenTile(&np2);
                Coord gp2;
                g->GetScreenTile(&gp2);
                dist = abs(np2.m_y - gp2.m_y) + abs(np.m_x - gp.m_x);
            }
            if (dist <= 0xa) {

                CRect box(
                    g->ScanCell().m_x - 5,
                    g->ScanCell().m_y - 5,
                    g->ScanCell().m_x + 5,
                    g->ScanCell().m_y + 5
                );
                CMapMgr* grid = m_board;
                arrivalMask = 0x20000dc7;
                grid->Clip(&box);
            }
            {
                Coord p;
                nb->GetScreenTile(&p);
                if (g->TileSwitch(p.m_x, p.m_y, 0, arrivalMask, 0, 0)) {
                    g->m_defenderState = AISTATE_ATTACK;
                    g->m_arrivalCell.Set(nb->m_playerIndex, nb->m_unitIndex);
                    g->m_dwell = 0;
                }
            }
            if (dist <= 0xa) {
                m_board->Clip(NULL);
            }
        }
        goto tail;
    }

    {
        CGrunt* cur = m_triggerMgr->UnitAt(g->ArrivalCell().m_x, g->ArrivalCell().m_y);
        if (cur != NULL) {
            CGameObject* s = cur->m_object;
            if (g->RectContains(s->m_screenX, s->m_screenY) != 0) {

                g->RecycleCoords();
                UNSET_COORD(g->m_arrivalCell);
                if (g != NULL && g->IsAtSavedScreenPos() && g->m_entranceCommitted != false
                    && g->m_deathAnimStarted == false && g->m_entranceActive == false
                    && g->m_poweredUp == false && BattlezActDiffersFromIGLPJCR(g)) {
                    HandleUnitContact(g, cur);
                }
                g->m_defenderState = AISTATE_SEEK;
                goto tail;
            }

            i32 dist;
            {
                Coord here = g->GetTilePos();
                Coord np = cur->GetTilePos();
                i32 dx = np.m_x - here.m_x;
                i32 dy = np.m_y - here.m_y;
                dist = static_cast<i32>(sqrt(static_cast<double>((SQR(abs(dx)) + SQR(abs(dy))))));
            }
            if (dist > m_defenderTargetMaxDistance) {
                if (static_cast<i32>(m_attackWaypoints.size()) != 0) {
                    Coord* e = CoordAt(rand() % static_cast<i32>(m_attackWaypoints.size()));
                    g->TileSwitch(e->m_x, e->m_y, 0, 0x983, 0, 0);
                }
                UNSET_COORD(g->m_arrivalCell);
                g->m_dwell = 0;
                g->m_defenderState = AISTATE_SEEK;
                g->RecycleCoords();
                g->m_dwell = 0;
                goto tail;
            }

            g->RecycleCoords();
            i32 arrivalMask = 0xdc7;
            i32 dist2;
            {
                Coord targetPos1;
                cur->GetScreenTile(&targetPos1);
                Coord gruntPos1;
                g->GetScreenTile(&gruntPos1);
                Coord targetPos2;
                cur->GetScreenTile(&targetPos2);
                Coord gruntPos2;
                g->GetScreenTile(&gruntPos2);
                dist2 = abs(targetPos1.m_x - gruntPos1.m_x) + abs(targetPos2.m_y - gruntPos2.m_y);
            }
            if (dist2 <= 0xa) {
                CRect box(
                    g->ScanCell().m_x - 5,
                    g->ScanCell().m_y - 5,
                    g->ScanCell().m_x + 5,
                    g->ScanCell().m_y + 5
                );
                CMapMgr* grid = m_board;
                arrivalMask = 0x20000dc7;
                grid->Clip(&box);
            }
            {
                Coord cp;
                cur->GetScreenTile(&cp);
                if (!g->TileSwitch(cp.m_x, cp.m_y, 0, arrivalMask, 0, 0)) {
                    ResetToSeek(g);
                }
            }
            if (dist2 <= 0xa) {
                m_board->Clip(NULL);
            }
            g->m_dwell = 0;
            goto tail;
        }
        ResetToSeek(g);
        g->RecycleCoords();
    }

tail:
    if (CanPlaySpecialAnim(g)) {
        if (g->CoordCount() == 0
            && static_cast<u32>(g->m_dwell) > static_cast<u32>(m_idleAttackWaypointDelay)
            && static_cast<i32>(m_attackWaypoints.size()) != 0) {
            Coord* e = CoordAt(rand() % static_cast<i32>(m_attackWaypoints.size()));
            g->TileSwitch(e->m_x, e->m_y, 0, 0x983, 0, 0);
            g->m_dwell = 0;
        }
    }
    return 1;
}
