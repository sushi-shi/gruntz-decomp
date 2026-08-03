#include <rva.h>

#include <Ints.h>
#include <Net/NetMgr.h>
#include <Rez/RezMgr.h>

#include <string.h>

RVA(0x001793b0, 0x46)
CNetPlayerListNode::~CNetPlayerListNode() {
    FreeStrings();
}

// @early-stop
RVA(0x00179420, 0x8a)
CNetSessionNode::~CNetSessionNode() {
    m_id = 0;
    m_listPosition = NULL;
    if (m_ownedBufferA) {
        ::operator delete(m_ownedBufferA);
    }
    m_ownedBufferA = NULL;
    if (m_ownedBufferB) {
        ::operator delete(m_ownedBufferB);
    }
    m_ownedBufferB = NULL;
}

RVA(0x001795a0, 0xdb)
i32 CNetPlayerListNode::Init(CNetSessionDesc* src) {
    if (!src) {
        return 0;
    }
    memcpy(&m_desc, src, sizeof(*src));
    m_desc.m_dwSize = 0x50;
    m_desc.m_lpszName = NULL;
    m_desc.m_lpszPassword = NULL;
    if (src->m_lpszName && strlen(src->m_lpszName)) {
        m_desc.m_lpszName = static_cast<char*>(::operator new(strlen(src->m_lpszName) + 8));
        strcpy(m_desc.m_lpszName, src->m_lpszName);
    }
    if (src->m_lpszPassword && strlen(src->m_lpszPassword)) {
        m_desc.m_lpszPassword = static_cast<char*>(::operator new(strlen(src->m_lpszPassword) + 8));
        strcpy(m_desc.m_lpszPassword, src->m_lpszPassword);
    }
    return 1;
}
