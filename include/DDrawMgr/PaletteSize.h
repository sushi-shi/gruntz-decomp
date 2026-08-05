#ifndef DDRAWMGR_PALETTESIZE_H
#define DDRAWMGR_PALETTESIZE_H

#include <Enums.h>

// Entries in an 8-bit palette.
//
// The Win32 calls state it: `SetEntries(0, 0, PALETTE_ENTRY_COUNT, m_palEntries)`
// and `GetSystemPaletteEntries(hdc, 0, PALETTE_ENTRY_COUNT, m_palEntries)`, over
// arrays declared `PALETTEENTRY entries[PALETTE_ENTRY_COUNT]`. Every walk over a
// palette - building a shade table, writing one to a BMP, or the
// nearest-colour search that scans `k = 1 .. PALETTE_ENTRY_COUNT` - counts to
// the same number.
//
// Written as both 0x100 and 256 before this header.
//
// NOT this constant, though they share the value: Blowfish's 256-entry S-boxes
// and the 256-byte save-game obfuscation buffer. Both were left as they were.
GZ_ENUM_CONST_BEGIN(PaletteSize)
    PALETTE_ENTRY_COUNT = 256,
    PALETTE_RGB_BYTE_COUNT = PALETTE_ENTRY_COUNT * 3,
    LOGICAL_PALETTE_VERSION = 0x300
GZ_ENUM_CONST_END(PaletteSize)

#endif // DDRAWMGR_PALETTESIZE_H
