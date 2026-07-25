// NetCmdSlot.h - the NetCmdSlot TU's exported globals/data.
#ifndef GRUNTZ_NET_NETCMDSLOT_H
#define GRUNTZ_NET_NETCMDSLOT_H

#include <Ints.h>
#include <rva.h>

// The command wire message CNetSession::SendOne (0xbfeb0) builds in place and hands
// to CNetMgr::SetData. PACKED, binary-proven two ways: retail places the three i32s
// at +0x01/+0x05/+0x09, and MSVC 4-byte-aligns a standalone i32 global - only a
// 1-byte-aligned record puts one at an odd address. SendOne's send length is
// `m_payloadLen + 0xe`, and 0xe is exactly this header (1+4+4+4+1), so the payload
// begins at +0xe. The record runs 0x24a058..0x24a8a7 (0x850), ending precisely where
// the gA_* message below starts. (Ex six separate DATA() globals gB_flag..gB_data.)
#pragma pack(push, 1)
struct NetCmdSendMsg {
    u8 m_flags;          // +0x00  seq-lookahead bits (0x10 = base+2 present, 0x20 = base+3)
    i32 m_val;           // +0x01  the command value SendOne was called with
    i32 m_baseSeq;       // +0x05  slot->m_baseSeq snapshot
    i32 m_checksum;      // +0x09  record checksum
    u8 m_count;          // +0x0d  record entry count
    u8 m_payload[0x842]; // +0x0e  command payload (memcpy'd; 0x24a066..0x24a8a7)
};
SIZE(0x850);
#pragma pack(pop)

// The grunt-record wire message CNetCmdSlot::SendGruntRecord (0xbfc70) builds in
// place. Same two proofs: m_seq is an i32 at +0x02 (only a packed record 1-byte-
// aligns an i32), and the send length is `m_payloadLen + 0xf` = this header. The
// record runs 0x24a8a8..0x24aca7 - exactly 0x400, ending where s_freeList begins.
// (Ex six separate DATA() globals gA_flag..gA_data.)
#pragma pack(push, 1)
struct NetGruntRecMsg {
    u8 m_flags;          // +0x00
    u8 m_slot;           // +0x01
    i32 m_seq;           // +0x02
    i32 m_06;            // +0x06  SendGruntRecord never writes it - role unrecovered
    i32 m_checksum;      // +0x0a  record checksum
    u8 m_count;          // +0x0e  record entry count
    u8 m_payload[0x3f1]; // +0x0f  grunt-record payload (0x24a8b7..0x24aca7)
};
SIZE(0x400);
#pragma pack(pop)

extern char g_lobbyRecvBuf[0x800];
extern NetCmdSendMsg g_netCmdSendMsg;
extern NetGruntRecMsg g_netGruntRecMsg;
#endif // GRUNTZ_NET_NETCMDSLOT_H
