#include <rva.h>
// UnknownSalazar.cpp - initializeUnknownLookupTable of the tomalla-named class
// UnknownSalazar.  It fills a 100-element static int table with
// getLookupTableValue results for indices 0..99.  getLookupTableValue itself
// and computeScaleFactor remain unmatched stubs; only the
// table-filling leaf is reconstructed here (the others are reloc-masked CRT-math
// leaves still on the backlog).
//
// Field names are placeholders; only OFFSETS + emitted code bytes are load-
// bearing (campaign doctrine).
// ---------------------------------------------------------------------------

// The lookup table buffer (100 ints). Written by
// initializeUnknownLookupTable with getLookupTableValue results.
DATA(0x00653ab8)
i32 g_salazarLookupTable[100];

class UnknownSalazar {
public:
    static i32 getLookupTableValue(i32 value); // still a stub
    static void initializeUnknownLookupTable();

    // Engine-label backlog stubs.
    UnknownSalazar();
    ~UnknownSalazar();
};

// ---------------------------------------------------------------------------
// UnknownSalazar::initializeUnknownLookupTable
// Fills the 100-element static lookup table with
// getLookupTableValue(i) for i = 0..99. Plain /O2 /MT leaf: no SEH frame.
// ---------------------------------------------------------------------------
RVA(0x001351a0, 0x23)
void UnknownSalazar::initializeUnknownLookupTable() {
    for (i32 i = 0; i < 100; i++) {
        g_salazarLookupTable[i] = getLookupTableValue(i);
    }
}

// Engine-label backlog stubs (moved from src/Stub/UnknownSalazar.cpp).
// getLookupTableValue is already declared above as `static int (int)`, so the
// moved body matches that signature (the stub's local `void ()` form was a
// per-TU placeholder for the same RVA).

// @confidence: high
// @source: tomalla
// @stub
RVA(0x001350b0, 0x5d)
i32 UnknownSalazar::getLookupTableValue(i32) {
    return 0;
}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x00136440, 0x74)
UnknownSalazar::UnknownSalazar() {}

// @confidence: high
// @source: tomalla
// @stub
RVA(0x001364c0, 0x1e)
UnknownSalazar::~UnknownSalazar() {}
