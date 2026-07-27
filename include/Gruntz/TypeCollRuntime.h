#ifndef GRUNTZ_GRUNTZ_TYPECOLLRUNTIME_H
#define GRUNTZ_GRUNTZ_TYPECOLLRUNTIME_H

#include <Ints.h>
#include <rva.h>

#include <Wap32/ZVec.h> // the canonical _zdvec allocating base (ex-CZArray2D)

class CAnimNameRecord;
class CString;

// Its own 1-slot vtable. VTBL names the datum ??_7CTypeCollRuntime@@6B@ (a typed vtable,
// not the ex-DATA `void* const CTypeCollRuntime_vtbl` global). The real dtor below (the
// only member the devs wrote) makes this the vtable's key TU, so cl emits the ??_7 + the
// `??_G' scalar-deleting destructor (0x16ea20) here and auto-stamps the vptr.
class CTypeCollRuntime : public _zdvec {
public:
    CTypeCollRuntime();
    virtual ~CTypeCollRuntime() OVERRIDE;

    // the key IS the act id (AnimWorkerObj::m_1c / ActFindId), not a pointer
    char** GetNameRecord(i32 key) {
        // _zdvec/_zvec are UNTYPED byte vectors - IndexToPtr returns char* - so every
        // typed accessor here is the one seam that puts the element type back on
        return reinterpret_cast<char**>(_zdvec::IndexToPtr(key));
    }
    CAnimNameRecord* GetNameRecords(i32 key) {
        // the same one seam: untyped byte vector in, element type out
        return reinterpret_cast<CAnimNameRecord*>(_zvec::IndexToPtr(key));
    }
    CAnimNameRecord* ScratchResolve(i32 key) {
        // the same one seam: untyped byte vector in, element type out
        return reinterpret_cast<CAnimNameRecord*>(_zvec::IndexToPtr(key));
    }

    // The element type is CString: the band is byte-addressed (base + (id-lo)*stride),
    // so the char* -> CString* pun lives here, at the one seam, instead of at every
    // per-TU act-name lookup.
    CString* Elem(i32 id) {
        return reinterpret_cast<CString*>(m_base + (id - m_lo) * m_stride);
    }
    // the same pun for the out-of-line accessor (0x310f0: base address + per-slot
    // construction), so the act-registration macros never re-spell it either
    CString* SlotOf(i32 id) { return reinterpret_cast<CString*>(_zdvec::IndexToPtr(id)); }
    CString* Slots() { return reinterpret_cast<CString*>(m_alloc); }
    CString* Scratch() { return reinterpret_cast<CString*>(m_spare); }
};
SIZE_UNKNOWN(); // _zdvec base (0x24) + no own fields; size not pinned

#endif // GRUNTZ_GRUNTZ_TYPECOLLRUNTIME_H
