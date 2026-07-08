// GlyphTable.cpp - a 128-entry int table (+0x1b0) indexed by a wrapped code
// (anonymous class).  Getter 0xc0430 and setter 0xc03f0 share the index idiom:
// the code is masked to a byte, (the setter biases it by +m_10), abs'd, masked to
// 7 bits, abs'd again - MSVC emits a single cdq feeding both branchless-abs pairs
// around the `& 0x7f`.
#include <rva.h>
#include <stdlib.h>

struct GlyphTable {
    char pad0[0x10];
    int m_10; // +0x10 setter bias
    char pad14[0x1b0 - 0x14];
    int m_1b0[128]; // +0x1b0
    RVA(0x000c0430, 0x1f)
    int Get(int c) {
        return m_1b0[(c & 0xff) % 128];
    }
    RVA(0x000c03f0, 0x29)
    void Set(int v, int c) {
        m_1b0[(m_10 + (c & 0xff)) % 128] = v;
    }
};

// GlyphTable::Get (0x000c0430) is now an inline member in the header.


// GlyphTable::Set (0x000c03f0) is now an inline member in the header.


SIZE_UNKNOWN(GlyphTable);
