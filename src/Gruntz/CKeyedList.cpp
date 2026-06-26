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

// The list node a key-add allocates: a CString key + two payload dwords (0xc B).
struct CNode {
    CString m_key; // +0x00
    i32 m_4;       // +0x04
    i32 m_8;       // +0x08
};

struct CKeyedList {
    char _00[4];
    CListNode* m_4; // +0x04  node list head
    char _08[0x1c - 8];
    i32 m_1c;                                        // +0x1c
    void BaseClear();                                // 0x1b48a6
    void Insert(CNode* n);                           // 0x1b4991
    void Clear();                                    // 0x379a0
    CNode* AddNode(const char* key, i32 a2, i32 a3); // 0x37a70
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

// 0x37a70: allocate a node, default-construct its CString key, zero the payload,
// then assign the key + payload and insert. /GX EH-framed (the CString member).
// @early-stop
// ~84%: the new(0xc) + null-guard, the m_4/m_8 zeros, the key operator=, the m_4/m_8
// payload stores, the Insert and the return are byte-faithful. The residual is the
// CString member's construction sequence (retail emits two ctor calls - 0x1b9b93 then
// 0x1b9c69 - with a trylevel bump between; a plain `CString m_key` member yields one),
// plus the EH-state-table addend (push $0x8 vs $0x0, a per-unit scope-table offset)
// and the this-register coloring (ebx vs edi). Documented EH/CString-ctor plateau.
RVA(0x00037a70, 0x97)
CNode* CKeyedList::AddNode(const char* key, i32 a2, i32 a3) {
    CNode* node = new CNode;
    node->m_4 = 0;
    node->m_8 = 0;
    node->m_key = key;
    node->m_4 = a2;
    node->m_8 = a3;
    Insert(node);
    return node;
}
