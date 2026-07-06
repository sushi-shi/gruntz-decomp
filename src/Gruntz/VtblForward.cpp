// VtblForward.cpp - an argument-forwarding dispatcher.
//   0x110460  CVtEmitRecv::Accept - guard, copy a 0x60-byte record into +0x2c, then
//             virtual-dispatch (slot 0) with all the args except the record source.
//
// Item-5 owner audit (sema xref): Accept has NO rel32 caller (its only referent is
// the ILT thunk 0x22e8, itself uncalled) and NO recorded data/vtable reference to
// 0x110460 - so it is reached purely through an indirect/computed dispatch. The
// owning class is therefore not recoverable from the xref graph; CVtEmitRecv stays a
// placeholder (a message/record acceptor: vptr@+0, busy gate@+0x20, 0x60-byte record
// sink@+0x2c, virtual Dispatch slot 0). Kept in its own TU with this evidence note.
#include <Ints.h>
#include <rva.h>
#include <string.h>

// ---------------------------------------------------------------------------
// 0x110460 - the receiving object: vptr @+0, a "busy" gate @+0x20, a 0x60-byte
// record sink @+0x2c.
// ---------------------------------------------------------------------------
struct CVtEmitRecv {
    virtual i32 Dispatch(i32, i32, i32, i32, i32, i32, i32, i32); // vtbl slot 0
    char m_pad4[0x20 - 4];
    i32 m_20; // +0x20  busy gate
    char m_pad24[0x2c - 0x24];
    char m_2c[0x60]; // +0x2c  record sink

    i32 Accept(i32 p1, i32 p2, i32 p3, i32 p4, i32 p5, i32* p6, i32 p7, i32 p8, i32 p9);
};

RVA(0x00110460, 0x64)
i32 CVtEmitRecv::Accept(i32 p1, i32 p2, i32 p3, i32 p4, i32 p5, i32* p6, i32 p7, i32 p8, i32 p9) {
    if (m_20 != 0) {
        return 0;
    }
    if (p2 == 4 && *p6 == 0) {
        return 0;
    }
    memcpy(m_2c, p6, 0x60);
    return Dispatch(p1, p2, p3, p4, p5, p7, p8, p9);
}
SIZE_UNKNOWN(CVtEmitRecv);

// --- vtable catalog ---
VTBL(CVtEmitRecv, 0x001e8cb4);
