#ifndef GRUNTZ_DDRAWMGR_CDDRAWCHILDGROUP_H
#define GRUNTZ_DDRAWMGR_CDDRAWCHILDGROUP_H

// DDrawChildGroup.h - THE single-source shape of CDDrawChildGroup, the DDraw
// surface-manager child held at CDDrawSurfaceMgr+0x08 (the m_childGroup slot).
// It is an intrusive-list "broadcast" manager: a CObList sub-object @+0x10 whose
// node-head sits at +0x14, plus two CMapStringToOb collections @+0x2c/+0x48, over
// a parent/root handle @+0x0c and a status word @+0x04. Every leaf method walks the
// +0x14 list dispatching one of the child's sibling virtuals, some following with a
// dispatch of the object's own +0x2c virtual (see DDrawChildGroup.cpp for bodies).
//
// IDENTITY: CDDrawChildGroup IS CWwdObjMgr - the SAME +0x08 object, reached in the
// game-object collection role as CWwdObjMgr in src/DDrawMgr/DDrawSubMgr.cpp (list
// @+0x10, two maps @+0x2c/+0x48, per-frame kill-cue tick 0x159a70) and, in the
// serializer-blit role, as the local CDDrawChildGroupOps view in
// DDrawSurfaceMgrSerialize.cpp. Those are further method-sets on THIS class.
// The two maps @+0x2c/+0x48 are CMapPtrToPtr, PROVEN by the Wwd worker: their Lookup
// is 0x001b8760 = ?Lookup@CMapPtrToPtr@@QBEHPAXAAPAX@Z (FID-confirmed), NOT the
// CMapStringToOb the shell previously assumed (matching-neutral here - the only use in
// DDrawChildGroup.cpp is RemoveAll, a reloc-masked call, same 0x1c-byte CMap layout).
// A full CWwdObjMgr<->CDDrawChildGroup method-set consolidation is still a separate pass
// (the +0x10 list sub-object typing - CObList vs CPtrList - remains to be reconciled).
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
// pPrev @4, data @8). The object is read two ways pending the FLAT-view merge
// (@identity-TODO: CGameObject (UserLogic.h flat view) == CWwdGameObjectA - one
// class): the DDraw walkers dispatch it as the real family (CWwdGameObjectE),
// the game-side warlord/grunt loaders (Play.cpp) read it as CGameObject - same
// pointer, union'd (the CSpriteFactoryHolder pattern). The former View.h
// CWarlordListHead/CWarlordListNode duplicate of this node shape is dissolved
// here (2026-07-14).
struct CGameObject; // <Gruntz/UserLogic.h> (game-side reading of the same object)
struct CDDrawGroupNode {
    CDDrawGroupNode* m_next; // +0x00
    i32 m_04;                // +0x04  (pPrev; not walked)
    union {
        CWwdGameObjectE* m_obj; // +0x08  the wide game object (any kind; real family)
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
    virtual i32 IsLoaded() OVERRIDE;      // slot 5  0x1575e0 (parent set && status != -1)
    virtual i32 IsReady() OVERRIDE;       // slot 6  0x1576c0 (own `return 1` copy)
    virtual void ForwardTo3C();           // slot 7  0x1591e0 -> DestroyChildren
    virtual StateId GetStateId();         // slot 8  0x157600 (STATE_CHILDGROUP = 0x10)
    // slot 9 (+0x24) = the per-frame kill-cue tick (0x159a70, ret 4 = 1 arg; body on
    // CWwdObjMgr in WwdObjMgr.cpp - same class). CMulti/CPlay's frame pump dispatches
    // it here with the frame delta, then slot 16 below.
    virtual void TickKillCues_159a70(i32 advance);       // slot 9  +0x24
    virtual void WalkDispatch2C(i32 a1);                 // slot 10 0x159c90 (child Render)
    virtual void WalkDispatch30(i32 a1, i32 a2);         // slot 11 0x159cc0 (child BltDirty)
    virtual void WalkDispatch34(i32 a1, i32 a2, i32 a3); // slot 12 0x159cf0 (child BltDirtyEx)
    virtual void WalkDispatch38(i32 a1, i32 a2, i32 a3); // slot 13 0x159d40 (child BltDirtyRegions)
    virtual void ResetChildD8();                         // slot 14 0x159d90
    virtual void DestroyChildren();                      // slot 15 0x1591f0
    virtual void CollideBroadcast();                     // slot 16 0x159f00 (pairwise collision)

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
    CMapPtrToPtr m_map2c;      // +0x2c  (CMapPtrToPtr::Lookup 0x1b8760, FID-confirmed)
    CMapPtrToPtr m_map48;      // +0x48

    // Engine-label backlog stub.
    void DrawObjectCounts_15a650(); // 0x15a650  per-object debug-count overlay

    // 0x159ef0 - non-virtual entry that virtual-dispatches slot 15 (DestroyChildren):
    // `mov eax,[ecx]; jmp [eax+0x3c]`. Receiver byte-proven = holder+0x08 (this class):
    // CDDrawSurfaceMgr::RestoreChildren calls it with `mov ecx,[esi+0x8]`, and +0x3c is
    // IN BOUNDS only on this 17-slot vtable (the old CDDrawSubMgrPages attribution read
    // past that class's 10-slot table into ??_7CFileMem - a fabricated slot 15).
    void DestroyChildren_159ef0();
};

SIZE_UNKNOWN(CDDrawChildGroup);
VTBL(CDDrawChildGroup, 0x001efdc0); // ??_7CDDrawChildGroup@@6B@ (17-slot vtable)
SIZE_UNKNOWN(CDDrawGroupNode);

// --- vtable catalog ---

#endif // GRUNTZ_DDRAWMGR_CDDRAWCHILDGROUP_H
