#ifndef DDRAWMGR_COLORDEPTH_H
#define DDRAWMGR_COLORDEPTH_H

#include <Enums.h>

// Bits per pixel, as a DOMAIN rather than a count.
//
// It earns a domain because the code branches on it rather than computing with
// it. CGruntzMgr::SetColorDepth rejects anything that is not one of exactly
// three - `depth != 8 && depth != 0x10 && depth != 0x18` - and then does
// something different for each: 8 clears the colour key outright, 16 packs one
// through the g_r/g_g/g_b shift pair, 24 takes the third path.
//
// The 32-bit value appears only where a display mode is enumerated, never in a
// blit path, which is why it is listed but is not one of the three the game will
// actually run at.
//
// Written both ways before this header - `bitcount == 8` beside
// `m_bitDepth == 0x10` - so the decimal and hex spellings of the same three
// values are unified here.
GZ_ENUM_BEGIN(ColorDepth)
    BPP_PALETTED_8 = 8,
    BPP_RGB_16 = 16,
    BPP_RGB_24 = 24,
    BPP_RGB_32 = 32
GZ_ENUM_END(ColorDepth)

#endif // DDRAWMGR_COLORDEPTH_H
