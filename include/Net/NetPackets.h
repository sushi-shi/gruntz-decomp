#ifndef GRUNTZ_NET_NETPACKETS_H
#define GRUNTZ_NET_NETPACKETS_H

#include <rva.h>

#include <Ints.h>

struct CNetConfigBlob {
    u8 m_flags;
    char m_pad1[3];
    i32 m_statId;
    i32 m_customLevel;
    char m_nameA[0x80];
    char m_nameB[0x80];
    i32 m_commandDelay;
    i32 m_resendInterval;
    i32 m_autoCommandDelay;
    i32 m_rngSeed;
};
SIZE(0x11c);

struct CNetMsg {
    u8 m_flags;
    char m_pad1[3];
    i32 m_msgId;
    i32 m_value;
    char m_payload[4];
};
SIZE_UNKNOWN();

#endif // GRUNTZ_NET_NETPACKETS_H
