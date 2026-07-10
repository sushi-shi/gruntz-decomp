// AnimWorkerCtor.cpp - the out-of-line 3-arg anim-worker constructor (0x15b300,
// __thiscall, ret 0xc; the body the 0x150eb0 factory / CreateWorker24 inlines).
// Split out of AnimWorkerHandlers.cpp (matcher-1): a distinct retail /Gy object
// (a real polymorphic class ctor with its own vtable), not a message handler.
//
// Real-polymorphic: cl auto-stamps the vptr (??_7WorkerFull@@6B@; retail vtable
// 0x5efb80, 10 declared-only slots reloc-mask) and seeds the three context fields
// (b -> +0x04, c -> +0x08, a -> +0x0c), zeroing the rest.
//
// Field names are placeholders (m_<hexoffset>); only OFFSETS + emitted code bytes are
// load-bearing (campaign doctrine).
#include <rva.h>
#include <Gruntz/AnimWorkerFull.h>

// The arg-store order (b,c,a into m_04/m_08/m_0c) is load-bearing.
// @early-stop
// vptr-last wall: retail stamps the vptr AFTER m_04/m_08/m_0c, but a real-virtual
// class forces cl's implicit vptr-first store at ctor entry. Field-store order
// preserved; only the vptr position diverges (mandate: convert anyway).
RVA(0x0015b300, 0x40)
WorkerFull::WorkerFull(i32 a, i32 b, i32 c) {
    m_04 = b;
    m_08 = c;
    m_0c = a;
    m_10 = 0;
    m_14 = 0;
    m_18 = 0;
    m_170 = 0;
    m_1c = 0;
    m_174 = 0;
    m_178 = 0;
}
SIZE_UNKNOWN(WorkerFull);
RELOC_VTBL(WorkerFull, 0x001efb80); // vtable reloc-masks a bound datum (dtor-stamp verified)
