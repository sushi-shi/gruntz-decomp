#include <StdAfx.h>

#include <Ints.h>

#include <Net/KeyedList.h>

#include <stddef.h>

void CKeyedList::Clear() {

    std::list<CKeyedNode*>::iterator pos = m_list.begin();
    while (pos != m_list.end()) {
        CKeyedNode* sub = static_cast<CKeyedNode*>(*(pos++));
        delete sub;
    }
    m_list.clear();
    m_mode = 0;
}

CKeyedNode::~CKeyedNode() {
    (m_key).erase();
    m_commandDelay = 0;
    m_resendInterval = 0;
}

CKeyedNode* CKeyedList::AddNode(const std::string& key, i32 commandDelay, i32 resendInterval) {
    CKeyedNode* node = new CKeyedNode;
    node->m_key = key;
    node->m_commandDelay = commandDelay;
    node->m_resendInterval = resendInterval;
    m_list.insert(m_list.end(), node);
    return node;
}
