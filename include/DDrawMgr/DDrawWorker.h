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
#include <Gruntz/Loadable.h> // canonical CLoadable : CWapObj : CObject (9-slot base)

// An owned CObject element: a real polymorphic object whose scalar-deleting
// destructor is at vtable slot 1 (byte +0x04), __thiscall (flags arg). Declared-
// only virtuals (the slot methods live in another TU) => cl emits NO ??_7 here;
// `el->Delete(1)` lowers to the same `mov ecx,el; push 1; mov eax,[el]; call
// [eax+0x04]` dispatch the old PMF table produced.
struct CWorkerElement {
    virtual void s0();               // +0x00
    virtual void* Delete(u32 flags); // +0x04  scalar-deleting dtor
    virtual void s2();               // +0x08  (declared-only fillers to slot 13)
    virtual void s3();               // +0x0c
    virtual void s4();               // +0x10
    virtual void s5();               // +0x14
    virtual void s6();               // +0x18
    virtual void s7();               // +0x1c
    virtual void s8();               // +0x20
    virtual void s9();               // +0x24
    virtual void s10();              // +0x28
    virtual void s11();              // +0x2c
    virtual void s12();              // +0x30
    virtual i32 Query34(i32 a, i32 b); // +0x34  slot 13 (range-query predicate)
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
    // CObArray::SetAtGrow(index, element) (reloc-masked rel32 callee 0x1b5822).
    void SetAtGrow(i32 index, void* element);
};

// The grand-base (CObject-like) dtor vtable, restamped manually by CWwdSpatialMgr's
// teardown (CDDrawWorkerHost.h). Same datum as ??_7CObject @0x5e8cb4.

// The "DDraw worker" base subobject is the canonical CLoadable (m_04/m_08/m_0c +
// the field-reset dtor + the grand-base 0x5e8cb4 re-stamp folded via ~CWapObj ->
// ~CObject). Its ctor/dtor fold into the leaf's, giving retail's two-phase
// vptr schedule + the destructible-base /GX frame.
class CSymTab; // Bute/SymTab.h - the name->record table slots 10/15 iterate

class CDDrawWorker : public CLoadable {
public:
    virtual ~CDDrawWorker() OVERRIDE; // slot 1 (scalar-deleting dtor)
    // slots 9-16: the 8 new virtuals CDDrawWorker adds over CLoadable's 9-slot base.
    // Declared-only => cl emits the full 17-slot ??_7CDDrawWorker @0x1efbe8 (this
    // realizes the vtable per the all-vtables mandate; was a bare VTBL() manual ref).
    // Bodies live at their retail RVAs (reloc-masked). Slots 10 (0x1521f0, byte-exact)
    // and 15 (0x1522b0, @early-stop regalloc wall) are HOMED in DDrawWorker.cpp: they
    // walk a CSymTab scope and self-dispatch InsertFrame (slot 14) / Slot40_1523b0
    // (slot 16). m_0c is the owning CDDrawSurfaceMgr (single-frame flag m_flags&0x100).
    virtual i32 SymValue_155810(void* rec);                // slot 9  @0x155810
    virtual i32 BuildFramesFromSymTab(CSymTab* tab);       // slot 10 @0x1521f0
    virtual i32 CreateFrame24(i32 a, i32 b, i32 c, i32 d); // slot 11 @0x152110
    virtual i32 CreateFrame28(i32 a, i32 b, i32 c, i32 d); // slot 12 @0x152060
    virtual i32 CreateFrame30(i32 a, i32 b, i32 c);        // slot 13 @0x151fb0
    virtual i32 InsertFrame(void* rec, i32 n, i32 flag);   // slot 14 @0x151f00
    virtual i32 ValidateFramesFromSymTab(CSymTab* tab);    // slot 15 @0x1522b0
    virtual i32 Slot40_1523b0(i32 rec, i32 n, i32 flag); // slot 16 @0x1523b0
    void DeleteAll(); // 0x151eb0  delete every owned element, RemoveAll, seed sentinels
    void AddFrameAt_1521c0(void* elem, i32 index); // 0x1521c0  SetAtGrow + widen [m_64,m_68]

    CWorkerObArray m_items;    // +0x10  owned-pointer array (m_pData@+0x14, m_nSize@+0x18)
    char m_pad24[0x64 - 0x24]; // +0x24..+0x63 (the family's per-node scratch block)
    i32 m_64;                  // +0x64  cached-index sentinel (DeleteAll seeds 99999)
    i32 m_68;                  // +0x68
};
VTBL(CDDrawWorker, 0x001efbe8); // ??_7CDDrawWorker@@6B@ (17-slot CLoadable-derived vtable)

// --- vtable catalog (reduced-view classes share their base vtable rva) ---

#endif // GRUNTZ_CDDRAWWORKER_H
