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
