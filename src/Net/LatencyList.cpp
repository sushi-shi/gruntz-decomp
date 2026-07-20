#include <Net/LatencyList.h>

RVA(0x00037b40, 0xb3)
i32 CLatencyList::Populate1() {
    if (!AddNode("Automatic", 0, 0)) {
        return 0;
    }
    if (!AddNode("Very Low Latency [ping < 50]", 2, 10)) {
        return 0;
    }
    if (!AddNode("Low Latency [ping < 100]", 4, 10)) {
        return 0;
    }
    if (!AddNode("Medium Latency [ping < 200]", 6, 10)) {
        return 0;
    }
    if (!AddNode("Medium-High [ping < 250]", 8, 10)) {
        return 0;
    }
    if (!AddNode("High Latency [ping < 400]", 12, 10)) {
        return 0;
    }
    if (!AddNode("Very High Latency [ping < 550]", 16, 10)) {
        return 0;
    }
    return AddNode("Last Resort", 24, 10) != 0;
}

RVA(0x00037d20, 0xb3)
i32 CLatencyList::Populate3() {
    if (!AddNode("Automatic", 0, 0)) {
        return 0;
    }
    if (!AddNode("Very Low Latency [ping < 50]", 2, 30)) {
        return 0;
    }
    if (!AddNode("Low Latency [ping < 100]", 4, 30)) {
        return 0;
    }
    if (!AddNode("Medium Latency [ping < 200]", 6, 30)) {
        return 0;
    }
    if (!AddNode("Medium-High [ping < 250]", 8, 30)) {
        return 0;
    }
    if (!AddNode("High Latency [ping < 400]", 12, 30)) {
        return 0;
    }
    if (!AddNode("Very High Latency [ping < 550]", 16, 30)) {
        return 0;
    }
    return AddNode("Last Resort", 24, 30) != 0;
}
