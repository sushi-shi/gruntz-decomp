#include <Net/LatencyList.h>
#include <rva.h>

// @early-stop
RVA(0x00037910, 0x70)
i32 CLatencyList::Dispatch(i32 mode) {
    m_mode = mode;
    switch (mode) {
        case 1:
            if (Populate1()) {
                break;
            }
            return 0;
        case 2:
            if (Populate2()) {
                break;
            }
            return 0;
        case 3:
            if (Populate3()) {
                break;
            }
            return 0;
        case 4:
            if (Populate4()) {
                break;
            }
            return 0;
        case 5:
            if (Populate5()) {
                break;
            }
            return 0;
        default:
            return 0;
    }
    return 1;
}
