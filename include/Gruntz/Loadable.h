#ifndef GRUNTZ_CLOADABLE_H
#define GRUNTZ_CLOADABLE_H

#include <rva.h>
#include <Ints.h>
#include <Wap32/WapObj.h> // CWapObj : CObject - slots 0..6 (IsLoaded/IsReady)

enum LoadableClassId {
    CLASSID_NONE = 0,         // base default @0x154a00 (xor eax,eax; ret)
    CLASSID_WORKERNODE = 8,   // CDDrawWorkerBase::GetClassId @0x157210 (mov eax,8)
    CLASSID_IMAGE = 10,       // CImage::GetClassId @0xd5de0 (mov eax,0xa)
    CLASSID_WORKER = 14,      // CDDrawWorker::GetClassId @0x155770 (mov eax,0xe)
    CLASSID_GAMELEVEL = 0x19, // CGameLevel::GetClassId @0x1611b0 (mov eax,0x19)
    // Id 5 = the serialize-map REFERENT kind: the serialize Read probes
    // (CSpotLight::SerializeMove focus resolve, CPlay::SerializeMove cell-entry
    // resolve) keep a deserialized id->object only when GetClassId()==5. Named by
    // role - the owning CWwdGameObject* kind is not yet recovered, but the value +
    // role are proven by both probe sites (matching-neutral: same immediate).
    CLASSID_SERIALREF = 5,
    // The wide game-object kinds (CWwdGameObject* family; slot-8 bodies are
    // `mov eax,<id>; ret` at the cited RVAs).
    CLASSID_WWDOBJ_C = 6,    // CWwdGameObjectC::GetClassId @0x15c020
    CLASSID_WWDOBJ_F = 0x16, // CWwdGameObjectF::GetClassId @0x15ba60
    CLASSID_WWDOBJ_B = 0x1b, // CWwdGameObject::GetClassId @0x15bce0
    CLASSID_WWDOBJ_A = 0x1c, // CWwdGameObjectA::GetClassId @0x15b760 (the CreateSprite kind)
};

SIZE(CLoadable, 0x10);
VTBL(CLoadable, 0x001efc30); // ??_7CLoadable (the shared 9-slot loadable-base vtable)
class CLoadable : public CWapObj {
public:
    // slot 5 IsLoaded: CLoadable's own default @0x155700 (distinct from CWapObj's
    // 0xd5dc0). Declared-only (reloc-masked). Every concrete leaf overrides it.
    virtual i32 IsLoaded() OVERRIDE; // [5] @+0x14  0x155700
    // slot 6 IsReady INHERITED from CWapObj (its `return 1` default @0xd5da0, now a
    // real body); not redeclared here (the redeclaration was a declared-only phantom).
    virtual i32 Unload(); // [7] @+0x1c  0x155740 (reset/unload hook; bare-ret i32 no-op)
    virtual i32 GetClassId();       // [8] @+0x20  0x154a00 -> CLASSID_NONE

    i32 m_04; // +0x04  (reset to -1 on teardown)
    i32 m_flags; // +0x08  (reset to 0; the wide-object collision/state flag word)
    i32 m_0c; // +0x0c  (reset to 0)

    CLoadable() {}
    // Arg-taking base ctor - OUT-OF-LINE at 0x156cb0 (DDrawSubMgr.cpp; the ex
    // "??0CDDrawSubMgr"). `owner` is the owning/parent context stored at m_0c
    // (IsLoaded checks it nonzero); field04/field08 are the two managed header
    // words. Retail call sites: CDDrawSurfaceMgr::Init 0x155900, CDDrawSubMgrPages::
    // CreateChildren 0x1588f0, CDDrawChildGroup::CreateObject 0x1598d0, CDDrawWorkerHost::
    // ReadPlaneObjects 0x162af0. NB retail ALSO shows the store triple fused into
    // ctors like CResolveNode(i32,i32,i32) @0x15b2c0 - those leaves spell the three
    // stores in their own ctor body (over the default base ctor), reproducing the
    // fused shape without a second definition.
    CLoadable(i32 owner, i32 field04, i32 field08);
    // The +0x0c owner context IS the CDDrawSurfaceMgr across the whole draw family
    // (surface pairs/children, workers, resolve nodes, cue leaves - every read site
    // agrees); plane/leaf embedders park OTHER context words in the same slot and
    // never call this. One value-read of the generic i32 handle, typed once.
    class CDDrawSurfaceMgr* OwnerMgr() { return reinterpret_cast<class CDDrawSurfaceMgr*>(m_0c); }
    // Field-reset base-subobject dtor: resets the three header fields; the grand-
    // base 0x5e8cb4 re-stamp folds in automatically via ~CWapObj -> ~CObject
    // (no manual `*(void**)this = &g_*Vtbl`).
    virtual ~CLoadable() OVERRIDE {
        m_04 = -1;
        m_flags = 0;
        m_0c = 0;
    }
};

#endif // GRUNTZ_CLOADABLE_H
