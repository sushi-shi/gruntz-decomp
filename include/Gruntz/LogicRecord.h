// LogicRecord.h - CLogicRecord, a CObject-derived ~0x17c-byte serialized "logic
// record" owned by the per-grunt logic class (the AddLogicHit/AddLogicAttack/
// AddLogicBump family at 0x150f50/0x151030/0x151110 store the new'd record at
// owner+0x80). The record carries a flat block of replayable state (offsets
// 0x10..0x178) read/written through a stream object whose vtable exposes
// Write@slot 0x2c and Read@slot 0x30 (CArchive/IStream-like). Non-RTTI vtable
// (0x1efb80) - the class has plain virtuals on CObject but no DECLARE_DYNAMIC,
// so the name is a reconstruction placeholder; only offsets + code bytes are
// load-bearing.
//
// IDENTITY: this 0x17c-byte class IS the "AnimWorkerObj" worker modeled in
// src/DDrawMgr/DDrawWorkerCache.cpp - same size, same most-derived vtable 0x1efb80
// (cl-emitted there as ??_7AnimWorkerObj via VTBL(), so the same RVA can't carry
// two names). The two coexist as a REQUIRED dual-view (vtable-realization-ctor-
// boundary): the CDDrawWorkerCache factory needs the real-polymorphic form so cl
// emits the ??_7 + implicit ctor vptr stamp, while this @early-stop /GX dtor still
// needs the manual-vptr non-polymorphic form (its frame needs the real base
// subobject). Unify into one shared class in the final vtable-reunification sweep,
// once ~CLogicRecord can be a real polymorphic dtor.
#ifndef GRUNTZ_LOGICRECORD_H
#define GRUNTZ_LOGICRECORD_H

#include <rva.h>
#include <Ints.h>

// The serializer the record reads/writes itself through. Its vtable exposes the
// archive primitives at fixed slots (Write@0x2c == slot 11, Read@0x30 == slot
// 12); modeled as a polymorphic class with the leading slots as dummy virtuals
// so `ar->Read(...)` lowers to `mov edx,[ar]; call [edx+0x30]` (__thiscall, no
// cleanup). External (engine) - no bodies emitted here.
SIZE_UNKNOWN(LogicArchive);
class LogicArchive {
public:
    virtual void Slot00();
    virtual void Slot04();
    virtual void Slot08();
    virtual void Slot0c();
    virtual void Slot10();
    virtual void Slot14();
    virtual void Slot18();
    virtual void Slot1c();
    virtual void Slot20();
    virtual void Slot24();
    virtual void Slot28();
    virtual void Write(void* buf, u32 cb); // slot 0x2c
    virtual void Read(void* buf, u32 cb);  // slot 0x30
};

// The polymorphic sub-record held at m_18: virtual slot 0 is the (scalar-
// deleting) destructor, slot 1 a per-frame step. External.
SIZE_UNKNOWN(LogicSub);
class LogicSub {
public:
    virtual void Destroy(u32 flags);                     // slot 0x00
    virtual i32 Step(i32 a, i32 mode, void* c, void* d); // slot 0x04
};

struct LogicContext; // owner context held at m_0c (defined below)

SIZE_UNKNOWN(CLogicRecord);
class CLogicRecord {
public:
    // Non-polymorphic model: the retail vtable (0x5efb80) lives in other TUs, so
    // we stamp m_vptr manually (reloc-masked DATA() store) rather than declare
    // virtuals - declaring them would make MSVC emit a divergent ??_7 vtable.
    ~CLogicRecord(); // 0x151da0 (/GX)

    i32 Init(void(__cdecl* callback)(void*), i32 frame); // 0x151e20 (binds m_10)
    i32 Consume(i32 amount);                             // 0x15b340
    i32 Dispatch(i32 a, i32 mode, void* c, void* d);     // 0x164830
    i32 CacheTargetId(void* a);                          // 0x164920 (Dispatch case 3)
    i32 Load(LogicArchive* ar);                          // 0x164960
    i32 Save(LogicArchive* ar);                          // 0x164d80 (external here)
    i32 ResolveTarget(void* a);                          // 0x1651b0 (Dispatch case 8)

    // --- layout (offsets confirmed from ctor 0x150eb0 + Load 0x164960) ---
    char _vft0[4];      // +0x00 foreign object vptr (reduced view; not owned/dispatched)
    i32 m_04;           // 0x04
    i32 m_08;           // 0x08  set by Init (frame), copied from owner+0x0c in ctor
    LogicContext* m_0c; // 0x0c  owner context (grid resolver at m_08+0x48)
    // +0x10  the record's fire callback - a plain __cdecl fn ptr taking the owning
    // object. PROVEN by its two live consumers: CWwdObjMgr::TickKillCues /
    // CSpriteFactory::AttachSprite call it directly (`call [rec+0x10]`), and the
    // CAnimWorker view of this same 0x17c object types the same slot m_collideNotify
    // (CGameLevel::BroadPhase fires it). Init's stored arg IS this callback; the
    // dtor zeroes it. (Was `void* m_10` - the untyped slot forced (KillCueFn) casts
    // at every fire site.)
    void(__cdecl* m_10)(void* owner);
    void* m_14;                           // 0x14  owned heap block (freed in dtor)
    LogicSub* m_18;                       // 0x18  owned polymorphic sub-record (destroyed in dtor)
    i32 m_1c;                             // 0x1c  state/type tag (switch key)
    i32 m_serial[(0x164 - 0x20) / 4 + 1]; // 0x20..0x164 flat serialized block
    i32 m_168;                            // 0x168 (zeroed by Init)
    i32 m_16c;                            // 0x16c (zeroed by Init)
    void* m_170;                          // 0x170 a resolved target object (has +0x188)
    i32 m_174;                            // 0x174 cached value pulled from m_170+0x188
    u32 m_178;                            // 0x178 payload byte count for the m_14 block
};

// m_170 points at an object exposing the cached value at +0x188.
SIZE_UNKNOWN(LogicTarget);
struct LogicTarget {
    char m_pad[0x188];
    i32 m_188;
};

// The resolver sub-object embedded at m_0c->m_08 + 0x48; its method
// (engine helper 0x1b8760, __thiscall) resolves a slot into an out-pointer and
// returns whether it succeeded.
SIZE_UNKNOWN(LogicResolver);
struct LogicResolver {
    i32 Resolve(void* slot, void** out); // 0x1b8760
};
SIZE_UNKNOWN(LogicContext);
struct LogicContext {
    void* m_00;
    void* m_04;
    char* m_08; // grid object; the resolver sits at +0x48
};

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

#endif // GRUNTZ_LOGICRECORD_H
