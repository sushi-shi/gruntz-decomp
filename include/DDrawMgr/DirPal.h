#ifndef GRUNTZ_DDRAWMGR_DIRPAL_H
#define GRUNTZ_DDRAWMGR_DIRPAL_H

#include <rva.h>

#include <Mfc.h>

#include <Ints.h>

union LogPal256 {
    LOGPALETTE m_lp;
    struct {
        u16 palVersion;
        u16 palNumEntries;
        PALETTEENTRY palPalEntry[256];
    };
};

union Palette256 {
    u8 m_bytes[0x400];
    PALETTEENTRY m_entries[256];
};

extern HINSTANCE g_resModule;

#endif // GRUNTZ_DDRAWMGR_DIRPAL_H
