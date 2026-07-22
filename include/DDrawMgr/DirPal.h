#ifndef GRUNTZ_DDRAWMGR_DIRPAL_H
#define GRUNTZ_DDRAWMGR_DIRPAL_H

#include <Ints.h>
#include <rva.h>

struct LogPal256 {
    u16 palVersion;                // +0x00
    u16 palNumEntries;             // +0x02
    PALETTEENTRY palPalEntry[256]; // +0x04
};
SIZE_UNKNOWN();

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern HINSTANCE g_resModule;

#endif // GRUNTZ_DDRAWMGR_DIRPAL_H
