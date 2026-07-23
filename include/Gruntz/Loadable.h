#ifndef GRUNTZ_CLOADABLE_H
#define GRUNTZ_CLOADABLE_H

#include <rva.h>
#include <Ints.h>
#include <Wap32/WapObj.h> // CWapObj : CObject - slots 0..6 (IsLoaded/IsReady)

enum LoadableClassId {
    CLASSID_NONE = 0,          // base default @0x154a00 (xor eax,eax; ret)
    CLASSID_SUBWORKER = 1,     // CDrawSubWorker::GetClassId @0x158f80
    CLASSID_SURFACECHILDA = 2, // CDDrawSurfaceChildA::GetClassId @0x159180
    CLASSID_SURFACEPAIR = 3,   // CDDrawSurfacePair::GetClassId @0x1590c0
    CLASSID_WWDOBJA = 5,       // CWwdGameObjectA::GetClassId @0x15b760
    CLASSID_WWDOBJC = 6,       // CWwdGameObjectC::GetClassId @0x15c020
    CLASSID_ANIMWORKER = 9,    // AnimWorkerObj::GetClassId @0x151d70
    CLASSID_WWDOBJF = 0x16,    // CWwdGameObjectF::GetClassId @0x15ba60
    CLASSID_WWDOBJB = 0x1b,    // CWwdGameObject::GetClassId @0x15bce0 (the B kind)
    CLASSID_WORKERNODE = 8,    // CDDrawWorkerBase::GetClassId @0x157210 (mov eax,8)
    CLASSID_IMAGE = 10,        // CImage::GetClassId @0xd5de0 (mov eax,0xa)
    CLASSID_WORKER = 14,       // CDDrawWorker::GetClassId @0x155770 (mov eax,0xe)
    // The DDraw sub-manager kinds (<Gruntz/StateId.h> still spells the not-yet-
    // rebased holdouts' ids in the SAME one retail id space - slot 8 is
    // GetClassId family-wide).
    CLASSID_SUBMGRPAGES = 0xf, // CDDrawSubMgrPages::GetClassId @0x1574a0 (ex STATE_SUBMGRPAGES)
    CLASSID_CHILDGROUP = 0x10, // CDDrawChildGroup::GetClassId @0x157600 (ex STATE_CHILDGROUP)
    CLASSID_WORKERLIST = 0x11, // CDDrawWorkerList::GetClassId @0x156f20 (ex STATE_WORKERLIST)
    CLASSID_WORKERREGISTRY =
        0x12, // CDDrawWorkerRegistry::GetClassId @0x156de0 (ex STATE_WORKERREGISTRY)
    CLASSID_WORKERCACHE = 0x13, // CDDrawWorkerCache::GetClassId @0x1576f0 (ex STATE_WORKERCACHE)
    CLASSID_WORKERMAPSMALL =
        0x14, // CDDrawWorkerMapSmall::GetClassId @0x156cf0 (ex STATE_WORKERMAPSMALL)
    CLASSID_GAMELEVEL = 0x19,  // CGameLevel::GetClassId @0x1611b0 (mov eax,0x19)
    CLASSID_WORKERHOST = 0x1a, // CDDrawWorkerHost::GetClassId @0x163ab0 (mov eax,0x1a)
    // Id 5 = CWwdGameObjectA's OWN class id (the CreateSprite kind): its slot 8
    // @0x15b760 is `mov eax,5; ret` (byte-proven). The serialize Read probes
    // (CSpotLight::SerializeMove focus resolve, CPlay::SerializeMove cell-entry
    // resolve, CStatusBarMgr::Deserialize) keep a deserialized id->object only
    // when GetClassId()==5 - i.e. only A-kind sprites.
    CLASSID_SERIALREF = 5,
    // The wide game-object kinds (CWwdGameObject* family; slot-8 bodies are
    // `mov eax,<id>; ret` at the cited RVAs).
    CLASSID_WWDOBJ_C = 6,    // CWwdGameObjectC::GetClassId @0x15c020
    CLASSID_WWDOBJ_F = 0x16, // CWwdGameObjectF::GetClassId @0x15ba60
    CLASSID_WWDOBJ_B = 0x1b, // CWwdGameObject::GetClassId @0x15bce0
    // 0x1c: NO slot-8 body anywhere returns 0x1c (exhaustive .text scan) - the
    // old "CLASSID_WWDOBJ_A = 0x1c" claim was a doc bug (A's id IS 5, above).
    // The one retail compare (WriteSnapshot @0x151c4e, `cmp eax,0x1c`) can never
    // be true for family receivers - a shipped dead branch; keep the immediate.
    CLASSID_SNAPSHOT_STALE = 0x1c,
};

class CLoadable : public CWapObj {
public:
    // slot 5 IsLoaded: CLoadable's own default @0x155700 (distinct from CWapObj's
    // 0xd5dc0). Declared-only (reloc-masked). Every concrete leaf overrides it.
    virtual i32 IsLoaded() OVERRIDE; // [5] @+0x14  0x155700
    // slot 6 IsReady INHERITED from CWapObj (its `return 1` default @0xd5da0, now a
    // real body); not redeclared here (the redeclaration was a declared-only phantom).
    virtual void Unload();    // [7] @+0x1c  0x155740 (reset/unload hook; bare-ret no-op -
                              // the bare c3 PROVES the slot is void: cl 5.0 hard-errors
                              // (C2561) on a return-less non-void function)
    virtual i32 GetClassId(); // [8] @+0x20  0x154a00 -> CLASSID_NONE

    // +0x04  per-child id/index (Init hands 0; the page pairs 1/2; the planes their
    // index) doubling as the liveness latch: teardown resets -1, every family
    // IsLoaded gates on `m_id != -1`.
    i32 m_id;
    i32 m_flags; // +0x08  (reset to 0; the wide-object collision/state flag word)
    // +0x0c  the owner-context handle (the CDDrawSurfaceMgr across the draw family
    // - read via OwnerMgr(); plane/leaf embedders park other context words here,
    // which is why the slot stays a generic i32). Reset to 0 on teardown.
    i32 m_ownerCtx;

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
    class CDDrawSurfaceMgr* OwnerMgr() {
        return reinterpret_cast<class CDDrawSurfaceMgr*>(m_ownerCtx);
    }
    // Field-reset base-subobject dtor: resets the three header fields; the grand-
    // base 0x5e8cb4 re-stamp folds in automatically via ~CWapObj -> ~CObject
    // (no manual `*(void**)this = &g_*Vtbl`).
    virtual ~CLoadable() OVERRIDE {
        m_id = -1;
        m_flags = 0;
        m_ownerCtx = 0;
    }
};
SIZE(0x10);

#endif // GRUNTZ_CLOADABLE_H
