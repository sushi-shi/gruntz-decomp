#include <rva.h>
// DDrawWorkerHost.cpp - the scalar (non-deleting) destructor (0x163af0) of the
// CLoadable-derived host (own vtable g_ddrawWorkerHostVtbl @0x5f0270). Stamps
// its own vtable, shuts down + frees the +0xb0 worker (PruneCount then delete),
// frees the two owned buffers (+0x20/+0x24), then the CWorkerObArray member (+0x9c)
// and the CLoadable grand-base teardown fold in under the /GX frame. See
// include/DDrawMgr/DDrawWorkerHost.h.
#include <Mfc.h> // /GX EH-frame helpers

#include <DDrawMgr/DDrawWorkerHost.h>

// The host's own primary vtable (0x5f0270) is now the cl-emitted
// ??_7CDDrawWorkerHost (real-polymorphic CLoadable-derived class; VTBL at
// EOF). The manual g_ddrawWorkerHostVtbl DATA-pin (Vtbl_1f0270 catalog) is gone.

// The engine Rez heap free (_RezFree 0x1b9b82, cdecl C) used for the two owned
// buffers (and, via CWwdSpatialMgr::operator delete, the worker).
extern "C" void RezFree(void* p);

// ===========================================================================
// 0x1615a0 - CDDrawWorkerHost(a1,a2,a3): the /GX EH ctor. cl inlines the
// CLoadable base ctor (vptr stamp -- reloc-masks the retail intermediate
// g_loadableVtbl 0x5efc30 -- then m_04=a2/m_08=a3/m_0c=a1), constructs the
// +0x9c CWorkerObArray member (0x1b55e9; its destructible-member trylevel supplies
// the EH frame), stamps the own vftable (0x5f0270), then arms the scalar fields
// (buffers/worker = 0, m_18/m_1c = 1.0f, m_50 = -1) and zero-fills the +0xf4 pool
// (25 dwords) with m_pool[0] = 100. Byte-exact (100%): the retail intermediate base
// stamp 0x5efc30 is reloc-masked, so the compiler-emitted ??_7CLoadable stamp
// matches at the byte level; the CLoadable ctor arg-order (m_04=a2/m_08=a3/
// m_0c=a1) + body store order reproduce the schedule exactly.
// ===========================================================================
RVA(0x001615a0, 0x9a)
CDDrawWorkerHost::CDDrawWorkerHost(i32 owner, i32 field04, i32 field08)
    : CLoadable(owner, field04, field08) {
    // m_obArray (CWorkerObArray) default-constructed here (0x1b55e9).
    m_buffer0 = 0;
    m_buffer1 = 0;
    m_spatialWorker = 0;
    m_18 = 1.0f;
    m_1c = 1.0f;
    m_50 = -1;
    memset(m_pool, 0, sizeof(m_pool));
    m_pool[0] = 100;
}

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
    if (m_spatialWorker != 0) {
        m_spatialWorker->PruneCount();
    }
    if (m_spatialWorker != 0) {
        delete m_spatialWorker;
    }
    if (m_buffer0 != 0) {
        RezFree(m_buffer0);
        m_buffer0 = 0;
    }
    if (m_buffer1 != 0) {
        RezFree(m_buffer1);
        m_buffer1 = 0;
    }
    // m_obArray.~CWorkerObArray() (0x1b561c) + ~CLoadable() (field resets + grand-base
    // vtable restore) fold here under the /GX frame.
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CDDrawWorkerHost);
// ??_7CDDrawWorkerHost (was g_ddrawWorkerHostVtbl @0x5f0270, Vtbl_1f0270 /
// vtbl-cluster-56). cl auto-emits it from the real-polymorphic host;
// retail's 12-slot datum is reloc-masked -> matching-neutral catalog tracking.
VTBL(CDDrawWorkerHost, 0x001f0270);
