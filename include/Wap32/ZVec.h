// ZVec.h - the WAP32 engine's `_zvec`/`_zdvec` dynamic-vector base and the
// `zDArray<T>` template that derives from it (C:\Proj\incs). RTTI in GRUNTZ.EXE
// names the most-derived instance here as
//   .?AV?$zDArray@P8CUserLogic@@AEHXZ@@   ==  zDArray<int (CUserLogic::*)(void)>
// with the class hierarchy  zDArray<T> : _zdvec : _zvec : zErrHandling.
//
// The vector stores a contiguous element band addressed by an integer index that
// may run negative: lower/upper bounds are tracked and the band is realloc'd
// (and zero-filled) on demand. Layout (all dword fields, from the ctor/accessor
// disasm):
//   +0x00  vptr
//   +0x04  zErrHandling* (the error-reporting subobject; receives Error())
//   +0x08  lo            (lowest valid index)
//   +0x0c  hi            (highest valid index)
//   +0x10  base          (element pointer biased so base + (idx-lo)*stride works)
//   +0x14  spare         (returned by the accessor's error path)
//   +0x18  stride        (element size in bytes)
//   +0x1c  alloc         (raw realloc base / per-element-fixup start)
//   +0x20  grown         (count of freshly-grown slots, scratch)
#ifndef GRUNTZ_WAP32_ZVEC_H
#define GRUNTZ_WAP32_ZVEC_H

#include <Ints.h>
#include <rva.h>

// (The empty zErrHandling placeholder that lived here - the stand-in base
// <Bute/ButeMgr.h>'s zPTree derives - moved into ButeMgr.h itself (wave2-H), so
// this header can coexist with the RTTI-real polymorphic zErrHandling of
// <Bute/PTreeNode.h> inside the merged TypeKeyColl TU. The _zvec head
// {vptr, m_err} IS that same container-error base - a pending dedup.)

// The error-slot the accessor invokes on overflow: m_err points to a CVariantSlot
// (its Set @0x16d850 is called at every overflow site). Was mis-typed as an empty
// `zErrHandling` placeholder; typed to the real pointee here.
struct CVariantSlot; // fwd (pointer member m_err; full def at the overflow call sites)

// The dynamic-vector base; `zDArray<T>` adds the per-element relocation override.
// REAL-POLYMORPHIC (implicit vptr@0): the virtual dtor drains the manual vptr field.
class _zvec {
public:
    void* GrowTo(i32 idx, i32 at); // 0x16da80
    i32 IndexToPtr(i32 idx);       // 0x312a0  (the plain base accessor)
    virtual ~_zvec();              // 0x16da60 (base scalar dtor, external TU; implicit vptr@0)

    // vptr @+0x00 (implicit, polymorphic)
    CVariantSlot* m_err; // +0x04
    i32 m_lo;            // +0x08
    i32 m_hi;            // +0x0c
    i32 m_base;          // +0x10
    i32 m_spare;         // +0x14
    i32 m_stride;        // +0x18
    i32 m_alloc;         // +0x1c
    i32 m_grown;         // +0x20
};

// zDArray<int (CUserLogic::*)(void)>: the derived vector whose elements are
// member-function pointers; its accessor override fixes up freshly-grown slots.
class zDArray : public _zvec {
public:
    i32 Destroy();               // 0x8750  (re-stamp live vtable + run ~zDArray)
    i32 IndexToPtr(i32 i);       // 0x310f0 (base accessor + per-slot member-ptr init)
    virtual ~zDArray() OVERRIDE; // 0x16df40 (cl auto-stamps ??_7zDArray at entry)
};

// --- vtable catalog ---

#endif // GRUNTZ_WAP32_ZVEC_H
