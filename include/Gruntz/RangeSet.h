#ifndef GRUNTZ_RANGESET_H
#define GRUNTZ_RANGESET_H

#include <Ints.h>
#include <rva.h>

struct CRange {
    u32 lo;
    u32 hi;
};
SIZE_UNKNOWN();

class CRangeSet {
public:
    bool Contains(u32 value);
    void AddRange(u32 lo, u32 hi);
    void AddFromString(char* str);

    u32 m_count;
    CRange m_pairs[16];
};
SIZE_UNKNOWN();

#endif // GRUNTZ_RANGESET_H
