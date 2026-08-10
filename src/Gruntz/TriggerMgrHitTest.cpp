#include <Gruntz/Brickz.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDirection.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/MapMgr.h>
#include <Gruntz/TileGrid.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TriggerMgrRecords.h>
#include <Wap32/TileGeometry.h>

#include <stddef.h>

// Unreferenced in retail: only the incremental-link thunk names this body, and
// the thunk has no callers. Its five out-of-line inline helpers likewise have
// no other callers; that does not identify the live movement path that replaced
// this implementation.
//
// Test the four neighbours at +/-45 and +/-90 degrees from dir. Diagonal steps
// additionally require both orthogonal cells to carry the route bit. The two
// unused parameters are not read.
// @early-stop
// The control flow is PROVEN exact: `--branches --diff` reports 556 branches and
// 96 rets on both sides with every symbolic target agreeing. The residue is 43
// instructions out of 6511, in three families, all confined to the last third of
// the body (asm 4191-6386, source lines 1457-2147) while the same source
// spellings earlier in the function are byte-identical:
//   A (9 sites) `mov edi,[esp+0x2d8]` before vs after the `mov edx,[eax]` of the
//     Coord copy at `*pCell = *stepN.Set(...)`
//   B (9 sites) the `mov ecx,[g_gameReg]` for the sideX CellFlagsAt scheduled
//     before vs after the first argument's `mov eax,[esp+0x1c]`
//   C (2 sites) the same swap on `[esp+0x2c4]`
// All nine family-B sites are the SECOND sideY/sideX block of a case arm; the
// first block of every arm matches, so the difference is register pressure in
// that region, not the spelling. Two levers were measured and are dead: a
// 13-cell TU-declaration-count sweep is flat at 99.4577, and rewriting all 96
// out-param copies field-wise (`pCell->m_x = c->m_x`) takes the diff from 23 to
// 30 hunks.
RVA(0x0006f2f0, 0x5227)
GruntDirectionCell __stdcall TmDeflectStep(
    CGrunt* g,
    i32 goalX,
    i32 goalY,
    i32 unusedX,
    i32 unusedY,
    GruntDirection dir,
    Coord* pCell,
    i32* pFlags
) {
    Coord step0;
    Coord step1;
    Coord step2;
    Coord step3;
    Coord step4;
    Coord step5;
    Coord step6;
    Coord step7;
    Coord step8;
    Coord step9;
    Coord step10;
    Coord step11;
    Coord step12;
    Coord step13;
    Coord step14;
    Coord step15;
    Coord step16;
    Coord step17;
    Coord step18;
    Coord step19;
    Coord step20;
    Coord step21;
    Coord step22;
    Coord step23;
    Coord step24;
    Coord step25;
    Coord step26;
    Coord step27;
    Coord step28;
    Coord step29;
    Coord step30;
    Coord step31;
    Coord step32;
    Coord step33;
    Coord step34;
    Coord step35;
    Coord step36;
    Coord step37;
    Coord step38;
    Coord step39;
    Coord step40;
    Coord step41;
    Coord step42;
    Coord step43;
    Coord step44;
    Coord step45;
    Coord step46;
    Coord step47;
    Coord step48;
    Coord step49;
    Coord step50;
    Coord step51;
    Coord step52;
    Coord step53;
    Coord step54;
    Coord step55;
    Coord step56;
    Coord step57;
    Coord step58;
    Coord step59;
    Coord step60;
    Coord step61;
    Coord step62;
    Coord step63;
    Coord step64;
    Coord step65;
    Coord step66;
    Coord step67;
    Coord step68;
    Coord step69;
    Coord step70;
    Coord step71;
    Coord step72;
    Coord step73;
    Coord step74;
    Coord step75;
    Coord step76;
    Coord step77;
    Coord step78;
    Coord step79;
    Coord step80;
    Coord step81;
    Coord step82;
    Coord step83;
    Coord step84;
    Coord step85;
    Coord step86;
    Coord step87;
    Coord step88;
    Coord step89;
    Coord step90;
    Coord step91;
    Coord step92;
    Coord step93;
    Coord step94;
    Coord step95;
    Coord sideStep0;
    Coord sideStep1;
    Coord sideStep2;
    Coord sideStep3;
    Coord sideStep4;
    Coord sideStep5;
    Coord sideStep6;
    Coord sideStep7;
    Coord sideStep8;
    Coord sideStep9;
    Coord sideStep10;
    Coord sideStep11;
    Coord sideStep12;
    Coord sideStep13;
    Coord sideStep14;
    Coord sideStep15;
    Coord sideStep16;
    Coord sideStep17;
    Coord sideStep18;
    Coord sideStep19;
    Coord sideStep20;
    Coord sideStep21;
    Coord sideStep22;
    Coord sideStep23;
    Coord sideStep24;
    Coord sideStep25;
    Coord sideStep26;
    Coord sideStep27;
    Coord sideStep28;
    Coord sideStep29;
    Coord sideStep30;
    Coord sideStep31;
    Coord sideStep32;
    Coord sideStep33;
    Coord sideStep34;
    Coord sideStep35;
    Coord sideStep36;
    Coord sideStep37;
    Coord sideStep38;
    Coord sideStep39;
    Coord sideStep40;
    Coord sideStep41;
    Coord sideStep42;
    Coord sideStep43;
    Coord sideStep44;
    Coord sideStep45;
    Coord sideStep46;
    Coord sideStep47;
    Coord sideStep48;
    Coord sideStep49;
    Coord sideStep50;
    Coord sideStep51;
    Coord sideStep52;
    Coord sideStep53;
    Coord sideStep54;
    Coord sideStep55;
    Coord sideStep56;
    Coord sideStep57;
    Coord sideStep58;
    Coord sideStep59;
    Coord sideStep60;
    Coord sideStep61;
    Coord sideStep62;
    Coord sideStep63;
    Coord sideStep64;
    Coord sideStep65;
    Coord sideStep66;
    Coord sideStep67;
    Coord sideStep68;
    Coord sideStep69;
    Coord sideStep70;
    Coord sideStep71;
    Coord sideStep72;
    Coord sideStep73;
    Coord sideStep74;
    Coord sideStep75;
    Coord sideStep76;
    Coord sideStep77;
    Coord sideStep78;
    Coord sideStep79;
    Coord sideStep80;
    Coord sideStep81;
    Coord sideStep82;
    Coord sideStep83;
    Coord sideStep84;
    Coord sideStep85;
    Coord sideStep86;
    Coord sideStep87;
    Coord sideStep88;
    Coord sideStep89;
    Coord sideStep90;
    Coord sideStep91;
    Coord sideStep92;
    Coord sideStep93;
    Coord sideStep94;
    Coord sideStep95;

    i32 sideY;
    Coord entrance = g->EntrancePx();
    if (entrance.m_x == goalX && entrance.m_y == goalY) {
        return s_gruntDirCenter;
    }
    i32 mask = g->m_arrivalFlags | BRICKZ_CELL_OCCUPIED;
    i32 lastX = g->m_lastTilePx.m_x;
    i32 lastY = g->m_lastTilePx.m_y;
    i32 pass = g->m_passableMask;
    switch (dir) {
        case DIR_NORTH:
            if (g->EntrancePx().m_x < goalX) {
                {
                    *pCell = *step0.Set(lastX - TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        sideStep0.Set(lastX, (lastY - TILE_SIZE_PX));
                        sideStep1.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep0.m_x >> TILE_SHIFT_PX,
                            sideStep0.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->m_tileGrid->CellFlagsAt(
                            sideStep1.m_x >> TILE_SHIFT_PX,
                            sideStep1.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthWest;
                        }
                    }
                }
                {
                    *pCell = *step1.Set(lastX - TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirWest;
                    }
                }
                {
                    *pCell = *step2.Set(lastX + TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->m_tileGrid->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        sideStep2.Set(lastX, (lastY - TILE_SIZE_PX));
                        sideStep3.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep2.m_x >> TILE_SHIFT_PX,
                            sideStep2.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep3.m_x >> TILE_SHIFT_PX,
                            sideStep3.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthEast;
                        }
                    }
                }
                {
                    *pCell = *step3.Set(lastX + TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirEast;
                    }
                }
                return s_gruntDirCenter;
            } else if (g->EntrancePx().m_x > goalX) {
                {
                    *pCell = *step4.Set(lastX + TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        sideStep4.Set(lastX, (lastY - TILE_SIZE_PX));
                        sideStep5.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep4.m_x >> TILE_SHIFT_PX,
                            sideStep4.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep5.m_x >> TILE_SHIFT_PX,
                            sideStep5.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthEast;
                        }
                    }
                }
                {
                    *pCell = *step5.Set(lastX + TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirEast;
                    }
                }
                {
                    *pCell = *step6.Set(lastX - TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        sideStep6.Set(lastX, (lastY - TILE_SIZE_PX));
                        sideStep7.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep6.m_x >> TILE_SHIFT_PX,
                            sideStep6.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep7.m_x >> TILE_SHIFT_PX,
                            sideStep7.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthWest;
                        }
                    }
                }
                {
                    *pCell = *step7.Set(lastX - TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirWest;
                    }
                }
                return s_gruntDirCenter;
            } else {
                {
                    *pCell = *step8.Set(lastX - TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        sideStep8.Set(lastX, (lastY - TILE_SIZE_PX));
                        sideStep9.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep8.m_x >> TILE_SHIFT_PX,
                            sideStep8.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep9.m_x >> TILE_SHIFT_PX,
                            sideStep9.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthWest;
                        }
                    }
                }
                {
                    *pCell = *step9.Set(lastX + TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        sideStep10.Set(lastX, (lastY - TILE_SIZE_PX));
                        sideStep11.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep10.m_x >> TILE_SHIFT_PX,
                            sideStep10.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep11.m_x >> TILE_SHIFT_PX,
                            sideStep11.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthEast;
                        }
                    }
                }
                {
                    *pCell = *step10.Set(lastX - TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirWest;
                    }
                }
                {
                    *pCell = *step11.Set(lastX + TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirEast;
                    }
                }
                return s_gruntDirCenter;
            }
            break;
        case DIR_SOUTH:
            if (g->EntrancePx().m_x < goalX) {
                {
                    *pCell = *step12.Set(lastX - TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        sideStep12.Set(lastX, (lastY + TILE_SIZE_PX));
                        sideStep13.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep12.m_x >> TILE_SHIFT_PX,
                            sideStep12.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep13.m_x >> TILE_SHIFT_PX,
                            sideStep13.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthWest;
                        }
                    }
                }
                {
                    *pCell = *step13.Set(lastX - TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirWest;
                    }
                }
                {
                    *pCell = *step14.Set(lastX + TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        sideStep14.Set(lastX, (lastY + TILE_SIZE_PX));
                        sideStep15.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep14.m_x >> TILE_SHIFT_PX,
                            sideStep14.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep15.m_x >> TILE_SHIFT_PX,
                            sideStep15.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthEast;
                        }
                    }
                }
                {
                    *pCell = *step15.Set(lastX + TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirEast;
                    }
                }
                return s_gruntDirCenter;
            } else if (g->EntrancePx().m_x > goalX) {
                {
                    *pCell = *step16.Set(lastX + TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        sideStep16.Set(lastX, (lastY + TILE_SIZE_PX));
                        sideStep17.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep16.m_x >> TILE_SHIFT_PX,
                            sideStep16.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep17.m_x >> TILE_SHIFT_PX,
                            sideStep17.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthEast;
                        }
                    }
                }
                {
                    *pCell = *step17.Set(lastX + TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirEast;
                    }
                }
                {
                    *pCell = *step18.Set(lastX - TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        sideStep18.Set(lastX, (lastY + TILE_SIZE_PX));
                        sideStep19.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep18.m_x >> TILE_SHIFT_PX,
                            sideStep18.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep19.m_x >> TILE_SHIFT_PX,
                            sideStep19.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthWest;
                        }
                    }
                }
                {
                    *pCell = *step19.Set(lastX - TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirWest;
                    }
                }
            } else {
                {
                    *pCell = *step20.Set(lastX - TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        sideStep20.Set(lastX, (lastY + TILE_SIZE_PX));
                        sideStep21.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep20.m_x >> TILE_SHIFT_PX,
                            sideStep20.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep21.m_x >> TILE_SHIFT_PX,
                            sideStep21.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthWest;
                        }
                    }
                }
                {
                    *pCell = *step21.Set(lastX + TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        sideStep22.Set(lastX, (lastY + TILE_SIZE_PX));
                        sideStep23.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep22.m_x >> TILE_SHIFT_PX,
                            sideStep22.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep23.m_x >> TILE_SHIFT_PX,
                            sideStep23.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthEast;
                        }
                    }
                }
                {
                    *pCell = *step22.Set(lastX - TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirWest;
                    }
                }
                {
                    *pCell = *step23.Set(lastX + TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirEast;
                    }
                }
            }
            break;
        case DIR_EAST:
            if (g->EntrancePx().m_y < goalY) {
                {
                    *pCell = *step24.Set(lastX + TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        sideStep24.Set(lastX, (lastY - TILE_SIZE_PX));
                        sideStep25.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep24.m_x >> TILE_SHIFT_PX,
                            sideStep24.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep25.m_x >> TILE_SHIFT_PX,
                            sideStep25.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthEast;
                        }
                    }
                }
                {
                    *pCell = *step25.Set(lastX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirNorth;
                    }
                }
                {
                    *pCell = *step26.Set(lastX + TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        sideStep26.Set(lastX, (lastY + TILE_SIZE_PX));
                        sideStep27.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep26.m_x >> TILE_SHIFT_PX,
                            sideStep26.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep27.m_x >> TILE_SHIFT_PX,
                            sideStep27.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthEast;
                        }
                    }
                }
                {
                    *pCell = *step27.Set(lastX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirSouth;
                    }
                }
                return s_gruntDirCenter;
            } else if (g->EntrancePx().m_y > goalY) {
                {
                    *pCell = *step28.Set(lastX + TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        sideStep28.Set(lastX, (lastY + TILE_SIZE_PX));
                        sideStep29.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep28.m_x >> TILE_SHIFT_PX,
                            sideStep28.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep29.m_x >> TILE_SHIFT_PX,
                            sideStep29.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthEast;
                        }
                    }
                }
                {
                    *pCell = *step29.Set(lastX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirSouth;
                    }
                }
                {
                    *pCell = *step30.Set(lastX + TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        sideStep30.Set(lastX, (lastY - TILE_SIZE_PX));
                        sideStep31.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep30.m_x >> TILE_SHIFT_PX,
                            sideStep30.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep31.m_x >> TILE_SHIFT_PX,
                            sideStep31.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthEast;
                        }
                    }
                }
                {
                    *pCell = *step31.Set(lastX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirNorth;
                    }
                }
                return s_gruntDirCenter;
            } else {
                {
                    *pCell = *step32.Set(lastX + TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        sideStep32.Set(lastX, (lastY - TILE_SIZE_PX));
                        sideStep33.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep32.m_x >> TILE_SHIFT_PX,
                            sideStep32.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep33.m_x >> TILE_SHIFT_PX,
                            sideStep33.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthEast;
                        }
                    }
                }
                {
                    *pCell = *step33.Set(lastX + TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        sideStep34.Set(lastX, (lastY + TILE_SIZE_PX));
                        sideStep35.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep34.m_x >> TILE_SHIFT_PX,
                            sideStep34.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep35.m_x >> TILE_SHIFT_PX,
                            sideStep35.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthEast;
                        }
                    }
                }
                {
                    *pCell = *step34.Set(lastX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirNorth;
                    }
                }
                {
                    *pCell = *step35.Set(lastX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirSouth;
                    }
                }
                return s_gruntDirCenter;
            }
            break;
        case DIR_WEST:
            if (g->EntrancePx().m_y < goalY) {
                {
                    *pCell = *step36.Set(lastX - TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        sideStep36.Set(lastX, (lastY - TILE_SIZE_PX));
                        sideStep37.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep36.m_x >> TILE_SHIFT_PX,
                            sideStep36.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep37.m_x >> TILE_SHIFT_PX,
                            sideStep37.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthWest;
                        }
                    }
                }
                {
                    *pCell = *step37.Set(lastX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirNorth;
                    }
                }
                {
                    *pCell = *step38.Set(lastX - TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        sideStep38.Set(lastX, (lastY + TILE_SIZE_PX));
                        sideStep39.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep38.m_x >> TILE_SHIFT_PX,
                            sideStep38.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->m_tileGrid->CellFlagsAt(
                            sideStep39.m_x >> TILE_SHIFT_PX,
                            sideStep39.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthWest;
                        }
                    }
                }
                {
                    *pCell = *step39.Set(lastX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirSouth;
                    }
                }
                return s_gruntDirCenter;
            } else if (g->EntrancePx().m_y > goalY) {
                {
                    *pCell = *step40.Set(lastX - TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        sideStep40.Set(lastX, (lastY + TILE_SIZE_PX));
                        sideStep41.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep40.m_x >> TILE_SHIFT_PX,
                            sideStep40.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep41.m_x >> TILE_SHIFT_PX,
                            sideStep41.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthWest;
                        }
                    }
                }
                {
                    *pCell = *step41.Set(lastX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirSouth;
                    }
                }
                {
                    *pCell = *step42.Set(lastX - TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        sideStep42.Set(lastX, (lastY - TILE_SIZE_PX));
                        sideStep43.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep42.m_x >> TILE_SHIFT_PX,
                            sideStep42.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep43.m_x >> TILE_SHIFT_PX,
                            sideStep43.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthWest;
                        }
                    }
                }
                {
                    *pCell = *step43.Set(lastX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirNorth;
                    }
                }
            } else {
                {
                    *pCell = *step44.Set(lastX - TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        sideStep44.Set(lastX, (lastY - TILE_SIZE_PX));
                        sideStep45.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep44.m_x >> TILE_SHIFT_PX,
                            sideStep44.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep45.m_x >> TILE_SHIFT_PX,
                            sideStep45.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthWest;
                        }
                    }
                }
                {
                    *pCell = *step45.Set(lastX - TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        sideStep46.Set(lastX, (lastY + TILE_SIZE_PX));
                        sideStep47.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep46.m_x >> TILE_SHIFT_PX,
                            sideStep46.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep47.m_x >> TILE_SHIFT_PX,
                            sideStep47.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthWest;
                        }
                    }
                }
                {
                    *pCell = *step46.Set(lastX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirNorth;
                    }
                }
                {
                    *pCell = *step47.Set(lastX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirSouth;
                    }
                }
            }
            break;
        case DIR_NORTHEAST: {
            i32 deltaY = goalY - g->EntrancePx().m_y;
            i32 deltaX = g->EntrancePx().m_x - goalX;
            if (deltaX < deltaY) {
                {
                    *pCell = *step48.Set(lastX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirNorth;
                    }
                }
                {
                    *pCell = *step49.Set(lastX - TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        sideStep48.Set(lastX, (lastY - TILE_SIZE_PX));
                        sideStep49.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep48.m_x >> TILE_SHIFT_PX,
                            sideStep48.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep49.m_x >> TILE_SHIFT_PX,
                            sideStep49.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthWest;
                        }
                    }
                }
                {
                    *pCell = *step50.Set(lastX + TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirEast;
                    }
                }
                {
                    *pCell = *step51.Set(lastX + TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        sideStep50.Set(lastX, (lastY + TILE_SIZE_PX));
                        sideStep51.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep50.m_x >> TILE_SHIFT_PX,
                            sideStep50.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep51.m_x >> TILE_SHIFT_PX,
                            sideStep51.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthEast;
                        }
                    }
                }
            } else if (deltaX > deltaY) {
                {
                    *pCell = *step52.Set(lastX + TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirEast;
                    }
                }
                {
                    *pCell = *step53.Set(lastX + TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        sideStep52.Set(lastX, (lastY + TILE_SIZE_PX));
                        sideStep53.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep52.m_x >> TILE_SHIFT_PX,
                            sideStep52.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep53.m_x >> TILE_SHIFT_PX,
                            sideStep53.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthEast;
                        }
                    }
                }
                {
                    *pCell = *step54.Set(lastX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirNorth;
                    }
                }
                {
                    *pCell = *step55.Set(lastX - TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        sideStep54.Set(lastX, (lastY - TILE_SIZE_PX));
                        sideStep55.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep54.m_x >> TILE_SHIFT_PX,
                            sideStep54.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideStep55.m_x >> TILE_SHIFT_PX,
                            sideStep55.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthWest;
                        }
                    }
                }
                return s_gruntDirCenter;
            } else {
                {
                    *pCell = *step56.Set(lastX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirNorth;
                    }
                }
                {
                    *pCell = *step57.Set(lastX + TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirEast;
                    }
                }
                {
                    *pCell = *step58.Set(lastX - TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord* sideYStep = sideStep56.Set(lastX, (lastY - TILE_SIZE_PX));
                        Coord* sideXStep = sideStep57.Set((lastX - TILE_SIZE_PX), lastY);
                        i32 sideY = g_gameReg->m_tileGrid->CellFlagsAt(
                            sideYStep->m_x >> TILE_SHIFT_PX,
                            sideYStep->m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep->m_x >> TILE_SHIFT_PX,
                            sideXStep->m_y >> TILE_SHIFT_PX
                        );

                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthWest;
                        }
                    }
                }
                {
                    *pCell = *step59.Set(lastX + TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *sideStep58.Set(lastX, (lastY + TILE_SIZE_PX));
                        Coord sideXStep = *sideStep59.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthEast;
                        }
                    }
                }
            }
            break;
        }
        case DIR_SOUTHEAST: {
            i32 deltaY = g->EntrancePx().m_y - goalY;
            i32 deltaX = g->EntrancePx().m_x - goalX;
            if (deltaX < deltaY) {
                {
                    *pCell = *step60.Set(lastX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirSouth;
                    }
                }
                {
                    *pCell = *step61.Set(lastX - TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *sideStep60.Set(lastX, (lastY + TILE_SIZE_PX));
                        Coord sideXStep = *sideStep61.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthWest;
                        }
                    }
                }
                {
                    *pCell = *step62.Set(lastX + TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirEast;
                    }
                }
                {
                    *pCell = *step63.Set(lastX + TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *sideStep62.Set(lastX, (lastY - TILE_SIZE_PX));
                        Coord sideXStep = *sideStep63.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthEast;
                        }
                    }
                }
                return s_gruntDirCenter;
            } else if (deltaX > deltaY) {
                {
                    *pCell = *step64.Set(lastX + TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirEast;
                    }
                }
                {
                    *pCell = *step65.Set(lastX + TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *sideStep64.Set(lastX, (lastY - TILE_SIZE_PX));
                        Coord sideXStep = *sideStep65.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthEast;
                        }
                    }
                }
                {
                    *pCell = *step66.Set(lastX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirSouth;
                    }
                }
                {
                    *pCell = *step67.Set(lastX - TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *sideStep66.Set(lastX, (lastY + TILE_SIZE_PX));
                        Coord sideXStep = *sideStep67.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthWest;
                        }
                    }
                }
                return s_gruntDirCenter;
            } else {
                {
                    *pCell = *step68.Set(lastX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirSouth;
                    }
                }
                {
                    *pCell = *step69.Set(lastX + TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirEast;
                    }
                }
                {
                    *pCell = *step70.Set(lastX - TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *sideStep68.Set(lastX, (lastY + TILE_SIZE_PX));
                        Coord sideXStep = *sideStep69.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthWest;
                        }
                    }
                }
                {
                    *pCell = *step71.Set(lastX + TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *sideStep70.Set(lastX, (lastY - TILE_SIZE_PX));
                        Coord sideXStep = *sideStep71.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthEast;
                        }
                    }
                }
            }
            break;
        }
        case DIR_NORTHWEST: {
            i32 deltaY = goalY - g->EntrancePx().m_y;
            i32 deltaX = goalX - g->EntrancePx().m_x;
            if (deltaX < deltaY) {
                {
                    *pCell = *step72.Set(lastX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirNorth;
                    }
                }
                {
                    *pCell = *step73.Set(lastX + TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *sideStep72.Set(lastX, (lastY - TILE_SIZE_PX));
                        Coord sideXStep = *sideStep73.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthEast;
                        }
                    }
                }
                {
                    *pCell = *step74.Set(lastX - TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirWest;
                    }
                }
                {
                    *pCell = *step75.Set(lastX - TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *sideStep74.Set(lastX, (lastY + TILE_SIZE_PX));
                        Coord sideXStep = *sideStep75.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthWest;
                        }
                    }
                }
            } else if (deltaX > deltaY) {
                {
                    *pCell = *step76.Set(lastX - TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirWest;
                    }
                }
                {
                    *pCell = *step77.Set(lastX - TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *sideStep76.Set(lastX, (lastY + TILE_SIZE_PX));
                        Coord sideXStep = *sideStep77.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthWest;
                        }
                    }
                }
                {
                    *pCell = *step78.Set(lastX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirNorth;
                    }
                }
                {
                    *pCell = *step79.Set(lastX + TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *sideStep78.Set(lastX, (lastY - TILE_SIZE_PX));
                        Coord sideXStep = *sideStep79.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthEast;
                        }
                    }
                }
                return s_gruntDirCenter;
            } else {
                {
                    *pCell = *step80.Set(lastX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirNorth;
                    }
                }
                {
                    *pCell = *step81.Set(lastX - TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirWest;
                    }
                }
                {
                    *pCell = *step82.Set(lastX + TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *sideStep80.Set(lastX, (lastY - TILE_SIZE_PX));
                        Coord sideXStep = *sideStep81.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthEast;
                        }
                    }
                }
                {
                    *pCell = *step83.Set(lastX - TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *sideStep82.Set(lastX, (lastY + TILE_SIZE_PX));
                        Coord sideXStep = *sideStep83.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthWest;
                        }
                    }
                }
                return s_gruntDirCenter;
            }
            break;
        }
        case DIR_SOUTHWEST: {
            i32 deltaY = g->EntrancePx().m_y - goalY;
            i32 deltaX = goalX - g->EntrancePx().m_x;
            if (deltaX < deltaY) {
                {
                    *pCell = *step84.Set(lastX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirSouth;
                    }
                }
                {
                    *pCell = *step85.Set(lastX + TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *sideStep84.Set(lastX, (lastY + TILE_SIZE_PX));
                        Coord sideXStep = *sideStep85.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthEast;
                        }
                    }
                }
                {
                    *pCell = *step86.Set(lastX - TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirWest;
                    }
                }
                {
                    *pCell = *step87.Set(lastX - TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *sideStep86.Set(lastX, (lastY - TILE_SIZE_PX));
                        Coord sideXStep = *sideStep87.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthWest;
                        }
                    }
                }
                return s_gruntDirCenter;
            } else if (deltaX > deltaY) {
                {
                    *pCell = *step88.Set(lastX - TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirWest;
                    }
                }
                {
                    *pCell = *step89.Set(lastX - TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *sideStep88.Set(lastX, (lastY - TILE_SIZE_PX));
                        Coord sideXStep = *sideStep89.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthWest;
                        }
                    }
                }
                {
                    *pCell = *step90.Set(lastX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirSouth;
                    }
                }
                {
                    *pCell = *step91.Set(lastX + TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *sideStep90.Set(lastX, (lastY + TILE_SIZE_PX));
                        Coord sideXStep = *sideStep91.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthEast;
                        }
                    }
                }
                return s_gruntDirCenter;
            } else {
                {
                    *pCell = *step92.Set(lastX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirSouth;
                    }
                }
                {
                    *pCell = *step93.Set(lastX - TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return s_gruntDirWest;
                    }
                }
                {
                    *pCell = *step94.Set(lastX + TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *sideStep92.Set(lastX, (lastY + TILE_SIZE_PX));
                        Coord sideXStep = *sideStep93.Set((lastX + TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirSouthEast;
                        }
                    }
                }
                {
                    *pCell = *step95.Set(lastX - TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        Coord sideYStep = *sideStep94.Set(lastX, (lastY - TILE_SIZE_PX));
                        Coord sideXStep = *sideStep95.Set((lastX - TILE_SIZE_PX), lastY);
                        sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideYStep.m_x >> TILE_SHIFT_PX,
                            sideYStep.m_y >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            sideXStep.m_x >> TILE_SHIFT_PX,
                            sideXStep.m_y >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return s_gruntDirNorthWest;
                        }
                    }
                }
            }
            break;
        }
        default:
            break;
    }
    return s_gruntDirCenter;
}

// @early-stop
RVA(0x00075af0, 0x111)
CGrunt* CTriggerMgr::HitTestCell(i32 x, i32 y, i32* outRow, i32* outCol, i32 exact) {
    i32 ix = x >> TILE_SHIFT_PX;
    i32 iy = y >> TILE_SHIFT_PX;
    CMapMgr* plane = g_gameReg->m_tileGrid;
    i32 attr;
    if (ix >= plane->m_width || iy >= plane->m_height) {
        attr = -1;
    } else {
        attr = plane->m_rowInts[iy][ix * 7 + 1];
    }
    if (attr == -1) {
        return 0;
    }
    i32 row = (attr >> 8) & 0xff;
    i32 col = attr & 0xff;
    CGrunt* cell = m_grid[col + row * TM_GRID_COLS];
    if (cell == NULL || cell->m_entranceCommitted == 0) {
        return 0;
    }

    if (exact == 0) {
        CGameObject* o = cell->m_object;
        RECT box;
        box.top = y - 7;
        box.bottom = y + 7;
        box.left = x - 7;
        box.right = x + 7;
        i32 ox = o->m_screenX - 7;
        i32 oy = o->m_screenY - 7;
        if (box.left > ox + 14 || box.right < ox || box.top > oy + 14 || box.bottom < oy) {
            return 0;
        }
        *outRow = row;
        *outCol = col;
        return cell;
    }
    CGameObject* o = cell->m_object;
    if (o->m_screenX != x || o->m_screenY != y) {
        return 0;
    }
    *outRow = row;
    *outCol = col;
    return cell;
}

// @early-stop
RVA(0x00075c60, 0x1ba)
CGrunt* CTriggerMgr::FindGruntAt(i32 px, i32 py, RECT* span, i32* outCol, i32* outRow, RECT* src) {
    i32 tcol = px >> TILE_SHIFT_PX;
    i32 trow = py >> TILE_SHIFT_PX;
    RECT rc;
    if (src) {
        CopyRect(&rc, src);
    } else {
        SetRect(
            &rc,
            px - span->left * 32 - 7,
            py - span->top * 32 - 7,
            span->right * 32 + px + 7,
            span->bottom * 32 + py + 7
        );
    }
    i32 xEnd = span->right + tcol + 1;
    i32 x = tcol - span->left - 1;

    if (static_cast<u32>(x) <= static_cast<u32>(xEnd)) {
        do {
            i32 yEnd = span->bottom + trow + 1;
            for (i32 y = trow - span->top - 1; static_cast<u32>(y) <= static_cast<u32>(yEnd); y++) {
                if (static_cast<u32>(x) >= static_cast<u32>(g_gameReg->m_tileGrid->m_width)) {
                    continue;
                }
                if (static_cast<u32>(y) >= static_cast<u32>(g_gameReg->m_tileGrid->m_height)) {
                    continue;
                }
                CMapMgr* grid = g_gameReg->m_tileGrid;
                i32 val;
                if (static_cast<u32>(x) < static_cast<u32>(grid->m_width)
                    && static_cast<u32>(y) < static_cast<u32>(grid->m_height)) {
                    val = grid->m_rows[y][x].m_occupantId;
                } else {
                    val = -1;
                }
                if (val == -1) {
                    continue;
                }
                i32 col = val & 0xff;
                i32 row = (val >> 8) & 0xff;
                CGrunt* g = m_grid[col + row * TM_GRID_COLS];
                if (!g) {
                    continue;
                }
                if (!g->m_entranceCommitted) {
                    continue;
                }
                i32 sx = g->m_object->m_screenX - 7;
                i32 sy = g->m_object->m_screenY - 7;
                if (rc.left <= sx + 0xe && rc.right >= sx && rc.top <= sy + 0xe
                    && rc.bottom >= sy) {
                    *outCol = row;
                    *outRow = col;
                    return g;
                }
            }
            x++;
        } while (static_cast<u32>(x) <= static_cast<u32>(xEnd));
    }
    return 0;
}
