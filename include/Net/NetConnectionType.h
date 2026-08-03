#ifndef GRUNTZ_NET_NETCONNECTIONTYPE_H
#define GRUNTZ_NET_NETCONNECTIONTYPE_H

#include <Enums.h>

// Which DirectPlay service provider the multiplayer session uses, as stored in
// CLatencyList::m_mode.
//
// Self-naming: CLatencyList::Dispatch switches on it and every arm calls that
// provider's own populate helper - PopulateIpxOptions, PopulateTcpIpOptions,
// PopulateModemOptions, PopulateSerialOptions, PopulateGenericOptions - so the
// value order is read off the code, not guessed.
GZ_ENUM_BEGIN(NetConnectionType)
    NETCONN_IPX = 1,
    NETCONN_TCPIP = 2,
    NETCONN_MODEM = 3,
    NETCONN_SERIAL = 4,
    // The catch-all provider: whatever DirectPlay enumerated that is none of the
    // four named above.
    NETCONN_GENERIC = 5,
    // One past the last provider, so a bound never names NETCONN_GENERIC.
    NETCONN_COUNT = 6
GZ_ENUM_END(NetConnectionType)

#endif // GRUNTZ_NET_NETCONNECTIONTYPE_H
