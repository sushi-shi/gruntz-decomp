// ZDArrayDerived.cpp - a zDArray-derived collection ctor (orphan COMDAT @0x8710).
//
// Construct(lo, hi) forwards to the shared CTypeKeyColl 2D-array ctor (0x16dda0)
// with stride 4 / scratch 1, then stamps the derived vtable (g_zDArrayVtbl,
// 0x5e70fc) and returns this. Placeholder class name; only OFFSETS + code bytes
// are load-bearing.
#include <Ints.h>
#include <rva.h>

DATA(0x001e70fc)
extern void* g_zDArrayVtbl; // 0x5e70fc

struct CZDArrayDerived {
    void* m_vtbl;          // +0x00
    char _04[0x1c - 0x04]; //
    void* m_1c;            // +0x1c  cursor (== m_buf, set by the base ctor)
    // The shared 2D-array base ctor (0x16dda0): (stride, lo, hi, scratch).
    CZDArrayDerived* BaseConstruct(i32 stride, i32 lo, i32 hi, void* scratch); // 0x16dda0
    CZDArrayDerived* Construct(i32 lo, i32 hi);                                // 0x8710
};

RVA(0x00008710, 0x2b)
CZDArrayDerived* CZDArrayDerived::Construct(i32 lo, i32 hi) {
    BaseConstruct(4, lo, hi, (void*)1);
    *(volatile i32*)&hi = (i32)m_1c; // write-back to the hi param slot (retail keeps it)
    m_vtbl = &g_zDArrayVtbl;
    return this;
}
