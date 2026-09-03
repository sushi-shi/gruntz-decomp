#include <rva.h>

#include <Mfc.h>
#include <MfcNoInline.h>
#include <MfcWin.h>

#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/MapCellFlags.h>
#include <Gruntz/ScanGridMacros.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeColl.h>
#include <Gruntz/TypeKeyColl.h>
#include <Ints.h>
#include <Lith/BDefs.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/ZVec.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define STEP_DRAIN(g)                                                                              \
    {                                                                                              \
        POSITION pos = (g)->m_coordList.GetHeadPosition();                                         \
        if (pos != 0) {                                                                            \
            do {                                                                                   \
                Coord* d = static_cast<Coord*>((g)->CoordListOps()->NextData(pos));                \
                if (d != 0) {                                                                      \
                    g_coordPool.Push(d);                                                           \
                }                                                                                  \
            } while (pos != 0);                                                                    \
        }                                                                                          \
        (g)->m_coordList.RemoveAll();                                                              \
    }

RVA(0x00033520, 0xbc3)
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
                m_defenderSearchRadius.m_x,
                m_defenderSearchRadius.m_y
            );
        }
        if (nb != NULL) {
            if (g->CoordCount() != 0) {
                STEP_DRAIN(g);
            }

            i32 arrivalMask =
                IDX(CELL_FLAG_SOLID | CELL_FLAG_SPECIAL | CELL_FLAG_TRIGGER
                    | CELL_FLAG_REVEALED_POWERUP | CELL_FLAG_ARROW | CELL_FLAG_WATER
                    | CELL_FLAG_SPIKES | CELL_FLAG_SINK_HAZARD);
            i32 dist;
            {
                Coord neighborTile;
                nb->GetScreenTile(&neighborTile);
                Coord gruntTile;
                g->GetScreenTile(&gruntTile);
                Coord delta = neighborTile - gruntTile;
                Coord distance = delta.GetAbs();
                dist = distance.m_x + distance.m_y;
            }
            if (dist <= 0xa) {

                Coord gruntTile;
                g->GetScreenTile(&gruntTile);
                CMapMgr* grid = m_board;
                CRect
                    box(gruntTile.m_x - 5, gruntTile.m_y - 5, gruntTile.m_x + 5, gruntTile.m_y + 5);
                arrivalMask |= BRICKZ_CELL_OCCUPIED;
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
        Coord targetUnit = g->m_arrivalCell;
        CGrunt* cur = m_triggerMgr->m_units[TM_UNITS_PER_PLAYER * targetUnit.m_x + targetUnit.m_y];
        if (cur != NULL) {
            CGameObject* s = cur->m_object;
            if (g->RectContains(s->m_screenPosition.m_x, s->m_screenPosition.m_y) != 0) {

                if (g->CoordCount() != 0) {
                    STEP_DRAIN(g);
                }
                g->m_arrivalCell.Set(-1, -1);
                {
                    bool eq;
                    if (g == NULL) {
                        goto seek;
                    }
                    if (!g->IsAtSavedScreenPos()) {
                        goto seek;
                    }
                    if (g->m_entranceCommitted == false) {
                        goto seek;
                    }
                    if (g->m_deathAnimStarted != false) {
                        goto seek;
                    }
                    if (g->m_entranceActive != false) {
                        goto seek;
                    }
                    if (g->m_poweredUp != false) {
                        goto seek;
                    }
                    eq =
                        (strcmp(*g_typeColl.GetNameRecord(g->m_logicRecord->EventCode()), "I")
                         == 0);
                    if (eq) {
                        goto seek;
                    }
                    eq =
                        (strcmp(*g_typeColl.GetNameRecord(g->m_logicRecord->EventCode()), "G")
                         == 0);
                    if (eq) {
                        goto seek;
                    }
                    eq =
                        (strcmp(*g_typeColl.GetNameRecord(g->m_logicRecord->EventCode()), "L")
                         == 0);
                    if (eq) {
                        goto seek;
                    }
                    eq =
                        (strcmp(*g_typeColl.GetNameRecord(g->m_logicRecord->EventCode()), "P")
                         == 0);
                    if (eq) {
                        goto seek;
                    }
                    eq =
                        (strcmp(*g_typeColl.GetNameRecord(g->m_logicRecord->EventCode()), "J")
                         == 0);
                    if (eq) {
                        goto seek;
                    }
                    eq =
                        (strcmp(*g_typeColl.GetNameRecord(g->m_logicRecord->EventCode()), "C")
                         == 0);
                    if (eq) {
                        goto seek;
                    }
                    eq =
                        (strcmp(*g_typeColl.GetNameRecord(g->m_logicRecord->EventCode()), "R")
                         == 0);
                    if (eq) {
                        goto seek;
                    }
                    HandleUnitContact(g, cur);
                }
            seek:
                g->m_defenderState = AISTATE_SEEK;
                goto tail;
            }

            i32 dist;
            {
                Coord here = g->GetTilePos();
                Coord np = cur->GetTilePos();
                dist = np.Dist(here);
            }
            if (dist > m_defenderTargetMaxDistance) {
                if (m_attackWaypoints.GetSize() != 0) {
                    Coord* e = CoordAt(rand() % m_attackWaypoints.GetSize());
                    g->TileSwitch(
                        e->m_x,
                        e->m_y,
                        0,
                        IDX(CELL_FLAG_SOLID | CELL_FLAG_SPECIAL | CELL_FLAG_ARROW | CELL_FLAG_WATER
                            | CELL_FLAG_SINK_HAZARD),
                        0,
                        0
                    );
                }
                g->m_arrivalCell.Set(-1, -1);
                g->m_dwell = 0;
                g->m_defenderState = AISTATE_SEEK;
                if (g->CoordCount() != 0) {
                    STEP_DRAIN(g);
                }
                g->m_dwell = 0;
                goto tail;
            }

            if (g->CoordCount() != 0) {
                STEP_DRAIN(g);
            }
            i32 arrivalMask =
                IDX(CELL_FLAG_SOLID | CELL_FLAG_SPECIAL | CELL_FLAG_TRIGGER
                    | CELL_FLAG_REVEALED_POWERUP | CELL_FLAG_ARROW | CELL_FLAG_WATER
                    | CELL_FLAG_SPIKES | CELL_FLAG_SINK_HAZARD);
            i32 dist2;
            {
                Coord targetTile;
                cur->GetScreenTile(&targetTile);
                Coord gruntTile;
                g->GetScreenTile(&gruntTile);
                Coord delta = targetTile - gruntTile;
                Coord distance = delta.GetAbs();
                dist2 = distance.m_x + distance.m_y;
            }
            if (dist2 <= 0xa) {
                Coord gruntTile;
                g->GetScreenTile(&gruntTile);
                CMapMgr* grid = m_board;
                CRect
                    box(gruntTile.m_x - 5, gruntTile.m_y - 5, gruntTile.m_x + 5, gruntTile.m_y + 5);
                arrivalMask |= BRICKZ_CELL_OCCUPIED;
                grid->Clip(&box);
            }
            {
                Coord cp;
                cur->GetScreenTile(&cp);
                if (!g->TileSwitch(cp.m_x, cp.m_y, 0, arrivalMask, 0, 0)) {
                    g->m_arrivalCell.Set(-1, -1);
                    g->m_defenderState = AISTATE_SEEK;
                }
            }
            if (dist2 <= 0xa) {
                m_board->Clip(NULL);
            }
            g->m_dwell = 0;
            goto tail;
        }
        g->m_arrivalCell.Set(-1, -1);
        g->m_defenderState = AISTATE_SEEK;
        g->RecycleCoords();
    }

tail:
    if (CanPlaySpecialAnim(g)) {
        if (g->CoordCount() == 0
            && static_cast<u32>(g->m_dwell) > static_cast<u32>(m_idleAttackWaypointDelay)
            && m_attackWaypoints.GetSize() != 0) {
            Coord* e = CoordAt(rand() % m_attackWaypoints.GetSize());
            g->TileSwitch(
                e->m_x,
                e->m_y,
                0,
                IDX(CELL_FLAG_SOLID | CELL_FLAG_SPECIAL | CELL_FLAG_ARROW | CELL_FLAG_WATER
                    | CELL_FLAG_SINK_HAZARD),
                0,
                0
            );
            g->m_dwell = 0;
        }
    }
    return 1;
}
