#ifndef GRUNTZ_TILECOORDMACROS_H
#define GRUNTZ_TILECOORDMACROS_H

#define SET_TILE_CENTER_PIXEL_PAIR(dstX, dstY, tileX, tileY)                                       \
    dstX = (tileX << TILE_SHIFT_PX) + TILE_HALF_PX;                                                \
    dstY = (tileY << TILE_SHIFT_PX) + TILE_HALF_PX;

#define DECLARE_TILE_CENTER_PIXEL_PAIR(pixelX, pixelY, tileX, tileY)                               \
    i32 pixelX = (tileX << TILE_SHIFT_PX) + TILE_HALF_PX;                                          \
    i32 pixelY = (tileY << TILE_SHIFT_PX) + TILE_HALF_PX;

#endif // GRUNTZ_TILECOORDMACROS_H
