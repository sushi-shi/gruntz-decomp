// TileGrid.h - the tile occupancy grid (the game registry's +0x70 object). A
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
#include <Gruntz/MapMgr.h>

// CTileGrid IS CMapMgr (RTTI .?AVCMapMgr@@, vtable 0x1ea3b4): the registry's +0x70 object
// is a CGruntzMapMgr whose CMapMgr base carries exactly the fields this view described -
// the row-pointer table at +0x08 (now typed i32** on the real class), the tile width at
// +0x0c and the height at +0x10 - and the view's `Notify` is a real CMapMgr method. The
// struct is dissolved; the NAME stays as a typedef so the ~26 consumers keep reading, and
// the phantom ?Notify@CTileGrid@@ (a method of a class owning no retail address, hence
// unlinkable) is gone with it.
typedef CMapMgr CTileGrid;

#endif // GRUNTZ_GRUNTZ_CTILEGRID_H
