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

struct CDDrawChildWorker; // CDDrawGroupChild+0x7c worker (WalkChildWorkers callback host)

// The child object dispatched per list node - the WIDE game object (the
// CWwdGameObject / CWwdGameObjectE factory family, vtables 0x5f0020/0x5f00a8/...):
// +0x78 is its CObList POSITION cache (== CWwdGameObject::m_posCache), +0x7c its
// 0x17c anim worker (AnimWorkerObj), +0xd8 its shadow dirty-rect flag
// (CWwdGameObjectE::m_d8), and the dispatched slots are the family's own (slot
// names/RVAs = the 0x5f0020 table's ground truth). Kept as the DDraw-side dispatch
// VIEW pending the wide-object unification pass (@identity-TODO:
// CDDrawGroupChild == CWwdGameObject == CGameObject (UserLogic.h) - one class).
// Slots 0-4 come from CObject and 5/6 (IsLoaded/IsReady) from CWapObj - inherited,
// never redeclared. Declarations only - never defined, so no ??_7 is emitted.
class CDDrawGroupChild : public CWapObj {
public:
    virtual ~CDDrawGroupChild(); // slot 1 (deleting dtor -> the family ??_G, e.g. 0x15b790)
    virtual void ReleaseSubs();  // slot 7  @0x15b5d0 (family base body)
    // slot 8 - the per-kind type tag (`mov eax,<id>; ret`: E=0, C=6, F=0x16, B=0x1b;
    // Find_15a8c0 / FindByWorker probe ==5). == the family's GetTypeId.
    virtual i32 GetTypeId();                             // slot 8  +0x20
    virtual i32 Slot24_164790();                         // slot 9  @0x164790 (family shared helper)
    virtual i32 Setup28(i32 a1, i32 a2, i32 a3, i32 a4); // slot 10  the 4-arg build dispatch
    // slots 11-14: the per-object render + dirty-rect blit hooks the group walkers
    // broadcast (args kept i32 in this view; the family types them
    // WwdRenderCtx* / CDDrawSurfacePair* - same 4-byte pushes).
    virtual void Render(i32 a1);                          // slot 11 +0x2c
    virtual void BltDirty(i32 a1, i32 a2);                // slot 12 +0x30
    virtual void BltDirtyEx(i32 a1, i32 a2, i32 a3);      // slot 13 +0x34
    virtual void BltDirtyRegions(i32 a1, i32 a2, i32 a3); // slot 14 +0x38

    // vtable pointer at +0x00 (4 B); m_78 caches the child's CObList POSITION when it
    // is linked into a broadcast list (CWwdGameObjectB Add/RemoveChild); m_d8 written
    // by ResetChildD8.
    char m_pad04[0x78 - 4];
    i32 m_78;                // +0x78  cached CObList POSITION
    CDDrawChildWorker* m_7c; // +0x7c  per-child worker (WalkChildWorkers callback host)
    char m_pad80[0xd8 - 0x80];
    i32 m_d8; // +0xd8
};

// The per-child worker/handler sub-object held at CDDrawGroupChild+0x7c - the same
// 0x17c AnimWorkerObj (vtable 0x1efb80) the wide object owns at +0x7c; its +0x10
// slot is the collide/expiry callback (AnimWorkerObj::m_collideNotify)
// CWwdGameObjectB::WalkChildWorkers_166880 invokes once per child (passing the
// child). Kept as the thin callback view pending the worker unification
// (@identity-TODO: CDDrawChildWorker == AnimWorkerObj == CLogicRecord).
struct CDDrawChildWorker {
    char m_pad00[0x10];
    void(__cdecl* m_fn10)(CDDrawGroupChild*); // +0x10  per-child callback (m_collideNotify)
};
SIZE_UNKNOWN(CDDrawChildWorker);

// One node of the intrusive list at +0x14: next pointer @0, child object @8.
struct CDDrawGroupNode {
    CDDrawGroupNode* m_next; // +0x00
    i32 m_04;                // +0x04
    CDDrawGroupChild* m_obj; // +0x08
};

// ---------------------------------------------------------------------------
// CDDrawChildGroup - the full 17-slot vtable (0x1efdc0), every slot proven from
// the retail table (vtable_hierarchy + the slot bodies' xrefs: 0x1575e0/0x1576c0/
// 0x157600 are referenced ONLY from ??_7CDDrawChildGroup+0x14/+0x18/+0x20).
// Slots 0-4 CObject, 5/6 CWapObj-scheme overrides (own copies), 7-16 own.
// ---------------------------------------------------------------------------
class CDDrawChildGroup : public CWapObj {
public:
    virtual ~CDDrawChildGroup() OVERRIDE; // slot 1  scalar-deleting dtor (0x157610)
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
    char m_pad10[0x14 - 0x10]; // +0x10..0x13 (the +0x10 CObList's vptr)
    CDDrawGroupNode* m_head;   // +0x14  the +0x10 CObList's node-head (intrusive walk)
    char m_pad18[0x2c - 0x18]; // +0x18..0x2b (rest of the +0x10 CObList)
    CMapPtrToPtr m_map2c;      // +0x2c  (CMapPtrToPtr::Lookup 0x1b8760, FID-confirmed)
    CMapPtrToPtr m_map48;      // +0x48

    // Engine-label backlog stub.
    void DrawObjectCounts_15a650(); // 0x15a650  per-object debug-count overlay
};

SIZE_UNKNOWN(CDDrawChildGroup);
SIZE_UNKNOWN(CDDrawGroupChild);
VTBL(CDDrawChildGroup, 0x001efdc0); // ??_7CDDrawChildGroup@@6B@ (17-slot vtable)
SIZE_UNKNOWN(CDDrawGroupNode);

// --- vtable catalog ---

#endif // GRUNTZ_DDRAWMGR_CDDRAWCHILDGROUP_H
