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
#include <Mfc.h>          // CMapPtrToPtr - the +0x2c / +0x48 collections (real MFC)
#include <Wap32/Object.h> // the shared WAP CObject grand-base (slots 0/2/3/4 base thunks)

struct CDDrawChildWorker; // CDDrawGroupChild+0x7c worker (WalkChildWorkers callback host)

// The child object dispatched per list node. Slots laid out so the broadcast
// virtuals land at +0x34 / +0x38, with +0x2c and +0x30 used by other methods.
// Declarations only - never defined, so no ??_7 is emitted.
class CDDrawGroupChild {
public:
    virtual void Slot00();                        // +0x00
    virtual ~CDDrawGroupChild();                  // slot 1 (deleting dtor -> cl-emitted ??_G)
    virtual void Slot08();                        // +0x08
    virtual void Slot0C();                        // +0x0c
    virtual void Slot10();                        // +0x10
    virtual void Slot14();                        // +0x14
    virtual void Slot18();                        // +0x18
    virtual void Slot1C();                        // +0x1c
    virtual void Slot20();                        // +0x20
    virtual void Slot24();                        // +0x24
    virtual void Slot28();                        // +0x28
    virtual void Slot2C(i32 a1);                  // +0x2c
    virtual void Slot30(i32 a1, i32 a2);          // +0x30
    virtual void Vfunc34(i32 a1, i32 a2, i32 a3); // +0x34
    virtual void Vfunc38(i32 a1, i32 a2, i32 a3); // +0x38

    // vtable pointer at +0x00 (4 B); m_78 caches the child's CObList POSITION when it
    // is linked into a broadcast list (CWwdGameObjectB Add/RemoveChild); m_d8 written
    // by ResetChildD8.
    char m_pad04[0x78 - 4];
    i32 m_78;                // +0x78  cached CObList POSITION
    CDDrawChildWorker* m_7c; // +0x7c  per-child worker (WalkChildWorkers callback host)
    char m_pad80[0xd8 - 0x80];
    i32 m_d8; // +0xd8
};

// The per-child worker/handler sub-object held at CDDrawGroupChild+0x7c. Its +0x10
// slot is a __cdecl callback CWwdGameObjectB::WalkChildWorkers_166880 invokes once
// per child (passing the child). The concrete worker class is not yet recovered
// (@identity-TODO); only the callback field is modeled so the fn-ptr call reproduces.
struct CDDrawChildWorker {
    char m_pad00[0x10];
    void(__cdecl* m_fn10)(CDDrawGroupChild*); // +0x10  per-child callback
};
SIZE_UNKNOWN(CDDrawChildWorker);

// One node of the intrusive list at +0x14: next pointer @0, child object @8.
struct CDDrawGroupNode {
    CDDrawGroupNode* m_next; // +0x00
    i32 m_04;                // +0x04
    CDDrawGroupChild* m_obj; // +0x08
};

// ---------------------------------------------------------------------------
// CDDrawChildGroup - only the load-bearing offsets are modeled: the +0x14 list
// anchor and the vtable slots used by leaf methods.  The matched methods
// occupy lower vtable slots (slot numbers not load-bearing, only bodies), so
// they are placed first; the slot sequence from +0x1c through +0x3c is
// padded around the real virtuals so each lands at the correct offset.
// ---------------------------------------------------------------------------
class CDDrawChildGroup : public CObject { // slots 0/2/3/4 = CObject base thunks
public:
    RVA(0x001575e0, 0x16)
    i32 IsReady() {
        if (m_parent == 0) {
            goto fail;
        }
        if (m_status != -1) {
            return 1;
        }

    fail:
        return 0;
    }
    void WalkDispatch34(i32 a1, i32 a2, i32 a3);
    void WalkDispatch38(i32 a1, i32 a2, i32 a3);

    // Slots 0/2/3/4 inherited from CObject (base thunks). Own slots below:
    virtual ~CDDrawChildGroup() OVERRIDE; // slot 1  scalar-deleting dtor (0x157610)
    virtual void Slot14();                // +0x14  slot 5
    virtual void Slot18();                // +0x18
    RVA(0x001591e0, 0x5)
    virtual void ForwardTo3C() {
        this->Slot3C();
    }
    virtual void Slot20();                       // +0x20
    virtual void Slot24();                       // +0x24
    virtual void WalkDispatch2C(i32 a1);         // +0x28
    virtual void WalkDispatch30(i32 a1, i32 a2); // +0x2c
    virtual void Slot30();                       // +0x30
    virtual void Slot34();                       // +0x34
    virtual void ResetChildD8();                 // +0x38
    virtual void Slot3C();                       // +0x3c  (referenced by +0x1c thunk)
    virtual void Slot40();                       // +0x40  0x159f00 (17th slot)

    i32 m_status;              // +0x04  initialized to -1 when inactive
    i32 m_flags08;             // +0x08  flags (bit 0x200000 = draw per-object debug counts)
    i32 m_parent;              // +0x0c  parent/root handle
    char m_pad10[0x14 - 0x10]; // +0x10..0x13 (the +0x10 CObList's vptr)
    CDDrawGroupNode* m_head;   // +0x14  the +0x10 CObList's node-head (intrusive walk)
    char m_pad18[0x2c - 0x18]; // +0x18..0x2b (rest of the +0x10 CObList)
    CMapPtrToPtr m_map2c;      // +0x2c  (CMapPtrToPtr::Lookup 0x1b8760, FID-confirmed)
    CMapPtrToPtr m_map48;      // +0x48

    // Engine-label backlog stubs.
    void DestroyChildren();
    void DrawObjectCounts_15a650(); // 0x15a650  per-object debug-count overlay
};

SIZE_UNKNOWN(CDDrawChildGroup);
SIZE_UNKNOWN(CDDrawGroupChild);
VTBL(CDDrawChildGroup, 0x001efdc0); // ??_7CDDrawChildGroup@@6B@ (17-slot vtable)
SIZE_UNKNOWN(CDDrawGroupNode);

// --- vtable catalog ---

#endif // GRUNTZ_DDRAWMGR_CDDRAWCHILDGROUP_H
