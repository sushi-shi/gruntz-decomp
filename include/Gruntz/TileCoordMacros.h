#ifndef GRUNTZ_TILECOORDMACROS_H
#define GRUNTZ_TILECOORDMACROS_H

#include <Gruntz/CoordNode.h>

#define DECLARE_TILE_CENTER_PIXEL_PAIR_Y_FIRST(pixelY, pixelX, tileY, tileX)                       \
    i32 pixelY = (tileY << TILE_SHIFT_PX) + TILE_HALF_PX;                                          \
    i32 pixelX = (tileX << TILE_SHIFT_PX) + TILE_HALF_PX;

#define DECLARE_TILE_CENTER_PIXEL_PAIR(pixelX, pixelY, tileX, tileY)                               \
    i32 pixelX = (tileX << TILE_SHIFT_PX) + TILE_HALF_PX;                                          \
    i32 pixelY = (tileY << TILE_SHIFT_PX) + TILE_HALF_PX;

#endif // GRUNTZ_TILECOORDMACROS_H
