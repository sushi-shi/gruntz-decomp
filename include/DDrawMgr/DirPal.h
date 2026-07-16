// DirPal.h - the CDDPalette stack-palette record (owner: DirPal.cpp).
// LAYOUT-ONLY: PALETTEENTRY comes from the real SDK, so an includer must pull
// <Win32.h>/<Mfc.h> (windows.h) BEFORE this header (the DDScreen.h convention).
#ifndef GRUNTZ_DDRAWMGR_DIRPAL_H
#define GRUNTZ_DDRAWMGR_DIRPAL_H

#include <Ints.h>

// A stack LOGPALETTE with a full 256-entry table (the 0x410-byte frame of
// CaptureSystemPalette / BlackoutSystemPalette).
struct LogPal256 {
    u16 palVersion;                // +0x00
    u16 palNumEntries;             // +0x02
    PALETTEENTRY palPalEntry[256]; // +0x04
};

#endif // GRUNTZ_DDRAWMGR_DIRPAL_H
