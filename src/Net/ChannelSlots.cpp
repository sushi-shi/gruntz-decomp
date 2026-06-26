// ChannelSlots.cpp - the global channel-slot in-use table (g_64c3f0[17]) and its
// free-function accessors, migrated from the engine_boundary backlog. The table is
// a 17-DWORD array of per-channel "free" flags (1 = free, 0 = taken); CNetMgr and
// CGruntzMgr init/scan/claim channels through these. Reached only as `call rel32`
// to these symbols + a DIR32 to the table (reloc-masked); the table is modeled as
// an extern (its storage lives in retail .data).
#include <Ints.h>
#include <rva.h>

DATA(0x0024c3f0)
extern "C" i32 g_64c3f0[17];

// 0x0db1d0 - reset every slot to "free" (1).
RVA(0x000db1d0, 0x14)
void ChannelSlots_InitAll() {
    for (i32 i = 0; i < 17; i++) {
        g_64c3f0[i] = 1;
    }
}

// 0x0db280 - return the index of the first free (non-zero) slot, else 0.
RVA(0x000db280, 0x1b)
i32 ChannelSlots_FindFree() {
    for (i32 i = 0; i < 17; i++) {
        if (g_64c3f0[i] != 0) {
            return i;
        }
    }
    return 0;
}

// 0x0db2b0 - set slot[i] = v.
RVA(0x000db2b0, 0x10)
void ChannelSlots_Set(i32 i, i32 v) {
    g_64c3f0[i] = v;
}

// 0x0db2d0 - return slot[i].
RVA(0x000db2d0, 0xc)
i32 ChannelSlots_Get(i32 i) {
    return g_64c3f0[i];
}
