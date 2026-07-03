// NetMgrMisc.cpp - small CNetMgr-cluster helpers reached through incremental-link
// thunks: a 4-dword range clear, a connect-state coordinator that fans out to the
// file-scope CNetMgr singleton (g_64bd5c), and two one-line forwarders onto
// engine singletons. All callees/globals are external (reloc-masked).
#include <Ints.h>
#include <Gruntz/CMulti.h> // the g_64bd5c singleton is a CMulti (xref-proven)
#include <rva.h>
#include <string.h>

// The file-scope multiplayer game-state singleton (g_64bd5c) the coordinator
// dispatches onto. XREF proves it is a CMulti: its +0x528 is-host latch / +0x5c0
// host index and the Broadcast* / ReportVersionMsg methods reached off it all belong
// to CMulti's 0xb6110-0xbc420 lobby cluster (Ghidra's "CNetMgr::" labels on
// 0xba810/0xbaf00 are heuristic mis-attributions of that same cluster). Re-declared
// by name here (own DATA binding for the CMulti* mangled name) so the load reloc-masks.
DATA(0x0024bd5c)
extern CMulti* g_64bd5c;

// The +0x5c sub-object whose Transform the else-branch calls.
struct CNetXform {
    i32 Transform(i32); // 0x92e80 (via thunk 0x2e00)
};
SIZE_UNKNOWN(CNetXform); // method-only transform view; retail size TBD

// The connect-state coordinator object ('this').
struct CNetConnCoord {
    char m_pad0[0x5c];
    CNetXform* m_transform; // +0x5c
    void Drive();           // c40b0
    void Step();            // c2a20
    void OpA(i32);          // c2ab0  external
    void OpC(i32);          // c4230  external
};
SIZE_UNKNOWN(CNetConnCoord); // connect-coordinator view (only +0x5c pinned); size TBD

// A slot object whose +0x3c..+0x48 range is cleared.
struct CNetSlotAux {
    char m_pad0[0x3c];
    i32 m_3c;          // +0x3c..+0x48 (4 dwords)
    void ClearRange(); // bf120
};
SIZE_UNKNOWN(CNetSlotAux); // slot-clear view (only +0x3c range pinned); size TBD

// Singletons the forwarders dispatch onto.
struct CNetSingletonBe90 {
    void Configure(i32 a, i32 b); // 0x8710 (via thunk 0x3742)
};
DATA(0x0024be90)
extern CNetSingletonBe90 g_netBe90; // VA 0x64be90

struct CNetSingletonE25c {
    i32 Poll(); // 0x1b9b93
};
SIZE_UNKNOWN(CNetSingletonE25c); // method-only singleton view; retail size TBD
DATA(0x0024e25c)
extern CNetSingletonE25c g_netE25c; // VA 0x64e25c

// ---------------------------------------------------------------------------
// Clear the slot's +0x3c..+0x48 dword range.
// ---------------------------------------------------------------------------
// @early-stop
// regalloc coin-flip (84%): the inline 16-byte memset (base pointer + 4 dword
// stores of a zero register) is byte-faithful in operation, but retail materializes
// the base in eax via `lea eax,[ecx+0x3c]` and reuses the dead `this` (ecx) as the
// zero, while cl advances ecx in place (`add ecx,0x3c`) and zeroes via eax - a pure
// pointer/zero register swap (zero-register-pinning.md family); not source-steerable
// (a plain 4-member `=0` instead folds to direct `[ecx+N]` stores, 71%). Deferred.
RVA(0x000bf120, 0x11)
void CNetSlotAux::ClearRange() {
    memset(&m_3c, 0, 16);
}

// ---------------------------------------------------------------------------
// Poll the file-scope net singleton.
// ---------------------------------------------------------------------------
RVA(0x000f9710, 0xa)
i32 NetPollE25c() {
    return g_netE25c.Poll();
}

// ---------------------------------------------------------------------------
// One connect step: option A then the connect drive.
// ---------------------------------------------------------------------------
RVA(0x000c2a20, 0x13)
void CNetConnCoord::Step() {
    OpA(1);
    Drive();
}

// ---------------------------------------------------------------------------
// Drive the connect state off the file-scope CNetMgr: if the gate is
// set, drop + advance; else transform the local id and submit it.
// ---------------------------------------------------------------------------
RVA(0x000c40b0, 0x42)
void CNetConnCoord::Drive() {
    CMulti* netMgr = g_64bd5c;
    if (netMgr->m_isHost != 0) {
        netMgr->BroadcastChannelTable(0);
        OpC(1);
    } else {
        i32 transformedPlayerId = m_transform->Transform(netMgr->m_hostIndex);
        g_64bd5c->BroadcastOneChannel(transformedPlayerId);
    }
}

// ---------------------------------------------------------------------------
// Configure the singleton with two fixed ids.
// ---------------------------------------------------------------------------
RVA(0x000c5f00, 0x15)
void NetConfigureBe90() {
    g_netBe90.Configure(0x7d0, 0x7da);
}
