#ifndef GRUNTZ_DDRAWMGR_DDRAWWORKERMAPSMALL_H
#define GRUNTZ_DDRAWMGR_DDRAWWORKERMAPSMALL_H

// DDrawWorkerMapSmall.h - CDDrawWorkerMapSmall, the 3-map keyed worker factory of
// the DDraw surface-manager family (13-slot vtable ??_7CDDrawWorkerMapSmall
// @0x1efcc8). Hoisted from DDrawWorkerMapSmall.cpp (wave4-L): the
// IsReady/Slot06/GetStateId/dtor quartet lives in the G obj (DDrawSubMgr.cpp),
// the factory/teardown meat in the T obj (DDrawSurfacePair.cpp).
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + emitted code
// bytes are load-bearing (campaign doctrine).

#include <Ints.h>
#include <rva.h>
#include <Wap32/Object.h>
#include <Gruntz/StateId.h> // StateId (GetStateId return type)
#include <Gruntz/MapStringToOb.h>

// The worker virtual interface. Slots laid out so the dispatched methods land at
// the byte offsets the target uses: +0x04 scalar-deleting dtor, +0x28/+0x2c the
// two factory siblings. Declarations only - never defined, so no ??_7 emitted.
class CDDrawMapWorker {
public:
    virtual void GetRuntimeClass();              // [0] 0x1bef01
    virtual ~CDDrawMapWorker();                  // slot 1 (deleting dtor -> cl-emitted ??_G)
    virtual void Serialize();                    // [2] 0x0028ec
    virtual void AssertValid();                  // [3] 0x00106e
    virtual void Dump();                         // [4] 0x004034
    virtual void Slot05_165d90();                // [5] 0x165d90
    virtual void IsValidImage();                 // [6] 0x001c08
    virtual void FreeBuf_168fb0();               // [7] 0x168fb0 (= CAniRecord::FreeBuf_168fb0)
    virtual void Slot08_165da0();                // [8] 0x165da0
    virtual void Slot09_168f20();                // [9] 0x168f20
    virtual i32 Vfunc28(i32 a1, i32 a3);         // [10] 0x168ee0
    virtual i32 Vfunc2C(i32 a1, i32 a3);         // [11] 0x168ea0
    virtual i32 Vfunc30(i32 a1, i32 a2, i32 a3); // [12] +0x30
};
SIZE_UNKNOWN(CDDrawMapWorker);
RELOC_VTBL(
    CDDrawMapWorker,
    0x001f02d8
); // shares CAniRecordBase2 vtable, COMDAT-folded (slot-fn RVAs match its vtable)

// The 0x14-byte worker layout. Only the seeded offsets are load-bearing.
// Real polymorphic: `new CDDrawMapWorkerObj` makes cl auto-emit ??_7CDDrawMapWorkerObj
// (masks the retail vtable 0x5f02d8 = CAniRecordBase2) and stamp the vptr in the ctor
// (ALL-VTABLES mandate).
struct CDDrawMapWorkerObj : public CDDrawMapWorker {
    CDDrawMapWorkerObj() {}
    i32 m_04; // +0x04  = parent->m_1c (an internal field of map1)
    i32 m_08; // +0x08  = 0
    i32 m_0c; // +0x0c  = parent->m_0c (the CDDrawSurfaceMgr handle)
    i32 m_10; // +0x10  = 0
}; // 0x14
SIZE(CDDrawMapWorkerObj, 0x14);

// The surface/resource arg passed to the two elaborate factories (0x1658c0/0x165a90):
// a __thiscall Lock/Unlock, a format-id probe, a +0x0c key handle, and a name at
// +0x00 (dispatched through the canonical CParseSource casts).
class CDDrawSurfaceSource {
public:
    const char* m_name; // +0x00
    char m_pad04[0x0c - 0x04];
    const char* m_0c; // +0x0c  key handle
};
SIZE_UNKNOWN(CDDrawSurfaceSource);

// ---------------------------------------------------------------------------
// CDDrawWorkerMapSmall - only the load-bearing offsets are modeled: m_0c (parent
// handle), m_1c (a CMapStringToOb-internal field of map1) and the three maps.
// The leaf vtable (13 slots) is declared in slot order so cl lays the emitted
// vtable out byte-for-byte.
// ---------------------------------------------------------------------------
class CDDrawWorkerMapSmall : public CObject {
public:
    i32 m_04, m_08, m_0c; // +0x04..0x0f (merged CDDrawWorkerMapBase)
public:
    virtual i32 IsReady();        // [5]  0x156cd0 (G obj)
    virtual i32 Slot06_156db0();  // [6]  0x156db0 (G obj)
    virtual void DestroyAll();    // [7]  0x165810 (T obj)
    virtual void Slot08_156cf0(); // [8]  0x156cf0 (shared, declared-only)
    virtual void* Factory_1658c0(CDDrawSurfaceSource* a1, const char* key, i32 a3); // [9] 0x1658c0
    virtual void* CreateWorker28(i32 a1, const char* key, i32 a3);                  // [10] 0x165990
    virtual void* CreateWorker2C(i32 a1, const char* key, i32 a3);                  // [11] 0x165a10
    virtual void* Factory_165a90(CDDrawSurfaceSource* a1, i32 a2, i32 a3);          // [12] 0x165a90
    virtual ~CDDrawWorkerMapSmall() OVERRIDE; // overrides slot [1]; 0x156d20 (G obj)

    // GetStateId (0x157600) is NOT a vtable slot - a plain method (G obj).
    StateId GetStateId(); // 0x157600

    CMapStringToOb m_map1; // +0x10  worker-by-key map 1 (0x10..0x2b)
    CMapStringToOb m_map2; // +0x2c  worker-by-key map 2 (0x2c..0x47)
    CMapStringToOb m_map3; // +0x48  worker-by-key map 3 (0x48..0x63)
    i32 m_64;              // +0x64  entry counter cleared by the teardown

    // Non-vtable teardown/remove helpers (T obj).
    void ResetSlots();                // 0x165b90
    i32 RemoveByValue(CObject* obj);  // 0x165c40
    i32 RemoveByKey(const char* key); // 0x165d30
};
SIZE_UNKNOWN(CDDrawWorkerMapSmall);
VTBL(CDDrawWorkerMapSmall, 0x001efcc8); // ??_7CDDrawWorkerMapSmall @0x5efcc8

#endif // GRUNTZ_DDRAWMGR_DDRAWWORKERMAPSMALL_H
