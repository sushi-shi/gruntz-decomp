// CKeyedList.cpp - a small keyed-list container (orphan COMDATs @0x379a0 /
// 0x379f0 / 0x37a70). Clear walks the m_4 node list freeing each node's owned
// sub-object; the EH-framed reset + add-node manage the backing CString store.
// Placeholder names; only OFFSETS + code bytes are load-bearing.
#include <Mfc.h> // CString teardown helpers (the nodes hold a CString)
#include <rva.h>

struct CNodeSub {
    void Dtor(); // 0x26a8 (ILT thunk)
};
struct CListNode {
    CListNode* m_0; // +0x00  next
    char _04[8 - 4];
    CNodeSub* m_8; // +0x08  owned sub-object
};
extern "C" void ZFree(void* p); // 0x1b9b82 (operator delete / free)

struct CKeyedList {
    char _00[4];
    CListNode* m_4; // +0x04  node list head
    char _08[0x1c - 8];
    i32 m_1c;         // +0x1c
    void BaseClear(); // 0x1b48a6
    void Clear();     // 0x379a0
};

// 0x379a0: free every node's owned sub-object (dtor + free), then run the base
// clear and zero m_1c.
RVA(0x000379a0, 0x3d)
void CKeyedList::Clear() {
    CListNode* node = m_4;
    if (node != 0) {
        do {
            CListNode* cur = node;
            node = node->m_0;
            CNodeSub* sub = cur->m_8;
            if (sub != 0) {
                sub->Dtor();
                ZFree(sub);
            }
        } while (node != 0);
    }
    BaseClear();
    m_1c = 0;
}
