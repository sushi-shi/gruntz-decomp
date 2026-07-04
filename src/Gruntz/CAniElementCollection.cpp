#include <rva.h>
// CAniElementCollection.cpp - the 0x28-byte owned-collection node of the DDrawMgr
// image-manager family (this IS CAniElement: vtable @0x5efba8 = ??_7CAniElementObj,
// same 0x28 layout as include/Gruntz/CAniElement.h - a duplicate polymorphic view
// kept for its /GX dtor model; dedup with CAniElement.h is a follow-up). Non-RTTI
// engine class; grand-base dtor vtable g_wapObjectDtorVtbl @0x5e8cb4.
// See include/Gruntz/CAniElementCollection.h.
//
// Two methods (retail-RVA order):
//   0x152e30  ~CAniElement (stamp own vtbl, DeleteAll, ~CObArray member, base)
//   0x165730  DeleteAll        (delete every owned element, free m_buf, RemoveAll)
//
// The base subobject + the destructible CObArray member give the dtor its /GX EH
// frame (the manual-vptr siblings can only reach the no-frame plateau; the real
// base/member model recovers the frame - cf. CWwdGrid::~CWwdGrid @0x1682a0).
#include <Mfc.h> // /GX EH-frame helpers

#include <Gruntz/CAniElementCollection.h>

// Engine heap free (RezFree, __cdecl, reloc-masked rel32). 0x1b9b82.
extern "C" void RezFree(void* p); // 0x1b9b82

// ===========================================================================
// 0x165730 - DeleteAll: delete every owned element via its scalar-deleting dtor
// (vtbl slot 1, arg 1), free the +0x1c buffer (RezFree), then RemoveAll the array.
// Plain /O2 leaf (no EH frame).
// ===========================================================================
RVA(0x00165730, 0x4c)
void CAniElement::DeleteAll() {
    for (i32 i = 0; i < m_records.m_nSize; i++) {
        CAniRecordView* el = m_records.m_pData[i];
        if (el != 0) {
            el->ScalarDtor(1);
        }
    }
    if (m_name != 0) {
        RezFree(m_name);
        m_name = 0;
    }
    m_records.SetSize(0, -1); // CObArray::RemoveAll (inlined as SetSize(0,-1))
}

// ===========================================================================
// 0x152e30 - ~CAniElement: stamp own vtable, run DeleteAll (the most-derived
// teardown), then the CObArray member destructs and ~Wap::CObject folds in to
// restore the grand-base vtable. /GX frame from the destructible base+member.
// ===========================================================================
// Real polymorphic now: cl emits the implicit ??_7CAniElement own-vptr stamp in
// the ENTRY state (stamp-first, == retail), then DeleteAll, then the member
// ~CAniRecordArray (trylevel 0) and ~Wap::CObject grand-base re-stamp fold in. /GX frame
// from the destructible base + member. (eh-dtor-implicit-vptr-stamp-first.md.)
RVA(0x00152e30, 0x53)
CAniElement::~CAniElement() {
    DeleteAll();
    // m_items.~CAniRecordArray() (trylevel 0) + ~Wap::CObject() (grand-base restore) fold here.
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(Wap::CObject);
SIZE_UNKNOWN(CAniElement);
SIZE_UNKNOWN(CAniRecordArray);
SIZE_UNKNOWN(CAniRecordView);
