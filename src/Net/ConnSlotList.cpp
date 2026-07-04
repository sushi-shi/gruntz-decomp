#include <rva.h>
// ConnSlotList.cpp - the per-connection latency "slot list" populated by
// CConnSlotList::BuildSlotList (0x161610 region). The object is a CObList-derived
// list (own CObList ctor @0x1b4867 stamps vtable 0x5eb054, block size 0xa) of
// 12-byte {CString name; int ping; int idx} entries, with a trailing selector at
// +0x1c. The dispatcher @0x437910 picks one of five populators by connection type
// (sel 1..5 -> 0x37b40/0x37c30/0x37d20/0x37e10/0x37f00); these three are sel 2/4/5.
//
// Each populator just appends eight latency rows via the AddSlot helper (0x37a70:
// new(0xc) node; CString = name; +4 = ping; +8 = idx; CObList::AddTail) and folds
// the per-call success test into one BOOL. AddSlot is a reloc-masked rel32 callee
// (modeled as a member with no body). Only the OFFSETS + emitted bytes are
// load-bearing; the names are placeholders.

// The CObList-derived latency-slot list. AddSlot (0x37a70) shares `this`, so the
// populators reach it with a plain __thiscall member call (no PMF reinterpret).
class CConnSlotList {
public:
    // 0x37a70 - allocate+append one latency row; returns the new node (non-null
    // = success). Reloc-masked external (no body in this TU).
    void* AddSlot(const char* name, i32 ping, i32 idx);

    i32 InitModem(); // 0x37c30
    i32 InitLan();   // 0x37e10
    i32 InitNet();   // 0x37f00
};
SIZE_UNKNOWN(CConnSlotList); // method-only CObList-derived view; retail size TBD

// ===========================================================================
// 0x37c30 - populate the modem/ramped latency ladder: ping thresholds climb
// 0,10,10,20,30,30,30,30 across the eight rows. Each append must succeed; the
// final row's success is returned as a clean BOOL.
// ===========================================================================
RVA(0x00037c30, 0xb3)
i32 CConnSlotList::InitModem() {
    if (!AddSlot("Automatic", 0, 0)) {
        return 0;
    }
    if (!AddSlot("Very Low Latency [ping < 50]", 2, 10)) {
        return 0;
    }
    if (!AddSlot("Low Latency [ping < 100]", 4, 10)) {
        return 0;
    }
    if (!AddSlot("Medium Latency [ping < 200]", 6, 20)) {
        return 0;
    }
    if (!AddSlot("Medium-High [ping < 250]", 8, 30)) {
        return 0;
    }
    if (!AddSlot("High Latency [ping < 400]", 12, 30)) {
        return 0;
    }
    if (!AddSlot("Very High Latency [ping < 550]", 16, 30)) {
        return 0;
    }
    return AddSlot("Last Resort", 24, 30) != 0;
}

// ===========================================================================
// 0x37e10 - populate the fixed-latency ladder: every threshold past the first is
// 30 (the "Automatic" row stays 0,0). Byte-identical body to InitNet (0x37f00).
// ===========================================================================
RVA(0x00037e10, 0xb3)
i32 CConnSlotList::InitLan() {
    if (!AddSlot("Automatic", 0, 0)) {
        return 0;
    }
    if (!AddSlot("Very Low Latency [ping < 50]", 2, 30)) {
        return 0;
    }
    if (!AddSlot("Low Latency [ping < 100]", 4, 30)) {
        return 0;
    }
    if (!AddSlot("Medium Latency [ping < 200]", 6, 30)) {
        return 0;
    }
    if (!AddSlot("Medium-High [ping < 250]", 8, 30)) {
        return 0;
    }
    if (!AddSlot("High Latency [ping < 400]", 12, 30)) {
        return 0;
    }
    if (!AddSlot("Very High Latency [ping < 550]", 16, 30)) {
        return 0;
    }
    return AddSlot("Last Resort", 24, 30) != 0;
}

// ===========================================================================
// 0x37f00 - the same fixed-latency ladder as InitLan (0x37e10); a distinct
// connection-type entry point with an identical body.
// ===========================================================================
RVA(0x00037f00, 0xb3)
i32 CConnSlotList::InitNet() {
    if (!AddSlot("Automatic", 0, 0)) {
        return 0;
    }
    if (!AddSlot("Very Low Latency [ping < 50]", 2, 30)) {
        return 0;
    }
    if (!AddSlot("Low Latency [ping < 100]", 4, 30)) {
        return 0;
    }
    if (!AddSlot("Medium Latency [ping < 200]", 6, 30)) {
        return 0;
    }
    if (!AddSlot("Medium-High [ping < 250]", 8, 30)) {
        return 0;
    }
    if (!AddSlot("High Latency [ping < 400]", 12, 30)) {
        return 0;
    }
    if (!AddSlot("Very High Latency [ping < 550]", 16, 30)) {
        return 0;
    }
    return AddSlot("Last Resort", 24, 30) != 0;
}
