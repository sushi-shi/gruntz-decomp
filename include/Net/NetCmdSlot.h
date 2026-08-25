#ifndef GRUNTZ_NET_NETCMDSLOT_H
#define GRUNTZ_NET_NETCMDSLOT_H

#include <rva.h>

#include <Ints.h>

class CGruntzCommand;

// 0x400, not the old 0x850. The old size was "fill to the next known object"
// (g_netGruntRecMsg at 0x24a8a8) and is DISPROVEN: the .CRT$XC initializers at
// 0xbec90/0xbed30/0xbedd0/0xbee70/0xbeec0 construct five 0xc-byte
// GruntDirectionCell objects at 0x24a458..0x24a4a4, i.e. inside the old extent,
// so g_netCmdSendMsg (0x24a058) cannot reach past 0x24a458 - a 0x400 block whose
// 0xe-byte header leaves 0x3f2 of payload. That is the same shape as the sibling
// NetGruntRecMsg below (0x400 total, 0xf header, 0x3f1 payload), and 0x400 is the
// only size that ends exactly where the next PROVEN object begins.
// 0x24a4a4..0x24a8a8 is then unclaimed and unreferenced - a real open gap, not
// this struct's tail.
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
