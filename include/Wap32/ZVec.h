#ifndef GRUNTZ_WAP32_ZVEC_H
#define GRUNTZ_WAP32_ZVEC_H

#include <Ints.h>
#include <Wap32/zBitVec.h> // the canonical zErrHandling - _zvec's real {vptr, sink} head
#include <rva.h>

struct CVariantSlot; // fwd (pointer member m_err; full def at the overflow call sites)

class _zvec : public zErrHandling {
public:
    // Pass-through to the error-sink base ctor (0x16d9c0): retail's allocating ctor
    // opens with exactly that one base call, so this stays inline (no _zvec body of
    // its own exists in the image).
    _zvec(void* errSink) : zErrHandling(errSink) {}

    void* GrowTo(i32 idx, i32 at); // 0x16da80
    char* IndexToPtr(i32 idx);     // 0x312a0  (the plain base accessor)

    // vptr @+0x00 and the error sink @+0x04 come from zErrHandling (which names the
    // sink m_errSink; this class's code still reads it under that name).
    i32 m_lo;      // +0x08
    i32 m_hi;      // +0x0c
    char* m_base;  // +0x10  element band (byte-addressed: base + (idx-lo)*stride)
    char* m_spare; // +0x14  scratch element / error-path result
    i32 m_stride;  // +0x18
    char* m_alloc; // +0x1c  raw realloc base / per-element-fixup start
    i32 m_grown;   // +0x20
};
SIZE_UNKNOWN(); // dynamic-vector base (partial: no true base chain)

class _zdvec : public _zvec {
public:
    // The allocating ctor (0x16de30, body in src/Bute/TypeKeyColl.cpp): records
    // [lo,hi] + the element stride, allocates the (hi-lo+1)*stride band (+ a scratch
    // element when none is supplied) and reports a fatal bounds/OOM through the
    // inherited error sink. /GX (the half-built zErrHandling base unwinds).
    _zdvec(i32 stride, i32 lo, i32 hi, void* scratch);
    i32 Destroy();               // 0x8750  (re-stamp live vtable + run ~_zdvec)
    char* IndexToPtr(i32 i);     // 0x310f0 (base accessor + per-slot member-ptr init)
    virtual ~_zdvec() OVERRIDE; // 0x16df40 (cl auto-stamps ??_7zDArray at entry)
};
SIZE_UNKNOWN(); // derived; adds override, no storage


// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern void* const zDArrayLiveTable; // 0x5e70fc

#endif // GRUNTZ_WAP32_ZVEC_H
