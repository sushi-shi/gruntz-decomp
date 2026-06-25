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

// The WwdGameReg singleton (g_gameReg, RVA 0x64556c).  Only +0x30 (the active
// game-manager pointer) is touched by the methods here; reloc-masked DIR32.
struct WwdGameReg {
    // The diagnostic ack reporter (RVA 0x8dc60, __thiscall): reports a (line, code)
    // pair when a switch-logic linkage check fails; reloc-masked rel32 callee.
    void Ack(i32 line, i32 code); // 0x8dc60

    char _pad00[0x30];
    void* m_30; // +0x30  game-manager pointer (null-checked)
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
    virtual void TransferIn(void* buf, i32 n); // +0x2c  read counterpart of Transfer
    virtual void Transfer(void* buf, i32 n);   // +0x30
};

// vftable.  Reconstructed from the methods below; fields only
// cover the touched offsets.  Size ~0x8c (0x2c base + 0x60 m_block).
class CTileTriggerSwitchLogic {
public:
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
    i32 LoadFlag74(CSerialStream* s);                        // 0x117e70 (read via slot +0x2c)
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

    void* m_vptr;                     // +0x00  vtable (0x5eae8c, stamped in ctor)
    i32 m_04;                         // +0x04  list head (owner) / key (data obj)
    i32 m_08;                         // +0x08  (not accessed here)
    i32 m_0c;                         // +0x0c  (not accessed here)
    i32 m_10;                         // +0x10  key1 (compared in RemoveByKeys/FindChild)
    i32 m_14;                         // +0x14  link-check gate (VerifyBlockLinks guard)
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
