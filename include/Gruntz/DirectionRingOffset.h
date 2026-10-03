#ifndef GRUNTZ_GRUNTZ_DIRECTIONRINGOFFSET_H
#define GRUNTZ_GRUNTZ_DIRECTIONRINGOFFSET_H

#include <Gruntz/CoordNode.h>
#include <Gruntz/DirectionRingIndex.h>

inline Coord DirectionRingOffset(DirectionRingIndex direction, i32 magnitude) {
    switch (direction) {
        case DIRECTION_RING_NORTH:
            return Coord(0, -magnitude);
        case DIRECTION_RING_NORTHEAST:
            return Coord(magnitude, -magnitude);
        case DIRECTION_RING_EAST:
            return Coord(magnitude, 0);
        case DIRECTION_RING_SOUTHEAST:
            return Coord(magnitude, magnitude);
        case DIRECTION_RING_SOUTH:
            return Coord(0, magnitude);
        case DIRECTION_RING_SOUTHWEST:
            return Coord(-magnitude, magnitude);
        case DIRECTION_RING_WEST:
            return Coord(-magnitude, 0);
        case DIRECTION_RING_NORTHWEST:
            return Coord(-magnitude, -magnitude);
    }
    return Coord(0, 0);
}

#endif // GRUNTZ_GRUNTZ_DIRECTIONRINGOFFSET_H
