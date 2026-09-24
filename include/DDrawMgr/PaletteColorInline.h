#ifndef GRUNTZ_DDRAWMGR_PALETTECOLORINLINE_H
#define GRUNTZ_DDRAWMGR_PALETTECOLORINLINE_H

#include <Mfc.h>

#include <DDrawMgr/PaletteSize.h>
#include <Lith/BDefs.h>

static inline i32 FindNearestColor(PALETTEENTRY* pal, u8 red, u8 green, u8 blue) {
    i32 best = 0;
    i32 bestd = SQR(red - pal->peRed) + SQR(green - pal->peGreen) + SQR(blue - pal->peBlue);
    for (i32 k = 1; k < PALETTE_ENTRY_COUNT; k++) {
        i32 d = SQR(red - pal[k].peRed) + SQR(green - pal[k].peGreen) + SQR(blue - pal[k].peBlue);
        if (d < bestd) {
            bestd = d;
            best = k;
            if (bestd == 0) {
                break;
            }
        }
    }
    return best;
}

static inline u8
WeightedPaletteChannel(u8 row, u8 column, i32 rowWeight, i32 columnWeight, i32 divisor) {
    return static_cast<u8>((column * columnWeight / 100 + row * rowWeight / 100) / divisor);
}

#endif // GRUNTZ_DDRAWMGR_PALETTECOLORINLINE_H
