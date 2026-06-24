// TileTriggerContainer.cpp - Gruntz CTileTriggerContainer (C:\Proj\Gruntz).
//
// A 3-CObList container (base @ +0x00, CObList members @ +0x1c / +0x38 / +0x54;
// heads @ +0x20 / +0x3c / +0x58).  The accessors find/move/remove the heap
// command elements (CTileGridCommand) across the three lists; an element is
// destroyed inline (stamp vtable 0x5eaea4 + RezFree) before its node is unlinked.
//
// The dynamic this-tracer originally lumped these RVAs under
// CTileTriggerSwitchLogic; they are a DIFFERENT shape (verified by the EH dtor
// 0xc8640 destroying three CObList sub-objects at +0x1c/+0x38/+0x54 and the base
// at +0x00 - not the +0x04 child-list switch-logic layout).
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + the emitted
// code bytes are load-bearing (campaign doctrine).
#include <rva.h>
#include <Mfc.h>
#include <Gruntz/TileTriggerContainer.h>

// The list1/list2 command element: its data is compared against an arg by the
// CTileGridCommand classifier (RVA 0x112970, a __thiscall returning 0/-1/+1).
class TtcElem {
public:
    i32 Classify(void* arg); // 0x112970
    void* m_vptr;            // +0x00
    char _pad04[0x18 - 0x04];
    i32 m_18;
    i32 m_1c; // +0x1c  cleared before delete
};

// ---------------------------------------------------------------------------
// CTileTriggerContainer::DtorBase
// When +0x74 is set, runs the no-arg helper Cleanup116fa0 (RVA 0x116fa0) and
// clears +0x74.  Reloc-masked rel32 callee invoked first by the EH dtor.
// ---------------------------------------------------------------------------
RVA(0x00115f30, 0x18)
void CTileTriggerContainer::DtorBase() {
    if (m_74 != 0) {
        Cleanup116fa0();
        m_74 = 0;
    }
}

// ---------------------------------------------------------------------------
// CTileTriggerContainer::DelFromList1
// Scans list1 (head @ +0x20) for the node whose data == arg; deletes that
// element inline (vtable 0x5eaea4 + [elem+0x1c]=0 + RezFree) and unlinks the
// node via list1.RemoveAt.  Returns 1 on a hit, 0 otherwise.
// ---------------------------------------------------------------------------
// @early-stop
// list-walk regalloc wall (~69%): logic + body identical; retail rotates the node
// through eax with twin ecx/esi copies and hoists arg to edx, vs our single
// callee-saved esi direct-deref.  See docs/patterns/linked-list-walk-node-eax-rotation.md
RVA(0x00116e60, 0x59)
i32 CTileTriggerContainer::DelFromList1(void* data) {
    TtcNode* node = m_list1.m_pNodeHead;
    if (node == 0) {
        return 0;
    }
    do {
        TtcNode* cur = node;
        node = node->m_next;
        TtcElem* elem = (TtcElem*)cur->m_data;
        if (elem == (TtcElem*)data) {
            if (elem != 0) {
                *(void**)elem = &g_tileGridCmdVtbl;
                elem->m_1c = 0;
                RezFree(elem);
            }
            m_list1.RemoveAt(cur);
            return 1;
        }
    } while (node != 0);
    return 0;
}

// ---------------------------------------------------------------------------
// CTileTriggerContainer::FindInLists12
// Scans list1 (head @ +0x20) then list2 (head @ +0x3c) for the first element
// whose +0x10 == a and (b == 0 || +0x04 == b); returns the element, else NULL.
// ---------------------------------------------------------------------------
RVA(0x00116f20, 0x51)
void* CTileTriggerContainer::FindInLists12(i32 a, i32 b) {
    TtcNode* node = m_list1.m_pNodeHead;
    if (node != 0) {
        do {
            TtcNode* cur = node;
            node = node->m_next;
            i32* elem = (i32*)cur->m_data;
            if (elem[4] == a) {
                if (b == 0) {
                    return elem;
                }
                if (elem[1] == b) {
                    return elem;
                }
            }
        } while (node != 0);
    }
    node = m_list2.m_pNodeHead;
    if (node != 0) {
        do {
            TtcNode* cur = node;
            node = node->m_next;
            i32* elem = (i32*)cur->m_data;
            if (elem[4] == a) {
                if (b == 0) {
                    return elem;
                }
                if (elem[1] == b) {
                    return elem;
                }
            }
        } while (node != 0);
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CTileTriggerContainer::FilterList2
// Walks list2 (head @ +0x3c); classifies each element via CTileGridCommand
// 0x112970.  result 0  -> remove from list2 + delete element (0x5eaea4 + RezFree);
//           result -1 -> move element from list2 to list1 (RemoveAt + AddTail).
// Returns 1.
// ---------------------------------------------------------------------------
// @early-stop
// list-walk regalloc wall (~84%): logic + Classify/RemoveAt/AddTail branches
// identical; same node-eax-rotation vs callee-saved-esi shape as the siblings.
// See docs/patterns/linked-list-walk-node-eax-rotation.md
RVA(0x001170b0, 0x72)
i32 CTileTriggerContainer::FilterList2(void* arg) {
    TtcNode* node = m_list2.m_pNodeHead;
    if (node != 0) {
        do {
            TtcNode* cur = node;
            node = node->m_next;
            TtcElem* elem = (TtcElem*)cur->m_data;
            i32 r = elem->Classify(arg);
            if (r == 0) {
                m_list2.RemoveAt(cur);
                if (elem != 0) {
                    *(void**)elem = &g_tileGridCmdVtbl;
                    elem->m_1c = 0;
                    RezFree(elem);
                }
            } else if (r == -1) {
                m_list2.RemoveAt(cur);
                m_list1.AddTail(elem);
            }
        } while (node != 0);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CTileTriggerContainer::MoveList1ToList2
// Scans list1 (head @ +0x20) for the node whose data == arg; removes it from
// list1 and appends the element to list2, then clears element+0x38.  Returns 1.
// ---------------------------------------------------------------------------
// @early-stop
// list-walk regalloc wall (~51%): logic + RemoveAt/AddTail/clear identical; same
// node-eax-rotation vs callee-saved-esi shape as the siblings.
// See docs/patterns/linked-list-walk-node-eax-rotation.md
RVA(0x00117150, 0x53)
i32 CTileTriggerContainer::MoveList1ToList2(void* data) {
    TtcNode* node = m_list1.m_pNodeHead;
    if (node == 0) {
        return 0;
    }
    do {
        TtcNode* cur = node;
        node = node->m_next;
        void* elem = cur->m_data;
        if (elem == data) {
            m_list1.RemoveAt(cur);
            m_list2.AddTail(elem);
            *((i32*)elem + 14) = 0; // elem+0x38
            return 1;
        }
    } while (node != 0);
    return 0;
}

// ---------------------------------------------------------------------------
// CTileTriggerContainer::DelFromList3
// Scans list3 (head @ +0x58) for the node whose data == arg; deletes that
// element inline ([elem+0x10]=0 + RezFree) and unlinks the node via
// list3.RemoveAt.  Returns 1 on a hit, 0 otherwise.
// ---------------------------------------------------------------------------
// @early-stop
// list-walk regalloc wall (~82%): logic + delete/RemoveAt identical; same
// node-eax-rotation vs callee-saved-esi shape as the siblings.
// See docs/patterns/linked-list-walk-node-eax-rotation.md
RVA(0x00117200, 0x53)
i32 CTileTriggerContainer::DelFromList3(void* data) {
    for (TtcNode* node = m_list3.m_pNodeHead; node != 0; node = node->m_next) {
        i32* elem = (i32*)node->m_data;
        if (elem == (i32*)data) {
            if (elem != 0) {
                elem[4] = 0; // elem+0x10
                RezFree(elem);
            }
            m_list3.RemoveAt(node);
            return 1;
        }
    }
    return 0;
}
