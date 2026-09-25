#ifndef WAP32_TILEGEOMETRY_H
#define WAP32_TILEGEOMETRY_H

#include <Enums.h>
#include <Ints.h>
#include <Lith/BDefs.h>

GZ_ENUM_CONST_BEGIN(TileGeometry)
    TILE_SIZE_PX = 32,
    TILE_HALF_PX = 16,
    TILE_SHIFT_PX = 5,
    TILE_MASK_PX = 0x1f
GZ_ENUM_CONST_END(TileGeometry)

inline i32 SquaredDistance(i32 dx, i32 dy) {
    return SQR(dx) + SQR(dy);
}

#define TILE_SHIFT_INTO(out, size)                                                                 \
    do {                                                                                           \
        (out) = 0;                                                                                 \
        i32 tileExtent = (size);                                                                   \
        while (tileExtent > 1) {                                                                   \
            tileExtent >>= 1;                                                                      \
            (out) = (out) + 1;                                                                     \
        }                                                                                          \
    } while (0)

inline i32 TileShiftForSize(i32 tileSize) {
    i32 shift = 0;
    while (tileSize > 1) {
        tileSize >>= 1;
        shift++;
    }
    return shift;
}

#define SCREEN_TILE_COMPONENT(pixel) ((pixel) >> TILE_SHIFT_PX)
#define TILE_CENTER_COMPONENT(tile) (((tile) << TILE_SHIFT_PX) + TILE_HALF_PX)
#define SNAP_TILE_CENTER_COMPONENT(pixel) (((pixel) & ~TILE_MASK_PX) + TILE_HALF_PX)

#define CLAMP_PIXEL_COMPONENT(pixel, extent)                                                       \
    if ((pixel) < 0) {                                                                             \
        (pixel) = 0;                                                                               \
    } else {                                                                                       \
        if ((pixel) >= (extent)) {                                                                 \
            (pixel) = (extent) - 1;                                                                \
        }                                                                                          \
    }

#define SQUARED_DISTANCE_COMPONENTS(dx, dy) ((dx) * (dx) + (dy) * (dy))

#define CLAMP_UPPER_INPLACE(value, upper)                                                          \
    if ((value) > (upper)) {                                                                       \
        (value) = (upper);                                                                         \
    }
#endif // WAP32_TILEGEOMETRY_H
