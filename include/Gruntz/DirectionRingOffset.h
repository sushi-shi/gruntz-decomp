#ifndef GRUNTZ_GRUNTZ_DIRECTIONRINGOFFSET_H
#define GRUNTZ_GRUNTZ_DIRECTIONRINGOFFSET_H

#include <Gruntz/CoordNode.h>
#include <Gruntz/DirectionRingIndex.h>

inline Coord DirectionRingOffset(DirectionRingIndex direction, i32 magnitude) {
    Coord offset = {0, 0};
    switch (direction) {
        case DIRECTION_RING_NORTH:
            offset.m_y = -magnitude;
            break;
        case DIRECTION_RING_NORTHEAST:
            offset.m_x = magnitude;
            offset.m_y = -magnitude;
            break;
        case DIRECTION_RING_EAST:
            offset.m_x = magnitude;
            break;
        case DIRECTION_RING_SOUTHEAST:
            offset.m_x = magnitude;
            offset.m_y = magnitude;
            break;
        case DIRECTION_RING_SOUTH:
            offset.m_y = magnitude;
            break;
        case DIRECTION_RING_SOUTHWEST:
            offset.m_x = -magnitude;
            offset.m_y = magnitude;
            break;
        case DIRECTION_RING_WEST:
            offset.m_x = -magnitude;
            break;
        case DIRECTION_RING_NORTHWEST:
            offset.m_x = -magnitude;
            offset.m_y = -magnitude;
            break;
    }
    return offset;
}

#endif // GRUNTZ_GRUNTZ_DIRECTIONRINGOFFSET_H
