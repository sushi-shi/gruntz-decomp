#ifndef NET_NETPROVIDERNODE_H
#define NET_NETPROVIDERNODE_H

#include <list>
struct CNetProviderNode;

#include <string>

#include <Ints.h>

#include <Ints.h>
#include <Wap32/Object.h>

struct CNetProviderNode : public CObject {
    GUID* m_providerGuid;
    std::string m_providerName;
    std::list<CNetProviderNode*>::iterator m_listPosition;

    CNetProviderNode() {
        m_providerGuid = NULL;

    }
    virtual ~CNetProviderNode()  ;
    std::string ProviderName();

    i32 IsIpxProvider();
    i32 IsTcpIpProvider();
    i32 IsModemProvider();
    i32 IsSerialProvider();
    i32 MatchesUnclassifiedProvider();
};

#endif
