#include <Net/LatencyList.h>
#include <rva.h>

RVA(0x00037c30, 0xb3)
i32 CLatencyList::Populate2() {
    if (!AddNode("Automatic", 0, 0)) {
        return 0;
    }
    if (!AddNode("Very Low Latency [ping < 50]", 2, 10)) {
        return 0;
    }
    if (!AddNode("Low Latency [ping < 100]", 4, 10)) {
        return 0;
    }
    if (!AddNode("Medium Latency [ping < 200]", 6, 20)) {
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

RVA(0x00037e10, 0xb3)
i32 CLatencyList::Populate4() {
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

RVA(0x00037f00, 0xb3)
i32 CLatencyList::Populate5() {
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
