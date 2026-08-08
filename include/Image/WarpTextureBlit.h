#ifndef GRUNTZ_IMAGE_WARPTEXTUREBLIT_H
#define GRUNTZ_IMAGE_WARPTEXTUREBLIT_H

#include <Image/RasterVtx.h>
#include <Ints.h>

extern "C" const float g_rasterScale;
extern "C" const float g_rasterScaleNeg;

i32 WarpIsPow2(i32 x);

extern i32 g_warpU;
extern i32 g_warpV;
extern i16* g_warpTexBase;
extern i32 g_warpUStep;
extern i32 g_warpVStep;
extern i32 g_warpUMask;
extern i16 g_warpColorkey;
#endif // GRUNTZ_IMAGE_WARPTEXTUREBLIT_H
