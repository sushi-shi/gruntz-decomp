#ifndef INCLUDE_NET_NETSESSHOST_H
#define INCLUDE_NET_NETSESSHOST_H
// CNetSessHost - the multiplayer session-host facet (the +0x5c per-player command-
// buffer array). SelectColor (0xc4b60) claims a colour slot for a player; it runs on
// `this` reading +0x5c, and is invoked BOTH on a real CNetSessHost (NetCmdMgr.cpp) and
// on the CMultiStartDlg dialog whose m_host (+0x5c) aliases the same buffer array
// (the per-slot colour handlers) - a heterogeneous +0x5c handle, so the dialog casts
// `this` to this facet at the call. Shared here so both TUs use one class (was a
// NetCmdMgr.cpp-local view).
#include <Ints.h>

class CNetCmdBuf; // the 0x238-byte per-player command buffer (Net/NetMgr.h)

struct CNetSessHost {
    char m_pad0[0x5c];
    CNetCmdBuf* m_cmdBuffers; // +0x5c  base of the per-player command-buffer array

    i32 SelectColor(i32 colorIndex, i32 playerId); // 0xc4b60
};
SIZE_UNKNOWN(CNetSessHost); // session-host view (only +0x5c pinned); size TBD

#endif // INCLUDE_NET_NETSESSHOST_H
