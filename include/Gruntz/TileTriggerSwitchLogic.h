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

// The WwdGameReg singleton (g_gameReg, RVA 0x64556c).  The switch-logic methods
// touch +0x30 (the active game-manager / resource holder), and the switch sprite
// loaders (LoadSwitch{Down,Up}Sprite) additionally reach the tile-grid notifier
// at +0x70 and the view-bounds rectangle at +0x13c..+0x148.  Reloc-masked DIR32.
// (This unified view subsumes the old StatusBarUpdaters CGameReg; the LoadSwitch
// bodies were re-homed here as CTileTriggerSwitchLogic's virtuals.)
struct CTileGrid;        // map tile grid (cell-state + row-offset tables)
struct CStatusBarHolder; // status-bar holder (embedded name->sprite hash table)
struct CTileNotifier;    // tile-system notifier
struct CRegHolder;       // the +0x30 resource holder (tile grid + status bar)
struct WwdGameReg {
    // The diagnostic ack reporter (RVA 0x8dc60, __thiscall): reports a (line, code)
    // pair when a switch-logic linkage check fails; reloc-masked rel32 callee.
    void Ack(i32 line, i32 code); // 0x8dc60

    char _pad00[0x30];
    CRegHolder* m_30; // +0x30  resource holder (null-checked)
    char m_pad34[0x70 - 0x34];
    CTileNotifier* m_70; // +0x70  tile-system notifier
    char m_pad74[0x13c - 0x74];
    i32 m_13c; // +0x13c  view min X
    i32 m_140; // +0x140  view min Y
    i32 m_144; // +0x144  view max X
    i32 m_148; // +0x148  view max Y
};
extern WwdGameReg* g_gameReg;

// A serialization stream: Vfunc30 (vtable slot 12) copies n bytes to/from a
// buffer.  Only the slot offset (+0x30) matters; reloc-masked virtual call.
class CSerialStream {
public:
    virtual void Slot00();
    virtual void Slot04();
    virtual void Slot08();
    virtual void Slot0C();
    virtual void Slot10();
    virtual void Slot14();
    virtual void Slot18();
    virtual void Slot1C();
    virtual void Slot20();
    virtual void Slot24();
    virtual void Slot28();
    virtual void Slot2C();
    virtual void Transfer(void* buf, i32 n); // +0x30
};

// vftable.  Reconstructed from the methods below; fields only
// cover the touched offsets.  Size ~0x8c (0x2c base + 0x60 m_block).
class CTileTriggerSwitchLogic {
public:
    // The 4 retail vtable slots (vtable @0x5eae8c). Real virtuals -> cl emits the
    // ??_7 vftable + the implicit ctor vptr-stamp. Each slot value is an ILT
    // `e9 rel32` jmp thunk in the low .text band; the REAL body is the jmp target
    // (recovered by gruntz.analysis.vtable_scan --emit-vfuncs). Bodies are DEFINED
    // in the .cpp at the resolved body RVAs so they delink as real, matchable
    // functions (slots 2/3 are the re-homed switch sprite loaders).
    virtual void Vf0();                  // slot 0  thunk 0x001749 -> body 0x1104f0
    virtual void Vf1();                  // slot 1  thunk 0x0022e8 -> body 0x110460
    virtual void LoadSwitchDownSprite(); // slot 2  thunk 0x002e0f -> body 0x110570
    virtual void LoadSwitchUpSprite();   // slot 3  thunk 0x0037e2 -> body 0x1106b0

    CTileTriggerSwitchLogic();
    i32 FindIndexByKey(i32 key);
    i32 VerifyBlockLinks(); // 0x112c70

    // CObList::RemoveAt is reached through the inherited CObList base (this == the
    // CObList; head @ +0x04).  Declared no-body, reloc-masked rel32 callee.
    void ListRemoveAt(void* pos);

    // Trace-discovered child-list accessors (list head @ +0x04; nodes
    // next@+0x00, data@+0x08; data objects are sibling CTileTriggerSwitchLogic
    // with keys at +0x04 / +0x10).
    i32 GetFlag74();                                         // 0x115f00
    i32 RemoveByKeys(i32 k1, i32 k2);                        // 0x116320
    CTileTriggerSwitchLogic* FindChild(i32 k1, i32 k2);      // 0x116ee0
    CTileTriggerSwitchLogic* FindByField0C(i32 key);         // 0x1171d0
    i32 ScanNeighborhood(i32 x, i32 y);                      // 0x117ec0
    i32 ValidateByType(void* obj, i32 type, i32 a3, i32 a4); // 0x113a90
    i32 TransferFlag74(CSerialStream* s);                    // 0x117e20
    i32 ApplyByType(void* obj, i32 type, i32 a3, i32 a4);    // 0x113d40
    i32 SerializeMatrix(CSerialStream* s);                   // 0x113dd0

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
    i32 m_04;                         // +0x04  list head (owner) / key (data obj)
    i32 m_08;                         // +0x08  switch tile X (LoadSwitch*Sprite)
    i32 m_0c;                         // +0x0c  switch tile Y (LoadSwitch*Sprite)
    i32 m_10;                         // +0x10  key1 (compared in RemoveByKeys/FindChild)
    i32 m_14;                         // +0x14  link-check gate / switch down(1)/up(0) flag
    char m_pad18[0x20 - 0x18];        // +0x18..0x1f
    i32 m_20;                         // +0x20  child-list head (owner) / cleared before delete
    CTileTriggerSwitchLogic* m_owner; // +0x24  back-pointer to the owning switch-logic
    char m_pad28[0x2c - 0x28];        // +0x28..0x2b
    i32 m_block[40];                  // +0x2c..0xcb  (first 24 zeroed in ctor)

    // Linked-list node: next@0x00, data@0x08.  Encapsulated inline.
    struct ListNode {
        ListNode* m_next;                // +0x00
        char _pad04[4];                  // +0x04
        CTileTriggerSwitchLogic* m_data; // +0x08
    };
};

#endif // SRC_GRUNTZ_TILETRIGGERSWITCHLOGIC_H
