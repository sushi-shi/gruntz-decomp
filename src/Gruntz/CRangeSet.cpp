// CRangeSet.cpp - a small fixed-capacity inclusive-range set (capacity 15) that
// lives in the 0x1845b0..0x185700 menu cluster. The object is { u32 m_count; then
// up to 16 { u32 lo; u32 hi } pairs starting at +0x04 }. AddRange appends a pair
// while count+1 < 16; Contains scans for the first pair whose [lo,hi] (unsigned)
// brackets the probe value. Both are __thiscall leaves with no externals.
//
// RTTI name does not survive; the class name is a placeholder (campaign doctrine -
// matching-neutral). Only offsets + code bytes are load-bearing.
#include <Ints.h>
#include <rva.h>

struct CRange {
    u32 lo; // +0x00
    u32 hi; // +0x04
};

class CRangeSet {
public:
    bool Contains(u32 value);      // 0x184ba0
    void AddRange(u32 lo, u32 hi); // 0x184be0

    u32 m_count;        // +0x00
    CRange m_pairs[16]; // +0x04
};

// ===========================================================================
// 0x184ba0 - Contains(value): true if any stored [lo,hi] range (unsigned)
// includes value. Walks m_pairs[0..m_count).
// ===========================================================================
RVA(0x00184ba0, 0x33)
bool CRangeSet::Contains(u32 value) {
    for (u32 i = 0; i < m_count; i++) {
        if (value >= m_pairs[i].lo && value <= m_pairs[i].hi) {
            return true;
        }
    }
    return false;
}

// ===========================================================================
// 0x184be0 - AddRange(lo, hi): append a pair if there is room (count+1 < 16).
// m_count is re-read after each store (the member array may alias it).
// ===========================================================================
RVA(0x00184be0, 0x24)
void CRangeSet::AddRange(u32 lo, u32 hi) {
    if (m_count + 1 < 16) {
        m_pairs[m_count].lo = lo;
        m_pairs[m_count].hi = hi;
        m_count = m_count + 1;
    }
}
