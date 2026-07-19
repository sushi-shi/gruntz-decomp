// DDrawPolyFill.h - the scanline polygon-fill edge record (owner:
// DDrawPolyFill.cpp; the g_rasterEdgeL/g_rasterEdgeR element).
#ifndef GRUNTZ_DDRAWMGR_DDRAWPOLYFILL_H
#define GRUNTZ_DDRAWMGR_DDRAWPOLYFILL_H

#include <Ints.h>

#include <Image/RasterVtx.h>
// The per-scanline edge record IS the raster vertex row (stride 0x1c, fixed x at
// +0x10 == ClipVtx::fx). The padded FillEdgeRow shell is dissolved (2026-07-19).
typedef ClipVtx FillEdgeRow;

#endif // GRUNTZ_DDRAWMGR_DDRAWPOLYFILL_H
