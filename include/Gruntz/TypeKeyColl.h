// TypeKeyColl.h - CTypeKeyColl (RTTI CTypeKeyColl, @0x6bf650) and its zDArray-style
// allocating base hierarchy CZArrayRoot <- CZArray2D <- CTypeKeyColl, the growable
// key collection that backs the per-class type registries (CKitchenSlime /
// CProjectile / CWarlord model the *registry* side in their own TUs; the
// collection's ctors / binary-search / global dynamic-init live in TypeKeyColl.cpp).
//
// The global g_typeColl @0x6bf650 is reached from many TUs, each of which used to
// re-declare its own one-method CTypeKeyColl view (SetAtGrow / IndexToPtr / Resolve
// / Find). This is the single shared shape; the view methods are declared-only so
// their __thiscall call rel32 reloc-masks. Only offsets + code bytes are load-
// bearing; field roles that are unproven keep m_<hex>.
#ifndef GRUNTZ_GRUNTZ_TYPEKEYCOLL_H
#define GRUNTZ_GRUNTZ_TYPEKEYCOLL_H

#include <rva.h>

// The error sink the array ctor reports a fatal alloc/bounds failure to (the owner
// stored at +0x04, set by the root ctor). __thiscall(this; arr, msg, code) 0x16d850.
SIZE_UNKNOWN(CZErrSink);
struct CZErrSink {};

// The zDArray construction hierarchy CZArrayRoot <- CZArray2D <- CTypeKeyColl. Each
// level is a REAL polymorphic class: its retail vtable (0x5f04cc/0x5f04d4/0x5f04d0)
// holds exactly one slot - its virtual scalar-deleting destructor (??_G 0x16da40/
// 0x16df20/0x16dde0, calling ??1 0x16da60/0x16df40/0x16de00; all in unmatched TUs,
// so the dtors are declared-only). cl emits the implicit ??_7 vptr stamp in each
// ctor (the retail manual `*(void**)this = &g_*Vtbl`). Giving CZArrayRoot a virtual
// dtor is also what gives CZArray2D's allocating ctor its /GX unwind frame.

// The deepest base (0x16d9c0 ctor, external): stows the error-sink owner (+0x04)
// from the data tag and one-time-inits the global tables.
SIZE_UNKNOWN(CZArrayRoot);
class CZArrayRoot {
public:
    CZArrayRoot(void* tag); // 0x16d9c0 (external no-body)
    virtual ~CZArrayRoot(); // [0] ??_G 0x16da40 (external no-body)
    CZErrSink* m_owner;     // +0x04  error-sink / owner
};

// The allocating zDArray base (0x16de30 ctor): records [lo,hi] + element stride,
// allocates the element buffer (+ a scratch element) and reports a fatal failure
// through the owner sink. /GX EH frame (unwinds the CZArrayRoot base on throw).
SIZE_UNKNOWN(CZArray2D);
class CZArray2D : public CZArrayRoot {
public:
    CZArray2D(i32 stride, i32 lo, i32 hi, void* scratch); // 0x16de30
    virtual ~CZArray2D() OVERRIDE;                        // [0] ??_G 0x16df20 (external)
    i32 m_lo;                                             // +0x08  index low bound
    i32 m_hi;                                             // +0x0c  index high bound
    void* m_buf;                                          // +0x10  primary element buffer
    void* m_buf2;                                         // +0x14  scratch element
    i32 m_stride;                                         // +0x18  element size
};

// The growable key collection itself (@0x6bf650, 0x16dda0 ctor). Find probes the
// sorted node array; the ctor forwards to the base and derives cursor + count. The
// remaining methods are the per-TU one-method views of the shared global g_typeColl:
// SetAtGrow (grow + assign, inlined in retail), IndexToPtr (thunk 0x403864 -> name
// node), Resolve (thunk 0x437c -> interned anim-code name). Declared-only ->
// reloc-masked.
VTBL(CTypeKeyColl, 0x001f04d0); // leaf ??_7CTypeKeyColl @0x5f04d0 (1-slot dtor vtable)
class CTypeKeyColl : public CZArray2D {
public:
    CTypeKeyColl(i32 stride, i32 lo, i32 hi, void* scratch); // 0x16dda0
    virtual ~CTypeKeyColl() OVERRIDE;                        // [0] ??_G 0x16dde0 (external)
    i32 Find(i32 key, i32 z);                                // 0x16da80 (external)
    void SetAtGrow(i32 id, const char* key);                 // grow + assign (inlined in retail)
    char** IndexToPtr(i32 idx); // thunk 0x403864 -> node (*node == name)
    void* Resolve(void* id);    // thunk 0x437c (__thiscall ret 4)
    void* m_cursor;             // +0x1c  (== m_buf)
    i32 m_count;                // +0x20  (== m_hi - m_lo + 1)
};

#endif // GRUNTZ_GRUNTZ_TYPEKEYCOLL_H
