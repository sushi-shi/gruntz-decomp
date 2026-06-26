// CAniAdvanceCursor.cpp - the WWD animation-advance cursor sub-object (trace
// class CAniAdvanceCursor; Ghidra ClassUnknown_73). Its ctor stamps the shared
// WWD sub-object vtable (g_wwdSubVtbl, 0x5f0128 - the same vtable WwdFile.cpp's
// ReadPlaneObjects stamps into the +0x1A0 embedded sub-object) and seeds the
// three scalar fields from its args. No destructible locals -> no /GX frame.
#include <rva.h>

#include <Ints.h>

// The shared WWD sub-object vtable; its DATA() symbol is pinned in WwdFile.cpp,
// so the DIR32 vptr store here reloc-masks against the named target symbol.
extern void* g_wwdSubVtbl[]; // 0x5f0128

class CAniAdvanceCursor {
public:
    CAniAdvanceCursor(i32 a1, i32 a2, i32 a3);

    void* m_0; // +0x00 vptr (g_wwdSubVtbl)
    i32 m_4;   // +0x04
    i32 m_8;   // +0x08
    i32 m_c;   // +0x0c
    i32 m_10;  // +0x10
    i32 m_14;  // +0x14
    i32 m_18;  // +0x18
};

// Arg-stores ordered so cl pins a3 (the edx-held value) by writing it second -
// see docs/patterns/arg-store-order-steers-schedule.md.
RVA(0x0015b730, 0x2b)
CAniAdvanceCursor::CAniAdvanceCursor(i32 a1, i32 a2, i32 a3) {
    m_4 = a2;
    m_8 = a3;
    m_c = a1;
    *(void**)this = &g_wwdSubVtbl;
    m_10 = 0;
    m_14 = 0;
    m_18 = 0;
}
