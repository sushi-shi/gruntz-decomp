#ifndef GRUNTZ_DDRAWMGR_CDDRAWCHILDGROUP_H
#define GRUNTZ_DDRAWMGR_CDDRAWCHILDGROUP_H

#include <rva.h>
#include <Ints.h>
#include <Mfc.h>            // CMapPtrToPtr - the +0x2c / +0x48 collections (real MFC)
#include <Gruntz/Loadable.h> // CLoadable - the real base (ctor 0x156cb0 at the Init new-site)

struct AnimWorkerObj; // the +0x7c worker/logic record (<DDrawMgr/AnimWorkerObj.h>)

struct CGameObject;

// One node of the intrusive list at +0x14 (the +0x10 CObList's CNode: pNext @0,
// pPrev @4, data @8). The object is read three ways pending the FLAT-view merge
// (@identity-TODO: CGameObject (UserLogic.h flat view) == CWwdGameObject ==
// CWwdGameObjectA - one class): the DDraw walkers dispatch it as the real family
// (CGameObject), the WWD collection walkers as CWwdGameObject, the game-side
// warlord/grunt loaders (Play.cpp) read it as CGameObject - same pointer, union'd
// (the CDDrawSurfaceMgr pattern). The former View.h CWarlordListHead/
// CWarlordListNode duplicate is dissolved here (2026-07-14); the former
// SpriteFactory.h "CSpriteListNode" (next/m_sprite) and WwdObjMgr.h "CWwdNode"
// (m_next/m_prev/m_obj) duplicates are dissolved here (2026-07-16).
struct CGameObject;   // <Gruntz/UserLogic.h> (game-side reading of the same object)
class CWwdGameObjectA; // the created-sprite kind (CreateSprite's product type)
class CWwdGameObject; // <Gruntz/WwdGameObject.h> (WWD collection reading)
struct CDDrawGroupNode {
    CDDrawGroupNode* m_next; // +0x00
    CDDrawGroupNode* m_prev; // +0x04  (pPrev; rarely walked)
    // +0x08  the wide game object (any kind - the item base; the ex m_wwd/m_gameObj
    // arms were per-view spellings; derived readers downcast).
    CGameObject* m_obj;
};
SIZE_UNKNOWN();

// (B)-form re-base 2026-07-22: CDDrawSurfaceMgr::Init constructs this with the
// CLoadable 3-arg base ctor 0x156cb0 (retail decode), and vtbl 0x5efdc0 slots
// 5-8 are the CLoadable scheme (IsLoaded/IsReady/Unload/GetClassId 0x10).
class CDDrawChildGroup : public CLoadable {
public:
    // slot 1: ??1 @0x157630 (defined in DDrawSubMgr.cpp, the family dtor pocket -
    // the ex CDDrawChildGroupDtorHost view; ??_G @0x157610 is cl-generated there).
    virtual ~CDDrawChildGroup() OVERRIDE;
    virtual i32 IsLoaded() OVERRIDE; // slot 5  0x1575e0 (parent set && status != -1)
    virtual i32 IsReady() OVERRIDE;  // slot 6  0x1576c0 (own `return 1` copy)
    virtual void Unload() OVERRIDE;    // slot 7  0x1591e0 -> DestroyChildren (ex "ForwardTo3C")
    virtual i32 GetClassId() OVERRIDE; // slot 8  0x157600 -> CLASSID_CHILDGROUP (0x10)
    // slot 9 (+0x24) = the per-frame kill-cue tick (0x159a70, ret 4 = 1 arg; body in
    // WwdObjMgr.cpp). CMulti/CPlay's frame pump dispatches it here with the frame
    // delta, then slot 16 below.
    virtual void TickKillCues(i32 advance);                // slot 9  +0x24
    virtual void WalkDispatch2C(class CDDrawSurfacePair* target); // slot 10 0x159c90 (child Render)
    virtual void WalkDispatch30(i32 a1, i32 a2);         // slot 11 0x159cc0 (child BltDirty)
    virtual void WalkDispatch34(i32 a1, i32 a2, i32 a3); // slot 12 0x159cf0 (child BltDirtyEx)
    virtual void WalkDispatch38(i32 a1, i32 a2, i32 a3); // slot 13 0x159d40 (child BltDirtyRegions)
    virtual void ResetChildD8();                         // slot 14 0x159d90
    virtual void DestroyChildren();                      // slot 15 0x1591f0
    virtual void CollideBroadcast();                     // slot 16 0x159f00 (pairwise collision)

    // --- the WWD collection / factory method set (bodies: WwdObjMgr.cpp + the
    // family pockets in CDDrawSubMgr.cpp / WwdSpatialMgr.cpp; the ex "CWwdObjMgr"
    // and "CSpriteFactory" names). ---
    // Per-kind object factories.
    CWwdGameObject* CreateObject_159250(int a1, int a2, int a3, int a4, int a5, int a6, int a7);
    CWwdGameObject* CreateObject_159440(int a1, int a2, int a3, int a4);
    CWwdGameObjectA* CreateObject_159600(int a1, int a2, int a3, int a4, int a5, int flags);
    CWwdGameObject* CreateObject_1598d0(int a1, int a2, int a3, int a4, int a5, int a6);
    // Name-resolving factory front-ends: resolve `name` through the owner's
    // worker-cache name map (OwnerMgr()->m_workerCache->m_10, the Ob-band Lookup
    // 0x1b8008) to an id, then forward it as the matching CreateObject argument.
    CWwdGameObject*
    CreateNamed_1593e0(int a1, int a2, int a3, int a4, const char* name, int a6, int a7);
    CWwdGameObject* CreateNamed_1595b0(int a1, int a2, const char* name, int a4); // 0x1595b0
    CWwdGameObject*
    CreateNamed_159a10(int a1, int a2, int a3, int a4, const char* name, int a6); // 0x159a10

    // The game-side sprite front-ends (the ex "CSpriteFactory" role): CreateSprite
    // (@0x1597b0) looks a sprite TEMPLATE up by class-NAME in the owner's
    // worker-cache table and forwards the build args + the resolved template to
    // CreateObject_159600, which `new`s the 0x1dc game-sprite instance
    // (CGameObject, <Gruntz/UserLogic.h>). __thiscall, ret 0x18.
    CWwdGameObjectA*
    CreateSprite(i32 kind, i32 geoB, i32 geoA, i32 hint, const char* name, i32 flags);
    // Initialise an already-allocated sprite against a named template (@0x159830).
    i32 AttachSprite(CWwdGameObject* obj, i32 a1, i32 a2, i32 a3, const char* name, i32 flags);

    // Level-load path (reader = the shared CFileMemBase == CFileMemBase).
    i32 LoadObjects(class CFileMemBase* reader, u32 count, i32 unused);

    // List / map ops.
    void RemoveAll(i32 pos, CWwdGameObject* obj);
    void RemoveByPosition(i32 pos, CWwdGameObject* obj);
    void AddToMap48(CWwdGameObject* obj);
    void PruneList();
    i32 CountActive();
    i32 ForEachDispatch(i32 a1, i32 a2, i32 a3);
    i32 ForEachProbe(i32 a1, i32 a2);
    i32 ForEachSerialize(class CFileMemBase* ar, i32 a2);
    i32 Deserialize(class CFileMemBase* ar, u32 count, i32 flag);
    i32 PruneOrphans();
    void RemoveAndDelete(CWwdGameObject* obj);   // 0x159db0
    void ReinsertUnflagged(CWwdGameObject* obj); // 0x159e10
    void InsertSorted(CGameObject* obj, i32 addToMaps);
    i32 CheckSortOrder();
    CWwdGameObject* FindByType04(i32 type);
    CWwdGameObject* FindByTypeProbe(i32 type);
    CWwdGameObject* FindByWorker(i32 type, void* key);
    CWwdGameObject* FindByField(i32 type, void* key);
    // 0x15a8c0 - first child whose type tag (vtable slot 8) == 5, whose +0x04 id ==
    // `id`, and whose +0x7c sub-object's +0x10 matches the object the worker-cache
    // name map yields for `key`.
    void* Find(i32 id, const char* key);
    CWwdGameObject* FindByKey(void* key); // 0x15a9a0 first obj with m_key==key
    CWwdGameObject*
    FindByStatusKey(void* key); // 0x15a9d0 first status-5 obj with m_key==key
    i32 IsKindUnique(i32 kind); // 0x15aa20 1 unless 2+ objs share m_04==kind
    i32 CountByKind(i32 kind);  // 0x15aa60 count of objs with m_04==kind
    i32 SumWeighted();          // 0x15aaf0 sum i*(m_5c+m_74+m_60+m_04)

    // (+0x04 m_04 / +0x08 m_flags / +0x0c m_0c are the INHERITED CLoadable header
    // trio - m_flags bit 0x200000 = draw per-object debug counts; the owner read
    // is OwnerMgr() == the ex "m_parent" CDDrawSurfaceMgr world root.)
    // +0x10  the REAL CObList (0x1c bytes: vptr, pNodeHead@+0x14, pTail, nCount@+0x1c,
    // free/blocks/blocksize). The intrusive walkers read the head via the inline
    // GetHeadPosition() (same `mov reg,[this+0x14]` bytes) cast to CDDrawGroupNode
    // (the CNode shape); DrawDebugStats reads GetCount() (inline m_nCount @+0x1c).
    // Was pads + raw m_head/m_count fields; the real member makes ~CDDrawChildGroup
    // emit the retail ~CObList teardown (0x1b5a2b) under its own /GX trylevel.
    CObList m_list;
    CMapPtrToPtr m_map2c; // +0x2c  key -> object, primary (Lookup 0x1b8760, FID-confirmed)
    CMapPtrToPtr m_map48; // +0x48  key -> object, serialize/dedup set (the ex GruntObjMap)
    // +0x64  transient list walk cursor (seeded from the list head;
    // CGruntzMapMgr::LoadAttributes footprint pass @0x0810f0).
    CDDrawGroupNode* m_walkCursor;
    // +0x68  second walk cursor: the battlez spawn-scan pop cursor Drain drains
    // (re-seeded from the list head). Ex the CQueueDrainHost::m_scan view field.
    CDDrawGroupNode* m_scanCursor;

    // Engine-label backlog stub.
    void DrawObjectCounts(); // 0x15a650  per-object debug-count overlay

    // 0x159ef0 - non-virtual entry that virtual-dispatches slot 15 (DestroyChildren):
    // `mov eax,[ecx]; jmp [eax+0x3c]`. Receiver byte-proven = holder+0x08 (this class):
    // CDDrawSurfaceMgr::RestoreChildren calls it with `mov ecx,[esi+0x8]`, and +0x3c is
    // IN BOUNDS only on this 17-slot vtable (the old CDDrawSubMgrPages attribution read
    // past that class's 10-slot table into ??_7CFileMem - a fabricated slot 15).
    void DestroyChildren_159ef0();

    // 0x31250 (body in QueueDrainHost.cpp): the battlez spawn-scan drain - pop nodes off
    // m_scanCursor until one whose payload GetClassId() (vtable slot 8) is CLASSID_SERIALREF;
    // 0 when exhausted. Ex CQueueDrainHost::Drain (that view IS this class).
    CGameObject* Drain();
};
SIZE_UNKNOWN();
SIZE_UNKNOWN();

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
SIZE_UNKNOWN();

#endif // GRUNTZ_DDRAWMGR_CDDRAWCHILDGROUP_H
