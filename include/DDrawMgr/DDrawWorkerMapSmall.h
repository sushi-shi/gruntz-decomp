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
    // INLINE ctor - expanded in place by CDDrawSurfaceMgr::Init @0x155acd (base call
    // 0x156cb0, the three map member ctors, the ??_7 stamp 0x5efcc8, +0x64 zeroed).
    CDDrawWorkerMapSmall(CDDrawSurfaceMgr* owner) : CLoadable(CLoadable::OwnerHandle(owner), 0, 0) {
        m_cachedWorker = 0;
    }
    virtual i32 IsLoaded() OVERRIDE; // [5]  0x156cd0 (G obj; the worker-gate)
    virtual i32 IsReady() OVERRIDE;  // [6]  0x156db0 (G obj; own return-1 copy)
    virtual void Unload() OVERRIDE;  // [7]  0x165810 (T obj; ex "DestroyAll")
    // [8] the REAL GetClassId (.mov eax,0x14; ret. @0x156cf0 - a Ghidra recovery
    // gap; body defined at the dtor pocket). The old "GetStateId 0x157600"
    // plain-method claim was a misbinding (that is CDDrawChildGroup.s id 0x10).
    virtual i32 GetClassId() OVERRIDE; // [8]  0x156cf0 -> CLASSID_WORKERMAPSMALL (0x14)
    virtual void* Factory_1658c0(CParseSource* a1, const char* key, i32 a3); // [9] 0x1658c0
    // [10] vs [11]: same body shape, but they dispatch DIFFERENT worker slots, so
    // their first args differ. [10] -> the worker's +0x28 (AllocBufMakeB -> MakeB,
    // whose loader @0x1474d0 reads 256 RGB triples) = an in-memory palette blob;
    // [11] -> the worker's +0x2c (AllocBufMakeB2 -> MakeB2 -> CDDPalette::LoadFromFile
    // @0x147410, which opens with `strrchr(a1,'.')`) = a FILE PATH. `key` is the cache
    // key in both (its only use is the m_map1 subscript), not a rival name.
    virtual void* CreateWorker28(void* data, const char* key, i32 flags); // [10] 0x165990
    virtual void* CreateWorker2C(char* path, const char* key, i32 flags); // [11] 0x165a10
    virtual void* Factory_165a90(CParseSource* a1, i32 a2, i32 a3);       // [12] 0x165a90
    virtual ~CDDrawWorkerMapSmall() OVERRIDE; // overrides slot [1]; 0x156d20 (G obj)

    CMapStringToOb m_map1; // +0x10  worker-by-key map 1 (0x10..0x2b)
    CMapStringToOb m_map2; // +0x2c  worker-by-key map 2 (0x2c..0x47)
    CMapStringToOb m_map3; // +0x48  worker-by-key map 3 (0x48..0x63)
    // +0x64  the cached/current worker out of m_map1. NOT the "entry counter" this
    // was typed as (i32 m_64): RemoveByValue @0x165c40 compares it against the worker
    // being removed and nulls it on a hit, and the teardown nulls it too - a counter is
    // neither compared to a worker pointer nor dereferenced.
    // IDENTITY CLOSED 2026-07-27: it is a CAniRecordBase2, the class every m_map1 value
    // already is here (DestroyAll/RemoveByValue/RemoveByKey all `delete` values as one).
    // The plane host's ResolveColorKey walks +0x10 -> +0x0c and indexes it [i*4+0..2]:
    // CAniRecordBase2::m_buf IS at +0x10 (a CDDPalette*), and CDDPalette::m_cacheA IS at
    // +0x0c - a 0x400-byte 256-entry PALETTEENTRY table, i.e. R/G/B at stride 4. Both
    // links match, so the ex-CPlanePalOwner/CPlanePalArr views are gone.
    CAniRecordBase2* m_cachedWorker;

    // Non-vtable teardown/remove helpers (T obj).
    void ResetSlots();                // 0x165b90
    i32 RemoveByValue(CObject* obj);  // 0x165c40
    i32 RemoveByKey(const char* key); // 0x165d30
};
SIZE_UNKNOWN();

#endif // GRUNTZ_DDRAWMGR_DDRAWWORKERMAPSMALL_H
