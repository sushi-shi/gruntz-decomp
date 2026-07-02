// CAniAdvanceCursor.cpp - the WWD animation-advance cursor sub-object (trace
// class CAniAdvanceCursor; Ghidra ClassUnknown_73). It is the WWD sub-object
// class whose vtable is 0x5f0128 (the same table WwdFile.cpp's ReadPlaneObjects
// stamps into the +0x1A0 embedded sub-object). REAL POLYMORPHIC (ALL-VTABLES
// phase): this TU owns ??_7CAniAdvanceCursor@@6B@ @0x5f0128 (9-slot WWD-object
// vtable; dtor at slot 1), so cl auto-stamps the vptr in the ctor - no manual
// sub-object-vtable stamp in this ctor. No destructible locals -> no /GX frame.
#include <rva.h>

#include <Ints.h>

// 9-slot WWD-object vtable (WwdSubVtbl): slot 0 a shared base virtual, slot 1 the
// scalar-deleting dtor, slots 2-6 the shared CObject-like WWD base virtuals, slot
// 7 Reset, slot 8 a shared virtual. Declared-only slots are referenced externally
// in the emitted vtable (reloc-masked). Owned here; WwdFile's sub-object stamp
// reloc-masks against the same datum.
class CAniAdvanceCursor {
public:
    CAniAdvanceCursor(i32 a1, i32 a2, i32 a3);
    virtual void V0();            // slot 0  (sub_1bef01)
    virtual ~CAniAdvanceCursor(); // slot 1  (scalar-deleting dtor 0x15b6b0)
    virtual void V2();            // slot 2  (sub_0028ec)
    virtual void V3();            // slot 3  (sub_00106e)
    virtual void V4();            // slot 4  (sub_004034)
    virtual void V5();            // slot 5  (sub_15b6a0)
    virtual void V6();            // slot 6  (sub_001c08)
    virtual i32 Reset();          // slot 7  (0x15c2c0)
    virtual void V8();            // slot 8  (sub_154a00)

    // vptr implicit @ +0x00
    i32 m_4;  // +0x04
    i32 m_8;  // +0x08
    i32 m_c;  // +0x0c
    i32 m_10; // +0x10
    i32 m_14; // +0x14
    i32 m_18; // +0x18
};
SIZE_UNKNOWN(CAniAdvanceCursor);
VTBL(CAniAdvanceCursor, 0x005f0128);

// cl auto-stamps the ??_7CAniAdvanceCursor vptr @+0, then seeds the three scalar
// fields from the args and zeroes m_10/m_14/m_18.
// @early-stop
// vptr-position wall: real polymorphism sinks the auto-vptr stamp to FIRST (was a
// hand-rolled mid-body store tuned to pin a3 in edx). Converted per the
// ALL-VTABLES mandate; logic byte-faithful.
RVA(0x0015b730, 0x2b)
CAniAdvanceCursor::CAniAdvanceCursor(i32 a1, i32 a2, i32 a3) {
    m_4 = a2;
    m_8 = a3;
    m_c = a1;
    m_10 = 0;
    m_14 = 0;
    m_18 = 0;
}
