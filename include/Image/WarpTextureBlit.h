// WarpTextureBlit.h - the WarpTextureBlit.cpp TU's exported globals/functions.
#ifndef GRUNTZ_IMAGE_WARPTEXTUREBLIT_H
#define GRUNTZ_IMAGE_WARPTEXTUREBLIT_H

#include <Ints.h>
#include <Image/RasterVtx.h>        // ClipVtx (for the extern below)

extern "C" i16* g_rasterDestPtr; // 0x002becf4

extern "C" u8* g_rasterDestRow; // 0x002a2ce8

extern "C" const float g_rasterScale;    // 0x001efb18  +16384.0f fixed-point scale
extern "C" const float g_rasterScaleNeg; // 0x001efb1c  -16384.0f

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---

i32 WarpIsPow2(i32 x); // 0x145e00 (defined in ImagePolyClip.cpp - birth position)

extern i32 g_warpU;
extern i32 g_warpV;
extern void* g_warpTexBase;
extern i32 g_warpUStep;
extern i32 g_warpVStep;
extern i32 g_warpUMask;
extern i16 g_warpColorkey;
#endif // GRUNTZ_IMAGE_WARPTEXTUREBLIT_H
