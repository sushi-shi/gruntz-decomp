#ifndef GRUNTZ_DDRAWMGR_DDRAWWORKERCACHE_H
#define GRUNTZ_DDRAWMGR_DDRAWWORKERCACHE_H

// DDrawWorkerCache.h - CDDrawWorkerCache, the string-keyed worker cache child of
// the DDraw surface-manager family (vtable ??_7CDDrawWorkerCache @0x1efd00).
// Hoisted from DDrawWorkerCache.cpp for the wave4-L original-TU partition: the
// IsReady/GetStateId/dtor quartet lives in the G (submgr-family) obj and the
// CreateWorker factory in the T (family-meat) obj.
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + emitted code
// bytes are load-bearing (campaign doctrine).

#include <Ints.h>
#include <rva.h>
#include <Wap32/Object.h>
#include <Gruntz/StateId.h> // StateId (GetStateId return type)
#include <Gruntz/MapStringToOb.h>

// Real polymorphic (own 10-slot vtable ??_7CDDrawWorkerCache @0x5efd00). Slots
// 0/2/3/4 are the shared CObject thunks, slot 1 the ??_G scalar-deleting dtor
// (0x157700), slots 5/6/7 leaf virtuals, slot 8 = GetStateId (0x1576f0) and
// slot 9 = CreateWorker (0x1652c0). cl auto-emits the vtable.
class CDDrawWorkerCache : public CObject {
public:
    i32 m_04, m_08, m_0c;                  // +0x04..0x0f (merged CDDrawWorkerCacheBase)
    virtual ~CDDrawWorkerCache() OVERRIDE; // [1] dtor 0x157720 (??_G 0x157700 pinned at def)
    // [5] 0x1576d0: ready iff +0x0c is bound and the +0x04 status latch isn't -1.
    RVA(0x001576d0, 0x16)
    virtual i32 IsReady() {
        if (m_0c == 0) {
            goto fail;
        }
        if (m_04 != -1) {
            return 1;
        }

    fail:
        return 0;
    }
    virtual void GetStateId_157790(); // [6] 0x157790 (= CDDrawSubMgr::GetStateId, declared-only)
    virtual void
    DestroyAll(); // [7] 0x165210 (= CDDrawWorkerRegistry::DestroyAll, defined in Registry TU)
    RVA(0x001576f0, 0x6)
    virtual StateId GetStateId() {
        return STATE_WORKERCACHE; // 0x13
    }
    virtual void* CreateWorker(i32 a1, const char* key, i32 a3); // [9] 0x1652c0

    // 0x9cab0 (body in StreamRecordLoaders.cpp - spatially adjacent at retail): the
    // out-param wrapper over m_10.Lookup (CMapStringToOb::Lookup @0x1b8008); returns
    // the found object (0 = absent). This is the logic-type registry's existence
    // probe - the tile-logic leaf ctors call it via thunk 0x1703 before dispatching
    // slot-9 CreateWorker to register a missing type (was the C9cab0::LookupPtr
    // placeholder / the CLogicTypeReg::Find view).
    i32 Find(const char* key); // 0x9cab0

    CMapStringToOb m_10; // +0x10  map (internal field at +0x1c seeds worker->m_04)
};
SIZE_UNKNOWN(CDDrawWorkerCache);
// ??_7CDDrawWorkerCache (10 slots). cl auto-emits it from the real-polymorphic
// class; retail datum is reloc-masked -> matching-neutral catalog tracking.
VTBL(CDDrawWorkerCache, 0x001efd00);

#endif // GRUNTZ_DDRAWMGR_DDRAWWORKERCACHE_H
