// NetMgrMisc.cpp - two residual CNetMgr-cluster helpers reached through
// incremental-link thunks: a 4-dword slot range clear (0xbf120, inside the
// 0x0bef80 netcmd interval - re-home when that interval's TU merges) and the
// one-line singleton forwarder NetPollE25c (0xf9710). wave2-F moved the five
// CMultiStartDlg connect-coordinator fns (0xc2a20/0xc2a50/0xc2a80/0xc40b0/
// 0xc5f00) into MultiStartDlgRoster.cpp - the WOVEN roster interval TU they
// belong to. All callees/globals are external (reloc-masked).
#include <Ints.h>
#include <Gruntz/Dialogs.h> // CString (the E25c forwarder's cast)
#include <rva.h>
#include <string.h>

// A slot object whose +0x3c..+0x48 range is cleared.
struct CNetSlotAux {
    char m_pad0[0x3c];
    i32 m_3c;          // +0x3c..+0x48 (4 dwords)
    void ClearRange(); // bf120
};
SIZE_UNKNOWN(CNetSlotAux); // slot-clear view (only +0x3c range pinned); size TBD

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
