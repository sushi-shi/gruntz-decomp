#ifndef GRUNTZ_RANGESET_H
#define GRUNTZ_RANGESET_H

#include <rva.h>

#include <Ints.h>

struct CRange {
    u32 lo;
    u32 hi;
};

class CRangeSet {
public:
    bool Contains(u32 value);
    void AddRange(u32 lo, u32 hi);
    void AddFromString(char* str);

    u32 m_count;
    CRange m_pairs[16];
};

#endif // GRUNTZ_RANGESET_H
