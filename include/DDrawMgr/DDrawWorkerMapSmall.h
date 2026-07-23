#ifndef GRUNTZ_DDRAWMGR_DDRAWWORKERMAPSMALL_H
#define GRUNTZ_DDRAWMGR_DDRAWWORKERMAPSMALL_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/Loadable.h> // CLoadable - the real base (slot scheme 5-8)
#include <Gruntz/MapStringToOb.h>
#include <DDrawMgr/AniRecordBase2.h>

#include <Gruntz/ParseSource.h> // the real parse-source record

// (B)-form re-base 2026-07-22: vtbl 0x5efcc8 slots 5-8 are the CLoadable scheme;
// the +0x04..+0x0c trio is the INHERITED CLoadable header (ex the "merged
// CDDrawWorkerMapBase" flat words).
class CDDrawWorkerMapSmall : public CLoadable {
public:
    virtual i32 IsLoaded() OVERRIDE; // [5]  0x156cd0 (G obj; the worker-gate)
    virtual i32 IsReady() OVERRIDE;  // [6]  0x156db0 (G obj; own return-1 copy)
    virtual void Unload() OVERRIDE;  // [7]  0x165810 (T obj; ex "DestroyAll")
    // [8] the REAL GetClassId (.mov eax,0x14; ret. @0x156cf0 - a Ghidra recovery
    // gap; body defined at the dtor pocket). The old "GetStateId 0x157600"
    // plain-method claim was a misbinding (that is CDDrawChildGroup.s id 0x10).
    virtual i32 GetClassId() OVERRIDE; // [8]  0x156cf0 -> CLASSID_WORKERMAPSMALL (0x14)
    virtual void* Factory_1658c0(CParseSource* a1, const char* key, i32 a3); // [9] 0x1658c0
    virtual void* CreateWorker28(i32 a1, const char* key, i32 a3);           // [10] 0x165990
    virtual void* CreateWorker2C(i32 a1, const char* key, i32 a3);           // [11] 0x165a10
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

#endif // GRUNTZ_DDRAWMGR_DDRAWWORKERMAPSMALL_H
