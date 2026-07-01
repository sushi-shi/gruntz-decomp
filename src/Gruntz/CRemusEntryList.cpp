#include <rva.h>
// CRemusEntryList.cpp - the 0x28-byte owned-collection node of the DDrawMgr "remus"
// image-manager family (placeholder name; engine "ClassUnknown_104"). Non-RTTI
// engine class; vtable @0x5efba8 (g_remusEntryListVtbl), grand-base dtor vtable
// g_remusBaseDtorVtbl @0x5e8cb4. See include/Gruntz/CRemusEntryList.h.
//
// Two methods (retail-RVA order):
//   0x152e30  ~CRemusEntryList (stamp own vtbl, DeleteAll, ~CObArray member, base)
//   0x165730  DeleteAll        (delete every owned element, free m_buf, RemoveAll)
//
// The base subobject + the destructible CObArray member give the dtor its /GX EH
// frame (the manual-vptr siblings can only reach the no-frame plateau; the real
// base/member model recovers the frame - cf. CWwdGrid::~CWwdGrid @0x1682a0).
#include <Mfc.h> // /GX EH-frame helpers

#include <Gruntz/CRemusEntryList.h>

// Engine heap free (RezFree, __cdecl, reloc-masked rel32). 0x1b9b82.
extern "C" void RezFree(void* p); // 0x1b9b82

// ===========================================================================
// 0x165730 - DeleteAll: delete every owned element via its scalar-deleting dtor
// (vtbl slot 1, arg 1), free the +0x1c buffer (RezFree), then RemoveAll the array.
// Plain /O2 leaf (no EH frame).
// ===========================================================================
RVA(0x00165730, 0x4c)
void CRemusEntryList::DeleteAll() {
    for (i32 i = 0; i < m_items.m_nSize; i++) {
        RemusObject* el = m_items.m_pData[i];
        if (el != 0) {
            (el->*(el->m_vptr->m_deleteDtor))(1);
        }
    }
    if (m_buf != 0) {
        RezFree(m_buf);
        m_buf = 0;
    }
    m_items.SetSize(0, -1); // CObArray::RemoveAll (inlined as SetSize(0,-1))
}

// ===========================================================================
// 0x152e30 - ~CRemusEntryList: stamp own vtable, run DeleteAll (the most-derived
// teardown), then the CObArray member destructs and ~CRemusBase folds in to
// restore the grand-base vtable. /GX frame from the destructible base+member.
// ===========================================================================
// Real polymorphic now: cl emits the implicit ??_7CRemusEntryList own-vptr stamp in
// the ENTRY state (stamp-first, == retail), then DeleteAll, then the member
// ~RemusObArray (trylevel 0) and ~CRemusBase grand-base re-stamp fold in. /GX frame
// from the destructible base + member. (eh-dtor-implicit-vptr-stamp-first.md.)
RVA(0x00152e30, 0x53)
CRemusEntryList::~CRemusEntryList() {
    DeleteAll();
    // m_items.~RemusObArray() (trylevel 0) + ~CRemusBase() (grand-base restore) fold here.
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CRemusBase);
SIZE_UNKNOWN(CRemusEntryList);
SIZE_UNKNOWN(RemusObArray);
SIZE_UNKNOWN(RemusObject);
