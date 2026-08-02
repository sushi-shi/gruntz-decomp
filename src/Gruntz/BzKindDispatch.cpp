#include <rva.h>

#include <Net/LatencyList.h>

RVA(0x00037910, 0x70)
i32 CLatencyList::Dispatch(i32 mode) {
    m_mode = mode;
    switch (mode) {
        case 1:
            if (PopulateIpxOptions()) {
                break;
            }
            return 0;
        case 2:
            if (PopulateTcpIpOptions()) {
                break;
            }
            return 0;
        case 3:
            if (PopulateModemOptions()) {
                break;
            }
            return 0;
        case 4:
            if (PopulateSerialOptions()) {
                break;
            }
            return 0;
        case 5:
            if (PopulateGenericOptions()) {
                break;
            }
            return 0;
        default:
            return 0;
    }
    return 1;
}
