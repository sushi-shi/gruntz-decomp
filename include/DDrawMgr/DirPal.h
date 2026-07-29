#ifndef GRUNTZ_DDRAWMGR_DIRPAL_H
#define GRUNTZ_DDRAWMGR_DIRPAL_H

#include <Ints.h>
#include <rva.h>

// A 256-entry logical palette. wingdi declares LOGPALETTE's colour table `[1]`, so
// the full-size form has to be named separately - but the SAME bytes are what
// CreatePalette takes, so the SDK view is a union arm and the call needs no pun.
union LogPal256 {
    LOGPALETTE m_lp; // the GDI argument view (CreatePalette / GetSystemPaletteEntries)
    struct {
        u16 palVersion;                // +0x00
        u16 palNumEntries;             // +0x02
        PALETTEENTRY palPalEntry[256]; // +0x04
    };
};
SIZE_UNKNOWN();

// The 0x400-byte 256-colour table as it sits on disk / in a staging buffer. The BMP
// swizzle walks it as FLAT BYTES (retail's loop is `mov cl,[esp+eax+..]` with a +4
// step), while the SDK takes the same bytes as PALETTEENTRY[256] - both readings are
// real, so the entry view is a union arm rather than a pun at the hand-off.
union Palette256 {
    u8 m_bytes[0x400];           // the flat byte staging form
    PALETTEENTRY m_entries[256]; // the SDK view
};
SIZE(0x400);

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern HINSTANCE g_resModule;

#endif // GRUNTZ_DDRAWMGR_DIRPAL_H
