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

// vftable.  Reconstructed from the methods below; fields only
// cover the touched offsets.  Size ~0x8c (0x2c base + 0x60 m_block).
class CTileTriggerSwitchLogic {
public:
    CTileTriggerSwitchLogic();
    int RemoveByKey(int key1, int key2); // (still a stub)
    int FindIndexByKey(int key);

    // Engine-label backlog stubs.
    void CTileTriggerSwitchLogic_115f60();
    void CTileTriggerSwitchLogic_116320();
    void BuildRockBreakInGameText();

    // +0x00 vptr (implicit)
    int m_04;                  // +0x04  list head (owner) / key (data obj)
    int m_08;                  // +0x08  (not accessed here)
    int m_0c;                  // +0x0c  (not accessed here)
    int m_10;                  // +0x10  key2 (compared in RemoveByKey)
    char m_pad14[0x20 - 0x14]; // +0x14..0x1f
    int m_20;                  // +0x20  cleared before delete
    char m_pad24[0x2c - 0x24]; // +0x24..0x2b
    int m_block[24];           // +0x2c..0x8b  (24 dwords, zeroed in ctor)

    // Linked-list node: next@0x00, data@0x08.  Encapsulated inline.
    struct ListNode {
        ListNode* m_next;                // +0x00
        char _pad04[4];                  // +0x04
        CTileTriggerSwitchLogic* m_data; // +0x08
    };
};

#endif // SRC_GRUNTZ_TILETRIGGERSWITCHLOGIC_H
