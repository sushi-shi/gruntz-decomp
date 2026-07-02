// CDDrawWorker.h - an owned-collection node of the DDrawMgr "DDraw worker"
// family (placeholder name; engine "ClassUnknown_35"). Non-RTTI engine class; its
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

#include <Ints.h>

// An owned CObject element: deleting destructor at vtable slot 1 (byte +0x04),
// __thiscall (flags arg). DeleteAll dispatches `el->vtbl[1](1)` per live element.
struct CWorkerElement {
    struct Vtbl {
        void* m_slot0;
        void* (CWorkerElement::*m_deleteDtor)(u32); // +0x04  scalar-deleting dtor
    }* m_vptr;
};

// The owned-pointer array embedded at +0x10 (engine CObArray; vtbl 0x5ed494).
// m_pData@+0x04 (= node+0x14), m_nSize@+0x08 (= node+0x18). Real member destructor
// calls the reloc-masked engine ~CObArray (0x1b561c) so the /GX member-teardown
// trylevel falls out (eh-dtor-model-members-as-destructible).
struct CWorkerObArray {
    void* m_vptr;             // +0x00
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

// The grand-base (CObject-like) dtor vtable restored at ~CLoadable exit.
extern void* g_wapObjectDtorVtbl; // 0x5e8cb4

// The "DDraw worker" base subobject: vptr @ +0x00 + three managed fields. Its
// non-trivial dtor resets the fields and restores the grand-base vtable. Inline so
// the leaf dtor FOLDS the base teardown (the field resets + vptr restore emit
// inline) and the /GX frame falls out of the destructible base subobject.
struct CLoadable {
    virtual ~CLoadable(); // implicit vptr @ +0x00; INLINE-defined below
    i32 m_04;             // +0x04
    i32 m_08;             // +0x08
    i32 m_0c;             // +0x0c
    CLoadable() {}
    // Arg-taking base ctor used by CDDrawWorkerHost: cl inlines the vptr stamp +
    // the three field stores at the head of the derived ctor.
    CLoadable(i32 a, i32 b, i32 c) {
        m_04 = a;
        m_08 = b;
        m_0c = c;
    }
};

class CDDrawWorker : public CLoadable {
public:
    ~CDDrawWorker();
    void DeleteAll(); // 0x151eb0  delete every owned element, RemoveAll, seed sentinels

    CWorkerObArray m_items;    // +0x10  owned-pointer array (m_pData@+0x14, m_nSize@+0x18)
    char m_pad24[0x64 - 0x24]; // +0x24..+0x63 (the family's per-node scratch block)
    i32 m_64;                  // +0x64  cached-index sentinel (DeleteAll seeds 99999)
    i32 m_68;                  // +0x68
};

// Call-free field resets; cl stamps ??_7CLoadable (masks g_wapObjectDtorVtbl
// @0x5e8cb4) and sinks the vptr store past the writes so it lands last, matching
// retail. Folded as the grand-base of every worker node.
inline CLoadable::~CLoadable() {
    m_04 = -1;
    m_08 = 0;
    m_0c = 0;
}

#endif // GRUNTZ_CDDRAWWORKER_H
