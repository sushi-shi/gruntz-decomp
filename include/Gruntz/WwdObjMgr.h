// CWwdObjMgr.h - the 0x159xxx-0x15bxxx WWD level-object collection manager. One
// class, reconstructed across four TUs (level-load in CWwdObjMgr.cpp, the per-kind
// factories in CWwdObjMgrFactories.cpp + CDDrawSubMgr.cpp, the list/map ops in
// CDDrawSubMgr.cpp, and the spatial routing in WwdSpatialMgr.cpp).
//
// Layout: a parent file handle at +0x0c, a sorted CPtrList at +0x10, and two
// CMapPtrToPtr key->object maps at +0x2c (primary) and +0x48 (active-set dedup).
// The +0x0c handle is a WwdFile* (LoadObjects dereferences it via ->BuildChild /
// ->m_14; the factories read it as a raw int handle passed to each object's ctor).
// Field names are placeholders; only offsets + code bytes are load-bearing.
#ifndef GRUNTZ_GRUNTZ_CWWDOBJMGR_H
#define GRUNTZ_GRUNTZ_CWWDOBJMGR_H

#include <rva.h>

#include <Mfc.h> // CPtrList, CMapPtrToPtr value members (real afxcoll layout)

// The managed object types + the level reader/file; each TU supplies the full
// definition it needs (only pointer/handle uses appear in this interface). The level
// reader is the shared CSerialArchive stream (Read @+0x2c), used to read descriptors.
struct WwdFile;
class CWwdGameObject;
class CWwdObject;
struct CSerialArchive; // the shared serialize stream (Read @+0x2c / Write @+0x30)

class CWwdObjMgr {
public:
    // Per-kind object factories (bodies: CWwdObjMgrFactories.cpp + CDDrawSubMgr.cpp).
    CWwdGameObject* CreateObject_159250(int a1, int a2, int a3, int a4, int a5, int a6, int a7);
    CWwdGameObject* CreateObject_159440(int a1, int a2, int a3, int a4);
    CWwdGameObject* CreateObject_159600(int a1, int a2, int a3, int a4, int a5, int flags);
    CWwdGameObject* CreateObject_1598d0(int a1, int a2, int a3, int a4, int a5, int a6);

    // Level-load path (bodies: CWwdObjMgr.cpp; Init_159830 is external/no-body).
    i32 LoadObjects(CSerialArchive* reader, u32 count, i32 unused);
    i32 Init_159830(void* obj, i32 a94, i32 a98, i32 a9c, const void* nameBuf, i32 z);

    // List / map ops (bodies: CDDrawSubMgr.cpp).
    void RemoveAll_15ab30(i32 pos, CWwdObject* obj);
    void RemoveByPosition_15ab70(i32 pos, CWwdObject* obj);
    void AddToMap48_15aba0(CWwdObject* obj);
    void PruneList_15aa90();
    i32 CountActive_15abc0();
    i32 ForEachDispatch_15ac20(i32 a1, i32 a2, i32 a3);
    i32 ForEachProbe_15acb0(i32 a1, i32 a2);
    i32 ForEachSerialize_15b020(CSerialArchive* ar, i32 a2);
    i32 Deserialize_15b0e0(CSerialArchive* ar, u32 count, i32 flag);
    i32 PruneOrphans_15b1d0();
    void InsertSorted_159e40(CWwdObject* obj, i32 addToMaps);
    void TickKillCues_159a70(i32 advance); // vtable slot 9 (per-frame kill-cue tick)
    CWwdObject* FindByWorker_15a860(i32 type, void* key);
    CWwdObject* FindByField_15a940(i32 type, void* key);

    char m_pad00[0x0c]; // +0x00..0x0b
    WwdFile* m_0c;      // +0x0c parent file handle (read as a raw int by the factories)
    CPtrList m_10;      // +0x10 sorted object list
    CMapPtrToPtr m_2c;  // +0x2c key -> object (primary)
    CMapPtrToPtr m_48;  // +0x48 key -> object (active set / dedup)
};

#endif // GRUNTZ_GRUNTZ_CWWDOBJMGR_H
