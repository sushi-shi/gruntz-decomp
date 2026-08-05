#ifndef GRUNTZ_GRUNTZ_DIRECTIONRINGINDEX_H
#define GRUNTZ_GRUNTZ_DIRECTIONRINGINDEX_H

#include <Enums.h>

// Zero-based positions in a clockwise eight-direction ring. This is the index
// space used by CBootyState::m_sprintSprites; GruntDirection itself is one-based
// because zero means the centre/no-move cell there.
GZ_ENUM_BEGIN(DirectionRingIndex)
    DIRECTION_RING_NORTH = 0,
    DIRECTION_RING_NORTHEAST = 1,
    DIRECTION_RING_EAST = 2,
    DIRECTION_RING_SOUTHEAST = 3,
    DIRECTION_RING_SOUTH = 4,
    DIRECTION_RING_SOUTHWEST = 5,
    DIRECTION_RING_WEST = 6,
    DIRECTION_RING_NORTHWEST = 7
GZ_ENUM_END(DirectionRingIndex)

#endif // GRUNTZ_GRUNTZ_DIRECTIONRINGINDEX_H
