#include <rva.h>
#include <Rez/RezAlloc.h> // RezAlloc/RezFree
#include <Mfc.h> // /GX EH-frame helpers

#include <DDrawMgr/DDrawWorkerHost.h>
#include <Wwd/WwdSpatialMgr.h> // the canonical spatial/scroll worker (m_scroll)

// ===========================================================================
// 0x163af0 - ~CDDrawWorkerHost: stamp own vtable; if the worker is live run its
// PruneCount (0x1688b0) and then delete it (its FreeGrids body 0x1682f0 + the +0x70
// base-vtable restamp fold into the inline ~CWwdSpatialMgr); free the two owned
// buffers; then the +0x9c CWorkerObArray member and ~CLoadable fold in.
// ===========================================================================
// @early-stop
// Canonical CLoadable re-base lifted this 82.3%->85.8%->88.1%. Residual is the
// multi-member /GX funclet/state ordering across the worker delete + two buffer
// frees + CWorkerObArray member + ~CLoadable fold (grand-base stamp position + EH
// state writes), same wall class as the entry-list dtor. Logic complete.
RVA(0x00163af0, 0xcd)
CDDrawWorkerHost::~CDDrawWorkerHost() {
    if (m_scroll != 0) {
        m_scroll->PruneCount();
    }
    if (m_scroll != 0) {
        // retail INLINES ~CWwdSpatialMgr here (`delete m_scroll` with the full
        // class visible): its own null check (the double cmp), EH state, call FreeGrids
        // (0x1682f0), the +0x70 ~m_iter ??_7CObject stamp (not reproducible from the
        // reduced view without a manual vptr write - deliberately omitted), then the
        // class operator delete == RezFree. Spelled explicitly so the call relocs bind
        // to retail's own targets (a decl-only-dtor `delete` would mis-bind to 0x163a40).
        CWwdSpatialMgr* w = m_scroll;
        if (w != 0) {
            w->FreeGrids();
            RezFree(w);
        }
    }
    if (m_tileGrid != 0) {
        RezFree(m_tileGrid);
        m_tileGrid = 0;
    }
    if (m_colOffsets != 0) {
        RezFree(m_colOffsets);
        m_colOffsets = 0;
    }
    // m_frameSets.~CWorkerObArray() (0x1b561c) + ~CLoadable() (field resets + grand-base
    // vtable restore) fold here under the /GX frame.
}

VTBL(CDDrawWorkerHost, 0x001f0270);
