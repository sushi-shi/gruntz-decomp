// NetMgrMisc.cpp - small CNetMgr-cluster helpers reached through incremental-link
// thunks: a 4-dword range clear, a connect-state coordinator that fans out to the
// file-scope CNetMgr singleton (g_64bd5c), and two one-line forwarders onto
// engine singletons. All callees/globals are external (reloc-masked).
#include <Ints.h>
#include <Wap32/ZDArrayDerived.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Dialogs.h>    // CMultiStartDlg (the connect-coordinator IS this dialog)
#include <Gruntz/Multi.h>      // the g_64bd5c singleton is a CMulti (xref-proven)
#include <Gruntz/NetDlgHost.h> // CMultiStartDlg::m_host (the +0x5c transform host)
#include <rva.h>
#include <string.h>
#include <Wap32/ZVec.h>

// The file-scope multiplayer game-state singleton (g_64bd5c) the coordinator
// dispatches onto. XREF proves it is a CMulti: its +0x528 is-host latch / +0x5c0
// host index and the Broadcast* / ReportVersionMsg methods reached off it all belong
// to CMulti's 0xb6110-0xbc420 lobby cluster (Ghidra's "CNetMgr::" labels on
// 0xba810/0xbaf00 are heuristic mis-attributions of that same cluster). Re-declared
// by name here (own DATA binding for the CMulti* mangled name) so the load reloc-masks.
DATA(0x0024bd5c)
extern CMulti* g_64bd5c;

// The +0x5c host sub-object (CMultiStartDlg::m_host) whose transform the else-branch
// calls is the canonical CNetDlgHost (<Gruntz/NetDlgHost.h>, unified from the former
// per-TU CNetXform + MultiStartDlgWorld's MpDlgHost/MpWorldReg views). m_host is a
// heterogeneous handle (also a slot-array base elsewhere) so the host-facet cast stays.
// (FindOptionsSlot @0x92e80 via thunk 0x2e00.)

// NOTE: the connect-state coordinator ('this') is CMultiStartDlg, PROVEN (matcher-5):
// Drive reads [this+0x5c] (== m_host) and self-calls UpdatePlayers (0xc4230) + the
// reconcile method (0xc2ab0) - both this class's own methods on `this`. The former
// placeholder `struct CNetConnCoord` is folded into <Gruntz/Dialogs.h>'s CMultiStartDlg;
// Step -> ConnectStep, Drive -> CMultiStartDlg::Drive. These bodies stay in this unit so
// the delinker packing is undisturbed.

// A slot object whose +0x3c..+0x48 range is cleared.
struct CNetSlotAux {
    char m_pad0[0x3c];
    i32 m_3c;          // +0x3c..+0x48 (4 dwords)
    void ClearRange(); // bf120
};
SIZE_UNKNOWN(CNetSlotAux); // slot-clear view (only +0x3c range pinned); size TBD

// Singletons the forwarders dispatch onto.
DATA(0x0024be90)
extern CZDArrayDerived g_netBe90; // VA 0x64be90

struct CNetSingletonE25c {
    // Poll @0x1b9b93 IS CString::~CString; cast at the call.
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
    ((CString*)&g_netE25c)->~CString();
    return 0;
}

// ---------------------------------------------------------------------------
// One connect step: reconcile slot 1 (0xc2ab0) then the connect drive (0xc40b0).
// ---------------------------------------------------------------------------
RVA(0x000c2a20, 0x13)
void CMultiStartDlg::ConnectStep() {
    SyncChannelSlot(1);
    Drive();
}

// ---------------------------------------------------------------------------
// Drive the connect state off the file-scope CMulti: if this is the host, broadcast
// the channel table + refresh players; else transform the local id and submit it.
// ---------------------------------------------------------------------------
RVA(0x000c40b0, 0x42)
void CMultiStartDlg::Drive() {
    CMulti* netMgr = g_64bd5c;
    if (netMgr->m_isHost != 0) {
        netMgr->BroadcastChannelTable(0);
        UpdatePlayers(1); // 0xc4230 (reloc-masked; return discarded)
    } else {
        i32 transformedPlayerId = (i32)((CNetDlgHost*)m_host)->FindOptionsSlot(netMgr->m_hostIndex);
        g_64bd5c->BroadcastOneChannel(transformedPlayerId);
    }
}

// ---------------------------------------------------------------------------
// Configure the singleton with two fixed ids.
// ---------------------------------------------------------------------------
RVA(0x000c5f00, 0x15)
void NetConfigureBe90() {
    ((CZDArrayDerived*)&g_netBe90)->Construct(0x7d0, 0x7da);
}
