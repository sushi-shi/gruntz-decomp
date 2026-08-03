#include <rva.h>

#include <Net/LatencyList.h>
#include <Net/NetConnectionType.h>

RVA(0x00037910, 0x70)
i32 CLatencyList::Dispatch(i32 mode) {
    m_mode = mode;
    switch (mode) {
        case NETCONN_IPX:
            if (PopulateIpxOptions()) {
                break;
            }
            return 0;
        case NETCONN_TCPIP:
            if (PopulateTcpIpOptions()) {
                break;
            }
            return 0;
        case NETCONN_MODEM:
            if (PopulateModemOptions()) {
                break;
            }
            return 0;
        case NETCONN_SERIAL:
            if (PopulateSerialOptions()) {
                break;
            }
            return 0;
        case NETCONN_GENERIC:
            if (PopulateGenericOptions()) {
                break;
            }
            return 0;
        default:
            return 0;
    }
    return 1;
}
