#ifndef GRUNTZ_NET_NETCMDSLOT_H
#define GRUNTZ_NET_NETCMDSLOT_H

#include <rva.h>

#include <Ints.h>

class CGruntzCommand;

#pragma pack(push, 1)
struct NetCmdSendMsg {
    u8 m_flags;
    i32 m_sequence;
    i32 m_windowBase;
    i32 m_checksum;
    u8 m_entryCount;
    u8 m_payload[0x3f2];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct NetGruntRecMsg {
    u8 m_flags;
    u8 m_slot;
    i32 m_sequence;
    i32 m_windowBase;
    i32 m_checksum;
    u8 m_entryCount;
    u8 m_payload[0x3f1];
};
#pragma pack(pop)

extern char g_lobbyRecvBuf[0x800];
extern NetCmdSendMsg g_netCmdSendMsg;
extern NetGruntRecMsg g_netGruntRecMsg;

void NoopSync(CGruntzCommand* p);

#endif // GRUNTZ_NET_NETCMDSLOT_H
