// CTileGrid.h - the tile occupancy grid (the game registry's +0x70 object). A
// flat row-table of pointers (m_8): m_8[tileY] is a row base, and each cell is a
// 0x1c-byte (7-dword) record indexed by tile-x, so a field at cell byte 0xN is
// ((i32*)m_8[tileY])[tileX*7 + N/4]. Cell dword 0 carries flag bits (byte+3 bit
// 0x20 = occupied), dword 1 is the packed owner word, dword 4 (+0x10) is the tile
// code ('A'=0x41 / 'B'=0x42 = action tile). m_c/m_10 are the grid's tile-width /
// tile-height bounds. Shared by CGameRegistry.h (the registry singleton), the
// CGrunt animation/entrance paths, and the vehicle-grunt sprite loader.
#ifndef GRUNTZ_GRUNTZ_CTILEGRID_H
#define GRUNTZ_GRUNTZ_CTILEGRID_H

#include <Ints.h>

struct CTileGrid {
    // The tile-system notifier facet (registry +0x70 viewed by the status-bar
    // updaters): flags a cell dirty/updated. NO-body -> the call reloc-masks.
    void Notify(i32 x, i32 y, i32 state);

    char m_pad0[0x8];
    i32** m_8; // +0x08  row-pointer table (cell = (i32*)m_8[tileY] + tileX*7)
    i32 m_c;   // +0x0c  width in tiles
    i32 m_10;  // +0x10  height in tiles
};

#endif // GRUNTZ_GRUNTZ_CTILEGRID_H
