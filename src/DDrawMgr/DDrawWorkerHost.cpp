#include <rva.h>
// DDrawWorkerHost.cpp - the host's destructor (0x163af0), which retail birth-
// positions PAST the plane/render TU boundary (0x163a00): the class's ctor
// (0x1615a0), ReadPlaneBlock gap (0x161640) and RegisterNamed (0x161c50) are
// woven INSIDE the plane TU and live in src/Gruntz/LevelPlane.cpp (interval
// dossier 0x15ccd0, wave1-C). The class definition stays canonical in
// include/DDrawMgr/DDrawWorkerHost.h.
//
// The dtor stamps its own vtable, shuts down + frees the +0xb0 worker (PruneCount
// then delete), frees the two owned buffers (+0x20/+0x24), then the CWorkerObArray
// member (+0x9c) and the CLoadable grand-base teardown fold in under the /GX frame.
#include <Mfc.h> // /GX EH-frame helpers

#include <DDrawMgr/DDrawWorkerHost.h>

// The host's own primary vtable (0x5f0270) is now the cl-emitted
// ??_7CDDrawWorkerHost (real-polymorphic CLoadable-derived class; VTBL at
// EOF). The manual g_ddrawWorkerHostVtbl DATA-pin (Vtbl_1f0270 catalog) is gone.

// The engine Rez heap free (_RezFree 0x1b9b82, cdecl C) used for the two owned
// buffers (and, via CWwdSpatialMgr::operator delete, the worker).
extern "C" void RezFree(void* p);

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

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
// Size PROVEN from the allocation site (push 0x158; call ??2 -> the ctor), and our
// reconstruction computes exactly that. Pinned so no future note can claim it unknown.
SIZE(CDDrawWorkerHost, 0x158);
// ??_7CDDrawWorkerHost (was g_ddrawWorkerHostVtbl @0x5f0270, Vtbl_1f0270 /
// vtbl-cluster-56). cl auto-emits it from the real-polymorphic host;
// retail's 12-slot datum is reloc-masked -> matching-neutral catalog tracking.
VTBL(CDDrawWorkerHost, 0x001f0270);
