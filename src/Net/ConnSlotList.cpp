#include <Net/LatencyList.h>
#include <rva.h>
// ConnSlotList.cpp - three of the five per-mode populators for the connection-
// latency slot list (CLatencyList, <Net/LatencyList.h>). The list is a CObList of
// 12-byte {CString text; int id; int param} rows (CLatencyItem) + a mode int at
// +0x1c; CLatencyList::Dispatch (0x37910) routes modes 1..5 to the populators
// (mode 2 -> 0x37c30, mode 4 -> 0x37e10, mode 5 -> 0x37f00; these three).
//
// Each populator appends eight latency rows via AddItem (0x37a70: new(0xc) node;
// CString = text; +4 = id; +8 = param; CObList::AddTail) and folds the per-call
// success test into one BOOL. AddItem is a reloc-masked rel32 callee (declared in
// the header, no body here). Folded from the former CConnSlotList view (wave 3);
// only the OFFSETS + emitted bytes are load-bearing, the names are placeholders.

// ===========================================================================
// 0x37c30 (mode 2) - populate the modem/ramped latency ladder: ping thresholds
// climb 0,10,10,20,30,30,30,30 across the eight rows. Each append must succeed;
// the final row's success is returned as a clean BOOL.
// ===========================================================================
RVA(0x00037c30, 0xb3)
i32 CLatencyList::Populate2() {
    if (!AddItem("Automatic", 0, 0)) {
        return 0;
    }
    if (!AddItem("Very Low Latency [ping < 50]", 2, 10)) {
        return 0;
    }
    if (!AddItem("Low Latency [ping < 100]", 4, 10)) {
        return 0;
    }
    if (!AddItem("Medium Latency [ping < 200]", 6, 20)) {
        return 0;
    }
    if (!AddItem("Medium-High [ping < 250]", 8, 30)) {
        return 0;
    }
    if (!AddItem("High Latency [ping < 400]", 12, 30)) {
        return 0;
    }
    if (!AddItem("Very High Latency [ping < 550]", 16, 30)) {
        return 0;
    }
    return AddItem("Last Resort", 24, 30) != 0;
}

// ===========================================================================
// 0x37e10 (mode 4) - populate the fixed-latency ladder: every threshold past the
// first is 30 (the "Automatic" row stays 0,0). Byte-identical body to Populate5
// (0x37f00).
// ===========================================================================
RVA(0x00037e10, 0xb3)
i32 CLatencyList::Populate4() {
    if (!AddItem("Automatic", 0, 0)) {
        return 0;
    }
    if (!AddItem("Very Low Latency [ping < 50]", 2, 30)) {
        return 0;
    }
    if (!AddItem("Low Latency [ping < 100]", 4, 30)) {
        return 0;
    }
    if (!AddItem("Medium Latency [ping < 200]", 6, 30)) {
        return 0;
    }
    if (!AddItem("Medium-High [ping < 250]", 8, 30)) {
        return 0;
    }
    if (!AddItem("High Latency [ping < 400]", 12, 30)) {
        return 0;
    }
    if (!AddItem("Very High Latency [ping < 550]", 16, 30)) {
        return 0;
    }
    return AddItem("Last Resort", 24, 30) != 0;
}

// ===========================================================================
// 0x37f00 (mode 5) - the same fixed-latency ladder as Populate4 (0x37e10); a
// distinct connection-mode entry point with an identical body.
// ===========================================================================
RVA(0x00037f00, 0xb3)
i32 CLatencyList::Populate5() {
    if (!AddItem("Automatic", 0, 0)) {
        return 0;
    }
    if (!AddItem("Very Low Latency [ping < 50]", 2, 30)) {
        return 0;
    }
    if (!AddItem("Low Latency [ping < 100]", 4, 30)) {
        return 0;
    }
    if (!AddItem("Medium Latency [ping < 200]", 6, 30)) {
        return 0;
    }
    if (!AddItem("Medium-High [ping < 250]", 8, 30)) {
        return 0;
    }
    if (!AddItem("High Latency [ping < 400]", 12, 30)) {
        return 0;
    }
    if (!AddItem("Very High Latency [ping < 550]", 16, 30)) {
        return 0;
    }
    return AddItem("Last Resort", 24, 30) != 0;
}
