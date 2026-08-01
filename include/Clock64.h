#ifndef GRUNTZ_CLOCK64_H
#define GRUNTZ_CLOCK64_H

#include <Ints.h>
#include <rva.h>

#pragma pack(push, 4)
union Clock64 {
    i64 m_v;
    struct {
        i32 m_lo;
        i32 m_hi;
    };
};
#pragma pack(pop)
SIZE(0x8);

#endif // GRUNTZ_CLOCK64_H
