#ifndef GRUNTZ_NET_NETCMDSLOT_H
#define GRUNTZ_NET_NETCMDSLOT_H

#include <Ints.h>
#include <rva.h>

#pragma pack(push, 1)
struct NetCmdSendMsg {
    u8 m_flags;
    i32 m_val;
    i32 m_baseSeq;
    i32 m_checksum;
    u8 m_count;
    u8 m_payload[0x842];
};
SIZE(0x850);
#pragma pack(pop)

#pragma pack(push, 1)
struct NetGruntRecMsg {
    u8 m_flags;
    u8 m_slot;
    i32 m_seq;
    i32 m_06;
    i32 m_checksum;
    u8 m_count;
    u8 m_payload[0x3f1];
};
SIZE(0x400);
#pragma pack(pop)

extern char g_lobbyRecvBuf[0x800];
extern NetCmdSendMsg g_netCmdSendMsg;
extern NetGruntRecMsg g_netGruntRecMsg;

void NoopSync(CGruntzCommand* p);

#endif // GRUNTZ_NET_NETCMDSLOT_H
