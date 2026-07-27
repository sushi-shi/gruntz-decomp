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

    // The element type is PROVEN CString (not merely asserted): _zdvec::IndexToPtr's
    // fixup loop @0x31156 walks m_alloc in steps of a constant 4 and calls
    // ??0CString@@QAE@XZ (0x1b9b93) on each new slot, and CInGameText::Update @0x997c0
    // runs that same loop inlined over g_typeColl's own m_alloc/m_grown (0x6bf66c /
    // 0x6bf670). The BASE cannot carry that type - _zvec::IndexToPtr computes
    // `m_base + (idx-m_lo)*m_stride` with m_stride loaded from the object at run time
    // (`imul esi,[edi+0x18]`), which is byte math no element type can express. So the
    // reinterpret_casts below are the one legitimate seam, not mis-modeling: exactly
    // one per accessor, at the boundary where the untyped band becomes typed.
    //
    // Two accessors, not one, because they are two DIFFERENT retail calls: the _zdvec
    // form (0x310f0, via ILT 0x437c) runs the CString construction fixup; the _zvec
    // form (0x312a0) does not.

    // the key IS the act id (AnimWorkerObj::m_1c / ActFindId), not a pointer
    char** GetNameRecord(i32 key) {
        // same slot + same call as SlotOf below, spelled as the raw char* the 159 call
        // sites read; CString's only member IS that char* (m_pchData)
        return reinterpret_cast<char**>(_zdvec::IndexToPtr(key));
    }
    // CAnimNameRecord (TypeKeyColl.h) is `struct { char* m_name; }` - the same 4-byte
    // slot in the same band, i.e. a stand-in for CString. Folding it is a follow-up:
    // 31 use sites over 6 files (`rec->m_name` -> the inline operator LPCTSTR, same
    // `mov eax,[eax]`), which is a wider sweep than this seam.
    CAnimNameRecord* GetNameRecords(i32 key) {
        return reinterpret_cast<CAnimNameRecord*>(_zvec::IndexToPtr(key));
    }
    CAnimNameRecord* ScratchResolve(i32 key) {
        return reinterpret_cast<CAnimNameRecord*>(_zvec::IndexToPtr(key));
    }

    CString* Elem(i32 id) {
        return reinterpret_cast<CString*>(m_base + (id - m_lo) * m_stride);
    }
    // the typed spelling of GetNameRecord (identical call), for the act-registration
    // macros that want the CString rather than its buffer
    CString* SlotOf(i32 id) { return reinterpret_cast<CString*>(_zdvec::IndexToPtr(id)); }
    CString* Slots() { return reinterpret_cast<CString*>(m_alloc); }
    CString* Scratch() { return reinterpret_cast<CString*>(m_spare); }
};
SIZE_UNKNOWN(); // _zdvec base (0x24) + no own fields; size not pinned

#endif // GRUNTZ_GRUNTZ_TYPECOLLRUNTIME_H
