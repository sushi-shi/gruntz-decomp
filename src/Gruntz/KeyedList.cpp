#include <StdAfx.h>

#include <Ints.h>

#include <Net/KeyedList.h>

#include <stddef.h>

void CKeyedList::Clear() {

    POSITION pos = m_list.GetHeadPosition();
    while (pos != NULL) {
        CKeyedNode* sub = static_cast<CKeyedNode*>(m_list.GetNext(pos));
        delete sub;
    }
    m_list.RemoveAll();
    m_mode = 0;
}

CKeyedNode::~CKeyedNode() {
    m_key.Empty();
    m_commandDelay = 0;
    m_resendInterval = 0;
}

CKeyedNode* CKeyedList::AddNode(const char* key, i32 commandDelay, i32 resendInterval) {
    CKeyedNode* node = new CKeyedNode;
    node->m_key = key;
    node->m_commandDelay = commandDelay;
    node->m_resendInterval = resendInterval;
    m_list.AddTail(node);
    return node;
}
