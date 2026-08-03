#ifndef GRUNTZ_GRUNTZ_ROCKNEIGHBORMASK_H
#define GRUNTZ_GRUNTZ_ROCKNEIGHBORMASK_H

#include <Enums.h>

// Which of a tile's four orthogonal neighbours is a TILEKIND_GIANT_ROCK, as
// built by CGrunt's rock-edge search before it decides which way to step.
//
// The bits are read straight off the four tests that build the mask, each of
// which indexes the tile grid one step away and compares against GIANT_ROCK:
//
//   1  [tileY + 1][tileX * 7 + 4]    the tile BELOW
//   2  [tileY - 1][tileX * 7 + 4]    the tile ABOVE
//   4  [tileY]    [tileX * 7 + 11]   the tile RIGHT   (+7 is one tile of stride)
//   8  [tileY]    [tileX * 7 - 3]    the tile LEFT    (-7 + 4)
//
// The switch that consumes it steps TOWARDS the rocks, which is what makes the
// combinations read: RIGHT|BELOW moves down-right, RIGHT|ABOVE|BELOW moves right
// only because the vertical pair cancels, LEFT|RIGHT|BELOW moves down only.
GZ_ENUM_FLAGS_BEGIN(RockNeighborMask, i32)
    ROCKADJ_BELOW = 1,
    ROCKADJ_ABOVE = 2,
    ROCKADJ_RIGHT = 4,
    ROCKADJ_LEFT = 8
GZ_ENUM_FLAGS_END(RockNeighborMask, i32)

#endif // GRUNTZ_GRUNTZ_ROCKNEIGHBORMASK_H
