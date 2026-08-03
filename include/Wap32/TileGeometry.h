#ifndef WAP32_TILEGEOMETRY_H
#define WAP32_TILEGEOMETRY_H

#include <Enums.h>

// The 32x32 pixel tile grid the whole game is laid out on.
//
// One size, spelled five ways across ~1300 sites, and every one of them is this
// grid. The four spellings pin each other:
//
//   x >> TILE_SHIFT_PX          pixel -> tile     (762 sites)
//   (ty << TILE_SHIFT_PX) + TILE_HALF_PX          tile -> pixel CENTRE
//   (x & ~TILE_MASK_PX) + TILE_HALF_PX            snap a pixel to that centre
//   y - TILE_SIZE_PX                              step exactly one tile
//
// TILE_HALF_PX being half of TILE_SIZE_PX is what makes the second and third
// forms agree, and CRollingBall stepping m_lastTilePx by 0x20 while
// CTriggerMgr's arrow arm steps by 32 is the same distance written two ways.
//
// NOT this domain, despite sharing the shift and the mask: a bit-array index
// (`p[idx >> 5] |= 1 << (idx & 0x1f)`), where 32 is the bits per word (see
// BitArray.cpp and TypeKeyColl.cpp) - and a 16-bit pixel's channel extract
// (`(a >> 5) & 0x1f`) in the shade blitter, where those are the green field of
// RGB565. Both are left alone.
GZ_ENUM_CONST_BEGIN(TileGeometry)
    TILE_SIZE_PX = 32,
    TILE_HALF_PX = 16,
    // log2(TILE_SIZE_PX): the shift that divides a pixel coordinate into a tile
    // index, and multiplies back.
    TILE_SHIFT_PX = 5,
    // TILE_SIZE_PX - 1: the offset WITHIN a tile. `& ~TILE_MASK_PX` clears it,
    // which is how a pixel snaps to its tile's origin.
    TILE_MASK_PX = 0x1f
GZ_ENUM_CONST_END(TileGeometry)

#endif // WAP32_TILEGEOMETRY_H
