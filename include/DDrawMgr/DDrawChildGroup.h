#ifndef GRUNTZ_DDRAWMGR_CDDRAWCHILDGROUP_H
#define GRUNTZ_DDRAWMGR_CDDRAWCHILDGROUP_H

// DDrawChildGroup.h - THE single-source shape of CDDrawChildGroup, the DDraw
// surface-manager child held at CDDrawSurfaceMgr+0x08 (the m_childGroup slot).
// It is an intrusive-list "broadcast" manager: a CObList sub-object @+0x10 whose
// node-head sits at +0x14, plus two CMapPtrToPtr collections @+0x2c/+0x48, over
// a parent/root handle @+0x0c and a status word @+0x04. Every leaf method walks the
// +0x14 list dispatching one of the child's sibling virtuals, some following with a
// dispatch of the object's own +0x2c virtual (see DDrawChildGroup.cpp for bodies).
//
// IDENTITY (consolidated 2026-07-16): CDDrawChildGroup IS the class that used to
// wear THREE names - the ex "CWwdObjMgr" (the WWD level-object collection role:
// factories, list/map ops, spatial finders) and the ex "CSpriteFactory" (the
// game-side created-sprite role: CreateSprite/AttachSprite + the live-object
// list) are method-sets of THIS one class. Both twins are dissolved here:
//  - one object: every holder reaches it at CDDrawSurfaceMgr+0x08 (m_childGroup
//    == the ex m_world->m_8 / m_c->m_8), new'd 0x6c by Init @0x155900 with the
//    ctor stamping ??_7CDDrawChildGroup (0x5efdc0);
//  - one TU: WwdObjMgr.cpp (0x159250..0x15b2xx) defines all three "classes'"
//    methods interleaved on the same receiver;
//  - one layout: vptr + m_status/m_flags08/m_parent + CObList @+0x10 +
//    CMapPtrToPtr @+0x2c/+0x48 (Lookup 0x001b8760 =
//    ?Lookup@CMapPtrToPtr@@QBEHPAXAAPAX@Z, FID-confirmed) + walk cursor @+0x64.
// The +0x10 list is the MFC CObList (AddTail/RemoveAt/InsertBefore =
// 0x1b5af6/0x1b5c2c/0x1b5bb0; teardown ~CObList 0x1b5a2b in this class's dtor).
//
// Field names are placeholders; only OFFSETS + emitted code bytes are load-bearing
// (campaign doctrine).

#include <rva.h>
#include <Ints.h>
#include <Mfc.h>            // CMapPtrToPtr - the +0x2c / +0x48 collections (real MFC)
#include <Wap32/WapObj.h>   // CWapObj - the IsLoaded/IsReady (slots 5/6) intermediate base
#include <Gruntz/StateId.h> // StateId - the slot-8 GetStateId tag space

struct AnimWorkerObj; // the +0x7c worker/logic record (<DDrawMgr/AnimWorkerObj.h>)

// (The former CDDrawGroupChild dispatch view of the wide game object is
// DISSOLVED onto the real family (<Wwd/WwdGameObjectFamily.h>, base
// CWwdGameObjectE): its ReleaseSubs was slot-7 Unload, its GetTypeId slot-8
// GetClassId, its m_78/m_7c/m_d8 the family m_posCache/m_7c/m_d8.)
class CWwdGameObjectE;

// One node of the intrusive list at +0x14 (the +0x10 CObList's CNode: pNext @0,
// pPrev @4, data @8). The object is read three ways pending the FLAT-view merge
// (@identity-TODO: CGameObject (UserLogic.h flat view) == CWwdGameObject ==
// CWwdGameObjectA - one class): the DDraw walkers dispatch it as the real family
// (CWwdGameObjectE), the WWD collection walkers as CWwdGameObject, the game-side
// warlord/grunt loaders (Play.cpp) read it as CGameObject - same pointer, union'd
// (the CSpriteFactoryHolder pattern). The former View.h CWarlordListHead/
// CWarlordListNode duplicate is dissolved here (2026-07-14); the former
// SpriteFactory.h "CSpriteListNode" (next/m_sprite) and WwdObjMgr.h "CWwdNode"
// (m_next/m_prev/m_obj) duplicates are dissolved here (2026-07-16).
struct CGameObject;   // <Gruntz/UserLogic.h> (game-side reading of the same object)
class CWwdGameObject; // <Gruntz/WwdGameObject.h> (WWD collection reading)
struct CDDrawGroupNode {
    CDDrawGroupNode* m_next; // +0x00
    CDDrawGroupNode* m_prev; // +0x04  (pPrev; rarely walked)
    union {
        CWwdGameObjectE* m_obj; // +0x08  the wide game object (any kind; real family)
        CWwdGameObject* m_wwd;  // +0x08  WWD collection reading (list/map ops)
        CGameObject* m_gameObj; // +0x08  game-side reading (UserLogic.h flat view)
    };
};

// ---------------------------------------------------------------------------
// CDDrawChildGroup - the full 17-slot vtable (0x1efdc0), every slot proven from
// the retail table (vtable_hierarchy + the slot bodies' xrefs: 0x1575e0/0x1576c0/
// 0x157600 are referenced ONLY from ??_7CDDrawChildGroup+0x14/+0x18/+0x20).
// Slots 0-4 CObject, 5/6 CWapObj-scheme overrides (own copies), 7-16 own.
// ---------------------------------------------------------------------------
class CDDrawChildGroup : public CWapObj {
public:
    // slot 1: ??1 @0x157630 (defined in DDrawSubMgr.cpp, the family dtor pocket -
    // the ex CDDrawChildGroupDtorHost view; ??_G @0x157610 is cl-generated there).
    virtual ~CDDrawChildGroup() OVERRIDE;
    virtual i32 IsLoaded() OVERRIDE; // slot 5  0x1575e0 (parent set && status != -1)
    virtual i32 IsReady() OVERRIDE;  // slot 6  0x1576c0 (own `return 1` copy)
    virtual void ForwardTo3C();      // slot 7  0x1591e0 -> DestroyChildren
    virtual StateId GetStateId();    // slot 8  0x157600 (STATE_CHILDGROUP = 0x10)
    // slot 9 (+0x24) = the per-frame kill-cue tick (0x159a70, ret 4 = 1 arg; body in
    // WwdObjMgr.cpp). CMulti/CPlay's frame pump dispatches it here with the frame
    // delta, then slot 16 below.
    virtual void TickKillCues_159a70(i32 advance);                // slot 9  +0x24
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
    CWwdGameObject* CreateObject_159600(int a1, int a2, int a3, int a4, int a5, int flags);
    CWwdGameObject* CreateObject_1598d0(int a1, int a2, int a3, int a4, int a5, int a6);
    // Name-resolving factory front-ends: resolve `name` through the owner's
    // worker-cache name map (m_parent->m_workerCache->m_10, the Ob-band Lookup
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
    CGameObject* CreateSprite(i32 kind, i32 geoB, i32 geoA, i32 hint, const char* name, i32 flags);
    // Initialise an already-allocated sprite against a named template (@0x159830).
    i32 AttachSprite(CWwdGameObject* obj, i32 a1, i32 a2, i32 a3, const char* name, i32 flags);

    // Level-load path (reader = the shared CSerialArchive == CFileMemBase).
    i32 LoadObjects(class CFileMemBase* reader, u32 count, i32 unused);

    // List / map ops.
    void RemoveAll_15ab30(i32 pos, CWwdGameObject* obj);
    void RemoveByPosition_15ab70(i32 pos, CWwdGameObject* obj);
    void AddToMap48_15aba0(CWwdGameObject* obj);
    void PruneList_15aa90();
    i32 CountActive_15abc0();
    i32 ForEachDispatch_15ac20(i32 a1, i32 a2, i32 a3);
    i32 ForEachProbe_15acb0(i32 a1, i32 a2);
    i32 ForEachSerialize_15b020(class CFileMemBase* ar, i32 a2);
    i32 Deserialize_15b0e0(class CFileMemBase* ar, u32 count, i32 flag);
    i32 PruneOrphans_15b1d0();
    void RemoveAndDelete_159db0(CWwdGameObject* obj);   // 0x159db0
    void ReinsertUnflagged_159e10(CWwdGameObject* obj); // 0x159e10
    void InsertSorted_159e40(CWwdGameObject* obj, i32 addToMaps);
    i32 CheckSortOrder_15a780();
    CWwdGameObject* FindByType04_15a7f0(i32 type);
    CWwdGameObject* FindByTypeProbe_15a810(i32 type);
    CWwdGameObject* FindByWorker_15a860(i32 type, void* key);
    CWwdGameObject* FindByField_15a940(i32 type, void* key);
    // 0x15a8c0 - first child whose type tag (vtable slot 8) == 5, whose +0x04 id ==
    // `id`, and whose +0x7c sub-object's +0x10 matches the object the worker-cache
    // name map yields for `key`.
    void* Find_15a8c0(i32 id, const char* key);
    CWwdGameObject* FindByKey_15a9a0(void* key); // 0x15a9a0 first obj with m_key==key
    CWwdGameObject*
    FindByStatusKey_15a9d0(void* key); // 0x15a9d0 first status-5 obj with m_key==key
    i32 IsKindUnique_15aa20(i32 kind); // 0x15aa20 1 unless 2+ objs share m_04==kind
    i32 CountByKind_15aa60(i32 kind);  // 0x15aa60 count of objs with m_04==kind
    i32 SumWeighted_15aaf0();          // 0x15aaf0 sum i*(m_5c+m_74+m_60+m_04)

    i32 m_status;  // +0x04  initialized to -1 when inactive
    i32 m_flags08; // +0x08  flags (bit 0x200000 = draw per-object debug counts)
    // +0x0c  the owning CDDrawSurfaceMgr (the world/display root; its +0x24 is the
    // CGameLevel and +0x04 the pages sub-manager).
    class CDDrawSurfaceMgr* m_parent;
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

    // Engine-label backlog stub.
    void DrawObjectCounts_15a650(); // 0x15a650  per-object debug-count overlay

    // 0x159ef0 - non-virtual entry that virtual-dispatches slot 15 (DestroyChildren):
    // `mov eax,[ecx]; jmp [eax+0x3c]`. Receiver byte-proven = holder+0x08 (this class):
    // CDDrawSurfaceMgr::RestoreChildren calls it with `mov ecx,[esi+0x8]`, and +0x3c is
    // IN BOUNDS only on this 17-slot vtable (the old CDDrawSubMgrPages attribution read
    // past that class's 10-slot table into ??_7CFileMem - a fabricated slot 15).
    void DestroyChildren_159ef0();
};

// The per-object descriptor the level reader fills (0xa0 bytes; LoadObjects reads
// one per record). +0x04 is the dedup id, +0x08 the kind selector, +0x14 the
// object's name string. (Hoisted from the ex WwdObjMgr.h.)
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

SIZE_UNKNOWN(CDDrawChildGroup);
VTBL(CDDrawChildGroup, 0x001efdc0); // ??_7CDDrawChildGroup@@6B@ (17-slot vtable)
SIZE_UNKNOWN(CDDrawGroupNode);

// --- vtable catalog ---

#endif // GRUNTZ_DDRAWMGR_CDDRAWCHILDGROUP_H
