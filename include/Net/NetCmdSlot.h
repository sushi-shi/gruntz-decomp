#ifndef GRUNTZ_NET_NETCMDSLOT_H
#define GRUNTZ_NET_NETCMDSLOT_H

#include <rva.h>

#include <Ints.h>
#include <Net/NetPacketLayout.h>

class CGruntzCommand;

#pragma pack(push, 1)
struct NetCmdSendMsg {
    u8 m_flags;
    i32 m_sequence;
    i32 m_windowBase;
    i32 m_checksum;
    u8 m_entryCount;
    u8 m_payload[NET_COMMAND_WIRE_BUFFER_BYTES - 3 * sizeof(i32) - 2 * sizeof(u8)];
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
    u8 m_payload[NET_COMMAND_WIRE_BUFFER_BYTES - 3 * sizeof(i32) - 3 * sizeof(u8)];
};
#pragma pack(pop)

extern char g_lobbyRecvBuf[NET_RECEIVE_BUFFER_BYTES];
extern NetCmdSendMsg g_netCmdSendMsg;
extern NetGruntRecMsg g_netGruntRecMsg;

void NoopSync(CGruntzCommand* p);

#endif // GRUNTZ_NET_NETCMDSLOT_H
