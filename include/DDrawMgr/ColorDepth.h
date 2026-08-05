#ifndef DDRAWMGR_COLORDEPTH_H
#define DDRAWMGR_COLORDEPTH_H

#include <Enums.h>

// Bits per pixel, as a DOMAIN rather than a count.
//
// It earns a domain because the code branches on it rather than computing with
// it. CGruntzMgr::SetColorDepth rejects anything that is not one of exactly
// three supported depths - paletted 8-bit, RGB 16-bit, and RGB 24-bit - and does
// something different for each: 8 clears the colour key outright, 16 packs one
// through the g_r/g_g/g_b shift pair, 24 takes the third path.
//
// The 32-bit value appears only where a display mode is enumerated, never in a
// blit path, which is why it is listed but is not one of the three the game will
// actually run at.
//
// The original sources used decimal and hexadecimal spellings interchangeably;
// the named members unify those spellings here.
GZ_ENUM_BEGIN(ColorDepth)
    BPP_UNSET = 0,
    BPP_MONO_1 = 1,
    BPP_PALETTED_2 = 2,
    BPP_PALETTED_4 = 4,
    BPP_PALETTED_8 = 8,
    BPP_RGB_16 = 16,
    BPP_RGB_24 = 24,
    BPP_RGB_32 = 32
GZ_ENUM_END(ColorDepth)

#endif // DDRAWMGR_COLORDEPTH_H
