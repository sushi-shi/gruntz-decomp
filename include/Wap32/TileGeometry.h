#ifndef WAP32_TILEGEOMETRY_H
#define WAP32_TILEGEOMETRY_H

#include <Enums.h>

GZ_ENUM_CONST_BEGIN(TileGeometry)
    TILE_SIZE_PX = 32,
    TILE_HALF_PX = 16,
    TILE_SHIFT_PX = 5,
    TILE_MASK_PX = 0x1f
GZ_ENUM_CONST_END(TileGeometry)

inline i32 TileShiftForSize(i32 tileSize) {
    i32 shift = 0;
    while (tileSize > 1) {
        tileSize >>= 1;
        shift++;
    }
    return shift;
}

#endif // WAP32_TILEGEOMETRY_H
