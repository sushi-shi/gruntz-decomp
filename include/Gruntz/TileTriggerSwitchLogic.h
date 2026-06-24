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

// The WwdGameReg singleton (g_gameReg, RVA 0x64556c).  Only +0x30 (the active
// game-manager pointer) is touched by the methods here; reloc-masked DIR32.
struct WwdGameReg {
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
    virtual void Slot2C();
    virtual void Transfer(void* buf, int n); // +0x30
};

// vftable.  Reconstructed from the methods below; fields only
// cover the touched offsets.  Size ~0x8c (0x2c base + 0x60 m_block).
class CTileTriggerSwitchLogic {
public:
    CTileTriggerSwitchLogic();
    int FindIndexByKey(int key);

    // CObList::RemoveAt is reached through the inherited CObList base (this == the
    // CObList; head @ +0x04).  Declared no-body, reloc-masked rel32 callee.
    void ListRemoveAt(void* pos);

    // Trace-discovered child-list accessors (list head @ +0x04; nodes
    // next@+0x00, data@+0x08; data objects are sibling CTileTriggerSwitchLogic
    // with keys at +0x04 / +0x10).
    int GetFlag74();                                         // 0x115f00
    int RemoveByKeys(int k1, int k2);                        // 0x116320
    CTileTriggerSwitchLogic* FindChild(int k1, int k2);      // 0x116ee0
    CTileTriggerSwitchLogic* FindByField0C(int key);         // 0x1171d0
    int ScanNeighborhood(int x, int y);                      // 0x117ec0
    int ValidateByType(void* obj, int type, int a3, int a4); // 0x113a90
    int TransferFlag74(CSerialStream* s);                    // 0x117e20
    int ApplyByType(void* obj, int type, int a3, int a4);    // 0x113d40
    int SerializeMatrix(CSerialStream* s);                   // 0x113dd0

    // __thiscall validators/appliers used by ApplyByType (reloc-masked).
    int ApplyBase(void* obj, int type, int a3, int a4);
    int ApplyType4(void* obj);
    int ApplyType7(void* obj);

    // Per-cell probe (reloc-masked rel32 callee); cell is (y) + (x << 8).
    int ProbeCell(int cell, int kind);

    // Engine-label backlog stubs.
    void CTileTriggerSwitchLogic_115f60();
    void BuildRockBreakInGameText();

    void* m_vptr;              // +0x00  vtable (0x5eae8c, stamped in ctor)
    int m_04;                  // +0x04  list head (owner) / key (data obj)
    int m_08;                  // +0x08  (not accessed here)
    int m_0c;                  // +0x0c  (not accessed here)
    int m_10;                  // +0x10  key1 (compared in RemoveByKeys/FindChild)
    char m_pad14[0x20 - 0x14]; // +0x14..0x1f
    int m_20;                  // +0x20  cleared before delete
    char m_pad24[0x2c - 0x24]; // +0x24..0x2b
    int m_block[40];           // +0x2c..0xcb  (first 24 zeroed in ctor)

    // Linked-list node: next@0x00, data@0x08.  Encapsulated inline.
    struct ListNode {
        ListNode* m_next;                // +0x00
        char _pad04[4];                  // +0x04
        CTileTriggerSwitchLogic* m_data; // +0x08
    };
};

#endif // SRC_GRUNTZ_TILETRIGGERSWITCHLOGIC_H
