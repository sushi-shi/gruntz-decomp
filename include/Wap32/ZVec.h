#ifndef GRUNTZ_WAP32_ZVEC_H
#define GRUNTZ_WAP32_ZVEC_H

#include <Ints.h>
#include <Wap32/zBitVec.h> // the canonical zErrHandling - _zvec's real {vptr, sink} head
#include <rva.h>

struct CVariantSlot; // fwd (pointer member m_err; full def at the overflow call sites)

class _zvec : public zErrHandling {
public:
    // 0x16de30: allocate the [lo, hi] element band and scratch slot.
    _zvec(i32 stride, i32 lo, i32 hi, void* scratch);

    void* GrowTo(i32 idx, i32 at); // 0x16da80
    char* IndexToPtr(i32 idx);     // 0x312a0  (the plain base accessor)
    virtual ~_zvec() OVERRIDE;     // 0x16df40

    // vptr @+0x00 and the error sink @+0x04 come from zErrHandling (which names the
    // sink m_errSink; this class's code still reads it under that name).
    i32 m_lo;      // +0x08
    i32 m_hi;      // +0x0c
    char* m_base;  // +0x10  element band (byte-addressed: base + (idx-lo)*stride)
    char* m_spare; // +0x14  scratch element / error-path result
    i32 m_stride;  // +0x18
    char* m_alloc; // +0x1c  raw realloc base / per-element construction cursor
    i32 m_grown;   // +0x20  number of elements constructed by the last grow
};
SIZE(0x24);

class _zdvec : public _zvec {
public:
    // 0x16dda0: construct the allocating base, then seed the element cursor/count.
    _zdvec(i32 stride, i32 lo, i32 hi, void* scratch);
    char* IndexToPtr(i32 i);    // 0x310f0 (base accessor + per-slot construction)
    virtual ~_zdvec() OVERRIDE; // 0x16de00
};
SIZE(0x24);

template<class T> class zDArray : public _zdvec {
public:
    zDArray(i32 lo, i32 hi);
    virtual ~zDArray() OVERRIDE;

    char* Resolve(i32 id);
    char* ResolveEntry(i32 id);
};
SIZE_UNKNOWN();

#endif // GRUNTZ_WAP32_ZVEC_H
