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
#include <Gruntz/ScanGridMacros.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeColl.h>
#include <Gruntz/TypeKeyColl.h>
#include <Ints.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/ZVec.h>

#include <math.h>
#include <new>
#include <stdlib.h>
#include <string.h>

#define STEP_DRAIN(g)                                                                              \
    {                                                                                              \
        POSITION pos = (g)->m_coordList.GetHeadPosition();                                         \
        if (pos != 0) {                                                                            \
            do {                                                                                   \
                void* d = (g)->CoordListOps()->NextData(pos);                                      \
                if (d != 0) {                                                                      \
                    g_coordPool.Push(d);                                                           \
                }                                                                                  \
            } while (pos != 0);                                                                    \
        }                                                                                          \
        (g)->m_coordList.RemoveAll();                                                              \
    }

#define STEP_BOUNDS(grid)                                                                          \
    {                                                                                              \
        RECT ra;                                                                                   \
        RECT rb;                                                                                   \
        static_cast<RECT*>(new (&ra) CRect(0, 0, (grid)->m_width, (grid)->m_height));              \
        RECT* pb = static_cast<RECT*>(new (&rb) CRect(0, 0, (grid)->m_width, (grid)->m_height));   \
        ra.left = pb->left;                                                                        \
        ra.top = pb->top;                                                                          \
        ra.right = pb->right;                                                                      \
        ra.bottom = pb->bottom;                                                                    \
        if (!IntersectRect(&(grid)->m_bounds, &ra, &rb)) {                                         \
            (grid)->m_bounds = ra;                                                                 \
        }                                                                                          \
        (grid)->m_gridW = (grid)->m_bounds.right - (grid)->m_bounds.left;                          \
        (grid)->m_gridH = (grid)->m_bounds.bottom - (grid)->m_bounds.top;                          \
    }

// retail inlines this at every "where is this unit, in tiles" site: the screen
// position lands in a fresh temp, is shifted in place, and is returned BY VALUE
// (hence the second store pair into the caller's Coord).
static __inline Coord ScreenTile(CUserLogic* u) {
    Coord c;
    u->GetScreenPos(&c);
    c.m_x = c.m_x >> TILE_SHIFT_PX;
    c.m_y = c.m_y >> TILE_SHIFT_PX;
    return c;
}

// @early-stop
// Reloc sequence matches retail's 81/81 in order. The residue is allocation: cl
// keeps each ScreenTile temp in a register and drops the write-back into the
// Coord it shifted, where retail homes both the temp and the destination (its
// extra `sar`/`mov` pairs), and cl holds the zero in a register so the null tests
// are `cmp reg,zero` in retail and `test reg,reg` here.
RVA(0x00033520, 0xbc3)
i32 CBattlezMapConfig::StepDefenderUnit(CGrunt* g) {
    GruntAiState state = g->m_defenderState;
    if (state == AISTATE_RETURN) {
        return 1;
    }
    if (state != AISTATE_ATTACK) {

        Coord tp;
        g->GetScreenPos(&tp);
        CGrunt* nb = FindIdleGruntInBox(
            tp.m_x >> TILE_SHIFT_PX,
            tp.m_y >> TILE_SHIFT_PX,
            m_defenderSearchRadiusX,
            m_defenderSearchRadiusY
        );
        if (nb != NULL) {
            if (g->CoordCount() != 0) {
                STEP_DRAIN(g);
            }

            Coord np = ScreenTile(nb);
            Coord gp = ScreenTile(g);
            Coord np2 = ScreenTile(nb);
            Coord gp2 = ScreenTile(g);
            i32 dist = abs(np2.m_y - gp2.m_y) + abs(np.m_x - gp.m_x);
            if (dist <= 0xa) {

                Coord b0, b1, b2, b3;
                g->GetScreenPos(&b0);
                g->GetScreenPos(&b1);
                g->GetScreenPos(&b2);
                g->GetScreenPos(&b3);
                CMapMgr* grid = m_board;
                RECT box;
                box.left = (b0.m_x >> TILE_SHIFT_PX) - 5;
                box.top = (b1.m_y >> TILE_SHIFT_PX) - 5;
                box.right = (b2.m_x >> TILE_SHIFT_PX) + 5;
                box.bottom = (b3.m_y >> TILE_SHIFT_PX) + 5;
                GRID_CLIP(grid, &box);
            }
            Coord p;
            nb->GetScreenPos(&p);
            if (g->TileSwitch(
                    p.m_x >> TILE_SHIFT_PX,
                    p.m_y >> TILE_SHIFT_PX,
                    0,
                    0x20000dc7,
                    0,
                    0
                )) {
                g->m_defenderState = AISTATE_ATTACK;
                g->m_arrivalCell.m_x = nb->m_tileOwnerHi;
                g->m_arrivalCell.m_y = nb->m_tileOwnerLo;
                g->m_dwell = 0;
            }
            if (dist <= 0xa) {
                STEP_BOUNDS(m_board);
            }
        }
        goto tail;
    }

    {
        i32 col = g->m_arrivalCell.m_x;
        i32 row = g->m_arrivalCell.m_y;
        CGrunt* cur = m_triggerMgr->m_grid[15 * col + row];
        if (cur != NULL) {
            CGameObject* s = cur->m_object;
            if (g->RectContains(s->m_screenX, s->m_screenY) != 0) {

                if (g->CoordCount() != 0) {
                    STEP_DRAIN(g);
                }
                g->m_arrivalCell.m_x = -1;
                g->m_arrivalCell.m_y = -1;
                {
                    // retail re-runs the act-key lookup for every letter; each
                    // comparison is materialised as a 0/1 before it is branched on.
                    bool eq;
                    if (g == NULL) {
                        goto seek;
                    }
                    if (!g->IsAtSavedScreenPos()) {
                        goto seek;
                    }
                    if (g->m_entranceCommitted == 0) {
                        goto seek;
                    }
                    if (g->m_deathAnimStarted != 0) {
                        goto seek;
                    }
                    if (g->m_entranceActive != 0) {
                        goto seek;
                    }
                    if (g->m_poweredUp != 0) {
                        goto seek;
                    }
                    eq = (strcmp(*g_typeColl.GetNameRecord(g->m_objAux->ActKey()), "I") == 0);
                    if (eq) {
                        goto seek;
                    }
                    eq = (strcmp(*g_typeColl.GetNameRecord(g->m_objAux->ActKey()), "G") == 0);
                    if (eq) {
                        goto seek;
                    }
                    eq = (strcmp(*g_typeColl.GetNameRecord(g->m_objAux->ActKey()), "L") == 0);
                    if (eq) {
                        goto seek;
                    }
                    eq = (strcmp(*g_typeColl.GetNameRecord(g->m_objAux->ActKey()), "P") == 0);
                    if (eq) {
                        goto seek;
                    }
                    eq = (strcmp(*g_typeColl.GetNameRecord(g->m_objAux->ActKey()), "J") == 0);
                    if (eq) {
                        goto seek;
                    }
                    eq = (strcmp(*g_typeColl.GetNameRecord(g->m_objAux->ActKey()), "C") == 0);
                    if (eq) {
                        goto seek;
                    }
                    eq = (strcmp(*g_typeColl.GetNameRecord(g->m_objAux->ActKey()), "R") == 0);
                    if (eq) {
                        goto seek;
                    }
                    HandleUnitContact(g, cur);
                }
            seek:
                g->m_defenderState = AISTATE_SEEK;
                goto tail;
            }

            Coord here, np;
            here = g->GetTilePos();
            np = cur->GetTilePos();
            i32 dx;
            i32 dy;
            i32 dist;
            dx = np.m_x - here.m_x;
            dy = np.m_y - here.m_y;
            dist = static_cast<i32>(
                sqrt(static_cast<double>((abs(dx) * abs(dx) + abs(dy) * abs(dy))))
            );
            if (dist > m_defenderTargetMaxDistance) {
                if (m_attackWaypoints.GetSize() != 0) {
                    Coord* e = CoordAt(rand() % m_attackWaypoints.GetSize());
                    g->TileSwitch(e->m_x, e->m_y, 0, 0x983, 0, 0);
                }
                g->m_arrivalCell.m_x = -1;
                g->m_dwell = 0;
                g->m_arrivalCell.m_y = -1;
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
            Coord c0 = ScreenTile(cur);
            Coord c1 = ScreenTile(g);
            Coord c2 = ScreenTile(cur);
            Coord c3 = ScreenTile(g);
            i32 dist2;
            dist2 = abs(c0.m_x - c1.m_x) + abs(c2.m_y - c3.m_y);
            if (dist2 <= 0xa) {
                Coord d0, d1, d2, d3;
                g->GetScreenPos(&d0);
                g->GetScreenPos(&d1);
                g->GetScreenPos(&d2);
                g->GetScreenPos(&d3);
                CMapMgr* grid = m_board;
                RECT box;
                box.left = (d0.m_x >> TILE_SHIFT_PX) - 5;
                box.top = (d1.m_y >> TILE_SHIFT_PX) - 5;
                box.right = (d2.m_x >> TILE_SHIFT_PX) + 5;
                box.bottom = (d3.m_y >> TILE_SHIFT_PX) + 5;
                GRID_CLIP(grid, &box);
            }
            Coord cp;
            cur->GetScreenPos(&cp);
            if (!g->TileSwitch(
                    cp.m_x >> TILE_SHIFT_PX,
                    cp.m_y >> TILE_SHIFT_PX,
                    0,
                    0x20000dc7,
                    0,
                    0
                )) {
                g->m_arrivalCell.m_x = -1;
                g->m_arrivalCell.m_y = -1;
                g->m_defenderState = AISTATE_SEEK;
            }
            if (dist2 <= 0xa) {
                m_board->Clip(0);
            }
            g->m_dwell = 0;
            goto tail;
        }
        // cur == NULL falls out of the target block into the reset path.
        g->m_arrivalCell.m_x = -1;
        g->m_arrivalCell.m_y = -1;
        g->m_defenderState = AISTATE_SEEK;
        g->RecycleCoords();
    }

tail:
    if (CanPlaySpecialAnim(g)) {
        if (g->CoordCount() == 0
            && static_cast<u32>(g->m_dwell) > static_cast<u32>(m_idleAttackWaypointDelay)
            && m_attackWaypoints.GetSize() != 0) {
            Coord* e = CoordAt(rand() % m_attackWaypoints.GetSize());
            g->TileSwitch(e->m_x, e->m_y, 0, 0x983, 0, 0);
            g->m_dwell = 0;
        }
    }
    return 1;
}
