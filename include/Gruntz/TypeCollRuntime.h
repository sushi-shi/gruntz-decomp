// TypeCollRuntime.h - CTypeCollRuntime, a zDArray<CString> runtime collection
// (vtable ??_7CTypeCollRuntime@@6B@ @0x5f04e4). Derives from the CZArray2D
// allocating base (the shared WAP32 2D-array hierarchy in TypeKeyColl.h); its own
// scalar-deleting destructor destructs the CString element array (m_buf, count =
// m_hi - m_lo + 1) then runs the ~CZArray2D base teardown. The global type-name
// runtime (@0x6bf650, its atexit dtor 0x16e7a0) is an instance.
#ifndef GRUNTZ_GRUNTZ_TYPECOLLRUNTIME_H
#define GRUNTZ_GRUNTZ_TYPECOLLRUNTIME_H

#include <Ints.h>
#include <rva.h>

#include <Gruntz/TypeKeyColl.h> // the CZArray2D allocating base

class CTypeCollRuntime : public CZArray2D {
public:
    // Its own 1-slot vtable (??_7CTypeCollRuntime @0x1f04d0) holds exactly the virtual
    // scalar-deleting dtor below; declared-only (the ??1 lives in an unmatched TU), so
    // cl emits no ??_7 here - it only records the override for the vtable audit.
    virtual ~CTypeCollRuntime() OVERRIDE; // slot 0 (??_G 0x16ea20 -> ??1)
    // 0x16ea20 - the `??_G` scalar-deleting destructor: stamps the CTypeCollRuntime
    // vptr, destructs the m_buf CString array, runs ~CZArray2D, conditionally frees.
    void* ScalarDelete(u32 flags);
};

#endif // GRUNTZ_GRUNTZ_TYPECOLLRUNTIME_H
