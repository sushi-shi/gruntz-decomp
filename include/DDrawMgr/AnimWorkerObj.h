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

// The 0x17c-byte worker layout. Only the seeded offsets are load-bearing.
// Real polymorphic: `new AnimWorkerObj` makes cl auto-emit ??_7AnimWorkerObj
// (masks the retail vtable 0x5efb80) and stamp the vptr in the ctor - no manual
// vptr store (ALL-VTABLES mandate). ONE class: the former declared-only
// `AnimWorker` dispatch-interface base (a fake intermediate whose implicit-ctor
// ??_7AnimWorker aliased this same vtable, RELOC_VTBL) and the former `WorkerFull`
// view (<Gruntz/AnimWorkerFull.h>, the 3-arg seed ctor @0x15b300) are BOTH folded
// in here - retail has a single class with the single vtable 0x5efb80.
struct AnimWorkerObj : public CObject {
    virtual ~AnimWorkerObj() OVERRIDE;   // slot 1  0x151d80 (deleting dtor; slots 0/2/3/4 CObject)
    virtual void Slot05_151d60();        // slot 5  0x151d60
    virtual void IsValidImage();         // slot 6  0x001c08
    virtual void Clear();                // slot 7  0x151e70
    virtual void Slot08_151d70();        // slot 8  0x151d70
    virtual i32 Vfunc24(i32 a1, i32 a3); // slot 9  0x151e20 (Init)
    AnimWorkerObj() {}
    // The full 3-arg seed ctor (0x15b300, WwdFactoryObject.cpp): m_04=b, m_08=c,
    // m_0c=a, zero the rest (was the WorkerFull view's ctor).
    AnimWorkerObj(i32 a, i32 b, i32 c);
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
