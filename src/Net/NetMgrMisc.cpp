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

// 0x24e25c is the global asset-root path CString (canonical `g_assetRoot`, also bound by
// SplashState.cpp / GruntzMgr.cpp / TitleAppStart.cpp); NetPollE25c just destructs it.
// Modeled as the real CString (was a fake CNetSingletonE25c view whose `g_netE25c` name
// won the per-rva keep-last dedup and starved TitleAppStart's alias -> single-sourced
// onto the canonical name here so every user's DIR32 binds to the one 0x24e25c symbol).
DATA(0x0024e25c)
extern CString g_assetRoot; // VA 0x64e25c

// ---------------------------------------------------------------------------
// Poll the file-scope net singleton (destruct the asset-root CString).
// ---------------------------------------------------------------------------
RVA(0x000f9710, 0xa)
i32 NetPollE25c() {
    g_assetRoot.~CString();
    return 0;
}
