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
#include <Mfc.h> // CMapPtrToPtr - the +0x2c / +0x48 collections (real MFC)

// The child object dispatched per list node. Slots laid out so the broadcast
// virtuals land at +0x34 / +0x38, with +0x2c and +0x30 used by other methods.
// Declarations only - never defined, so no ??_7 is emitted.
class CDDrawGroupChild {
public:
    virtual void Slot00();                        // +0x00
    virtual i32 ScalarDtor(i32 flag);             // +0x04  scalar-deleting destructor
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

    // Data member used by ResetChildD8 (write to +0xd8).
    // vtable pointer at +0x00 (4 B); pad from +0x04 to +0xd7.
    char m_pad04[0xd8 - 4];
    i32 m_d8; // +0xd8
};

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
class CDDrawChildGroup {
public:
    i32 IsReady();
    void WalkDispatch34(i32 a1, i32 a2, i32 a3);
    void WalkDispatch38(i32 a1, i32 a2, i32 a3);

    // --- vtable padding so the leaf virtuals land at their target slots ---
    virtual void Slot00();                       // +0x00
    virtual ~CDDrawChildGroup();                 // +0x04  slot 1 scalar-deleting dtor
    virtual void Slot08();                       // +0x08
    virtual void Slot0C();                       // +0x0c
    virtual void Slot10();                       // +0x10
    virtual void Slot14();                       // +0x14
    virtual void Slot18();                       // +0x18
    virtual void ForwardTo3C();                  // +0x1c  thunk -> +0x3c
    virtual void Slot20();                       // +0x20
    virtual void Slot24();                       // +0x24
    virtual void WalkDispatch2C(i32 a1);         // +0x28
    virtual void WalkDispatch30(i32 a1, i32 a2); // +0x2c
    virtual void Slot30();                       // +0x30
    virtual void Slot34();                       // +0x34
    virtual void ResetChildD8();                 // +0x38
    virtual void Slot3C();                       // +0x3c  (referenced by +0x1c thunk)

    i32 m_status;              // +0x04  initialized to -1 when inactive
    char m_pad08[0x0c - 0x08]; // +0x08..0x0b
    i32 m_parent;              // +0x0c  parent/root handle
    char m_pad10[0x14 - 0x10]; // +0x10..0x13 (the +0x10 CObList's vptr)
    CDDrawGroupNode* m_head;   // +0x14  the +0x10 CObList's node-head (intrusive walk)
    char m_pad18[0x2c - 0x18]; // +0x18..0x2b (rest of the +0x10 CObList)
    CMapPtrToPtr m_map2c;      // +0x2c  (CMapPtrToPtr::Lookup 0x1b8760, FID-confirmed)
    CMapPtrToPtr m_map48;      // +0x48

    // Engine-label backlog stubs.
    void DestroyChildren();
};

SIZE_UNKNOWN(CDDrawChildGroup);
SIZE_UNKNOWN(CDDrawGroupChild);
SIZE_UNKNOWN(CDDrawGroupNode);

#endif // GRUNTZ_DDRAWMGR_CDDRAWCHILDGROUP_H
