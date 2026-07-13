// DDrawWorkerHost.h - a larger DDrawMgr DDraw-worker host (ctor 0x1615a0, dtor
// 0x163af0; own primary vtable g_ddrawWorkerHostVtbl @0x5f0270). Like
// CDDrawWorker it derives from the CLoadable grand-base (m_04/m_08/m_0c
// reset on teardown, then the grand-base dtor vtable g_wapObjectDtorVtbl
// @0x5e8cb4 restored). It owns two raw buffers (+0x20/+0x24, RezFree'd), a
// CObArray (+0x9c, ~ via 0x1b561c - 0x1b561c is ~CObArray, NOT ~CByteArray: that was a
// FID AMBIG mislabel; the ctor 0x1b55e9 stamps the vtable CRuntimeClass-named CObArray), and a
// CWwdSpatialMgr worker subobject (+0xb0). Only the offsets + emitted bytes are
// load-bearing; names are placeholders.
#ifndef GRUNTZ_CDDRAWWORKERHOST_H
#define GRUNTZ_CDDRAWWORKERHOST_H

#include <Ints.h>
#include <Wap32/Object.h>
#include <DDrawMgr/DDrawWorker.h> // CLoadable (m_obArray is the real MFC ::CObArray)

// The spatial-grid worker subobject at +0xb0 is a CWwdSpatialMgr (its real class,
// defined in src/Gruntz/WwdSpatialMgr.cpp - the same object CImageSet3 owns at +0xb0).
// Its FreeGrids body (0x1682f0) runs on teardown, then the host restamps the worker's
// +0x70 base vtable, then operator delete frees it. PruneCount (0x1688b0) is invoked
// first, as a separate guarded call. (matcher-3 owns the CWwd* family; this is a
// minimal member-view of the same class - the two reloc-masked callees resolve to
// WwdSpatialMgr.cpp's real definitions.)
struct CWwdSpatialMgr {
    char _vft0[4];             // +0x00 foreign object vptr (reduced view; not owned/dispatched)
    char m_pad04[0x70 - 0x04]; // +0x04..+0x6f
    void* m_baseVtbl;          // +0x70  base/secondary vtable restamped on teardown
    i32 PruneCount();          // 0x1688b0  (reloc-masked external)
    i32 GetSize();             // 0x168430  (reloc-masked external; serialized-size accessor)
    void FreeGrids();          // 0x1682f0  (reloc-masked external scalar-dtor body)
    ~CWwdSpatialMgr();         // inline: FreeGrids() then restamp m_baseVtbl
};

inline CWwdSpatialMgr::~CWwdSpatialMgr() {
    FreeGrids();
    // +0x70 secondary-base vptr restamp dropped (MI; manual stamp removed, % ok)
}

class CDDrawWorkerHost : public CObject {
public:
    CDDrawWorkerHost(i32 owner, i32 field04, i32 field08); // 0x1615a0
    virtual ~CDDrawWorkerHost() OVERRIDE;                  // slot 1 (scalar-deleting dtor)
    // 0x161c50: cache the object named `key` (resolved via the owner context's map)
    // at m_obArray[index], or null on a miss.
    void RegisterNamed(char index, const char* key);

    i32 m_04, m_08, m_0c;            // +0x04..0x0f (merged CLoadable base fields)
    char m_pad10[0x18 - 0x10];       // +0x10..+0x17
    float m_18;                      // +0x18  (=1.0f)
    float m_1c;                      // +0x1c  (=1.0f)
    char* m_buffer0;                 // +0x20  owned buffer (RezFree'd)
    char* m_buffer1;                 // +0x24  owned buffer (RezFree'd)
    char m_pad28[0x50 - 0x28];       // +0x28..+0x4f
    i32 m_50;                        // +0x50  (=-1)
    char m_pad54[0x9c - 0x54];       // +0x54..+0x9b
    ::CObArray m_obArray;            // +0x9c  owned-pointer array (ctor 0x1b55e9 / ~ 0x1b561c)
    CWwdSpatialMgr* m_spatialWorker; // +0xb0  spatial-grid worker subobject
    char m_padB4[0xf4 - 0xb4];       // +0xb4..+0xf3
    i32 m_pool[0x19];                // +0xf4..+0x157  (25 dwords; memset 0 then m_pool[0]=100)
    virtual void VtSlotFill0();      // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill1();      // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill2();      // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill3();      // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill4();      // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill5();      // vtable-slot filler (real slot; declared-only)
    virtual void VtSlotFill6();      // vtable-slot filler (real slot; declared-only)
};

#endif // GRUNTZ_CDDRAWWORKERHOST_H
