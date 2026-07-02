// CAniElementCollection.h - the (0x28-byte) owned-collection node of the DDrawMgr
// image/surface-manager family. This IS CAniElement (its own primary vtable @0x1efba8
// = ??_7CAniElementObj; same 0x28 layout as include/Gruntz/CAniElement.h) - a
// duplicate polymorphic view kept for its /GX dtor model (dedup is a follow-up).
// The g_wapObjectDtorVtbl @0x5e8cb4 base lineage is shared with CResolveNode /
// CWwdGrid / CDDrawSurfacePair. The base subobject restores the CObject-like
// grand-base dtor vtable g_wapObjectDtorVtbl @0x5e8cb4 at teardown.
//
// Layout recovered from the factory (0x1528d0, `new` 0x28 bytes) + the dtor +
// DeleteAll helper:
//   +0x00 vptr (CWapObject)
//   +0x04 m_04   (zeroed at construct)
//   +0x08 m_items  CObArray of owned CObject* (vtbl 0x5ed494; m_pData@+0xc, m_nSize@+0x10)
//   +0x1c m_buf  a heap buffer freed (RezFree) on teardown
// Only the offsets + emitted bytes are load-bearing; field names are placeholders.
#ifndef GRUNTZ_CANIELEMENTCOLLECTION_H
#define GRUNTZ_CANIELEMENTCOLLECTION_H

#include <Ints.h>

// An owned CObject element: the deleting destructor is vtable slot 1 (byte +0x04),
// __thiscall (flags arg). DeleteAll dispatches `el->vtbl[1](1)` per live element.
struct CAniRecordView {
    struct Vtbl {
        void* m_slot0;
        void* (CAniRecordView::*m_deleteDtor)(u32); // +0x04  scalar-deleting dtor
    }* m_vptr;
};

// The owned-pointer array embedded at +0x08 (engine CObArray; vtbl 0x5ed494).
// m_pData@+0x04 (= node+0x0c), m_nSize@+0x08 (= node+0x10). Modeled with a real
// member destructor that calls the reloc-masked engine ~CObArray (0x1b561c) so the
// /GX member-teardown trylevel falls out (eh-dtor-model-members-as-destructible).
struct CAniRecordArray {
    void* m_vptr;             // +0x00
    CAniRecordView** m_pData; // +0x04
    i32 m_nSize;              // +0x08
    i32 m_nMaxSize;           // +0x0c
    i32 m_nGrowBy;            // +0x10
    void Dtor_1b561c();       // ~CObArray (reloc-masked rel32 callee)
    ~CAniRecordArray() {
        Dtor_1b561c();
    }
    // CObArray::SetSize(newSize, growBy) (reloc-masked rel32 callee 0x1b5653);
    // RemoveAll() is inlined by retail as SetSize(0, -1), so call it directly.
    void SetSize(i32 newSize, i32 growBy);
};

// The CWapObject engine base (CObject-like, grand-base vtable @0x5e8cb4): the implicit
// vptr @+0x00 + the 5-slot CObject-style interface (slot0/dtor/slot2/slot3/slot4 ->
// the shared sub_1bef01/scalar-dtor/sub_0028ec/sub_00106e/sub_004034). Real
// polymorphic: the empty inline virtual dtor makes cl emit the implicit
// ??_7CWapObject grand-base re-stamp (reloc-masks 0x5e8cb4) folded into the leaf
// dtor, and the destructible base subobject gives ~CAniElement its /GX frame.
struct CWapObject {
    virtual void WapV0();  // slot 0 (sub_1bef01)
    virtual ~CWapObject(); // slot 1 (scalar-deleting dtor)
    virtual void WapV2();  // slot 2 (sub_0028ec)
    virtual void WapV3();  // slot 3 (sub_00106e)
    virtual void WapV4();  // slot 4 (sub_004034)
    CWapObject() {}
};

class CAniElement : public CWapObject {
public:
    virtual ~CAniElement();
    void DeleteAll(); // 0x165730  delete every owned element, free m_buf, RemoveAll

    i32 m_04;                // +0x04
    CAniRecordArray m_items; // +0x08  owned-pointer array (m_pData@+0xc, m_nSize@+0x10)
    void* m_buf;             // +0x1c  heap buffer (RezFree'd on teardown)
};

// Empty body -> cl emits ONLY the implicit grand-base vptr re-stamp (0x5e8cb4),
// folded into the leaf dtor as the last store.
inline CWapObject::~CWapObject() {}

#endif // GRUNTZ_CANIELEMENTCOLLECTION_H
