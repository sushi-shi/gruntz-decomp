// TileTriggerSwitchLogic.h - Gruntz CTileTriggerSwitchLogic (C:\Proj\Gruntz).
// A tile-trigger "switch" that owns a linked list of sibling objects (anchor
// at +0x04, singly-linked nodes with next@+0x00, data@+0x08).  Matched methods:
//   ctor            vtable + zero m_block + m_20=0
//   FindIndexByKey  linear scan of the 24-dword m_block
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + the emitted
// code bytes are load-bearing (campaign doctrine).
#ifndef SRC_GRUNTZ_TILETRIGGERSWITCHLOGIC_H
#define SRC_GRUNTZ_TILETRIGGERSWITCHLOGIC_H

#include <Ints.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/SerialArchive.h> // the shared CSerialArchive stream (Read @ +0x2c / Write @ +0x30)
#include <rva.h>                  // SIZE_UNKNOWN class-metadata macros used below

// The CGameRegistry singleton (g_gameReg, RVA 0x64556c).  Only +0x30 (the active
// game-manager pointer) is touched by the methods here; reloc-masked DIR32.
// g_gameReg decl dropped (convenience only; consumers include GameRegistry.h) - clashed with Grunt.h WwdGameReg* view

class CTileTriggerLogic; // the SEPARATE 0x9c logic family (not a base: 0x8c < 0x9c)

// sizeof = 0x8c, PROVEN TWICE and independently:
//   (1) the allocation sites - `push 0x8c; call ??2; mov ecx,eax; call ??0CTileTriggerSwitchLogic`
//       at 0x115f96 (the AddSwitchLogic factory) and 0x11796b (CTileTriggerFactory::Build);
//       every one of the five derived switch logics is allocated at 0x8c too, so none of
//       them adds a data member.
//   (2) the ctor (0x110430) zeroes exactly 24 dwords from +0x2c -> +0x2c + 0x60 = 0x8c.
// The two agree with zero slack, so m_block is [24] and the object ENDS at 0x8c.
//
// This class is NOT related to CTileTriggerLogic (0x9c) by inheritance - retail keeps two
// parallel families and stamps the owner at a DIFFERENT offset in each (Build: `mov
// [esi+0x24],ebp` for the 0x8c switch family vs `mov [esi+0x20],ebp` for the 0x9c family).
class CTileTriggerSwitchLogic {
public:
    struct ListNode;  // +0x04 sibling-list node (defined below)
    struct ChildNode; // +0x20 child-list node (defined below)

    // The 4 retail vtable slots (0x5eae8c). Real virtuals now -> cl emits the
    // ??_7 vftable + the implicit ctor vptr-stamp (replaces the manual struct
    // stamp). Bodies live in unmatched engine TUs; declared-only here, named on
    // the target via deterministic @data-symbol/@rva-symbol in the .cpp.
    // slot 0 (thunk 0x1749) - the one-shot Setup virtual, body @0x1104f0. Its 8-arg
    // build signature is corroborated by CheckpointSwitchBuild.cpp's BaseBuild view.
    virtual i32 Vf0(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7);
    // slot 1 -> 0x0022e8 (body in an unmatched engine TU). Its real signature is recovered
    // from the ONE reconstructed override, CCheckpointTriggerSwitchLogic::BuildSmall
    // (0x112a50, `sema class` says slot 1, origin CTileTriggerSwitchLogic): 9 args, returns
    // i32. It was declared `void Vf1()` here, which silently emitted the BASE slot into the
    // derived vtable instead of the override.
    virtual i32
    BuildSmall(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, const i32* rect, i32 a7, i32 a8, i32 a9);
    virtual i32 Vf2(); // slot 2 -> 0x002e0f (CTileTimeTriggerSwitchLogic overrides @0x112840)
    virtual i32 Vf3(); // slot 3 -> 0x0037e2 (returns i32; base slot typed void in retail callers)

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

    // RemoveByKeys reuses the MFC CPtrList::RemoveAt (0x1b4ac7) directly on `this`
    // (its leading {vptr,head,tail,count} overlay a CPtrList; own vtable 0x1eae8c
    // differs, so a `(CPtrList*)this` reinterpret, not inheritance) - see the .cpp.

    // Trace-discovered child-list accessors (list head @ +0x04; nodes
    // next@+0x00, data@+0x08; data objects are sibling CTileTriggerSwitchLogic
    // with keys at +0x04 / +0x10).
    i32 GetFlag74();                  // 0x115f00 (out-of-line: test-and-set m_block[18])
    i32 RemoveByKeys(i32 k1, i32 k2); // 0x116320
    // The children are 0x9c CTileTriggerLogic (VerifyBlockLinks scans them at +0x3c..+0x9b).
    CTileTriggerSwitchLogic* FindChild(i32 k1, i32 k2); // 0x116ee0 (walks the +0x04 sibling list)
    CTileTriggerSwitchLogic* FindByField0C(i32 key);    // 0x1171d0 (same list)
    i32 ScanNeighborhood(i32 x, i32 y);                 // 0x117ec0
    i32 TransferFlag74(CSerialArchive* s);              // 0x117e20
    i32 LoadFlag74(CSerialArchive* s);                  // 0x117e70 (read via slot +0x2c)
    i32 LoadState(CSerialArchive* s);                   // 0x1139a0 (read via slot +0x2c)
    // ValidateByType (0x113a90) / ApplyByType (0x113d40) / SerializeMatrix (0x113dd0) /
    // DeserializeMatrix (0x113e70) are NOT members - they were misattributed here. Retail's
    // CTileTriggerFactory::Build calls them on freshly-`new`ed objects of the OTHER family:
    // ApplyByType on a 0xc8 CGiantRockLogic (0x117b49: push 0xc8 / ??0CGiantRockLogic /
    // mov ecx,esi / call 0x1d39), ValidateByType on a 0x9c CTileTriggerLogic (0x117aa7).
    // They live in TileTriggerLogic.h now. SerializeMatrix reaches this+0xc0/+0xc4, which an
    // 0x8c object cannot hold - that overrun was the "m_block[37]/[38]" contradiction.

    // Per-cell probe (reloc-masked rel32 callee); cell is (y) + (x << 8).
    i32 ProbeCell(i32 cell, i32 kind);

    // Engine-label backlog stubs.
    void CTileTriggerSwitchLogic_115f60();
    void BuildRockBreakInGameText();

    // +0x00  implicit vptr (real virtuals above; was an explicit m_vptr struct stamp)
    i32 m_04;        // +0x04  list head (owner) / key (data obj); genuinely dual-role
    i32 m_08;        // +0x08  (serialized in LoadState)
    i32 m_key0c;     // +0x0c  secondary key (compared in FindByField0C)
    i32 m_key1;      // +0x10  primary key (FindIndexByKey/RemoveByKeys/FindChild)
    i32 m_linkGate;  // +0x14  link-check gate (VerifyBlockLinks guard)
    i32 m_18;        // +0x18  (serialized in LoadState)
    i32 m_1c;        // +0x1c  (serialized in LoadState)
    ChildNode* m_20; // +0x20  CHILD list head (0x9c CTileTriggerLogic elements; see ChildNode)
    CTileTriggerSwitchLogic* m_owner; // +0x24  back-pointer to the owning switch-logic
    i32 m_28;                         // +0x28  (serialized in LoadState)
    i32 m_block[24]; // +0x2c..0x8b  (the ctor zeroes exactly these 24; ends at 0x8c)

    // TWO DIFFERENT intrusive lists hang off this class, and they hold DIFFERENT element
    // types. Both nodes are {next@+0x00, data@+0x08}; only the element type differs.
    //
    // (1) the +0x04 SIBLING list (walked by RemoveByKeys / FindChild / FindByField0C):
    //     the elements are CTileTriggerSwitchLogic. PROVEN by RemoveByKeys' inlined delete -
    //     retail emits `mov [data],offset ??_7; mov dword ptr [data+0x20],0`, and zeroing
    //     +0x20 is THIS class's dtor (m_20). CTileTriggerLogic's dtor would zero +0x1c.
    struct ListNode {
        ListNode* m_next;                // +0x00
        char _pad04[4];                  // +0x04
        CTileTriggerSwitchLogic* m_data; // +0x08
    };

    // (2) the +0x20 CHILD list (walked by VerifyBlockLinks via m_owner->m_20): the elements
    //     are 0x9c CTileTriggerLogic. PROVEN by the scan - VerifyBlockLinks does
    //     `lea esi,[child+0x3c]` + 24 dwords AND calls child->FindIndexByKey (itself
    //     `add ecx,0x3c` + 24), reaching child+0x9b. An 0x8c switch logic cannot hold that;
    //     CTileTriggerLogic's m_block[24] @ +0x3c ends at exactly 0x9c.
    struct ChildNode {
        ChildNode* m_next;         // +0x00
        char _pad04[4];            // +0x04
        CTileTriggerLogic* m_data; // +0x08
    };
};
SIZE(CTileTriggerSwitchLogic, 0x8c);
VTBL(CTileTriggerSwitchLogic, 0x001eae8c); // vtable_names -> code (RTTI game class)

// --- the five derived switch logics -----------------------------------------------------
// Each is allocated at 0x8c (same as the base) at TWO independent new-sites, so NONE of them
// adds a data member; they only override vtable slots. Homed here out of the .cpp (they were
// .cpp-local views; CCheckpointTriggerSwitchLogic needs to be shared with CheckpointSwitchBuild.cpp).
class CTileMultiTriggerSwitchLogic : public CTileTriggerSwitchLogic {
public:
    CTileMultiTriggerSwitchLogic(); // 0x111f10
};
SIZE(CTileMultiTriggerSwitchLogic, 0x8c);
VTBL(CTileMultiTriggerSwitchLogic, 0x001eaeb4);

class CTileExclusiveTriggerSwitchLogic : public CTileTriggerSwitchLogic {
    virtual i32 Vf2() OVERRIDE; // slot 2
public:
    CTileExclusiveTriggerSwitchLogic(); // 0x112050
};
SIZE(CTileExclusiveTriggerSwitchLogic, 0x8c);
VTBL(CTileExclusiveTriggerSwitchLogic, 0x001eaecc);

class CTileSecretTriggerSwitchLogic : public CTileTriggerSwitchLogic {
    virtual i32 Vf2() OVERRIDE; // slot 2
public:
    CTileSecretTriggerSwitchLogic(); // 0x112790
};
SIZE(CTileSecretTriggerSwitchLogic, 0x8c);
VTBL(CTileSecretTriggerSwitchLogic, 0x001eaf24);

class CTileTimeTriggerSwitchLogic : public CTileTriggerSwitchLogic {
    virtual i32 Vf2() OVERRIDE; // slot 2
    virtual i32 Vf3() OVERRIDE; // slot 3
public:
    CTileTimeTriggerSwitchLogic(); // 0x1127c0
};
SIZE(CTileTimeTriggerSwitchLogic, 0x8c);
VTBL(CTileTimeTriggerSwitchLogic, 0x001eaf3c);

// The 0x60-byte block BuildSmall `rep movsd`s into this+0x2c IS the base's m_block[24]
// (24 dwords at +0x2c) - it is not a distinct "rect" member, and the class adds nothing.
class CCheckpointTriggerSwitchLogic : public CTileTriggerSwitchLogic {
    virtual i32 Vf2() OVERRIDE; // slot 2
    virtual i32 Vf3() OVERRIDE; // slot 3
public:
    CCheckpointTriggerSwitchLogic(); // 0x1127f0
    // slot 1 (0x112a50): the checkpoint build. Uses the BASE's m_20 gate (+0x20) and copies
    // the caller's 24-dword block into the BASE's m_block (+0x2c) - `rep movsd` ecx=0x18.
    virtual i32
    BuildSmall(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, const i32* rect, i32 a7, i32 a8, i32 a9)
        OVERRIDE;
};
SIZE(CCheckpointTriggerSwitchLogic, 0x8c);
VTBL(CCheckpointTriggerSwitchLogic, 0x001eaf54);

#endif // SRC_GRUNTZ_TILETRIGGERSWITCHLOGIC_H
