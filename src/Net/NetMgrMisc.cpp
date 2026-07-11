// NetMgrMisc.cpp - the one-line singleton forwarder NetPollE25c (0xf9710).
// waveM-mech moved the 4-dword slot-range clear ClearRange (0xbf120) into the
// net command/session TU (src/Net/NetCmdSlot.cpp) it belongs to (== that TU's
// CLobbyChannel::InitSub3c). wave2-F moved the five CMultiStartDlg connect-
// coordinator fns (0xc2a20/0xc2a50/0xc2a80/0xc40b0/0xc5f00) into
// MultiStartDlgRoster.cpp - the WOVEN roster interval TU they belong to. All
// callees/globals are external (reloc-masked).
#include <Ints.h>
#include <Gruntz/Dialogs.h> // CString (the E25c forwarder's cast)
#include <rva.h>
#include <string.h>

struct CNetSingletonE25c {
    // Poll @0x1b9b93 IS CString::~CString; cast at the call.
};
SIZE_UNKNOWN(CNetSingletonE25c); // method-only singleton view; retail size TBD
DATA(0x0024e25c)
extern CNetSingletonE25c g_netE25c; // VA 0x64e25c

// ---------------------------------------------------------------------------
// Poll the file-scope net singleton.
// ---------------------------------------------------------------------------
RVA(0x000f9710, 0xa)
i32 NetPollE25c() {
    ((CString*)&g_netE25c)->~CString();
    return 0;
}
