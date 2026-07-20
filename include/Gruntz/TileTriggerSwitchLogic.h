#ifndef SRC_GRUNTZ_TILETRIGGERSWITCHLOGIC_H
#define SRC_GRUNTZ_TILETRIGGERSWITCHLOGIC_H

#include <Ints.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/SerialArchive.h> // the shared CSerialArchive stream (Read @ +0x2c / Write @ +0x30)
#include <rva.h>                  // SIZE_UNKNOWN class-metadata macros used below

class CTileTriggerLogic;     // the SEPARATE 0x9c logic family (not a base: 0x8c < 0x9c)
class CTileTriggerContainer; // the owning 4-list container (m_owner back-pointer)

class CTileTriggerSwitchLogic {
public:
    // The 4 retail vtable slots (0x5eae8c). Real virtuals now -> cl emits the
    // ??_7 vftable + the implicit ctor vptr-stamp (replaces the manual struct
    // stamp). Bodies live in unmatched engine TUs; declared-only here, named on
    // the target via deterministic @data-symbol/@rva-symbol in the .cpp.
    // slot 0 (thunk 0x1749) - the one-shot Setup virtual, body @0x1104f0. Its 8-arg
    // build signature is corroborated by CheckpointSwitchBuild.cpp's BaseBuild view.
    // arg0 is the owning CONTAINER (Setup stamps it into m_owner - the same object
    // LoadElement stamps there).
    virtual i32
    Setup(CTileTriggerContainer* owner, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7);
    // slot 1 -> 0x0022e8 (body in an unmatched engine TU). Its real signature is recovered
    // from the ONE reconstructed override, CCheckpointTriggerSwitchLogic::BuildSmall
    // (0x112a50, `sema class` says slot 1, origin CTileTriggerSwitchLogic): 9 args, returns
    // i32. It was declared `void Vf1()` here, which silently emitted the BASE slot into the
    // derived vtable instead of the override. arg0 chains straight into Setup's owner slot.
    virtual i32 BuildSmall(
        CTileTriggerContainer* owner,
        i32 a2,
        i32 a3,
        i32 a4,
        i32 a5,
        const i32* rect,
        i32 a7,
        i32 a8,
        i32 a9
    );
    virtual i32
    SwitchDown(); // slot 2 -> 0x002e0f (CTileTimeTriggerSwitchLogic overrides @0x112840)
    virtual i32
    SwitchUp(); // slot 3 -> 0x0037e2 (returns i32; base slot typed void in retail callers)

    CTileTriggerSwitchLogic();
    // Non-virtual dtor (the 4 vtable slots are all regular methods, no dtor slot).
    // A polymorphic class's dtor restamps the vptr, so an explicit call inlines
    // `mov [this],offset ??_7; mov [this+0x20],0` - exactly retail's inlined delete
    // in RemoveByKeys.
    ~CTileTriggerSwitchLogic() {
        m_20 = 0;
    }
    // FindIndexByKey (0x110820) is NOT a member: it does `add ecx,0x3c` then scans 24
    // dwords (-> this+0x3c..+0x9b), which overruns an 0x8c object. It is CTileTriggerLogic's
    // (m_block[24] @ +0x3c, size 0x9c) - see TileTriggerLogic.h. VerifyBlockLinks calls it on
    // the CHILD it then scans at child+0x3c, which pins the child to the 0x9c family.
    i32 VerifyBlockLinksB(); // 0x111f40 (FindChild(key, 3) variant)
    i32 VerifyBlockLinks();  // 0x112c70
    // Broadcast (0x112080): walk the m_block key array; for each key resolve the
    // sibling (m_owner->FindChild(key, 4)) and run every m_list1 logic child that
    // claims it. Was the .cpp-local `CGroupBroadcast` view in GroupOps.cpp (same
    // layout field-for-field: m_10=m_key1, m_14=m_linkGate, m_24=m_owner,
    // m_2c[24]=m_block; its RVA sits inside THIS TU's interval).
    i32 Broadcast(); // 0x112080

    // GetFlag74 / RemoveByKeys / FindChild / FindByField0C / ScanNeighborhood /
    // TransferFlag74 / LoadFlag74 are NOT members - they are CTileTriggerContainer
    // methods, misattributed here by the thiscall-only dynamic tracer. Proof:
    // ModeObjInit (0xc7ec0) CONSTRUCTS the receiver (t->m_2e4) as the container
    // (four in-place CPtrList ctors at +0x00/+0x1c/+0x38/+0x54 + [+0x74]=0) and
    // destroys it via ~CTileTriggerContainer; every walk they do is a container
    // list (m_base head @+0x04, m_list3 head @+0x58) or the m_74 gate.
    // See <Gruntz/TileTriggerContainer.h>.

    // The 0x8c family's save/load dispatcher (mode 4 = save -> SaveState, 7 = load
    // -> LoadState), the twin of CTileTriggerLogic::ValidateByType. __thiscall,
    // ret 0x10; this flows through in ecx untouched to the two state helpers -
    // which is why the old `__stdcall Gate113860(obj,...)` free-fn model scored
    // only ~93% (its callers dropped retail's `mov ecx,<element>`).
    i32 ValidateByType(CSerialArchive* s, i32 mode, i32 a3, i32 a4); // 0x113860
    i32 SaveState(CSerialArchive* s); // 0x1138b0 (write via slot +0x30; was the
                                      // "CTileTriggerData::LoadV4" view - same fields)
    i32 LoadState(CSerialArchive* s); // 0x1139a0 (read via slot +0x2c)
    // ValidateByType (0x113a90) / ApplyByType (0x113d40) / SerializeMatrix (0x113dd0) /
    // DeserializeMatrix (0x113e70) are NOT members - they were misattributed here. Retail's
    // CTileTriggerFactory::Build calls them on freshly-`new`ed objects of the OTHER family:
    // ApplyByType on a 0xc8 CGiantRockLogic (0x117b49: push 0xc8 / ??0CGiantRockLogic /
    // mov ecx,esi / call 0x1d39), ValidateByType on a 0x9c CTileTriggerLogic (0x117aa7).
    // They live in TileTriggerLogic.h now. SerializeMatrix reaches this+0xc0/+0xc4, which an
    // 0x8c object cannot hold - that overrun was the "m_block[37]/[38]" contradiction.

    // +0x00  implicit vptr (real virtuals above; was an explicit m_vptr struct stamp)
    i32 m_04;       // +0x04  type id (the factory switch id 1..8; LoadElement stamps it,
                    //        Setup seeds it; CTileTriggerContainer::FindChild matches on it)
    i32 m_08;       // +0x08  (serialized in LoadState)
    i32 m_key0c;    // +0x0c  secondary key
    i32 m_key1;     // +0x10  primary key (the container's FindChild/RemoveByKeys match key)
    i32 m_linkGate; // +0x14  link-check gate (VerifyBlockLinks guard)
    i32 m_18;       // +0x18  (serialized in LoadState)
    i32 m_1c;       // +0x1c  (serialized in LoadState)
    i32 m_20;       // +0x20  init gate (ctor + dtor zero it, Setup sets 1 - the 0x8c
                    //        family's twin of CTileTriggerLogic::m_1c; the old
                    //        "ChildNode* child-list head" reading belonged to the
                    //        CONTAINER's +0x20 = m_list1 head, not to this class)
    // +0x24  the owning CTileTriggerContainer. Settled (was a container-vs-switch-logic
    // attribution conflict): LoadElement (0x117800) stamps its own `this` here, and that
    // `this` is the object ModeObjInit builds with four in-place CPtrList ctors + [+0x74]=0
    // - the container, beyond doubt. VerifyBlockLinks' m_owner->m_20 walk reads the
    // container's m_list1 head (+0x20), whose elements are the 0x9c CTileTriggerLogic
    // children it scans at +0x3c..+0x9b.
    CTileTriggerContainer* m_owner;
    i32 m_28;        // +0x28  (serialized in LoadState)
    i32 m_block[24]; // +0x2c..0x8b  (the ctor zeroes exactly these 24; ends at 0x8c)
};
SIZE(CTileTriggerSwitchLogic, 0x8c);
VTBL(CTileTriggerSwitchLogic, 0x001eae8c); // vtable_names -> code (RTTI game class)

class CTileMultiTriggerSwitchLogic : public CTileTriggerSwitchLogic {
public:
    CTileMultiTriggerSwitchLogic(); // 0x111f10
};
SIZE(CTileMultiTriggerSwitchLogic, 0x8c);
VTBL(CTileMultiTriggerSwitchLogic, 0x001eaeb4);

class CTileExclusiveTriggerSwitchLogic : public CTileTriggerSwitchLogic {
    virtual i32 SwitchDown() OVERRIDE; // slot 2
public:
    CTileExclusiveTriggerSwitchLogic(); // 0x112050
};
SIZE(CTileExclusiveTriggerSwitchLogic, 0x8c);
VTBL(CTileExclusiveTriggerSwitchLogic, 0x001eaecc);

class CTileSecretTriggerSwitchLogic : public CTileTriggerSwitchLogic {
    virtual i32 SwitchDown() OVERRIDE; // slot 2
public:
    CTileSecretTriggerSwitchLogic(); // 0x112790
};
SIZE(CTileSecretTriggerSwitchLogic, 0x8c);
VTBL(CTileSecretTriggerSwitchLogic, 0x001eaf24);

class CTileTimeTriggerSwitchLogic : public CTileTriggerSwitchLogic {
    virtual i32 SwitchDown() OVERRIDE; // slot 2
    virtual i32 SwitchUp() OVERRIDE;   // slot 3
public:
    CTileTimeTriggerSwitchLogic(); // 0x1127c0
};
SIZE(CTileTimeTriggerSwitchLogic, 0x8c);
VTBL(CTileTimeTriggerSwitchLogic, 0x001eaf3c);

class CCheckpointTriggerSwitchLogic : public CTileTriggerSwitchLogic {
    virtual i32 SwitchDown() OVERRIDE; // slot 2
    virtual i32 SwitchUp() OVERRIDE;   // slot 3
public:
    CCheckpointTriggerSwitchLogic(); // 0x1127f0
    // slot 1 (0x112a50): the checkpoint build. Uses the BASE's m_20 gate (+0x20) and copies
    // the caller's 24-dword block into the BASE's m_block (+0x2c) - `rep movsd` ecx=0x18.
    virtual i32 BuildSmall(
        CTileTriggerContainer* owner,
        i32 a2,
        i32 a3,
        i32 a4,
        i32 a5,
        const i32* rect,
        i32 a7,
        i32 a8,
        i32 a9
    ) OVERRIDE;
};
SIZE(CCheckpointTriggerSwitchLogic, 0x8c);
VTBL(CCheckpointTriggerSwitchLogic, 0x001eaf54);

#endif // SRC_GRUNTZ_TILETRIGGERSWITCHLOGIC_H
