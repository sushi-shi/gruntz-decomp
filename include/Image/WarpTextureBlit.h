#ifndef GRUNTZ_IMAGE_WARPTEXTUREBLIT_H
#define GRUNTZ_IMAGE_WARPTEXTUREBLIT_H

#include <Enums.h>
#include <Image/RasterVtx.h>
#include <Ints.h>

GZ_ENUM_CONST_BEGIN(WarpTextureFixedPoint)
    WARP_TEXTURE_FRACTION_BITS = 14
GZ_ENUM_CONST_END(WarpTextureFixedPoint)

extern const float g_rasterScale;
extern const float g_rasterScaleNeg;

i32 WarpIsPow2(i32 x);

extern i32 g_warpU;
extern i32 g_warpV;
extern i16* g_warpTexBase;
extern i32 g_warpUStep;
extern i32 g_warpVStep;
extern i32 g_warpUMask;
extern i16 g_warpColorkey;
#endif // GRUNTZ_IMAGE_WARPTEXTUREBLIT_H
