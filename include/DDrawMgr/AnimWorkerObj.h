#ifndef GRUNTZ_DDRAWMGR_ANIMWORKEROBJ_H
#define GRUNTZ_DDRAWMGR_ANIMWORKEROBJ_H

// AnimWorkerObj.h - the 0x17c-byte per-object anim/logic worker (vtable
// ??_7AnimWorkerObj @0x1efb80 / VA 0x5efb80), hoisted from DDrawWorkerCache.cpp so
// the wave4-L original-TU partition can define its methods in their retail objs:
// Clear @0x151e70 lives in the 0x1504d0 wwd-game-object obj (WwdGameObject.cpp);
// the CDDrawWorkerCache factory @0x1652c0 lives in the 0x163a40 family-meat obj
// (DDrawSurfacePair.cpp). IDENTITY NOTE (dossier #15): this class is the SAME
// retail class as CLogicRecord (its dtor 0x151da0 re-stamps 0x1efb80) and the
// CAnimWorker the UserBaseLink EnsureWorker* fns build - the three views'
// unification is a separate identity pass.
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + emitted code
// bytes are load-bearing (campaign doctrine).

#include <Ints.h>
#include <Wap32/Object.h>
#include <rva.h>

// The worker's +0x18 owned sub-object, torn down via its vtable slot 0 in Clear.
struct AnimWorkerKillable {
    virtual void Destroy(i32 flags); // slot 0
};
SIZE_UNKNOWN(AnimWorkerKillable);

// The worker virtual interface. Slots laid out so the dispatched method lands
// at byte offset +0x24. Declarations only - never defined, so no ??_7 emitted.
class AnimWorker : public CObject {
public:
    virtual ~AnimWorker() OVERRIDE; // slot 1 (deleting dtor); slots 0/2/3/4 inherited from CObject
    virtual void Slot05_151d60();   // [5] 0x151d60
    virtual void IsValidImage();    // [6] 0x001c08
    virtual void Clear();           // [7] 0x151e70 = Clear (B_151e70)
    virtual void Slot08_151d70();   // [8] 0x151d70
    virtual i32 Vfunc24(i32 a1, i32 a3); // [9] 0x151e20 (Init)
};
SIZE_UNKNOWN(AnimWorker);
RELOC_VTBL(
    AnimWorker,
    0x001efb80
); // shares AnimWorkerObj vtable, COMDAT-folded (slot-fn RVAs match its vtable)

// The 0x17c-byte worker layout. Only the seeded offsets are load-bearing.
// Real polymorphic: `new AnimWorkerObj` makes cl auto-emit ??_7AnimWorkerObj
// (masks the retail vtable 0x5efb80) and stamp the vptr in the ctor - no manual
// vptr store (ALL-VTABLES mandate).
struct AnimWorkerObj : public AnimWorker {
    virtual ~AnimWorkerObj() OVERRIDE;            // slot 1  0x151d80
    virtual void Slot05_151d60() OVERRIDE;        // slot 5  0x151d60
    virtual void IsValidImage() OVERRIDE;         // slot 6  0x001c08
    virtual void Clear() OVERRIDE;                // slot 7  0x151e70
    virtual void Slot08_151d70() OVERRIDE;        // slot 8  0x151d70
    virtual i32 Vfunc24(i32 a1, i32 a3) OVERRIDE; // slot 9  0x151e20
    AnimWorkerObj() {}
    i32 m_04;                 // +0x04  = parent->m_1c
    i32 m_08;                 // +0x08  = 0
    i32 m_0c;                 // +0x0c  = parent->m_0c
    i32 m_10;                 // +0x10  = 0
    void* m_14;               // +0x14  = 0  owned buffer (RezFree'd in Clear)
    AnimWorkerKillable* m_18; // +0x18  = 0  owned sub-object (Destroy(1)'d in Clear)
    i32 m_1c;                 // +0x1c  = 0
    char m_pad20[0x170 - 0x20];
    i32 m_170; // +0x170 = 0
    i32 m_174; // +0x174 = 0
    i32 m_178; // +0x178 = 0
}; // size = 0x17c
SIZE(AnimWorkerObj, 0x17c);
VTBL(
    AnimWorkerObj,
    0x001efb80
); // ??_7AnimWorkerObj@@6B@ (10-slot vtable; the +0x7c/LogicRecord worker)

#endif // GRUNTZ_DDRAWMGR_ANIMWORKEROBJ_H
