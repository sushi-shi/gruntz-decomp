#ifndef GRUNTZ_NET_NETMGRINLINE_H
#define GRUNTZ_NET_NETMGRINLINE_H

#include <Net/NetMgr.h>
#include <Net/NetProviderNode.h>

inline CNetProviderNode* CNetMgr::GetFirstProvider() {
    m_providerCursor = m_providers.GetHeadPosition();
    return m_providerCursor != NULL
               ? static_cast<CNetProviderNode*>(m_providers.GetNext(m_providerCursor))
               : NULL;
}

inline CNetProviderNode* CNetMgr::GetNextProvider() {
    if (m_providerCursor != NULL) {
        CNetProviderNode* next =
            static_cast<CNetProviderNode*>(m_providers.GetAt(m_providerCursor));
        m_providers.GetNext(m_providerCursor);
        return next;
    }
    return NULL;
}

inline CNetSessionListNode* CNetMgr::GetFirstSession() {
    m_sessionCursor = m_sessionListings.GetHeadPosition();
    return m_sessionCursor != NULL
               ? static_cast<CNetSessionListNode*>(m_sessionListings.GetNext(m_sessionCursor))
               : NULL;
}

inline CNetSessionListNode* CNetMgr::GetNextSession() {
    if (m_sessionCursor != NULL) {
        CNetSessionListNode* next =
            static_cast<CNetSessionListNode*>(m_sessionListings.GetAt(m_sessionCursor));
        m_sessionListings.GetNext(m_sessionCursor);
        return next;
    }
    return NULL;
}

inline CNetPlayerNode* CNetMgr::GetFirstPlayer() {
    m_playerCursor = m_players.GetHeadPosition();
    return m_playerCursor != NULL ? static_cast<CNetPlayerNode*>(m_players.GetNext(m_playerCursor))
                                  : NULL;
}

inline CNetPlayerNode* CNetMgr::GetNextPlayer() {
    if (m_playerCursor != NULL) {
        CNetPlayerNode* next = static_cast<CNetPlayerNode*>(m_players.GetAt(m_playerCursor));
        m_players.GetNext(m_playerCursor);
        return next;
    }
    return NULL;
}

#endif // GRUNTZ_NET_NETMGRINLINE_H
