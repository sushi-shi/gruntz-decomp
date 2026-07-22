// WarpTextureBlit.h - the WarpTextureBlit.cpp TU's exported globals/functions.
#ifndef GRUNTZ_IMAGE_WARPTEXTUREBLIT_H
#define GRUNTZ_IMAGE_WARPTEXTUREBLIT_H

#include <Ints.h>

extern i32 g_rasterDestPtr; // 0x002becf4

extern i32 g_rasterDestRow; // 0x002a2ce8

extern "C" const float g_rasterScale;    // 0x001efb18  +16384.0f fixed-point scale
extern "C" const float g_rasterScaleNeg; // 0x001efb1c  -16384.0f

#endif // GRUNTZ_IMAGE_WARPTEXTUREBLIT_H
