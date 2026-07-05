// RangeSet.h - THE canonical shape of CRangeSet, a small fixed-capacity inclusive-
// range set (capacity 15) in the 0x1845b0..0x185700 menu cluster. Layout:
// { u32 m_count; then up to 16 { u32 lo; u32 hi } pairs at +0x04 }. AddRange appends
// while count+1 < 16; Contains scans for the first pair whose [lo,hi] (unsigned)
// brackets the probe. Unifies RangeSet.cpp's full class with DebugPrintf.cpp's
// Contains-only reinterpret view (the debug-channel set overlaid on the sink object).
//
// RTTI name does not survive; the class name is a placeholder (matching-neutral).
// Only offsets + code bytes are load-bearing.
#ifndef GRUNTZ_RANGESET_H
#define GRUNTZ_RANGESET_H

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
    void AddFromString(char* str); // 0x184c10

    u32 m_count;        // +0x00
    CRange m_pairs[16]; // +0x04
};
SIZE_UNKNOWN(CRange);
SIZE_UNKNOWN(CRangeSet);

#endif // GRUNTZ_RANGESET_H
