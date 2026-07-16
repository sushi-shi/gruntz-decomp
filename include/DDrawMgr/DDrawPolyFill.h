// DDrawPolyFill.h - the scanline polygon-fill edge record (owner:
// DDrawPolyFill.cpp; the g_rasterEdgeL/g_rasterEdgeR element).
#ifndef GRUNTZ_DDRAWMGR_DDRAWPOLYFILL_H
#define GRUNTZ_DDRAWMGR_DDRAWPOLYFILL_H

#include <Ints.h>

// A per-scanline edge record (stride 0x1c); the interpolated fixed-point x lands
// at +0x10. The two active-edge tables hold the left/right span endpoints per row.
struct FillEdgeRow {
    char p0[0x10];
    i32 m_10; // +0x10  interpolated x (14-bit fixed point)
    char p1[0x1c - 0x14];
};

#endif // GRUNTZ_DDRAWMGR_DDRAWPOLYFILL_H
