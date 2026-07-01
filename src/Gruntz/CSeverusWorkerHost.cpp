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
// 0x1615a0 - CSeverusWorkerHost(a1,a2,a3): the /GX EH ctor. cl inlines the
// CSeverusBase base ctor (vptr stamp -- reloc-masks the retail intermediate
// g_severusWorkerBaseVtbl 0x5efc30 -- then m_04=a2/m_08=a3/m_0c=a1), constructs the
// +0x9c SeverusObArray member (0x1b55e9; its destructible-member trylevel supplies
// the EH frame), stamps the own vftable (0x5f0270), then arms the scalar fields
// (buffers/worker = 0, m_18/m_1c = 1.0f, m_50 = -1) and zero-fills the +0xf4 pool
// (25 dwords) with m_f4[0] = 100. Byte-exact (100%): the retail intermediate base
// stamp 0x5efc30 is reloc-masked, so the compiler-emitted ??_7CSeverusBase stamp
// matches at the byte level; the CSeverusBase ctor arg-order (m_04=a2/m_08=a3/
// m_0c=a1) + body store order reproduce the schedule exactly.
// ===========================================================================
RVA(0x001615a0, 0x9a)
CSeverusWorkerHost::CSeverusWorkerHost(i32 a1, i32 a2, i32 a3) : CSeverusBase(a2, a3, a1) {
    // m_9c (SeverusObArray) default-constructed here (0x1b55e9).
    m_20 = 0;
    m_24 = 0;
    m_b0 = 0;
    m_18 = 1.0f;
    m_1c = 1.0f;
    m_50 = -1;
    memset(m_f4, 0, sizeof(m_f4));
    m_f4[0] = 100;
}

// ===========================================================================
// 0x163af0 - ~CSeverusWorkerHost: stamp own vtable; if the worker is live run its
// PreDestroy (0x1688b0) and then delete it (its scalar dtor 0x1682f0 + the +0x70
// base-vtable restamp fold into the inline ~CSeverusWorker); free the two owned
// buffers; then the +0x9c SeverusObArray member and ~CSeverusBase fold in.
// ===========================================================================
// @early-stop
// Vtable recovery applied (real polymorphic CSeverusBase): own-vptr stamp is now
// compiler-emitted (82.3%->85.8%). Residual is the multi-member /GX funclet/state
// ordering across the worker delete + two buffer frees + SeverusObArray member +
// ~CSeverusBase fold (grand-base stamp position + EH state writes), same wall class
// as the entry-list dtor. Logic complete.
RVA(0x00163af0, 0xcd)
CSeverusWorkerHost::~CSeverusWorkerHost() {
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

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CSeverusWorkerHost);
