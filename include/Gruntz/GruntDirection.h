#ifndef GRUNTZ_GRUNTZ_GRUNTDIRECTION_H
#define GRUNTZ_GRUNTZ_GRUNTDIRECTION_H

#include <Enums.h>

// The eight-way step direction carried in GruntDirectionCell::direction, plus
// the no-move centre. Clockwise from North, which is what makes
// GruntDirectionCell::RotateClockwise/RotateCounterclockwise a plain +/- on the
// value.
//
// Proven four independent ways by the eight globals in GruntSteps.cpp, whose
// ctor is GruntDirectionCell(row, column, direction) over a 3x3 neighbour grid
// (row 0 = north, 2 = south; column 0 = west, 2 = east):
//
//   g_gruntMoveDirNorth     (0, 1, 1)   g_gruntMoveDirSouth     (2, 1, 5)
//   g_gruntMoveDirNorthEast (0, 2, 2)   g_gruntMoveDirSouthWest (2, 0, 6)
//   g_gruntMoveDirEast      (1, 2, 3)   g_gruntMoveDirWest      (1, 0, 7)
//   g_gruntMoveDirSouthEast (2, 2, 4)   g_gruntMoveDirNorthWest (0, 0, 8)
//   g_gruntMoveDirCenter    (1, 1, 0)
//
//   1. the global's own name;
//   2. the (row, column) pair, which is that name's cell in the 3x3;
//   3. the pixel delta its CGruntSteps arm applies (North -> y - 0x20,
//      NorthEast -> x + 0x20 and y - 0x20, ...);
//   4. the rotation helpers, which require the ring to be in rotational order.
GZ_ENUM_BEGIN(GruntDirection)
// The centre of the 3x3: "no step". Not part of the ring, so the rotate
// helpers never produce it.
    DIR_CENTER = 0,
    DIR_NORTH = 1,
    DIR_NORTHEAST = 2,
    DIR_EAST = 3,
    DIR_SOUTHEAST = 4,
    DIR_SOUTH = 5,
    DIR_SOUTHWEST = 6,
    DIR_WEST = 7,
    DIR_NORTHWEST = 8
GZ_ENUM_END(GruntDirection)

#endif // GRUNTZ_GRUNTZ_GRUNTDIRECTION_H
