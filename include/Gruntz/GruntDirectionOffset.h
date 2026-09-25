#ifndef GRUNTZ_GRUNTZ_GRUNTDIRECTIONOFFSET_H
#define GRUNTZ_GRUNTZ_GRUNTDIRECTIONOFFSET_H

#include <Gruntz/CoordNode.h>
#include <Gruntz/GruntDirection.h>
#include <Wap32/TileGeometry.h>

inline Coord GruntDirectionPixelOffset(const GruntDirectionCell& direction) {
    Coord result = {(direction.m_column - 1) * TILE_SIZE_PX, (direction.m_row - 1) * TILE_SIZE_PX};
    return result;
}

#endif // GRUNTZ_GRUNTZ_GRUNTDIRECTIONOFFSET_H
