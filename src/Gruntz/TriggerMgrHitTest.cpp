#include <Gruntz/Brickz.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDirection.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/MapMgr.h>
#include <Gruntz/TileGrid.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TriggerMgrRecords.h>
#include <Wap32/TileGeometry.h>

#include <stddef.h>

// The eight-way step deflector: given a grunt whose step in `dir` is blocked,
// pick the neighbouring cell it should try instead.
//
// UNREFERENCED IN RETAIL. Nothing calls 0x0006f2f0 and nothing calls its
// incremental-link thunk (0x3a4e) either - the only rel32 in the whole image
// that names the body is that thunk. It is a finished, shipped, dead feature:
// the routine survives because its .obj was on the link line. That is why
// 21,031 bytes of real x86 sat uncarved in config/retail/functions.tsv.
//
// It is nine near-identical blocks deep and that is what makes it 21 KB: a
// three-way tie-break times four candidate cells times eight directions, all
// written out. The structure is completely regular:
//
//   * candidates are always the ring neighbours +-45 and +-90 degrees off
//     `dir`, never `dir` itself and never anything further round;
//   * which of the two SIDES is tried first is the three-way test - the goal
//     lies to one side, to the other, or dead ahead;
//   * a candidate is accepted when TmFlagsAllow passes on its cell, and a
//     DIAGONAL candidate additionally needs BRICKZ_CELL_ROUTE_MASKB on both of
//     its orthogonal neighbours, which is the corner-cut rule;
//   * nothing matched, or `dir` out of range, returns the centre cell.
//
// Retail tail-merged the identical `return <cell>` epilogues both inside and
// ACROSS the eight arms, which is where the nine shared exits at 0x707a7,
// 0x72f13, 0x7211f, 0x742a8, 0x731f0, 0x7421b, 0x73bb7, 0x73845 and 0x744e7
// come from, and it stopped inlining Lookup/GetTileGrid/Coord::Set/
// TmFlagsAllow partway through the fifth arm, which is why those four have
// out-of-line bodies in this TU at all and why they have exactly one caller
// each.
//
// Parameters 4 and 5 are dead: `ret 0x24` cleans nine dwords, the hidden
// return buffer plus eight, and the two slots at E+0x14 / E+0x18 are never
// read on any path.
// @early-stop
// 21,031 bytes, 96 return sites, eight hand-written arms plus cl's cross-arm
// tail merging. The shape above is transcribed from the disassembly; the
// stack-temp numbering, the tail merging and the inline/out-of-line split are
// all whole-function optimizer state and will not land on the first pass.
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
    if (g->EntrancePx().m_x == goalX && g->EntrancePx().m_y == goalY) {
        return g_gruntDirCenter;
    }
    i32 mask = g->m_arrivalFlags | BRICKZ_CELL_OCCUPIED;
    i32 lastX = g->m_lastTilePx.m_x;
    i32 lastY = g->m_lastTilePx.m_y;
    i32 pass = g->m_passableMask;
    switch (dir) {
        case DIR_NORTH:
            if (g->EntrancePx().m_x < goalX) {
                {
                    Coord step;
                    *pCell = *step.Set(lastX - TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY - TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX - TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirNorthWest;
                        }
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX - TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirWest;
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX + TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY - TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX + TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirNorthEast;
                        }
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX + TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirEast;
                    }
                }
            } else if (g->EntrancePx().m_x > goalX) {
                {
                    Coord step;
                    *pCell = *step.Set(lastX + TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY - TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX + TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirNorthEast;
                        }
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX + TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirEast;
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX - TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY - TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX - TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirNorthWest;
                        }
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX - TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirWest;
                    }
                }
            } else {
                {
                    Coord step;
                    *pCell = *step.Set(lastX - TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY - TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX - TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirNorthWest;
                        }
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX + TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY - TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX + TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirNorthEast;
                        }
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX - TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirWest;
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX + TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirEast;
                    }
                }
            }
            break;
        case DIR_SOUTH:
            if (g->EntrancePx().m_x < goalX) {
                {
                    Coord step;
                    *pCell = *step.Set(lastX - TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY + TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX - TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirSouthWest;
                        }
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX - TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirWest;
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX + TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY + TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX + TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirSouthEast;
                        }
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX + TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirEast;
                    }
                }
            } else if (g->EntrancePx().m_x > goalX) {
                {
                    Coord step;
                    *pCell = *step.Set(lastX + TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY + TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX + TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirSouthEast;
                        }
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX + TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirEast;
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX - TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY + TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX - TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirSouthWest;
                        }
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX - TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirWest;
                    }
                }
            } else {
                {
                    Coord step;
                    *pCell = *step.Set(lastX - TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY + TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX - TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirSouthWest;
                        }
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX + TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY + TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX + TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirSouthEast;
                        }
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX - TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirWest;
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX + TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirEast;
                    }
                }
            }
            break;
        case DIR_EAST:
            if (g->EntrancePx().m_y < goalY) {
                {
                    Coord step;
                    *pCell = *step.Set(lastX + TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY - TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX + TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirNorthEast;
                        }
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirNorth;
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX + TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY + TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX + TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirSouthEast;
                        }
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirSouth;
                    }
                }
            } else if (g->EntrancePx().m_y > goalY) {
                {
                    Coord step;
                    *pCell = *step.Set(lastX + TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY + TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX + TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirSouthEast;
                        }
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirSouth;
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX + TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY - TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX + TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirNorthEast;
                        }
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirNorth;
                    }
                }
            } else {
                {
                    Coord step;
                    *pCell = *step.Set(lastX + TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY - TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX + TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirNorthEast;
                        }
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX + TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY + TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX + TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirSouthEast;
                        }
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirNorth;
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirSouth;
                    }
                }
            }
            break;
        case DIR_WEST:
            if (g->EntrancePx().m_y < goalY) {
                {
                    Coord step;
                    *pCell = *step.Set(lastX - TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY - TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX - TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirNorthWest;
                        }
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirNorth;
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX - TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY + TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX - TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirSouthWest;
                        }
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirSouth;
                    }
                }
            } else if (g->EntrancePx().m_y > goalY) {
                {
                    Coord step;
                    *pCell = *step.Set(lastX - TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY + TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX - TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirSouthWest;
                        }
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirSouth;
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX - TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY - TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX - TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirNorthWest;
                        }
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirNorth;
                    }
                }
            } else {
                {
                    Coord step;
                    *pCell = *step.Set(lastX - TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY - TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX - TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirNorthWest;
                        }
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX - TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY + TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX - TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirSouthWest;
                        }
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirNorth;
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirSouth;
                    }
                }
            }
            break;
        case DIR_NORTHEAST:
            if (g->EntrancePx().m_x - goalX < goalY - g->EntrancePx().m_y) {
                {
                    Coord step;
                    *pCell = *step.Set(lastX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirNorth;
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX - TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY - TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX - TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirNorthWest;
                        }
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX + TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirEast;
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX + TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY + TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX + TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirSouthEast;
                        }
                    }
                }
            } else if (g->EntrancePx().m_x - goalX > goalY - g->EntrancePx().m_y) {
                {
                    Coord step;
                    *pCell = *step.Set(lastX + TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirEast;
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX + TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY + TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX + TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirSouthEast;
                        }
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirNorth;
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX - TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY - TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX - TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirNorthWest;
                        }
                    }
                }
            } else {
                {
                    Coord step;
                    *pCell = *step.Set(lastX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirNorth;
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX + TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirEast;
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX - TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY - TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX - TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirNorthWest;
                        }
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX + TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY + TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX + TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirSouthEast;
                        }
                    }
                }
            }
            break;
        case DIR_SOUTHEAST:
            if (g->EntrancePx().m_x - goalX < g->EntrancePx().m_y - goalY) {
                {
                    Coord step;
                    *pCell = *step.Set(lastX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirSouth;
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX - TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY + TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX - TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirSouthWest;
                        }
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX + TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirEast;
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX + TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY - TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX + TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirNorthEast;
                        }
                    }
                }
            } else if (g->EntrancePx().m_x - goalX > g->EntrancePx().m_y - goalY) {
                {
                    Coord step;
                    *pCell = *step.Set(lastX + TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirEast;
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX + TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY - TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX + TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirNorthEast;
                        }
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirSouth;
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX - TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY + TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX - TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirSouthWest;
                        }
                    }
                }
            } else {
                {
                    Coord step;
                    *pCell = *step.Set(lastX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirSouth;
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX + TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirEast;
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX - TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY + TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX - TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirSouthWest;
                        }
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX + TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY - TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX + TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirNorthEast;
                        }
                    }
                }
            }
            break;
        case DIR_NORTHWEST:
            if (goalX - g->EntrancePx().m_x < goalY - g->EntrancePx().m_y) {
                {
                    Coord step;
                    *pCell = *step.Set(lastX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirNorth;
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX + TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY - TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX + TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirNorthEast;
                        }
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX - TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirWest;
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX - TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY + TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX - TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirSouthWest;
                        }
                    }
                }
            } else if (goalX - g->EntrancePx().m_x > goalY - g->EntrancePx().m_y) {
                {
                    Coord step;
                    *pCell = *step.Set(lastX - TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirWest;
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX - TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY + TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX - TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirSouthWest;
                        }
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirNorth;
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX + TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY - TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX + TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirNorthEast;
                        }
                    }
                }
            } else {
                {
                    Coord step;
                    *pCell = *step.Set(lastX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirNorth;
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX - TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirWest;
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX + TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY - TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX + TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirNorthEast;
                        }
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX - TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY + TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX - TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirSouthWest;
                        }
                    }
                }
            }
            break;
        case DIR_SOUTHWEST:
            if (goalX - g->EntrancePx().m_x < g->EntrancePx().m_y - goalY) {
                {
                    Coord step;
                    *pCell = *step.Set(lastX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirSouth;
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX + TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY + TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX + TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirSouthEast;
                        }
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX - TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirWest;
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX - TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY - TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX - TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirNorthWest;
                        }
                    }
                }
            } else if (goalX - g->EntrancePx().m_x > g->EntrancePx().m_y - goalY) {
                {
                    Coord step;
                    *pCell = *step.Set(lastX - TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirWest;
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX - TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY - TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX - TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirNorthWest;
                        }
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirSouth;
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX + TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY + TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX + TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirSouthEast;
                        }
                    }
                }
            } else {
                {
                    Coord step;
                    *pCell = *step.Set(lastX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirSouth;
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX - TILE_SIZE_PX, lastY);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        return g_gruntDirWest;
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX + TILE_SIZE_PX, lastY + TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY + TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX + TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirSouthEast;
                        }
                    }
                }
                {
                    Coord step;
                    *pCell = *step.Set(lastX - TILE_SIZE_PX, lastY - TILE_SIZE_PX);
                }
                {
                    i32 cell = g_gameReg->GetTileGrid()->CellFlagsAt(
                        pCell->m_x >> TILE_SHIFT_PX,
                        pCell->m_y >> TILE_SHIFT_PX
                    );
                    *pFlags = cell;
                    if (TmFlagsAllow(cell, mask, pass)) {
                        i32 sideY = g_gameReg->GetTileGrid()->CellFlagsAt(
                            lastX >> TILE_SHIFT_PX,
                            (lastY - TILE_SIZE_PX) >> TILE_SHIFT_PX
                        );
                        i32 sideX = g_gameReg->GetTileGrid()->CellFlagsAt(
                            (lastX - TILE_SIZE_PX) >> TILE_SHIFT_PX,
                            lastY >> TILE_SHIFT_PX
                        );
                        if ((sideY & BRICKZ_CELL_ROUTE_MASKB) != 0
                            && (sideX & BRICKZ_CELL_ROUTE_MASKB) != 0) {
                            return g_gruntDirNorthWest;
                        }
                    }
                }
            }
            break;
        default:
            break;
    }
    return g_gruntDirCenter;
}

// The five helpers this function calls (CGrunt::EntrancePx, Coord::Set,
// CMapMgr::CellFlagsAt, TmFlagsAllow, CGruntzMgr::GetTileGrid) are header
// inlines whose single out-of-line COMDAT retail left in THIS unit, at
// 0x000759e0 / 0x00075a10 / 0x00075a40 / 0x00075a90 / 0x00075ad0. They carry
// their RVA() pins on the header definitions; cl only emits a body here
// because TmDeflectStep exhausts its inline budget partway through.

// @early-stop
RVA(0x00075af0, 0x111)
CGrunt* CTriggerMgr::HitTestCell(i32 x, i32 y, i32* outRow, i32* outCol, i32 exact) {
    CMapMgr* plane = g_gameReg->m_tileGrid;
    i32 ix = x >> TILE_SHIFT_PX;
    i32 iy = y >> TILE_SHIFT_PX;
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
        i32 ylo = y - 7;
        i32 yhi = y + 7;
        i32 xlo = x - 7;
        i32 xhi = x + 7;
        i32 ox = o->m_screenX - 7;
        i32 oy = o->m_screenY - 7;
        if (xlo > ox + 14 || xhi < ox || ylo > oy + 14 || yhi < oy) {
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
                CMapMgr* grid = g_gameReg->m_tileGrid;
                if (static_cast<u32>(y) >= static_cast<u32>(grid->m_height)) {
                    continue;
                }
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
