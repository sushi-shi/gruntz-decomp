// CSeverusWorkerHost.h - a larger DDrawMgr "severus" host (ctor 0x1615a0, dtor
// 0x163af0; own primary vtable g_severusWorkerHostVtbl @0x5f0270). Like
// CSeverusEntryList it derives from the CSeverusBase grand-base (m_04/m_08/m_0c
// reset on teardown, then the grand-base dtor vtable g_severusBaseDtorVtbl
// @0x5e8cb4 restored). It owns two raw buffers (+0x20/+0x24, RezFree'd), a
// CByteArray (+0x9c, ~ via 0x1b561c, modeled as SeverusObArray), and a
// CWwdSpatialMgr worker subobject (+0xb0). Only the offsets + emitted bytes are
// load-bearing; names are placeholders.
#ifndef GRUNTZ_CSEVERUSWORKERHOST_H
#define GRUNTZ_CSEVERUSWORKERHOST_H

#include <Ints.h>
#include <Gruntz/CSeverusEntryList.h> // SeverusObArray, CSeverusBase, g_severusBaseDtorVtbl

// The worker subobject deleted at +0xb0. Its scalar dtor body (0x1682f0) runs,
// then the host restamps the worker's +0x70 base vtable, then operator delete
// frees it. PreDestroy (0x1688b0) is invoked first, as a separate guarded call.
struct CSeverusWorker {
    void* m_vptr;              // +0x00
    char m_pad04[0x70 - 0x04]; // +0x04..+0x6f
    void* m_70;                // +0x70  base/secondary vtable restamped on teardown
    void PreDestroy();         // 0x1688b0  (reloc-masked external)
    void DtorImpl();           // 0x1682f0  (reloc-masked external scalar-dtor body)
    ~CSeverusWorker();         // inline: DtorImpl() then restamp m_70
};

inline CSeverusWorker::~CSeverusWorker() {
    DtorImpl();
    m_70 = &g_severusBaseDtorVtbl;
}

class CSeverusWorkerHost : public CSeverusBase {
public:
    ~CSeverusWorkerHost();

    char m_pad10[0x20 - 0x10];  // +0x10..+0x1f
    char* m_20;                 // +0x20  owned buffer (RezFree'd)
    char* m_24;                 // +0x24  owned buffer (RezFree'd)
    char m_pad28[0x9c - 0x28];  // +0x28..+0x9b
    SeverusObArray m_9c;        // +0x9c  owned-pointer array (~ via 0x1b561c)
    CSeverusWorker* m_b0;       // +0xb0  worker subobject
    char m_padB4[0x158 - 0xb4]; // +0xb4..+0x157  (ctor footprint; unread here)
};

#endif // GRUNTZ_CSEVERUSWORKERHOST_H
