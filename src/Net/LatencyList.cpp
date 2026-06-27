// LatencyList.cpp - two of the five multiplayer latency-preset fillers (the other
// three, 0x37c30/0x37e10/0x37f00, are sibling fillers homed elsewhere). Each is a
// short-circuit `&&` chain of AddItem(text, id, param) appends: the chain stops
// (returning 0) on the first failed append and normalizes the last result to 0/1
// (neg/sbb/neg). The two differ only in the per-row `param` column (10 vs 30).
//
// See include/Net/LatencyList.h for the container shape + dispatch.
#include <Net/LatencyList.h>

// ===========================================================================
// CLatencyList::PopulateModemSlow  (0x37b40)  - param column = 10
// ===========================================================================
RVA(0x00037b40, 0xb3)
i32 CLatencyList::PopulateModemSlow() {
    if (!AddItem("Automatic", 0, 0))
        return 0;
    if (!AddItem("Very Low Latency [ping < 50]", 2, 10))
        return 0;
    if (!AddItem("Low Latency [ping < 100]", 4, 10))
        return 0;
    if (!AddItem("Medium Latency [ping < 200]", 6, 10))
        return 0;
    if (!AddItem("Medium-High [ping < 250]", 8, 10))
        return 0;
    if (!AddItem("High Latency [ping < 400]", 12, 10))
        return 0;
    if (!AddItem("Very High Latency [ping < 550]", 16, 10))
        return 0;
    return AddItem("Last Resort", 24, 10) != 0;
}

// ===========================================================================
// CLatencyList::PopulateModemFast  (0x37d20)  - param column = 30
// ===========================================================================
RVA(0x00037d20, 0xb3)
i32 CLatencyList::PopulateModemFast() {
    if (!AddItem("Automatic", 0, 0))
        return 0;
    if (!AddItem("Very Low Latency [ping < 50]", 2, 30))
        return 0;
    if (!AddItem("Low Latency [ping < 100]", 4, 30))
        return 0;
    if (!AddItem("Medium Latency [ping < 200]", 6, 30))
        return 0;
    if (!AddItem("Medium-High [ping < 250]", 8, 30))
        return 0;
    if (!AddItem("High Latency [ping < 400]", 12, 30))
        return 0;
    if (!AddItem("Very High Latency [ping < 550]", 16, 30))
        return 0;
    return AddItem("Last Resort", 24, 30) != 0;
}
