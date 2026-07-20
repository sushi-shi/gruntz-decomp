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
    m_list.AddTail(node); // CPtrList::AddTail (0x1b4991) - stores void*, no CObject cast
    return node;
}
