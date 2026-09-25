#ifndef GRUNTZ_GRUNTZ_CARDINALDIRECTIONOFFSET_H
#define GRUNTZ_GRUNTZ_CARDINALDIRECTIONOFFSET_H

#include <Gruntz/CardinalDir.h>
#include <Gruntz/CoordNode.h>

inline Coord CardinalDirectionOffset(CardinalDir direction, i32 magnitude) {
    Coord offset = {0, 0};
    switch (direction) {
        case CARDINAL_NORTH:
            offset.m_y = -magnitude;
            break;
        case CARDINAL_EAST:
            offset.m_x = magnitude;
            break;
        case CARDINAL_SOUTH:
            offset.m_y = magnitude;
            break;
        case CARDINAL_WEST:
            offset.m_x = -magnitude;
            break;
    }
    return offset;
}

#endif // GRUNTZ_GRUNTZ_CARDINALDIRECTIONOFFSET_H
