#ifndef GRUNTZ_NET_NETCMDSLOT_H
#define GRUNTZ_NET_NETCMDSLOT_H

#include <rva.h>

#include <Enums.h>
#include <Ints.h>
#include <Net/NetPacketLayout.h>

class CGruntzCommand;

GZ_ENUM_FLAGS_BEGIN(NetCmdReceiptFlags, u8)
    NET_CMD_RECEIVED_WINDOW_BASE_PLUS_TWO = 0x10,
    NET_CMD_RECEIVED_WINDOW_BASE_PLUS_THREE = 0x20
GZ_ENUM_FLAGS_END(NetCmdReceiptFlags, u8)
GZ_ENUM_FLAGS_OPS(NetCmdReceiptFlags)

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
