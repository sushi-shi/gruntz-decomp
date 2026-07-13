// WwdObjMgr.h - the 0x159xxx-0x15bxxx WWD level-object collection manager. One
// class, reconstructed across four TUs (level-load in CWwdObjMgr.cpp, the per-kind
// factories in CWwdObjMgrFactories.cpp + CDDrawSubMgr.cpp, the list/map ops in
// CDDrawSubMgr.cpp, and the spatial routing in WwdSpatialMgr.cpp).
//
// Layout: a parent surface-manager pointer at +0x0c, a sorted CPtrList at +0x10, and
// two CMapPtrToPtr key->object maps at +0x2c (primary) and +0x48 (active-set dedup).
// The +0x0c owner is the CANONICAL CDDrawSurfaceMgr, not a local "WwdFile" view -
// PROVEN by the call sites' rel32: the former view's `BuildChild` @0x156a90 IS
// ?InvokeCallback@CDDrawSurfaceMgr@@QAEHPAXHHH@Z (100% EXACT), and its `m_14 + 0x10`
// string-resolve map IS m_workerCache (+0x14, CDDrawWorkerCache) -> m_10 (the +0x10
// CMapStringToOb). The factories read the pointer as a raw int handle per object ctor.
// Field names are placeholders; only offsets + code bytes are load-bearing.
#ifndef GRUNTZ_GRUNTZ_CWWDOBJMGR_H
#define GRUNTZ_GRUNTZ_CWWDOBJMGR_H

#include <rva.h>

#include <Mfc.h> // CPtrList, CMapPtrToPtr value members (real afxcoll layout)

// The managed object types + the level reader/file; each TU supplies the full
// definition it needs (only pointer/handle uses appear in this interface). The level
// reader is the shared CSerialArchive stream (Read @+0x2c), used to read descriptors.
class CDDrawSurfaceMgr; // +0x0c owner (InvokeCallback + the m_workerCache name map)

// The per-object descriptor the level reader fills (0xa0 bytes; LoadObjects reads
// one per record). +0x04 is the dedup id, +0x08 the kind selector, +0x14 the
// object's name string.
struct WwdObjDesc {
    i32 m_00;               // +0x00  passed to the factory
    i32 m_04;               // +0x04  dedup key / object id
    i32 m_08;               // +0x08  kind selector
    i32 m_0c;               // +0x0c  ARM-0x1c child tag
    i32 m_10;               // +0x10  merge child-build selector
    char m_14[0x94 - 0x14]; // +0x14  name string (resolver key)
    i32 m_94;               // +0x94
    i32 m_98;               // +0x98
    i32 m_9c;               // +0x9c
};
SIZE_UNKNOWN(WwdObjDesc);

class CWwdGameObject;  // the managed wide objects (<Gruntz/WwdGameObject.h>; the
                       // ex-CWwdObject element view is dissolved onto it)
struct CSerialArchive; // the shared serialize stream (Read @+0x2c / Write @+0x30)

class CWwdObjMgr {
public:
    // Per-kind object factories (bodies: CWwdObjMgrFactories.cpp + CDDrawSubMgr.cpp).
    CWwdGameObject* CreateObject_159250(int a1, int a2, int a3, int a4, int a5, int a6, int a7);
    CWwdGameObject* CreateObject_159440(int a1, int a2, int a3, int a4);
    CWwdGameObject* CreateObject_159600(int a1, int a2, int a3, int a4, int a5, int flags);
    CWwdGameObject* CreateObject_1598d0(int a1, int a2, int a3, int a4, int a5, int a6);

    // Name-resolving factory front-ends (bodies: CWwdObjMgrFactories.cpp): resolve
    // `name` through the owner's worker-cache name map (m_0c->m_workerCache->m_10) to an id,
    // then forward it as the matching CreateObject argument.
    CWwdGameObject* CreateNamed_1593e0(
        int a1,
        int a2,
        int a3,
        int a4,
        const char* name,
        int a6,
        int a7
    );                                                                            // 0x1593e0
    CWwdGameObject* CreateNamed_1595b0(int a1, int a2, const char* name, int a4); // 0x1595b0
    CWwdGameObject*
    CreateNamed_159a10(int a1, int a2, int a3, int a4, const char* name, int a6); // 0x159a10

    // Level-load path (bodies: CWwdObjMgr.cpp). (The ex-`Init_159830(void*,...)`
    // decl was a PHANTOM second name for CSpriteFactory::AttachSprite @0x159830 -
    // LoadObjects calls the real method through the shared `this`.)
    i32 LoadObjects(CSerialArchive* reader, u32 count, i32 unused);

    // List / map ops (bodies: CDDrawSubMgr.cpp).
    void RemoveAll_15ab30(i32 pos, CWwdGameObject* obj);
    void RemoveByPosition_15ab70(i32 pos, CWwdGameObject* obj);
    void AddToMap48_15aba0(CWwdGameObject* obj);
    void PruneList_15aa90();
    i32 CountActive_15abc0();
    i32 ForEachDispatch_15ac20(i32 a1, i32 a2, i32 a3);
    i32 ForEachProbe_15acb0(i32 a1, i32 a2);
    i32 ForEachSerialize_15b020(CSerialArchive* ar, i32 a2);
    i32 Deserialize_15b0e0(CSerialArchive* ar, u32 count, i32 flag);
    i32 PruneOrphans_15b1d0();
    void RemoveAndDelete_159db0(CWwdGameObject* obj);   // 0x159db0
    void ReinsertUnflagged_159e10(CWwdGameObject* obj); // 0x159e10
    void InsertSorted_159e40(CWwdGameObject* obj, i32 addToMaps);
    void TickKillCues_159a70(i32 advance); // vtable slot 9 (per-frame kill-cue tick)
    i32 CheckSortOrder_15a780();
    CWwdGameObject* FindByType04_15a7f0(i32 type);
    CWwdGameObject* FindByTypeProbe_15a810(i32 type);
    CWwdGameObject* FindByWorker_15a860(i32 type, void* key);
    CWwdGameObject* FindByField_15a940(i32 type, void* key);
    // 0x15a8c0 - first child whose type tag (vtable slot 8) == 5, whose +0x04 id == `id`, and
    // whose +0x7c sub-object's +0x10 matches the object the worker-cache name map yields for
    // `key`. (Was the placeholder class CChildFinder_15a8c0, whose "m_parent @+0x0c" and
    // "m_listHead @+0x14" are this class's own m_0c and m_10-head.)
    void* Find_15a8c0(i32 id, const char* key);
    CWwdGameObject* FindByKey_15a9a0(void* key); // 0x15a9a0 first obj with m_key==key
    CWwdGameObject*
    FindByStatusKey_15a9d0(void* key); // 0x15a9d0 first status-5 obj with m_key==key
    i32 IsKindUnique_15aa20(i32 kind); // 0x15aa20 1 unless 2+ objs share m_04==kind
    i32 CountByKind_15aa60(i32 kind);  // 0x15aa60 count of objs with m_04==kind
    i32 SumWeighted_15aaf0();          // 0x15aaf0 sum i*(m_5c+m_74+m_60+m_04)

    char m_pad00[0x0c];     // +0x00..0x0b
    CDDrawSurfaceMgr* m_0c; // +0x0c owning surface manager (raw int handle to the factories)
    CObList m_10;           // +0x10 sorted object list (MFC CObList - stores CObject*;
                            // RemoveAt/AddTail/InsertBefore = 0x1b5c2c/0x1b5af6/0x1b5bb0)
    CMapPtrToPtr m_2c;      // +0x2c key -> object (primary)
    CMapPtrToPtr m_48;      // +0x48 key -> object (active set / dedup)
};
SIZE_UNKNOWN(CWwdObjMgr);

#endif // GRUNTZ_GRUNTZ_CWWDOBJMGR_H
