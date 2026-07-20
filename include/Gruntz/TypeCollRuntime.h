// TypeCollRuntime.h - CTypeCollRuntime, a zDArray<CString> runtime collection
// (vtable ??_7CTypeCollRuntime@@6B@ @0x5f04e4). Derives from the canonical _zdvec
// allocating base (<Wap32/ZVec.h>; the former `CZArray2D` model of that same class is
// folded onto it); its own scalar-deleting destructor destructs the CString element
// array (m_base, count = m_hi - m_lo + 1) then runs the ~_zdvec base teardown. The global type-name
// runtime (@0x6bf650, its atexit dtor 0x16e7a0) is an instance.
#ifndef GRUNTZ_GRUNTZ_TYPECOLLRUNTIME_H
#define GRUNTZ_GRUNTZ_TYPECOLLRUNTIME_H

#include <Ints.h>
#include <rva.h>

#include <Wap32/ZVec.h> // the canonical _zdvec allocating base (ex-CZArray2D)

class CTypeCollRuntime : public _zdvec {
public:
    // Its own 1-slot vtable (??_7CTypeCollRuntime @0x1f04e4; slot 0 = the ??_G body
    // 0x16ea20 direct, byte-verified - the 0x1f04d0 previously recorded here is the
    // tail of the PRECEDING vtable) holds exactly the virtual scalar-deleting dtor
    // below; declared-only (the ??1 lives in an unmatched TU), so cl emits no ??_7
    // here - it only records the override for the vtable audit.
    virtual ~CTypeCollRuntime() OVERRIDE; // slot 0 (??_G 0x16ea20 -> ??1)
    // 0x16ea20 - the `??_G` scalar-deleting destructor: stamps the CTypeCollRuntime
    // vptr, destructs the m_base CString array, runs ~_zdvec, conditionally frees.
    void* ScalarDelete(u32 flags);
};
SIZE_UNKNOWN(
    CTypeCollRuntime
); // _zdvec base (0x24) + no own fields; size not independently pinned

#endif // GRUNTZ_GRUNTZ_TYPECOLLRUNTIME_H
