// CDDrawWorkerHost.h - a larger DDrawMgr DDraw-worker host (ctor 0x1615a0, dtor
// 0x163af0; own primary vtable g_ddrawWorkerHostVtbl @0x5f0270). Like
// CDDrawWorker it derives from the CLoadable grand-base (m_04/m_08/m_0c
// reset on teardown, then the grand-base dtor vtable g_wapObjectDtorVtbl
// @0x5e8cb4 restored). It owns two raw buffers (+0x20/+0x24, RezFree'd), a
// CByteArray (+0x9c, ~ via 0x1b561c, modeled as CWorkerObArray), and a
// CWwdSpatialMgr worker subobject (+0xb0). Only the offsets + emitted bytes are
// load-bearing; names are placeholders.
#ifndef GRUNTZ_CDDRAWWORKERHOST_H
#define GRUNTZ_CDDRAWWORKERHOST_H

#include <Ints.h>
#include <Gruntz/CDDrawWorker.h> // CWorkerObArray, CLoadable, g_wapObjectDtorVtbl

// The worker subobject deleted at +0xb0. Its scalar dtor body (0x1682f0) runs,
// then the host restamps the worker's +0x70 base vtable, then operator delete
// frees it. PreDestroy (0x1688b0) is invoked first, as a separate guarded call.
struct CDDrawWorkerChild {
    void* m_vptr;              // +0x00
    char m_pad04[0x70 - 0x04]; // +0x04..+0x6f
    void* m_70;                // +0x70  base/secondary vtable restamped on teardown
    void PreDestroy();         // 0x1688b0  (reloc-masked external)
    void DtorImpl();           // 0x1682f0  (reloc-masked external scalar-dtor body)
    ~CDDrawWorkerChild();      // inline: DtorImpl() then restamp m_70
};

inline CDDrawWorkerChild::~CDDrawWorkerChild() {
    DtorImpl();
    m_70 = &g_wapObjectDtorVtbl;
}

class CDDrawWorkerHost : public CLoadable {
public:
    CDDrawWorkerHost(i32 a1, i32 a2, i32 a3); // 0x1615a0
    ~CDDrawWorkerHost();

    char m_pad10[0x18 - 0x10]; // +0x10..+0x17
    float m_18;                // +0x18  (=1.0f)
    float m_1c;                // +0x1c  (=1.0f)
    char* m_20;                // +0x20  owned buffer (RezFree'd)
    char* m_24;                // +0x24  owned buffer (RezFree'd)
    char m_pad28[0x50 - 0x28]; // +0x28..+0x4f
    i32 m_50;                  // +0x50  (=-1)
    char m_pad54[0x9c - 0x54]; // +0x54..+0x9b
    CWorkerObArray m_9c;       // +0x9c  owned-pointer array (ctor 0x1b55e9 / ~ 0x1b561c)
    CDDrawWorkerChild* m_b0;   // +0xb0  worker subobject
    char m_padB4[0xf4 - 0xb4]; // +0xb4..+0xf3
    i32 m_f4[0x19];            // +0xf4..+0x157  (25 dwords; memset 0 then m_f4[0]=100)
};

#endif // GRUNTZ_CDDRAWWORKERHOST_H
