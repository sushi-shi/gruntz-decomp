#ifndef INCLUDE_NET_NETSESSHOST_H
#define INCLUDE_NET_NETSESSHOST_H
#include <Ints.h>

class CNetCmdBuf; // the 0x238-byte per-player command buffer (Net/NetMgr.h)

struct CNetSessHost {
    char m_pad0[0x5c];
    CNetCmdBuf* m_cmdBuffers; // +0x5c  base of the per-player command-buffer array

    i32 SelectColor(i32 colorIndex, i32 playerId); // 0xc4b60
};
SIZE_UNKNOWN(CNetSessHost); // session-host view (only +0x5c pinned); size TBD

#endif // INCLUDE_NET_NETSESSHOST_H
