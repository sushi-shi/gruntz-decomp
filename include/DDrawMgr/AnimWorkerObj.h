#ifndef GRUNTZ_DDRAWMGR_ANIMWORKEROBJ_H
#define GRUNTZ_DDRAWMGR_ANIMWORKEROBJ_H

// AnimWorkerObj.h - THE 0x17c-byte per-object logic/anim worker record (vtable
// ??_7AnimWorkerObj @0x1efb80 / VA 0x5efb80). ONE class: the 2026-07-13 worker
//   CLogicRecord      (LogicRecord.h)       - the kill-cue/serialize record API
//   CGameObjAux       (UserLogic.h)         - the gameplay "+0x7c aux" view
//   CDDrawChildWorker (DDrawChildGroup.h)   - the walk-broadcast callback view
//   CWwdWorker        (WwdWorker.h)         - the factory Ctor/Kick view (Kick was
//                                             a WRONG vtbl-dispatch shape: retail
//                                             fires the +0x10 FN POINTER)
//   CWwdNotifier      (WwdFactoryObject.h)  - the +0x80/+0x88 notifier view
//   WwdAnimWorkerInit (WwdGameObjCtor.h)    - the ctor-inline-construction view
//   CSpriteInner      (Grunt.h)             - GruntObjEntry's +0x7c inner view
// Same 0x17c size, same vtable, same +0x10 callback + +0x18 logic leaf in every
// view; the merged layout below carries the union of the proven field knowledge.
//
// Every game object owns one at +0x7c (built by the CWwdObjMgr factories /
// CWwdGameObj ctor 0x15b390) and lazily builds three more at +0x80/+0x88/+0x90
// (EnsureWorker80/88/90 - the Hit/Attack/Bump logic handlers).
//
// Field names are placeholders (m_<hexoffset>) where the role is unproven; only
// the OFFSETS + emitted code bytes are load-bearing (campaign doctrine).

#include <Ints.h>
#include <Wap32/Object.h>
#include <rva.h>

// The worker's +0x18 owned sub-object IS the bound logic leaf - a CUserBase/
// CUserLogic (<Gruntz/UserLogic.h>). Its "slot-0 Destroy(1)" teardown is the
// CUserBase virtual scalar-deleting dtor (slot 0), i.e. plain `delete`; its
// "slot-1 Step(a,mode,c,d)" is CUserBase::SerializeMove. The former
class CUserLogic;
class CFileMemBase; // the serialize stream (CSerialArchive == CFileMemBase)
typedef CFileMemBase CSerialArchive;
struct CGameObject; // the owning wide game object (<Gruntz/UserLogic.h>)

// The worker's fire/notify callback held at +0x10 - a plain __cdecl fn ptr
// taking the OWNING game object, dispatched as `mov edx,[w+0x10]; push obj;
// call edx; add esp,4` (NEVER a vtable slot - proven at 0x151150 Play /
// 0x151c00 WriteSnapshot / 0x159250 the C factory / 0x15f0xx BroadPhase).
// Fired on three occasions, same slot each time: post-create activation (the
// creators' `spr->m_7c->m_notify(spr)`), the play-state dance (m_1c = 0x50..
// 0x53 around the call), and kill-cue expiry (CWwdObjMgr::TickKillCues).
// Zero = "no callback".
typedef i32(__cdecl* GameObjNotifyFn)(CGameObject* obj);

// m_0c owner context IS the CDDrawSurfaceMgr (== the owning object's m_0c, now
// typed in <Gruntz/UserLogic.h>). Its "+0x08 grid/key-table with the id->object
// resolver map at +0x48" is the canonical m_childGroup (CDDrawChildGroup) with
// its m_map48 (CMapPtrToPtr, Lookup @0x1b8760) - the SAME map Play's case 8
// reaches. LogicRecord.cpp reads the typed path.)
class CDDrawSurfaceMgr;

// Real polymorphic: `new AnimWorkerObj` makes cl auto-emit ??_7AnimWorkerObj
// (masks the retail vtable 0x5efb80) and stamp the vptr in the ctor - no manual
// vptr store (ALL-VTABLES mandate).
struct AnimWorkerObj : public CObject {
    // slot 1 deleting dtor ??_G @0x151d80; body @0x151da0 (was ~CLogicRecord):
    // free the m_14 payload, `delete` the bound logic leaf, zero the live fields.
    virtual ~AnimWorkerObj() OVERRIDE; // 0x151da0 (/GX; slots 0/2/3/4 CObject)
    virtual void Slot05_151d60();      // slot 5  0x151d60
    virtual void IsValidImage();       // slot 6  0x001c08
    virtual void Clear();              // slot 7  0x151e70 (reset/reuse hook)
    virtual void Slot08_151d70();      // slot 8  0x151d70
    // slot 9 - bind the fire callback + frame stamp, zero the working fields
    // (was BOTH "Vfunc24(i32,i32)" and "CLogicRecord::Init" - one body 0x151e20).
    virtual i32 Init(GameObjNotifyFn callback, i32 frame); // slot 9  0x151e20

    AnimWorkerObj() {}
    // The full 3-arg seed ctor (0x15b300, out-of-line in WwdFactoryObject.cpp;
    // the CWwdObjMgr factories construct through it): m_04=b, m_08=c, m_0c=a,
    // zero the rest. The arg-store order (b,c,a) is load-bearing.
    AnimWorkerObj(i32 a, i32 b, i32 c);
    // The inline 2-arg construction the 0x15b390 game-object ctor folds (was the
    // WwdAnimWorkerInit view): same stores with m_08 = 0.
    AnimWorkerObj(i32 a, i32 b) {
        m_04 = b;
        m_08 = 0;
        m_0c = (CDDrawSurfaceMgr*)a; // (mangling-pinned i32 arg; a IS the mgr)
        m_notify = 0;
        m_14 = 0;
        m_logic = 0;
        m_170 = 0;
        m_1c = 0;
        m_174 = 0;
        m_178 = 0;
    }

    // --- the record's runtime/IO method set (bodies: WwdGameObject.cpp /
    // WwdFactoryObject.cpp / DDrawMgr/LogicRecord.cpp) ---
    i32 Consume(i32 amount);                         // 0x15b340 (kill-cue budget m_20)
    i32 Dispatch(i32 a, i32 mode, void* c, void* d); // 0x164830
    i32 CacheTargetId(void* a);                      // 0x164920 (Dispatch case 3)
    i32 Save(CSerialArchive* ar);                    // 0x164960 (writes, slot 12 +0x30)
    i32 Load(CSerialArchive* ar);                    // 0x164d80 (reads, slot 11 +0x2c;
                                                     //   allocates the m_14 payload)
    i32 ResolveTarget(void* a);                      // 0x1651b0 (Dispatch case 8)

    i32 m_04;           // +0x04  = owner->m_04 (object id/kind)
    i32 m_08;           // +0x08  frame stamp (Init); bits 1/2 fold into owner
                        //        flags 0x800000/0x1000000 (Setup 0x150d60)
    CDDrawSurfaceMgr* m_0c; // +0x0c  = owner->m_0c (the owner/world context; the
                            //         id->object resolver is m_childGroup->m_map48)
    // +0x10  the fire/notify callback (see typedef). ALIAS: this is also the
    // collision-notify (ex-name m_collideNotify): CGameLevel::BroadPhase fires
    // `obj->m_collideWorker->m_notify(obj)` - a raw fn-ptr load off the worker,
    // NOT a vtable dispatch; zero-stamped at worker build = "no callback".
    GameObjNotifyFn m_notify;
    void* m_14;                // +0x14  owned serialized payload (RezFree'd in Clear/dtor)
    CUserLogic* m_logic;       // +0x18  the owned bound-logic leaf (CUserBase slot-0
                               //        scalar dtor via plain `delete`; slot-1
                               //        SerializeMove is the per-frame Step)
    void* m_1c;                // +0x1c  a genuine int|ptr role-union (no union per the
                               //        toolchain, kept void* with casts at the int sites):
                               //        the record/play state tag (0 = unbuilt, 0x1d/0x1e +
                               //        0x50..0x53 = the play-state dance keys, 0x3e8 =
                               //        built/idle, 0x1c = error latch) AND the bute-tree
                               //        animset node the eyecandy leaves save/restore.
    i32 m_20;                  // +0x20  kill-cue remaining budget (Consume debits it)
    i32 m_24;                  // +0x24  kill-cue refcount (TickKillCues decrements)
    i32 m_28;                  // +0x28  (zeroed by Init)
    i32 m_2c;                  // +0x2c  spawn-record param A (PlaceStartGruntz/AddGrunt arg 11)
    i32 m_30;                  // +0x30  spawn-record param B (AddGrunt arg 12)
    i32 m_34;                  // +0x34  (zeroed by Init)
    i32 m_38;                  // +0x38  (zeroed by Init)
    char m_pad3c[0x4c - 0x3c]; // +0x3c  flat serialized state (Save/Load stream it)
    i32 m_scrollTargetX;       // +0x4c  demo auto-scroll per-axis target (DemoAutoScrollStep,
    i32 m_scrollTargetY;       // +0x50  Demo.cpp; part of the flat serialized band)
    char m_pad54[0xbc - 0x54]; // +0x54  flat serialized state (continued)
    i32 m_bc;                  // +0xbc  per-tile time (teleporter reads the bound clock here;
                               //        rolling-ball speed in LoadGruntAbilityTuning)
    char m_padc0[0xf0 - 0xc0];
    // +0xf0/+0x100: two 4-dword L/T/R/B rect quads the tile-switch registrar takes
    // BY VALUE (CPlay::ValidateLevelTiles pushes both, 16 bytes each, into every
    // RegisterSwitchLogic call).
    i32 m_f0, m_f4, m_f8, m_fc;     // +0xf0
    i32 m_100, m_104, m_108, m_10c; // +0x100
    char m_pad110[0x130 - 0x110];
    i32 m_130; // +0x130
    char m_pad134[0x168 - 0x134];
    i32 m_168;          // +0x168 (zeroed by Init)
    i32 m_16c;          // +0x16c (zeroed by Init)
    CGameObject* m_170; // +0x170  resolved target object (ResolveTarget; id = its m_188)
    i32 m_174;          // +0x174  cached target id (from m_170->m_188)
    u32 m_178;          // +0x178  payload byte count for the m_14 block
}; // size = 0x17c
SIZE(AnimWorkerObj, 0x17c);
VTBL(AnimWorkerObj,
     0x001efb80); // ??_7AnimWorkerObj@@6B@ (10-slot vtable; the +0x7c worker/record)

#endif // GRUNTZ_DDRAWMGR_ANIMWORKEROBJ_H
