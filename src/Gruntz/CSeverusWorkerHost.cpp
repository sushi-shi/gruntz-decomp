#include <rva.h>
// CSeverusWorkerHost.cpp - the scalar (non-deleting) destructor (0x163af0) of the
// CSeverusBase-derived host (own vtable g_severusWorkerHostVtbl @0x5f0270). Stamps
// its own vtable, shuts down + frees the +0xb0 worker (PreDestroy then delete),
// frees the two owned buffers (+0x20/+0x24), then the SeverusObArray member (+0x9c)
// and the CSeverusBase grand-base teardown fold in under the /GX frame. See
// include/Gruntz/CSeverusWorkerHost.h.
#include <Mfc.h> // /GX EH-frame helpers

#include <Gruntz/CSeverusWorkerHost.h>

// The host's own primary vtable (foreign engine datum, reloc-masked DATA()).
DATA(0x001f0270)
extern void* g_severusWorkerHostVtbl; // 0x5f0270

// The engine Rez heap free (_RezFree 0x1b9b82, cdecl C) used for the two owned
// buffers (and, via CWwdSpatialMgr::operator delete, the worker).
extern "C" void RezFree(void* p);

// ===========================================================================
// 0x163af0 - ~CSeverusWorkerHost: stamp own vtable; if the worker is live run its
// PreDestroy (0x1688b0) and then delete it (its scalar dtor 0x1682f0 + the +0x70
// base-vtable restamp fold into the inline ~CSeverusWorker); free the two owned
// buffers; then the +0x9c SeverusObArray member and ~CSeverusBase fold in.
// ===========================================================================
// @early-stop
// /GX multi-member EH funclet/state-machine wall (~82%; same plateau class as
// CSeverusEntryList::~ 0x1557a0 and CWwdGrid::~ 0x1682a0). Logic is complete and
// the callee identities are confirmed from the delinked relocs: the +0xb0 worker
// is a CWwdSpatialMgr (PreDestroy = PruneCount 0x1688b0, DtorImpl = FreeGrids
// 0x1682f0, freed via _RezFree); the +0x9c member is a CByteArray (~ 0x1b561c);
// the two owned buffers are RezFree'd. The residual is structural: cl emits the
// member unwind funclets (~CSeverusBase / the array dtor / __CxxFrameHandler)
// INTO this COMDAT and reserves a 2-slot trylevel frame (sub esp,8 + [esp+0x1c]/
// [esp+0x20]) whose state-write ordering and own-vptr-stamp placement the delinked
// target carries differently - not steerable from C.
RVA(0x00163af0, 0xcd)
CSeverusWorkerHost::~CSeverusWorkerHost() {
    m_vptr = &g_severusWorkerHostVtbl;
    if (m_b0 != 0) {
        m_b0->PreDestroy();
    }
    if (m_b0 != 0) {
        delete m_b0;
    }
    if (m_20 != 0) {
        RezFree(m_20);
        m_20 = 0;
    }
    if (m_24 != 0) {
        RezFree(m_24);
        m_24 = 0;
    }
    // m_9c.~SeverusObArray() (0x1b561c) + ~CSeverusBase() (field resets + grand-base
    // vtable restore) fold here under the /GX frame.
}
