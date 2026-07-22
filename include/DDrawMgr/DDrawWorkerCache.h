#ifndef GRUNTZ_DDRAWMGR_DDRAWWORKERCACHE_H
#define GRUNTZ_DDRAWMGR_DDRAWWORKERCACHE_H

#include <Ints.h>
#include <DDrawMgr/AnimWorkerObj.h> // GameObjNotifyFn (the CreateWorker registrant ABI)
#include <rva.h>
#include <Wap32/Object.h>
#include <Gruntz/StateId.h> // StateId (GetStateId return type)
#include <Gruntz/MapStringToOb.h>

class CDDrawWorker;             // CImageSet IS CDDrawWorker (<DDrawMgr/DDrawWorker.h>);
typedef CDDrawWorker CImageSet; // identical repeat of ImageSet.h's typedef - legal, and

class CDDrawWorkerCache : public CObject {
public:
    i32 m_04, m_08, m_0c;                  // +0x04..0x0f (merged CDDrawWorkerCacheBase)
    virtual ~CDDrawWorkerCache() OVERRIDE; // [1] dtor 0x157720 (??_G 0x157700 pinned at def)
    // [5] 0x1576d0 IsLoaded (the CLoadable-scheme slot-5 predicate): loaded iff +0x0c
    // is bound and the +0x04 status latch isn't -1. (Renamed from "IsReady" - slot 6
    // below is the scheme's IsReady.)
    RVA(0x001576d0, 0x16)
    virtual i32 IsLoaded() {
        if (m_0c == 0) {
            goto fail;
        }
        if (m_04 != -1) {
            return 1;
        }

    fail:
        return 0;
    }
    // [6] 0x157790: the class's OWN compiled copy of the CWapObj `return 1` IsReady
    // default (no MSVC5 ICF - same pattern as CDDrawChildGroup::IsReady @0x1576c0).
    // Only reference in the binary: ??_7CDDrawWorkerCache@@6B@+0x18. (Ex the fictional
    // "CDDrawSubMgr::GetStateId returning STATE_SUBMGR=1".)
    RVA(0x00157790, 0x6)
    virtual i32 IsReady() {
        return 1;
    }
    // [7] 0x165210 (body in DDrawSurfacePair.cpp): delete every m_10 map value,
    // RemoveAll. Proven THIS class's slot body by exhaustive binary xref (one call
    // site = this class's dtor, one data ref = this vtable's slot 7); was
    // mis-attributed to CDDrawWorkerRegistry (whose real teardown is MapTeardown).
    virtual void DestroyAll();
    RVA(0x001576f0, 0x6)
    virtual StateId GetStateId() {
        return STATE_WORKERCACHE; // 0x13
    }
    // The registered per-type notify/factory callback: the body hands it straight to
    // AnimWorkerObj::Init as the worker's GameObjNotifyFn (a1 was `i32` - every one
    // of the 82 call sites passed reinterpret_cast<i32>(&Factory)).
    virtual void* CreateWorker(GameObjNotifyFn factory, const char* key, i32 a3); // [9] 0x1652c0

    // 0x9cab0 (body in StreamRecordLoaders.cpp - spatially adjacent at retail): the
    // out-param wrapper over m_10.Lookup (CMapStringToOb::Lookup @0x1b8008); returns
    // the found object (0 = absent). This is the logic-type registry's existence
    // probe - the tile-logic leaf ctors call it via thunk 0x1703 before dispatching
    // slot-9 CreateWorker to register a missing type (was the C9cab0::LookupPtr
    // placeholder / the CLogicTypeReg::Find view).
    i32 Find(const char* key); // 0x9cab0

    // 0x165360 (map scan): return by value the key of the first m_10 entry whose value's
    // +0x10 dword equals target's; empty CString if none. The ONLY callers (CWwdGameObject::
    // Serialize/WriteSnapshot, xref-confirmed) reverse-look-up a WORKER in THIS worker cache,
    // so this reverse-lookup is a CDDrawWorkerCache method (was mis-attributed to the +0x10
    // CDDrawWorkerRegistry sibling, which shares the byte-identical map@+0x10 layout).
    CString FindKeyOfValue(CObject* target); // 0x165360  (the m_10 map's
    // true element base: values are heterogeneous - CImageSet/worker templates AND
    // the objects' own AnimWorkerObj records get reverse-looked-up here)

    CMapStringToOb m_10; // +0x10  map (internal field at +0x1c seeds worker->m_04)
};
SIZE_UNKNOWN();

#endif // GRUNTZ_DDRAWMGR_DDRAWWORKERCACHE_H
