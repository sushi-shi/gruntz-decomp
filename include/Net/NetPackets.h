#ifndef GRUNTZ_NET_NETPACKETS_H
#define GRUNTZ_NET_NETPACKETS_H

#include <Ints.h>
#include <rva.h>

// (CNetJoinPacket DISSOLVED 2026-07-27: it was a second, unnamed view of the SAME
// 0x28-byte stat-0x3f9 record that <Net/NetMgr.h>'s CNetChannelPacket already
// names field-for-field - same flag byte, same statId slot, same +0x08..+0x0e byte
// run, same i32 at +0x10 and 0x14-byte name at +0x14. CreateLocalPlayer now builds
// the named one.)

struct CNetConfigBlob {
    u8 m_0; // +0x000  flag byte (bit7)
    char m_pad1[3];
    i32 m_statId;       // +0x004  stat id (0x416)
    i32 m_8;            // +0x008  m_5b0
    char m_nameA[0x80]; // +0x00c  config name A
    char m_nameB[0x80]; // +0x08c  config name B
    i32 m_10c;          // +0x10c  <- CMulti::m_5a4
    i32 m_110;          // +0x110  <- CMulti::m_drainReload
    i32 m_114;          // +0x114  <- CMulti::m_600
    i32 m_118;          // +0x118  <- CMulti::m_rngSeed
};
SIZE(0x11c);

struct CNetMsg {
    u8 m_0; // +0x00  flag byte (bit7 => "process me")
    char m_pad1[3];
    i32 m_msgId; // +0x04  message id (switch tag)
    i32 m_8;     // +0x08  payload word (id / value / timestamp; byte +0x09 = channel)
    char m_c[4]; // +0x0c  chat text start / channel payload (byte +0x0d)
    i32 m_10;    // +0x10
    i32 m_14;    // +0x14  player id (channel-assign path)
};
SIZE_UNKNOWN();

#endif // GRUNTZ_NET_NETPACKETS_H
