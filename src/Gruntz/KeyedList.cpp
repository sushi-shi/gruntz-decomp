#include <Net/KeyedList.h>
#include <rva.h>

RVA(0x000379f0, 0x57)
CKeyedNode::~CKeyedNode() {
    m_key.Empty();
    m_4 = 0;
    m_8 = 0;
}

RVA(0x000379a0, 0x3d)
void CKeyedList::Clear() {
    // The inline POSITION walk (GetHeadPosition/GetNext) lowers to the same
    // head-load + pNext/data loop as the raw node walk did.
    POSITION pos = m_list.GetHeadPosition();
    while (pos != 0) {
        CKeyedNode* sub = static_cast<CKeyedNode*>(m_list.GetNext(pos));
        delete sub; // ~CKeyedNode non-virtual -> direct dtor + ??3
    }
    m_list.RemoveAll();
    m_mode = 0;
}

// 0x37a70: allocate a node, default-construct its CString key, zero the payload,
// then assign the key + payload and insert. /GX EH-framed (the CString member).
// @early-stop
RVA(0x00037a70, 0x9a)
CKeyedNode* CKeyedList::AddNode(const char* key, i32 a2, i32 a3) {
    CKeyedNode* node = new CKeyedNode;
    node->m_4 = 0;
    node->m_8 = 0;
    node->m_key = key;
    node->m_4 = a2;
    node->m_8 = a3;
    m_list.AddTail(node); // CPtrList::AddTail (0x1b4991) - stores void*, no CObject cast
    return node;
}
