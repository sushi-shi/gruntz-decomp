// TileTriggerContainer.h - Gruntz CTileTriggerContainer (C:\Proj\Gruntz).
//
// A 3-CObList container: a base sub-object at +0x00 plus three CObList members
// at +0x1c / +0x38 / +0x54 (their m_pNodeHead at +0x20 / +0x3c / +0x58).  The
// lists hold heap command objects; the accessors here move/find/remove those
// objects across the three lists.  The list elements are sibling command objects
// (the CTileGridCommand class) destroyed inline (vtable 0x5eaea4 stamp + RezFree)
// before the node is unlinked.
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + the emitted
// code bytes are load-bearing (campaign doctrine).
#ifndef SRC_GRUNTZ_TILETRIGGERCONTAINER_H
#define SRC_GRUNTZ_TILETRIGGERCONTAINER_H

// The Rez heap free (RVA 0x1b9b82, _RezFree); reloc-masked rel32 callee.
extern "C" void RezFree(void* p);

// A list element (command) object's vftable, stamped into the element by the
// inlined element-destructor before RezFree.  Reloc-masked DIR32 datum.
extern void* g_tileGridCmdVtbl; // 0x5eaea4

// A singly-walked CObList node: next@+0x00, data@+0x08 (MFC CList layout, the
// +0x04 prev slot unused by these walkers).
struct TtcNode {
    TtcNode* m_next; // +0x00
    char _pad04[4];  // +0x04 (prev)
    void* m_data;    // +0x08
};

// One MFC CObList sub-object (0x1c bytes): only m_pNodeHead (+0x04) is read by
// the walkers; RemoveAt / AddTail are the reloc-masked rel32 callees.
class TtcObList {
public:
    void RemoveAt(void* pos); // 0x1b4ac7
    void* AddTail(void* obj); // 0x1b4991
    void* m_vptr;             // +0x00
    TtcNode* m_pNodeHead;     // +0x04
    char _pad08[0x1c - 0x08]; // +0x08..0x1b
};

class CTileTriggerContainer {
public:
    int DelFromList1(void* data);      // 0x116e60
    void* FindInLists12(int a, int b); // 0x116f20
    int FilterList2(void* arg);        // 0x1170b0
    int MoveList1ToList2(void* data);  // 0x117150
    int DelFromList3(void* data);      // 0x117200

    // A no-arg __thiscall helper (RVA 0x116fa0) invoked by DtorBase when +0x74 is
    // set; reloc-masked rel32 callee (body still stubbed for the final sweep).
    void Cleanup116fa0(); // 0x116fa0

    // The base sub-object's own destructor; runs Cleanup116fa0 then clears +0x74.
    void DtorBase(); // 0x115f30

    char m_pad00[0x1c]; // +0x00..0x1b  base sub-object
    TtcObList m_list1;  // +0x1c (head @ +0x20)
    TtcObList m_list2;  // +0x38 (head @ +0x3c)
    TtcObList m_list3;  // +0x54 (head @ +0x58)
    char m_pad70[4];    // +0x70..0x73
    void* m_74;         // +0x74  gates DtorBase's Cleanup116fa0 call, then cleared
};

#endif // SRC_GRUNTZ_TILETRIGGERCONTAINER_H
