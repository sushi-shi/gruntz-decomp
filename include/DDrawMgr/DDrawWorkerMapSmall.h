#ifndef GRUNTZ_DDRAWMGR_DDRAWWORKERMAPSMALL_H
#define GRUNTZ_DDRAWMGR_DDRAWWORKERMAPSMALL_H

#include <Ints.h>
#include <rva.h>
#include <Wap32/Object.h>
#include <Gruntz/StateId.h> // StateId (GetStateId return type)
#include <Gruntz/MapStringToOb.h>
#include <DDrawMgr/AniRecordBase2.h>

#include <Gruntz/ParseSource.h> // the real parse-source record

class CDDrawWorkerMapSmall : public CObject {
public:
    i32 m_04, m_08, m_0c; // +0x04..0x0f (merged CDDrawWorkerMapBase)
public:
    virtual i32 IsLoaded();      // [5]  0x156cd0 (G obj; the worker-gate - CWapObj-scheme slot 5)
    virtual i32 IsReady();       // [6]  0x156db0 (G obj; own return-1 copy of the scheme default)
    virtual void DestroyAll();   // [7]  0x165810 (T obj)
    // [8] the class's REAL GetStateId (`mov eax,0x14; ret` @0x156cf0 - a Ghidra
    // recovery gap, declared-only). The old "GetStateId 0x157600" plain-method
    // claim was a misbinding: 0x157600 is CDDrawChildGroup's slot 8 (id 0x10).
    virtual StateId GetStateId(); // [8]  0x156cf0 (STATE_WORKERMAPSMALL = 0x14)
    virtual void* Factory_1658c0(CParseSource* a1, const char* key, i32 a3); // [9] 0x1658c0
    virtual void* CreateWorker28(i32 a1, const char* key, i32 a3);                  // [10] 0x165990
    virtual void* CreateWorker2C(i32 a1, const char* key, i32 a3);                  // [11] 0x165a10
    virtual void* Factory_165a90(CParseSource* a1, i32 a2, i32 a3);          // [12] 0x165a90
    virtual ~CDDrawWorkerMapSmall() OVERRIDE; // overrides slot [1]; 0x156d20 (G obj)

    CMapStringToOb m_map1; // +0x10  worker-by-key map 1 (0x10..0x2b)
    CMapStringToOb m_map2; // +0x2c  worker-by-key map 2 (0x2c..0x47)
    CMapStringToOb m_map3; // +0x48  worker-by-key map 3 (0x48..0x63)
    // +0x64  the cached/current worker out of m_map1. NOT the "entry counter" this
    // was typed as (i32 m_64): RemoveByValue @0x165c40 compares it against the
    // CObject* worker being removed and nulls it on a hit, and the teardown nulls it
    // too - a counter is neither compared to a worker pointer nor dereferenced.
    // The plane host's ResolveColorKey reads THIS slot's +0x10 palette -> +0x0c RGB888
    // triples, so the pointee is a palette-bearing worker; its concrete class is the
    // one link of that chain still unproven (@identity-TODO, see <Wwd/WwdFile.h>), so
    // the slot keeps the map's element type and that one reader casts.
    CObject* m_cachedWorker;

    // Non-vtable teardown/remove helpers (T obj).
    void ResetSlots();                // 0x165b90
    i32 RemoveByValue(CObject* obj);  // 0x165c40
    i32 RemoveByKey(const char* key); // 0x165d30
};
SIZE_UNKNOWN();
VTBL(CDDrawWorkerMapSmall, 0x001efcc8); // ??_7CDDrawWorkerMapSmall @0x5efcc8

#endif // GRUNTZ_DDRAWMGR_DDRAWWORKERMAPSMALL_H
