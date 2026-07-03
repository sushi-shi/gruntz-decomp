#include <rva.h>
// CDDrawWorker.cpp - the owned-collection node of the DDrawMgr "DDraw worker"
// family (placeholder name; engine "tomalla-35"). Non-RTTI engine class;
// vtable @0x5efbe8 (g_ddrawWorkerVtbl), grand-base dtor vtable
// g_wapObjectDtorVtbl @0x5e8cb4. See include/Gruntz/CDDrawWorker.h.
//
// Two methods (retail-RVA order):
//   0x151eb0  DeleteAll        (delete every owned element, RemoveAll, seed sentinels)
//   0x1557a0  ~CDDrawWorker (stamp own vtbl, DeleteAll, ~CObArray member, base)
//
// The CLoadable-shaped base subobject + the destructible CObArray member give
// the dtor its /GX EH frame (cf. CWwdGrid::~CWwdGrid @0x1682a0).
#include <Mfc.h> // /GX EH-frame helpers

#include <Gruntz/CDDrawWorker.h>

// The class is real-polymorphic: cl emits ??_7CLoadable (grand-base @0x5e8cb4)
// + the derived vtable @0x5efbe8 implicitly. Both former manual externs
// (g_ddrawWorkerVtbl / g_wapObjectDtorVtbl) were dead here and are removed;
// the derived vtable 0x1efbe8 is named by VTBL(CDDrawWorker) in
// CDDrawWorkerRegistry.cpp (same retail class, fuller 17-slot model).

// ===========================================================================
// 0x151eb0 - DeleteAll: delete every owned element via its scalar-deleting dtor
// (vtbl slot 1, arg 1), RemoveAll the array, then seed the +0x64 cached-index
// sentinel (99999) and clear +0x68. Plain /O2 leaf (no EH frame).
// ===========================================================================
RVA(0x00151eb0, 0x43)
void CDDrawWorker::DeleteAll() {
    for (i32 i = 0; i < m_items.m_nSize; i++) {
        CWorkerElement* el = m_items.m_pData[i];
        if (el != 0) {
            el->Delete(1);
        }
    }
    m_items.SetSize(0, -1); // CObArray::RemoveAll (inlined as SetSize(0,-1))
    m_64 = 99999;
    m_68 = 0;
}

// ===========================================================================
// 0x1557a0 - ~CDDrawWorker: stamp own vtable, run DeleteAll (most-derived
// teardown), then the CObArray member destructs and ~CLoadable folds in
// (resets m_04/m_08/m_0c, restores the grand-base vtable). /GX frame from the
// destructible base+member.
// ===========================================================================
// 100%: re-basing onto the canonical CLoadable : CWapObj : Wap::CObject resolved the
// grand-base vptr-stamp-position wall - the real CObject grand-base sinks the 0x5e8cb4
// re-stamp after the m_04/m_08/m_0c resets exactly as retail (was ~95% on the 1-slot
// CLoadable stand-in that stamped the vptr before the field writes).
RVA(0x001557a0, 0x68)
CDDrawWorker::~CDDrawWorker() {
    DeleteAll();
    // m_items.~CWorkerObArray() (trylevel 0) + ~CLoadable() (field resets +
    // grand-base vtable stamp) fold here.
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
// SIZE(CLoadable) now comes from the canonical <Gruntz/CLoadable.h>.
SIZE_UNKNOWN(CDDrawWorker);
SIZE_UNKNOWN(CWorkerObArray);
SIZE_UNKNOWN(CWorkerElement);
