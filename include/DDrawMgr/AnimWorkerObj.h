#ifndef GRUNTZ_DDRAWMGR_ANIMWORKEROBJ_H
#define GRUNTZ_DDRAWMGR_ANIMWORKEROBJ_H

#include <Ints.h>
#include <Gruntz/Loadable.h> // CLoadable : CWapObj : CObject - the real 9-slot base
#include <rva.h>

class CUserLogic;
class CFileMemBase; // the serialize stream (CFileMemBase == CFileMemBase)

struct CGameObject; // the owning wide game object (<Gruntz/UserLogic.h>)

typedef i32(__cdecl* GameObjNotifyFn)(CGameObject* obj);

class CDDrawSurfaceMgr;

// BASE CORRECTED 2026-07-28: `: CWapObj` -> `: CLoadable`, byte-proven by the dtor.
// ~AnimWorkerObj @0x151da0 ends with `mov [esi+8],0 / mov [esi+0xc],0 /
// mov [esi+4],-1 / mov [esi],0x5e8cb4` (0x151e00-0x151e0d) - that is ~CLoadable's
// body verbatim, in its order, followed by the CWapObj/CObject grand-base restamp,
// i.e. the base dtor chained and inlined. The header trio and the slot scheme agree:
// +0x04/+0x08/+0x0c ARE m_id/m_flags/m_ownerCtx (the 3-arg ctor stores the same
// (id, stateFlags, owner) triple CLoadable's out-of-line ctor @0x156cb0 stores), and
// slots 5/6/7/8 are exactly CLoadable's IsLoaded / inherited IsReady / Unload /
// GetClassId, with only slot 9 Init new. The old `: CWapObj` forced all four to be
// declared fresh and the three dtor resets to be hand-spelled.
struct AnimWorkerObj : public CLoadable {
    // slot 1 deleting dtor ??_G @0x151d80; body @0x151da0 (was ~CLogicRecord):
    // free the m_payload blob, `delete` the bound logic leaf, zero the live fields.
    virtual ~AnimWorkerObj() OVERRIDE; // 0x151da0 (/GX; slots 0/2/3/4 CObject)
    virtual i32 IsLoaded() OVERRIDE;   // slot 5  0x151d60 (overrides CLoadable)
    // slot 6 IsReady INHERITED from CWapObj (its `return 1` default @0xd5da0, reached
    // via the 0x001c08 thunk); not redeclared (that was a phantom own "IsValidImage").
    // slot 7 - the family's Unload hook. Read as "Clear" here before the rebase: it
    // frees the payload blob, deletes the bound logic leaf and zeroes the live fields
    // so the record can be reused; that IS CLoadable's reset/unload contract.
    virtual void Unload() OVERRIDE;    // slot 7  0x151e70
    virtual i32 GetClassId() OVERRIDE; // slot 8  0x151d70 (CLASSID_ANIMWORKER)
    // slot 9 - bind the fire callback + frame stamp, zero the working fields
    // (was BOTH "Vfunc24(i32,i32)" and "CLogicRecord::Init" - one body 0x151e20).
    virtual i32 Init(GameObjNotifyFn callback, i32 frame); // slot 9  0x151e20

    AnimWorkerObj() {}
    // The full 3-arg seed ctor (0x15b300, out-of-line in WwdFactoryObject.cpp; the
    // CDDrawChildGroup factories construct through it): m_04=id, m_08=stateFlags,
    // m_0c=owner, zero the rest. The arg-STORE order (id, stateFlags, owner) is
    // load-bearing.
    //
    // All three args are typed/named from the slots they land in, not from the
    // creation chain's i32 spelling. `owner` IS the owning CDDrawSurfaceMgr (the
    // factories now pass CLoadable::OwnerMgr() - the ONE seam where the proven-
    // heterogeneous +0x0c handle becomes the draw family's type - instead of the raw
    // i32 each ctor re-cast). `id` and `stateFlags` are the SAME pair CLoadable
    // names m_id (+0x04 per-child id / liveness latch) and m_flags (+0x08 the
    // collision/state flag word): the object factories hand this ctor and the
    // CLoadable/CResolveNode base ctor the identical (owner, a1, flags) triple.
    AnimWorkerObj(CDDrawSurfaceMgr* owner, i32 id, i32 stateFlags);
    // The inline 2-arg construction the 0x15b390 game-object ctor folds (was the
    // WwdAnimWorkerInit view): same stores with m_08 = 0.
    // The base trio goes through CLoadable's ctor, not through body assignments:
    // retail's inlined copies (EnsureWorker80/88/90 @0x150eb0/f90/0x151070, and
    // CGameObject::CGameObject @0x15b390) all store m_id/m_flags/m_ownerCtx BEFORE the
    // ??_7AnimWorkerObj@@6B@ stamp, and the vptr stamp always splits the base/
    // mem-init half from the ctor body (docs/patterns/vptr-stamp-splits-meminit-
    // from-body.md). Spelling the trio in the body puts the stamp first instead.
    AnimWorkerObj(CDDrawSurfaceMgr* owner, i32 id) : CLoadable(owner, id) {
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
    i32 Consume(i32 amount); // 0x15b340 (kill-cue budget m_20)
    // arg3 is the i32 move/mode payload (it went straight into SerializeMove's i32).
    i32 Dispatch(CFileMemBase* a, i32 mode, i32 c, void* d); // 0x164830
    i32 CacheTargetId(void* a);                              // 0x164920 (Dispatch case 3)
    i32 Save(CFileMemBase* ar);                              // 0x164960 (writes, slot 12 +0x30)
    i32 Load(CFileMemBase* ar);                              // 0x164d80 (reads, slot 11 +0x2c;
                                                             //   allocates the m_payload blob)
    i32 ResolveTarget(void* a);                              // 0x1651b0 (Dispatch case 8)

    // +0x04/+0x08/+0x0c are CLoadable's m_id / m_flags / m_ownerCtx. What this class
    // adds to their reading: m_id is copied from the owning game object's own m_id
    // (the object id/kind); m_flags is the frame stamp Init writes, whose bits 1/2
    // fold into the owner's flags 0x800000/0x1000000 (Setup 0x150d60); m_ownerCtx is
    // copied from the owning game object's m_ownerCtx (the world root - the
    // id->object resolver is m_childGroup->m_map48).
    // +0x10  the fire/notify callback (see typedef). ALIAS: this is also the
    // collision-notify (ex-name m_collideNotify): CGameLevel::BroadPhase fires
    // `obj->m_collideWorker->m_notify(obj)` - a raw fn-ptr load off the worker,
    // NOT a vtable dispatch; zero-stamped at worker build = "no callback".
    GameObjNotifyFn m_notify;
    u8* m_payload;       // +0x14  owned serialized payload blob (RezFree'd in Clear/dtor)
    CUserLogic* m_logic; // +0x18  the owned bound-logic leaf (CUserBase slot-0
                         //        scalar dtor via plain `delete`; slot-1
                         //        SerializeMove is the per-frame Step)
    i32 ActKey() const {
        return m_1c;
    }
    void SetActKey(i32 id) {
        m_1c = id;
    }
    i32 m_1c; // +0x1c  the act/anim-set ID: the record/play state tag
              //        (0 = unbuilt, 0x1d/0x1e + 0x50..0x53 = the
              //        play-state dance keys, 0x3e8 = built/idle,
              //        0x1c = error latch) or a g_buteTree act id
              //        (ActFindId). Never dereferenced - the bute tree
              //        just parks small ints in its void* value slot.
    i32 m_20; // +0x20  kill-cue remaining budget (Consume debits it)
    i32 m_24; // +0x24  kill-cue refcount (TickKillCues decrements)
    i32 m_28; // +0x28  (zeroed by Init)
    i32 m_2c; // +0x2c  spawn-record param A / projectile lo bound A (0 => default MIN)
    i32 m_30; // +0x30  spawn-record param B / projectile hi bound A (0 => default MAX)
    i32 m_34; // +0x34  projectile lo bound B (zeroed by Init)
    i32 m_38; // +0x38  projectile hi bound B (zeroed by Init)
    char m_pad3c[0x40 - 0x3c];
    i32 m_40;            // +0x040  (serialized)
    i32 m_44;            // +0x044  (serialized)
    i32 m_48;            // +0x048  (serialized)
    i32 m_scrollTargetX; // +0x4c  demo auto-scroll per-axis target (DemoAutoScrollStep,
    i32 m_scrollTargetY; // +0x50  Demo.cpp; part of the flat serialized band)
    char m_pad54[0x58 - 0x54];
    i32 m_58; // +0x058  (serialized)
    i32 m_5c; // +0x05c  (serialized)
    i32 m_60; // +0x060  (serialized)
    i32 m_64; // +0x064  (serialized)
    i32 m_68; // +0x068  (serialized)
    i32 m_6c; // +0x06c  (serialized)
    i32 m_70; // +0x070  (serialized)
    i32 m_74; // +0x074  (serialized)
    i32 m_78; // +0x078  (serialized)
    i32 m_7c; // +0x07c  (serialized)
    i32 m_80; // +0x080  (serialized)
    i32 m_84; // +0x084  (serialized)
    i32 m_88; // +0x088  (serialized)
    i32 m_8c; // +0x08c  (serialized)
    i32 m_90; // +0x090  (serialized)
    i32 m_94; // +0x094  (serialized)
    i32 m_98; // +0x098  (serialized)
    i32 m_9c; // +0x09c  (serialized)
    i32 m_a0; // +0x0a0  (serialized)
    i32 m_a4; // +0x0a4  (serialized)
    i32 m_a8; // +0x0a8  (serialized)
    i32 m_ac; // +0x0ac  (serialized)
    i32 m_b0; // +0x0b0  (serialized)
    i32 m_b4; // +0x0b4  (serialized)
    i32 m_b8; // +0x0b8  (serialized)
    i32 m_bc; // +0xbc  per-tile time (teleporter reads the bound clock here;
              //        rolling-ball speed in LoadGruntAbilityTuning)
    char m_padc0[0xc4 - 0xc0];
    i32 m_c4;  // +0x0c4  (serialized)
    i32 m_c8;  // +0x0c8  (serialized)
    i32 m_cc;  // +0x0cc  (serialized)
    RECT m_d0; // +0x0d0  (serialized, 0x10 B)
    RECT m_e0; // +0x0e0  (serialized, 0x10 B)
    // +0xf0/+0x100: the two REAL RECTs the tile-switch registrar takes BY VALUE
    // (CPlay::ValidateLevelTiles pushes both, 16 bytes each, into every
    // RegisterSwitchLogic call).
    RECT m_switchRectA; // +0xf0
    RECT m_switchRectB; // +0x100
    char m_pad110[0x120 - 0x110];
    RECT m_120; // +0x120  (serialized, 0x10 B)
    i32 m_130;  // +0x130
    char m_pad134[0x138 - 0x134];
    i32 m_138;             // +0x138  (serialized)
    i32 m_13c;             // +0x13c  (serialized)
    i32 m_140;             // +0x140  (serialized)
    i32 m_144;             // +0x144  (serialized)
    i32 m_148;             // +0x148  (serialized)
    i32 m_14c;             // +0x14c  (serialized)
    i32 m_150;             // +0x150  (serialized)
    i32 m_154;             // +0x154  (serialized)
    i32 m_158;             // +0x158  (serialized)
    i32 m_15c;             // +0x15c  (serialized)
    i32 m_160;             // +0x160  (serialized)
    i32 m_164;             // +0x164  (serialized)
    i32 m_168;             // +0x168 (zeroed by Init)
    i32 m_16c;             // +0x16c (zeroed by Init)
    CGameObject* m_target; // +0x170  resolved target object (ResolveTarget; id = its m_188)
    i32 m_targetId;        // +0x174  cached target id (from m_target->m_188)
    u32 m_payloadSize;     // +0x178  payload byte count for the m_payload block
}; // size = 0x17c
SIZE(0x17c);
VTBL(AnimWorkerObj, 0x001efb80); // ??_7AnimWorkerObj@@6B@ (10-slot vtable; the +0x7c worker/record)

#endif // GRUNTZ_DDRAWMGR_ANIMWORKEROBJ_H
