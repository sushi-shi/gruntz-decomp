#ifndef GRUNTZ_NET_NETPACKETS_H
#define GRUNTZ_NET_NETPACKETS_H

#include <rva.h>

#include <Ints.h>
#include <Net/NetMsgId.h>

struct CNetGameConfigPacket {
    u8 m_flags;
    char m_pad1[3];
    NetMsgId m_messageId;
    i32 m_usesCustomLevel;
    char m_builtInLevelName[0x80];
    char m_customLevelName[0x80];
    i32 m_commandDelay;
    i32 m_resendInterval;
    i32 m_autoCommandDelay;
    i32 m_rngSeed;
};

struct CNetMsg {
    u8 m_flags;
    char m_pad1[3];
    NetMsgId m_messageId;
    i32 m_value;
    char m_payload[4];
};

#endif // GRUNTZ_NET_NETPACKETS_H
