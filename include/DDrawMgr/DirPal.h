#ifndef GRUNTZ_DDRAWMGR_DIRPAL_H
#define GRUNTZ_DDRAWMGR_DIRPAL_H

#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/PaletteSize.h>
#include <Ints.h>

union LogPal256 {
    LOGPALETTE m_lp;
    struct {
        u16 palVersion;
        u16 palNumEntries;
        PALETTEENTRY palPalEntry[PALETTE_ENTRY_COUNT];
    };
};

union Palette256 {
    u8 m_bytes[PALETTE_ENTRY_COUNT * sizeof(PALETTEENTRY)];
    PALETTEENTRY m_entries[PALETTE_ENTRY_COUNT];
};

extern HINSTANCE g_resModule;

#define COPY_RGB_PALETTE(entries, rgb, index, count)                                               \
    for (i32 index = 0; index < count; index++) {                                                  \
        entries[index].peRed = *rgb++;                                                             \
        entries[index].peGreen = *rgb++;                                                           \
        entries[index].peBlue = *rgb++;                                                            \
        entries[index].peFlags = 0;                                                                \
    }

#define COPY_RGB_PALETTE_DO(entries, rgb, index, count)                                            \
    i32 index = 0;                                                                                 \
    do {                                                                                           \
        entries[index].peRed = *rgb++;                                                             \
        entries[index].peGreen = *rgb++;                                                           \
        entries[index].peBlue = *rgb++;                                                            \
        entries[index].peFlags = 0;                                                                \
        index++;                                                                                   \
    } while (index < count);

#define COPY_BGRX_PALETTE(entries, colors, index, count)                                           \
    for (i32 index = 0; index < count; index++) {                                                  \
        entries[index].peRed = colors[index].rgbRed;                                               \
        entries[index].peGreen = colors[index].rgbGreen;                                           \
        entries[index].peBlue = colors[index].rgbBlue;                                             \
        entries[index].peFlags = 0;                                                                \
    }

#endif // GRUNTZ_DDRAWMGR_DIRPAL_H
