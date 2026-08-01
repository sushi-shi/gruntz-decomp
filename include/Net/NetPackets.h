#ifndef GRUNTZ_NET_NETPACKETS_H
#define GRUNTZ_NET_NETPACKETS_H

#include <Ints.h>
#include <rva.h>

struct CNetConfigBlob {
    u8 m_0;
    char m_pad1[3];
    i32 m_statId;
    i32 m_8;
    char m_nameA[0x80];
    char m_nameB[0x80];
    i32 m_10c;
    i32 m_110;
    i32 m_114;
    i32 m_118;
};
SIZE(0x11c);

struct CNetMsg {
    u8 m_0;
    char m_pad1[3];
    i32 m_msgId;
    i32 m_8;
    char m_c[4];
    i32 m_10;
    i32 m_14;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_NET_NETPACKETS_H
