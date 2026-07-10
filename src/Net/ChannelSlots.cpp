// ChannelSlots.cpp - the global channel-slot in-use table (g_64c3f0[17]) and its
// free-function accessors. The table is
// a 17-DWORD array of per-channel "free" flags (1 = free, 0 = taken); CNetMgr and
// CGruntzMgr init/scan/claim channels through these. Reached only as `call rel32`
// to these symbols + a DIR32 to the table (reloc-masked); the table is modeled as
// an extern (its storage lives in retail .data).
#include <Ints.h>
#include <rva.h>
#include <Globals.h>

// Reset every slot to "free" (1).
RVA(0x000db1d0, 0x14)
void ChannelSlots_InitAll() {
    for (i32 i = 0; i < 17; i++) {
        g_64c3f0[i] = 1;
    }
}

// ---------------------------------------------------------------------------
// 0x0db200 (spatially re-homed from src/Stub/BoundaryLowerMethods.cpp). Swap the
// +0x08 holder to `arg`: no-op when already equal, else validate (0x11f9), toggle
// old off / new on (0x3bbb), and store. @orphan (owner unrecovered).
extern "C" i32 Check11f9(void* p);          // 0x11f9
extern "C" void Toggle3bbb(void* p, i32 f); // 0x3bbb
struct Cdb200 {
    char pad0[8];
    void* m_8; // +0x08
    i32 Swap(void* arg);
};
RVA(0x000db200, 0x51)
i32 Cdb200::Swap(void* arg) {
    if (m_8 == arg) {
        return 1;
    }
    if (Check11f9(arg)) {
        Toggle3bbb(m_8, 1);
        Toggle3bbb(arg, 0);
        m_8 = arg;
        return 1;
    }
    return 0;
}
SIZE_UNKNOWN(Cdb200);

// Return the index of the first free (non-zero) slot, else 0.
RVA(0x000db280, 0x1b)
i32 ChannelSlots_FindFree() {
    for (i32 i = 0; i < 17; i++) {
        if (g_64c3f0[i] != 0) {
            return i;
        }
    }
    return 0;
}

// Set slot[i] = v.
RVA(0x000db2b0, 0x10)
void ChannelSlots_Set(i32 i, i32 v) {
    g_64c3f0[i] = v;
}

// Return slot[i].
RVA(0x000db2d0, 0xc)
i32 ChannelSlots_Get(i32 i) {
    return g_64c3f0[i];
}

// ---------------------------------------------------------------------------
// 0x0db2f0 (spatially re-homed from src/Stub/BoundaryLowerMethods.cpp). Finalize:
// when +0x20 is live, run the +0x38 teardown (0x02ade0) iff +0x14 is clear, then
// reset +0x20. Returns 1/0. @orphan (owner unrecovered; the +0x38 sub is the
// 0x02ade0 CBattlezMapConfig::Clear, kept as a local reloc-masked method view).
struct CSubdb2f0 {
    void Clear_02ade0(); // 0x02ade0 (reloc-masked)
};
struct Cdb2f0 {
    char pad0[0x14];
    i32 m_14; // +0x14
    char pad18[0x20 - 0x18];
    i32 m_20; // +0x20
    char pad24[0x38 - 0x24];
    CSubdb2f0 m_38; // +0x38
    i32 Finalize();
};
RVA(0x000db2f0, 0x2b)
i32 Cdb2f0::Finalize() {
    if (m_20 == 0) {
        return 0;
    }
    if (m_14 == 0) {
        m_38.Clear_02ade0();
    }
    m_20 = 0;
    return 1;
}
SIZE_UNKNOWN(CSubdb2f0);
SIZE_UNKNOWN(Cdb2f0);
