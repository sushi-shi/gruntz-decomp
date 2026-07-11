// KeyedList.cpp - the real keyed-list container (COMDATs @0x379a0 / 0x379f0 /
// 0x37a70). CKeyedList : CObList (the object is allocated via the CObList ctor
// 0x1b4867 and carries CObList's vtable 0x5eb054); Clear walks the CObList node list
// freeing each node's owned payload, then RemoveAll's the base and zeros the mode.
// The shared class def lives in <Net/KeyedList.h> so the fake `CLatencyList : CObList`
// view folds onto it (CLatencyList : CKeyedList). Placeholder names; only OFFSETS +
// code bytes are load-bearing.
#include <Net/KeyedList.h>
#include <rva.h>

// 0x379f0: CNode destructor - the body empties the key + zeros the payload, then
// the compiler appends the m_key member ~CString. /GX EH-framed (state 0 -> -1
// around the destructible CString member at +0).
RVA(0x000379f0, 0x57)
CKeyedNode::~CKeyedNode() {
    m_key.Empty();
    m_4 = 0;
    m_8 = 0;
}

// 0x379a0: free every node's owned payload (dtor + operator delete), then RemoveAll
// the backing CObList and zero the mode.
RVA(0x000379a0, 0x3d)
void CKeyedList::Clear() {
    CObList::CNode* node = m_pNodeHead;
    if (node != 0) {
        do {
            CObList::CNode* cur = node;
            node = node->pNext;
            CKeyedNode* sub = (CKeyedNode*)cur->data;
            if (sub != 0) {
                sub->~CKeyedNode();
                ::operator delete(sub);
            }
        } while (node != 0);
    }
    RemoveAll();
    m_mode = 0;
}

// 0x37a70: allocate a node, default-construct its CString key, zero the payload,
// then assign the key + payload and insert. /GX EH-framed (the CString member).
// @early-stop
// ~84%: the new(0xc) + null-guard, the m_4/m_8 zeros, the key operator=, the m_4/m_8
// payload stores, the AddTail and the return are byte-faithful. The residual is the
// CString member's construction sequence (retail emits two ctor calls - 0x1b9b93 then
// 0x1b9c69 - with a trylevel bump between; a plain `CString m_key` member yields one),
// plus the EH-state-table addend (push $0x8 vs $0x0, a per-unit scope-table offset)
// and the this-register coloring (ebx vs edi). Documented EH/CString-ctor plateau.
RVA(0x00037a70, 0x97)
CKeyedNode* CKeyedList::AddNode(const char* key, i32 a2, i32 a3) {
    CKeyedNode* node = new CKeyedNode;
    node->m_4 = 0;
    node->m_8 = 0;
    node->m_key = key;
    node->m_4 = a2;
    node->m_8 = a3;
    AddTail((CObject*)node); // CObList::AddTail (0x1b4991)
    return node;
}
