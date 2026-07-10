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

// vftable.  Reconstructed from the methods below; fields only
// cover the touched offsets.  Size ~0x8c (0x2c base + 0x60 m_block).
class CTileTriggerSwitchLogic {
public:
    struct ListNode; // child-list node (defined below); m_20 is a list head

    // The 4 retail vtable slots (0x5eae8c). Real virtuals now -> cl emits the
    // ??_7 vftable + the implicit ctor vptr-stamp (replaces the manual struct
    // stamp). Bodies live in unmatched engine TUs; declared-only here, named on
    // the target via deterministic @data-symbol/@rva-symbol in the .cpp.
    // slot 0 (thunk 0x1749) - the one-shot Setup virtual, body @0x1104f0. Its 8-arg
    // build signature is corroborated by CheckpointSwitchBuild.cpp's BaseBuild view.
    virtual i32 Vf0(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7);
    virtual void Vf1(); // slot 1 -> 0x0022e8
    virtual i32 Vf2();  // slot 2 -> 0x002e0f (CTileTimeTriggerSwitchLogic overrides @0x112840)
    virtual i32 Vf3();  // slot 3 -> 0x0037e2 (returns i32; base slot typed void in retail callers)

    CTileTriggerSwitchLogic();
    // Non-virtual dtor (the 4 vtable slots are all regular methods, no dtor slot).
    // A polymorphic class's dtor restamps the vptr, so an explicit call inlines
    // `mov [this],offset ??_7; mov [this+0x20],0` - exactly retail's inlined delete
    // in RemoveByKeys.
    ~CTileTriggerSwitchLogic() {
        m_20 = 0;
    }
    i32 FindIndexByKey(i32 key);
    i32 VerifyBlockLinksB(); // 0x111f40 (FindChild(key, 3) variant)
    i32 VerifyBlockLinks();  // 0x112c70

    // CObList::RemoveAt is reached through the inherited CObList base (this == the
    // CObList; head @ +0x04).  Declared no-body, reloc-masked rel32 callee.
    void ListRemoveAt(void* pos);

    // Trace-discovered child-list accessors (list head @ +0x04; nodes
    // next@+0x00, data@+0x08; data objects are sibling CTileTriggerSwitchLogic
    // with keys at +0x04 / +0x10).
    i32 GetFlag74();                  // 0x115f00 (out-of-line: test-and-set m_block[18])
    i32 RemoveByKeys(i32 k1, i32 k2); // 0x116320
    CTileTriggerSwitchLogic* FindChild(i32 k1, i32 k2);      // 0x116ee0
    CTileTriggerSwitchLogic* FindByField0C(i32 key);         // 0x1171d0
    i32 ScanNeighborhood(i32 x, i32 y);                      // 0x117ec0
    i32 ValidateByType(void* obj, i32 type, i32 a3, i32 a4); // 0x113a90
    i32 TransferFlag74(CSerialArchive* s);                   // 0x117e20
    i32 LoadFlag74(CSerialArchive* s);                       // 0x117e70 (read via slot +0x2c)
    i32 ApplyByType(void* obj, i32 type, i32 a3, i32 a4);    // 0x113d40
    i32 SerializeMatrix(CSerialArchive* s);   // 0x113dd0 (write; == ApplyType4 target)
    i32 DeserializeMatrix(CSerialArchive* s); // 0x113e70 (read; == ApplyType7 target)
    i32 LoadState(CSerialArchive* s);         // 0x1139a0 (read via slot +0x2c)

    // __thiscall validators/appliers used by ApplyByType (reloc-masked).
    i32 ApplyBase(void* obj, i32 type, i32 a3, i32 a4);
    i32 ApplyType4(void* obj);
    i32 ApplyType7(void* obj);

    // Per-cell probe (reloc-masked rel32 callee); cell is (y) + (x << 8).
    i32 ProbeCell(i32 cell, i32 kind);

    // Engine-label backlog stubs.
    void CTileTriggerSwitchLogic_115f60();
    void BuildRockBreakInGameText();

    // +0x00  implicit vptr (real virtuals above; was an explicit m_vptr struct stamp)
    i32 m_04;       // +0x04  list head (owner) / key (data obj); genuinely dual-role
    i32 m_08;       // +0x08  (serialized in LoadState)
    i32 m_key0c;    // +0x0c  secondary key (compared in FindByField0C)
    i32 m_key1;     // +0x10  primary key (FindIndexByKey/RemoveByKeys/FindChild)
    i32 m_linkGate; // +0x14  link-check gate (VerifyBlockLinks guard)
    i32 m_18;       // +0x18  (serialized in LoadState)
    i32 m_1c;       // +0x1c  (serialized in LoadState)
    ListNode* m_20; // +0x20  child-list head (owner) / cleared before delete
    CTileTriggerSwitchLogic* m_owner; // +0x24  back-pointer to the owning switch-logic
    i32 m_28;                         // +0x28  (serialized in LoadState)
    i32 m_block[40];                  // +0x2c..0xcb  (first 24 zeroed in ctor)

    // Linked-list node: next@0x00, data@0x08.  Encapsulated inline.
    struct ListNode {
        ListNode* m_next;                // +0x00
        char _pad04[4];                  // +0x04
        CTileTriggerSwitchLogic* m_data; // +0x08
    };
};
SIZE_UNKNOWN(CTileTriggerSwitchLogic);
VTBL(CTileTriggerSwitchLogic, 0x001eae8c); // vtable_names -> code (RTTI game class)

#endif // SRC_GRUNTZ_TILETRIGGERSWITCHLOGIC_H
