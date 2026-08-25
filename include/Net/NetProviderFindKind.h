#ifndef GRUNTZ_NET_NETPROVIDERFINDKIND_H
#define GRUNTZ_NET_NETPROVIDERFINDKIND_H

#include <Enums.h>

// Provider selector accepted by CNetMgr::FindProvider. This is deliberately not
// NetConnectionType: FindProvider's dispatch calls the TCP/IP predicate for value 1
// and the IPX predicate for value 2.
GZ_ENUM_CONST_BEGIN(NetProviderFindKind)
    NETPROVIDER_FIND_TCPIP = 1,
    NETPROVIDER_FIND_IPX = 2,
    NETPROVIDER_FIND_GENERIC = 5
GZ_ENUM_CONST_END(NetProviderFindKind)

#endif // GRUNTZ_NET_NETPROVIDERFINDKIND_H
