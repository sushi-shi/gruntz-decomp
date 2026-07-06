// ZDArrayDerived.h - CZDArrayDerived, a WAP32 2D-array base. Promoted so the reduced
// per-TU construct-views fold onto it.
#ifndef GRUNTZ_WAP32_ZDARRAYDERIVED_H
#define GRUNTZ_WAP32_ZDARRAYDERIVED_H

#include <Ints.h>
#include <rva.h>

SIZE_UNKNOWN(CZDArrayDerived);
struct CZDArrayDerived {
    void* m_vtbl;          // +0x00
    char _04[0x1c - 0x04]; //
    void* m_1c;            // +0x1c  cursor (== m_buf, set by the base ctor)
    CZDArrayDerived* BaseConstruct(i32 stride, i32 lo, i32 hi, void* scratch); // 0x16dda0
    CZDArrayDerived* Construct(i32 lo, i32 hi);                                // 0x8710
};

#endif // GRUNTZ_WAP32_ZDARRAYDERIVED_H
