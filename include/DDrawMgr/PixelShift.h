// PixelShift.h - the engine's RGB565 channel shift amounts (16bpp pixel format).
//
// The six per-channel shift counts derived once from the primary surface's pixel
// format: g_rUp/g_gUp/g_bUp are the left-shifts that PACK an 8-bit channel into its
// 16-bit slot, g_rDown/g_gDown/g_bDown the right-shifts that REDUCE a source byte to
// the channel's bit width. Every blit/shade/team-recolor path across DDrawMgr, Image,
// Font and Gruntz reads them. Their ONE definition + DATA pins live in
// src/DDrawMgr/DDSurface.cpp; each is a plain C++-linkage int (?g_rUp@@3HA ...), so a
// reference is DATA-reloc-masked. Declared once here (a minimal owner header the four
// modules can share) so consumers stop re-`extern`-ing them per-TU.
#ifndef INCLUDE_DDRAWMGR_PIXELSHIFT_H
#define INCLUDE_DDRAWMGR_PIXELSHIFT_H
#include <Ints.h>

extern i32 g_rUp;   // 0x683ea0  red   pack left-shift
extern i32 g_gUp;   // 0x683ea4  green pack left-shift
extern i32 g_bUp;   // 0x683ea8  blue  pack left-shift
extern i32 g_rDown; // 0x683eac  red   reduce right-shift
extern i32 g_gDown; // 0x683eb0  green reduce right-shift
extern i32 g_bDown; // 0x683eb4  blue  reduce right-shift

#endif // INCLUDE_DDRAWMGR_PIXELSHIFT_H
