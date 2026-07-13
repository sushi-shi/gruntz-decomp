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
    // `new CPlane` (CGameLevelPlanes::ReadPlane/ReadObjectPlane) needs an accessible
    // operator new; MFC CObject's PASCAL one is not usable under MSVC5, so forward to
    // global new (byte-identical: the same `push 0x158; call ??2@YAPAXI@Z`).
    void* operator new(size_t n) {
        return ::operator new(n);
    }
    // 0x161c50: cache the object named `key` (resolved via the owner context's map)
    // at m_obArray[index], or null on a miss.
    void RegisterNamed(char index, const char* key);

    i32 m_04;                  // +0x04  (merged CLoadable base field; ctor arg2)
    u32 m_flags;               // +0x08  plane flags (ctor arg3): bit0 = MAIN/origin-fixed plane,
                               //        bit1 hidden, bit2/3 = wrap X/Y (CPlane/CPlaneRender views)
    i32 m_0c;                  // +0x0c  owner context (ctor arg1; merged CLoadable base field)
    char m_pad10[0x18 - 0x10]; // +0x10..+0x17 (scaledX/scaledY in the plane views)
    float m_18;                // +0x18  X parallax scale (=1.0f)
    float m_1c;                // +0x1c  Y parallax scale (=1.0f)
    char* m_buffer0;           // +0x20  owned buffer (the tile-handle grid; RezFree'd)
    char* m_buffer1;           // +0x24  owned buffer (the per-column offsets; RezFree'd)
    char m_pad28[0x50 - 0x28]; // +0x28..+0x4f
    i32 m_50;                  // +0x50  (=-1)
    char m_pad54[0x9c - 0x54]; // +0x54..+0x9b
    ::CObArray m_obArray;      // +0x9c  owned-pointer array (ctor 0x1b55e9 / ~ 0x1b561c)
    CWwdSpatialMgr* m_spatialWorker; // +0xb0  spatial-grid worker subobject
    char m_padB4[0xf4 - 0xb4];       // +0xb4..+0xf3
    i32 m_pool[0x19];                // +0xf4..+0x157  (25 dwords; memset 0 then m_pool[0]=100)

    // --- own vtable slots 5..11 (retail ??_7 @0x1f0270 is 12 slots; per-slot RTTI
    // map from `vtable_hierarchy --class CDDrawWorkerHost`). Slots whose bodies are
    // still homed on sibling VIEWS of this same plane object are declared-only (the
    // emitted vtable references them reloc-masked; the vtable datum is not diffed);
    // the 5-view unification (CPlane/CPlaneRender/CLevelPlane/the LevelPlane.cpp
    // "CImageSet3" grid-owner pocket) is deferred structural work. ------------------
    virtual i32 IsLoaded();        // slot 5  (+0x14) 0x163a90 "plane loaded?" gate -
                                   //         ValidateTiles dispatches it (ex CPlaneRenderPoly)
    virtual void VtSlot6_1c08();   // slot 6  (+0x18) 0x001c08 shared CWapObj-family default
    virtual void Cleanup_161bf0(); // slot 7  (+0x1c) 0x161bf0 grid/buffer teardown (body on
                                   //         the LevelPlane.cpp grid-owner view; @identity-TODO)
    virtual void VtSlot8_163ab0(); // slot 8  (+0x20) 0x163ab0 (role unrecovered)
    // slot 9 (+0x24) 0x1619f0 - the 8-arg object-plane/geometry reader
    // CGameLevelPlanes::ReadObjectPlane dispatches; the BODY is currently homed as
    // CLevelPlane::InitGeometry_1619f0 (the GameLevel.h view of this same object).
    virtual i32
    ReadObjects(i32 w, i32 h, i32 tileW, i32 tileH, i32 depthX, i32 depthY, void* bounds, i32 a8);
    // slot 10 (+0x28) 0x161640 - the 3-arg plane-block reader
    // CGameLevelPlanes::ReadPlane dispatches (parses one WwdPlaneHeader, fans out to
    // the tile/imageset/object sub-readers). Stub body in LevelPlane.cpp (ex Gap_161640).
    virtual i32 Read(void* planeData, void* blockBase, void* bounds);
    virtual void VtSlot11_163ac0(); // slot 11 (+0x2c) 0x163ac0 (role unrecovered)
};

// The engine plane object CGameLevelPlanes::ReadPlane/ReadObjectPlane build (one per
// WWD plane, `new` size 0x158 == this class: the +0xf4 pool ends at +0x158) - the
// WwdFile.h "CPlane" view was this class; the alias is kept for its readers.
typedef CDDrawWorkerHost CPlane;

#endif // GRUNTZ_CDDRAWWORKERHOST_H
