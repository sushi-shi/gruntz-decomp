#ifndef GRUNTZ_FONT_FONTBLENDINLINE_H
#define GRUNTZ_FONT_FONTBLENDINLINE_H

#include <Mfc.h>
#include <MfcWin.h>

#include <DDrawMgr/PixelShift.h>

static inline LONG RunRightEdge(const CRect& rc, i32 x) {
    return x - rc.left + rc.right;
}

static inline u8 BlendChannel(u8 dest, i32 source, u8 cover) {
    return static_cast<u8>((dest * (255 - cover)) / 256 + (source * cover) / 256);
}

static inline u16 BlendPixel16(u16 pixel, u8 cover, i32 red, i32 green, i32 blue) {
    u8 dr = static_cast<u8>((static_cast<u8>((pixel >> g_rUp)) << g_rDown));
    u8 dg = static_cast<u8>((static_cast<u8>((pixel >> g_gUp)) << g_gDown));
    u8 db = static_cast<u8>((static_cast<u8>(pixel) << g_bDown));
    return PackPixel16(
        BlendChannel(dr, red, cover),
        BlendChannel(dg, green, cover),
        BlendChannel(db, blue, cover)
    );
}

#endif // GRUNTZ_FONT_FONTBLENDINLINE_H
