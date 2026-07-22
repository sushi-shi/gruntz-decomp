#ifndef GRUNTZ_DDRAWMGR_ANIMWORKEROBJ_H
#define GRUNTZ_DDRAWMGR_ANIMWORKEROBJ_H

#include <Ints.h>
#include <Wap32/WapObj.h> // CWapObj : CObject - slots 5/6 (IsLoaded/IsReady default)
#include <rva.h>

class CUserLogic;
class CFileMemBase; // the serialize stream (CFileMemBase == CFileMemBase)

struct CGameObject; // the owning wide game object (<Gruntz/UserLogic.h>)

typedef i32(__cdecl* GameObjNotifyFn)(CGameObject* obj);

class CDDrawSurfaceMgr;

struct AnimWorkerObj : public CWapObj {
    // slot 1 deleting dtor ??_G @0x151d80; body @0x151da0 (was ~CLogicRecord):
    // free the m_payload blob, `delete` the bound logic leaf, zero the live fields.
    virtual ~AnimWorkerObj() OVERRIDE; // 0x151da0 (/GX; slots 0/2/3/4 CObject)
    virtual i32 IsLoaded() OVERRIDE;   // slot 5  0x151d60 (overrides CWapObj)
    // slot 6 IsReady INHERITED from CWapObj (its `return 1` default @0xd5da0, reached
    // via the 0x001c08 thunk); not redeclared (that was a phantom own "IsValidImage").
    virtual void Clear();              // slot 7  0x151e70 (reset/reuse hook)
    virtual void GetClassId();      // slot 8  0x151d70
    // slot 9 - bind the fire callback + frame stamp, zero the working fields
    // (was BOTH "Vfunc24(i32,i32)" and "CLogicRecord::Init" - one body 0x151e20).
    virtual i32 Init(GameObjNotifyFn callback, i32 frame); // slot 9  0x151e20

    AnimWorkerObj() {}
    // The full 3-arg seed ctor (0x15b300, out-of-line in WwdFactoryObject.cpp;
    // the CDDrawChildGroup factories construct through it): m_04=b, m_08=c, m_0c=a,
    // zero the rest. The arg-store order (b,c,a) is load-bearing.
    AnimWorkerObj(i32 a, i32 b, i32 c);
    // The inline 2-arg construction the 0x15b390 game-object ctor folds (was the
    // WwdAnimWorkerInit view): same stores with m_08 = 0.
    AnimWorkerObj(i32 a, i32 b) {
        m_04 = b;
        m_08 = 0;
        m_0c = reinterpret_cast<CDDrawSurfaceMgr*>(a); // (mangling-pinned i32 arg; a IS the mgr)
        m_notify = 0;
        m_payload = 0;
        m_logic = 0;
        m_target = 0;
        m_1c = 0;
        m_targetId = 0;
        m_payloadSize = 0;
    }

    // --- the record's runtime/IO method set (bodies: WwdGameObject.cpp /
    // WwdFactoryObject.cpp / DDrawMgr/LogicRecord.cpp) ---
    i32 Consume(i32 amount);                         // 0x15b340 (kill-cue budget m_20)
    i32 Dispatch(i32 a, i32 mode, void* c, void* d); // 0x164830
    i32 CacheTargetId(void* a);                      // 0x164920 (Dispatch case 3)
    i32 Save(CFileMemBase* ar);                    // 0x164960 (writes, slot 12 +0x30)
    i32 Load(CFileMemBase* ar);                    // 0x164d80 (reads, slot 11 +0x2c;
                                                     //   allocates the m_payload blob)
    i32 ResolveTarget(void* a);                      // 0x1651b0 (Dispatch case 8)

    i32 m_04;               // +0x04  = owner->m_04 (object id/kind)
    i32 m_08;               // +0x08  frame stamp (Init); bits 1/2 fold into owner
                            //        flags 0x800000/0x1000000 (Setup 0x150d60)
    CDDrawSurfaceMgr* m_0c; // +0x0c  = owner->m_0c (the owner/world context; the
                            //         id->object resolver is m_childGroup->m_map48)
    // +0x10  the fire/notify callback (see typedef). ALIAS: this is also the
    // collision-notify (ex-name m_collideNotify): CGameLevel::BroadPhase fires
    // `obj->m_collideWorker->m_notify(obj)` - a raw fn-ptr load off the worker,
    // NOT a vtable dispatch; zero-stamped at worker build = "no callback".
    GameObjNotifyFn m_notify;
    u8* m_payload;                  // +0x14  owned serialized payload blob (RezFree'd in Clear/dtor)
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
    i32 m_2c;                  // +0x2c  spawn-record param A / projectile lo bound A (0 => default MIN)
    i32 m_30;                  // +0x30  spawn-record param B / projectile hi bound A (0 => default MAX)
    i32 m_34;                  // +0x34  projectile lo bound B (zeroed by Init)
    i32 m_38;                  // +0x38  projectile hi bound B (zeroed by Init)
    char m_pad3c[0x4c - 0x3c]; // +0x3c  flat serialized state (Save/Load stream it)
    i32 m_scrollTargetX;       // +0x4c  demo auto-scroll per-axis target (DemoAutoScrollStep,
    i32 m_scrollTargetY;       // +0x50  Demo.cpp; part of the flat serialized band)
    char m_pad54[0xbc - 0x54]; // +0x54  flat serialized state (continued)
    i32 m_bc;                  // +0xbc  per-tile time (teleporter reads the bound clock here;
                               //        rolling-ball speed in LoadGruntAbilityTuning)
    char m_padc0[0xf0 - 0xc0];
    // +0xf0/+0x100: the two REAL RECTs the tile-switch registrar takes BY VALUE
    // (CPlay::ValidateLevelTiles pushes both, 16 bytes each, into every
    // RegisterSwitchLogic call).
    RECT m_switchRectA; // +0xf0
    RECT m_switchRectB; // +0x100
    char m_pad110[0x130 - 0x110];
    i32 m_130; // +0x130
    char m_pad134[0x168 - 0x134];
    i32 m_168;          // +0x168 (zeroed by Init)
    i32 m_16c;          // +0x16c (zeroed by Init)
    CGameObject* m_target; // +0x170  resolved target object (ResolveTarget; id = its m_188)
    i32 m_targetId;          // +0x174  cached target id (from m_target->m_188)
    u32 m_payloadSize;          // +0x178  payload byte count for the m_payload block
}; // size = 0x17c
SIZE(0x17c);
VTBL(AnimWorkerObj, 0x001efb80); // ??_7AnimWorkerObj@@6B@ (10-slot vtable; the +0x7c worker/record)

#endif // GRUNTZ_DDRAWMGR_ANIMWORKEROBJ_H
