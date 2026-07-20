#ifndef GRUNTZ_DDRAWMGR_DIRPAL_H
#define GRUNTZ_DDRAWMGR_DIRPAL_H

#include <Ints.h>

struct LogPal256 {
    u16 palVersion;                // +0x00
    u16 palNumEntries;             // +0x02
    PALETTEENTRY palPalEntry[256]; // +0x04
};

#endif // GRUNTZ_DDRAWMGR_DIRPAL_H
