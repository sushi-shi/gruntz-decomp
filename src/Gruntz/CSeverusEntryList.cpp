#include <rva.h>
// CSeverusEntryList.cpp - the owned-collection node of the DDrawMgr "severus worker"
// family (placeholder name; engine "ClassUnknown_35"). Non-RTTI engine class;
// vtable @0x5efbe8 (g_severusEntryListVtbl), grand-base dtor vtable
// g_severusBaseDtorVtbl @0x5e8cb4. See include/Gruntz/CSeverusEntryList.h.
//
// Two methods (retail-RVA order):
//   0x151eb0  DeleteAll        (delete every owned element, RemoveAll, seed sentinels)
//   0x1557a0  ~CSeverusEntryList (stamp own vtbl, DeleteAll, ~CObArray member, base)
//
// The CSeverusWorker-shaped base subobject + the destructible CObArray member give
// the dtor its /GX EH frame (cf. CWwdGrid::~CWwdGrid @0x1682a0).
#include <Mfc.h> // /GX EH-frame helpers

#include <Gruntz/CSeverusEntryList.h>

// The node's own primary vtable (foreign engine datum, reloc-masked DATA()).
DATA(0x001efbe8)
extern void* g_severusEntryListVtbl; // 0x5efbe8

// The grand-base dtor vtable (declared in the header for the inline ~CSeverusBase);
// pin its address here (reloc-masked DATA()).
DATA(0x005e8cb4)
extern void* g_severusBaseDtorVtbl; // 0x5e8cb4

// ===========================================================================
// 0x151eb0 - DeleteAll: delete every owned element via its scalar-deleting dtor
// (vtbl slot 1, arg 1), RemoveAll the array, then seed the +0x64 cached-index
// sentinel (99999) and clear +0x68. Plain /O2 leaf (no EH frame).
// ===========================================================================
RVA(0x00151eb0, 0x43)
void CSeverusEntryList::DeleteAll() {
    for (i32 i = 0; i < m_items.m_nSize; i++) {
        SeverusObject* el = m_items.m_pData[i];
        if (el != 0) {
            (el->*(el->m_vptr->m_deleteDtor))(1);
        }
    }
    m_items.SetSize(0, -1); // CObArray::RemoveAll (inlined as SetSize(0,-1))
    m_64 = 99999;
    m_68 = 0;
}

// ===========================================================================
// 0x1557a0 - ~CSeverusEntryList: stamp own vtable, run DeleteAll (most-derived
// teardown), then the CObArray member destructs and ~CSeverusBase folds in
// (resets m_04/m_08/m_0c, restores the grand-base vtable). /GX frame from the
// destructible base+member.
// ===========================================================================
// @early-stop
// EH-state-machine order wall (eh-dtor-vptr-stamp-vs-trylevel-order): body
// byte-identical, but retail emits the own-vptr stamp before the entry trylevel
// write (the /GX state machine's order; not steerable from C). Same plateau class
// as CWwdGrid::~CWwdGrid (0x1682a0, ~93%). Logic complete.
RVA(0x001557a0, 0x68)
CSeverusEntryList::~CSeverusEntryList() {
    m_vptr = &g_severusEntryListVtbl;
    DeleteAll();
    // m_items.~SeverusObArray() (trylevel 0) + ~CSeverusBase() (field resets +
    // grand-base restore) fold here.
}
