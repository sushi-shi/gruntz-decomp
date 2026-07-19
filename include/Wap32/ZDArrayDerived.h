// ZDArrayDerived.h - CZDArrayDerived, a WAP32 2D-array base. Promoted so the reduced
// per-TU construct-views fold onto it.
#ifndef GRUNTZ_WAP32_ZDARRAYDERIVED_H
#define GRUNTZ_WAP32_ZDARRAYDERIVED_H

#include <Ints.h>
#include <rva.h>

SIZE_UNKNOWN(CZDArrayDerived);
struct CZDArrayDerived {
    char _vft0[4];         // +0x00 foreign/base object vptr (reduced view; not owned/dispatched)
    char _04[0x1c - 0x04]; //
    u8* m_1c;              // +0x1c  raw element cursor (== m_buf, set by the base ctor)
    CZDArrayDerived* BaseConstruct(i32 stride, i32 lo, i32 hi, void* scratch); // 0x16dda0
    CZDArrayDerived* Construct(i32 lo, i32 hi);                                // 0x8710
};

#endif // GRUNTZ_WAP32_ZDARRAYDERIVED_H
