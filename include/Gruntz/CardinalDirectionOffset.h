#ifndef GRUNTZ_GRUNTZ_CARDINALDIRECTIONOFFSET_H
#define GRUNTZ_GRUNTZ_CARDINALDIRECTIONOFFSET_H

#include <Gruntz/CardinalDir.h>
#include <Gruntz/CoordNode.h>

inline Coord CardinalDirectionOffset(CardinalDir direction, i32 magnitude) {
    switch (direction) {
        case CARDINAL_NORTH:
            return Coord(0, -magnitude);
        case CARDINAL_EAST:
            return Coord(magnitude, 0);
        case CARDINAL_SOUTH:
            return Coord(0, magnitude);
        case CARDINAL_WEST:
            return Coord(-magnitude, 0);
    }
    return Coord(0, 0);
}

#endif // GRUNTZ_GRUNTZ_CARDINALDIRECTIONOFFSET_H
