#include "../rva.h"
// UnknownSalazar.cpp - initializeUnknownLookupTable of the tomalla-named class
// UnknownSalazar.  It fills a 100-element static int table with
// getLookupTableValue results for indices 0..99.  getLookupTableValue itself
// (0x1350b0) and computeScaleFactor (0x135110) remain unmatched stubs; only the
// table-filling leaf is reconstructed here (the others are reloc-masked CRT-math
// leaves still on the backlog).
//
// Field names are placeholders; only OFFSETS + emitted code bytes are load-
// bearing (campaign doctrine).
// ---------------------------------------------------------------------------

// The lookup table buffer (binary @0x653ab8, 100 ints). Written by
// initializeUnknownLookupTable with getLookupTableValue results.
DATA(0x653ab8)
int g_salazarLookupTable[100];

class UnknownSalazar {
public:
    static int getLookupTableValue(int value);   // @0x1350b0 (still a stub)
    static void initializeUnknownLookupTable();
};

// ---------------------------------------------------------------------------
// UnknownSalazar::initializeUnknownLookupTable  @0x1351a0  (static, void)
// Fills the 100-element static lookup table at 0x653ab8 with
// getLookupTableValue(i) for i = 0..99. Plain /O2 /MT leaf: no SEH frame.
// ---------------------------------------------------------------------------
RVA(0x1351a0, 0x23)
void UnknownSalazar::initializeUnknownLookupTable()
{
    for (int i = 0; i < 100; i++)
        g_salazarLookupTable[i] = getLookupTableValue(i);
}
