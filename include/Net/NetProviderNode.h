#ifndef NET_NETPROVIDERNODE_H
#define NET_NETPROVIDERNODE_H

#include <rva.h>

#include <Mfc.h>

#include <Ints.h>
#include <Wap32/Object.h>

struct CNetProviderNode : public CObject {
    GUID* m_providerGuid;
    CString m_providerName;
    __POSITION* m_listPosition;

    CNetProviderNode() {
        m_providerGuid = NULL;
        m_listPosition = NULL;
    }
    virtual ~CNetProviderNode() OVERRIDE;
    CString ProviderName();

    i32 IsIpxProvider();
    i32 IsTcpIpProvider();
    i32 IsModemProvider();
    i32 IsSerialProvider();
    i32 MatchesUnclassifiedProvider();
};

#endif // NET_NETPROVIDERNODE_H
