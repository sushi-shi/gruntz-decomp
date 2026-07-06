// DDrawWorker.h - an owned-collection node of the DDrawMgr "DDraw worker"
// family (placeholder name; engine "tomalla-35"). Non-RTTI engine class; its
// own primary vtable is at RVA 0x1efbe8 (g_ddrawWorkerVtbl). It derives from a
// CLoadable-shaped base subobject (m_04/m_08/m_0c reset on teardown, then the
// CObject-like grand-base dtor vtable g_wapObjectDtorVtbl @0x5e8cb4 restored).
//
// Layout recovered from the dtor (0x1557a0) + DeleteAll helper (0x151eb0):
//   +0x00 vptr (CLoadable)
//   +0x04 m_04  (reset to -1 on teardown)
//   +0x08 m_08  (reset to 0)
//   +0x0c m_0c  (reset to 0)
//   +0x10 m_items  CObArray of owned CObject* (vtbl 0x5ed494; m_pData@+0x14, m_nSize@+0x18)
//   +0x64 m_64  cached-index sentinel (DeleteAll seeds it to 99999 = 0x1869f)
//   +0x68 m_68  (reset to 0 by DeleteAll)
// Only the offsets + emitted bytes are load-bearing; field names are placeholders.
#ifndef GRUNTZ_CDDRAWWORKER_H
#define GRUNTZ_CDDRAWWORKER_H
#include <rva.h>

#include <Ints.h>
#include <Gruntz/Loadable.h> // canonical CLoadable : CWapObj : Wap::CObject (9-slot base)

// An owned CObject element: a real polymorphic object whose scalar-deleting
// destructor is at vtable slot 1 (byte +0x04), __thiscall (flags arg). Declared-
// only virtuals (the slot methods live in another TU) => cl emits NO ??_7 here;
// `el->Delete(1)` lowers to the same `mov ecx,el; push 1; mov eax,[el]; call
// [eax+0x04]` dispatch the old PMF table produced.
struct CWorkerElement {
    void s0();               // +0x00
    void* Delete(u32 flags); // +0x04  scalar-deleting dtor
};

// The owned-pointer array embedded at +0x10 (engine CObArray; vtbl 0x5ed494).
// m_pData@+0x04 (= node+0x14), m_nSize@+0x08 (= node+0x18). Real member destructor
// calls the reloc-masked engine ~CObArray (0x1b561c) so the /GX member-teardown
// trylevel falls out (eh-dtor-model-members-as-destructible).
struct CWorkerObArray {
    char _vft0[4];            // +0x00 foreign object vptr (reduced view; not owned/dispatched)
    CWorkerElement** m_pData; // +0x04
    i32 m_nSize;              // +0x08
    i32 m_nMaxSize;           // +0x0c
    i32 m_nGrowBy;            // +0x10
    CWorkerObArray();         // 0x1b55e9 CObArray default ctor (reloc-masked rel32 callee)
    void Dtor_1b561c();       // ~CObArray (reloc-masked rel32 callee)
    ~CWorkerObArray() {
        Dtor_1b561c();
    }
    // CObArray::SetSize(newSize, growBy) (reloc-masked rel32 callee 0x1b5653);
    // RemoveAll() is inlined by retail as SetSize(0, -1), so call it directly.
    void SetSize(i32 newSize, i32 growBy);
};

// The grand-base (CObject-like) dtor vtable, restamped manually by CWwdSpatialMgr's
// teardown (CDDrawWorkerHost.h). Same datum as ??_7CObject @0x5e8cb4.

// The "DDraw worker" base subobject is the canonical CLoadable (m_04/m_08/m_0c +
// the field-reset dtor + the grand-base 0x5e8cb4 re-stamp folded via ~CWapObj ->
// ~Wap::CObject). Its ctor/dtor fold into the leaf's, giving retail's two-phase
// vptr schedule + the destructible-base /GX frame.
class CDDrawWorker : public CLoadable {
public:
    ~CDDrawWorker();  // slot 1 (scalar-deleting dtor)
    void DeleteAll(); // 0x151eb0  delete every owned element, RemoveAll, seed sentinels

    CWorkerObArray m_items;    // +0x10  owned-pointer array (m_pData@+0x14, m_nSize@+0x18)
    char m_pad24[0x64 - 0x24]; // +0x24..+0x63 (the family's per-node scratch block)
    i32 m_64;                  // +0x64  cached-index sentinel (DeleteAll seeds 99999)
    i32 m_68;                  // +0x68
};

// --- vtable catalog (reduced-view classes share their base vtable rva) ---

#endif // GRUNTZ_CDDRAWWORKER_H
