#ifndef GRUNTZ_CLOCK64_H
#define GRUNTZ_CLOCK64_H

#include <rva.h>

#include <Ints.h>

#pragma pack(push, 4)
union Clock64 {
    i64 m_v;
    struct {
        i32 m_lo;
        i32 m_hi;
    };
};
#pragma pack(pop)

#endif // GRUNTZ_CLOCK64_H
