// LatencyList.h - the multiplayer "connection latency preset" list (the network
// start dialog, C:\Proj\NetMgr / Gruntz). BuildSlotList (0x0c1e60) news a 0x20-byte
// CObList-derived container into the dialog's m_60 slot (ctor 0x1b4867 with block
// size 0xa), then dispatches one of five Populate* fillers by a connection-type
// index via the slot-dispatcher 0x37910:
//   index 1 -> 0x37b40   index 2 -> 0x37c30   index 3 -> 0x37d20
//   index 4 -> 0x37e10   index 5 -> 0x37f00
// Each filler appends eight latency presets via AddItem (0x37a70), which news a
// 12-byte {CString text, int id, int param} node and CObList::AddTail's it; the
// fillers differ only in the per-row `param` column (10 vs 30 vs ...).
//
// Container shape (CObList @+0x00 .. +0x1b, the dispatch mode int @+0x1c) is
// recovered from BuildSlotList + the dispatcher 0x37910; only `this` + the AddItem
// calls are load-bearing for the fillers, so the layout is documented, not
// modeled. Field/method names are placeholders (campaign doctrine).
#ifndef GRUNTZ_NET_LATENCYLIST_H
#define GRUNTZ_NET_LATENCYLIST_H

#include <Ints.h>
#include <rva.h>

// The latency-preset list (a CObList of {CString, int, int} nodes + a mode int at
// +0x1c). AddItem (0x37a70) is the shared node-append; reloc-masked (no body).
class CLatencyList {
public:
    // 0x37a70: new a {text, id, param} node and CObList::AddTail it; returns the
    // node (non-zero on success). __thiscall.
    i32 AddItem(const char* text, i32 id, i32 param);

    i32 PopulateModemSlow(); // 0x37b40 (param column = 10)
    i32 PopulateModemFast(); // 0x37d20 (param column = 30)
};

#endif // GRUNTZ_NET_LATENCYLIST_H
