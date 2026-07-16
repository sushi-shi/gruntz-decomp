// NetPackets.h - the fixed-layout DirectPlay stat payloads CMulti builds on the stack
// and ships through the stat writer. These are FULLY KNOWN wire structs (every field is
// pinned, both sizes are exact), not placeholder views - they were defined inside
// src/Gruntz/Multi.cpp, which is the one thing a type may never be. The shape belongs in
// a header; the TU that fills them includes it.
//
// Both share the stat-payload prologue: a flag byte at +0x00 whose bit 7 marks the
// packet, three bytes of padding, then the stat id at +0x04.
#ifndef GRUNTZ_NET_NETPACKETS_H
#define GRUNTZ_NET_NETPACKETS_H

#include <Ints.h>
#include <rva.h>

// The "player joined" announce packet CreateLocalPlayer builds and ships as stat 0x3f9:
// a flag byte, the stat id, a small fixed config block, the local player id, then the
// 0x14-byte name buffer (strcpy'd).
SIZE(CNetJoinPacket, 0x28);
struct CNetJoinPacket {
    u8 m_0; // +0x00  flag byte (bit7)
    char m_pad1[3];
    i32 m_4;         // +0x04  stat id (0x3f9)
    u8 m_8;          // +0x08
    u8 m_9;          // +0x09
    u8 m_a;          // +0x0a
    u8 m_b;          // +0x0b
    u8 m_c;          // +0x0c
    u8 m_d;          // +0x0d
    u8 m_e;          // +0x0e
    char m_padf;     // +0x0f
    i32 m_10;        // +0x10  local player id (m_localPlayerId)
    char m_14[0x14]; // +0x14  player name (strcpy)
};

// The command-timing config blob CMulti::SaveConfig serializes and ships as stat 0x416:
// the config word, the two config names (wsprintfA'd in), and the four timing dwords.
SIZE(CNetConfigBlob, 0x11c);
struct CNetConfigBlob {
    u8 m_0; // +0x000  flag byte (bit7)
    char m_pad1[3];
    i32 m_4;            // +0x004  stat id (0x416)
    i32 m_8;            // +0x008  m_5b0
    char m_nameA[0x80]; // +0x00c  config name A
    char m_nameB[0x80]; // +0x08c  config name B
    i32 m_10c;          // +0x10c  m_cmdDelay
    i32 m_110;          // +0x110  m_resend
    i32 m_114;          // +0x114  m_600
    i32 m_118;          // +0x118  m_2d8
};

// The RECEIVE-side message CMulti::DispatchRecvMsg reinterprets an incoming DirectPlay
// data buffer as (0xb9750). Sibling of the send structs above: the same flag-byte@+0x00
// (bit7 => "process me") + message-id@+0x04 prologue, then a payload word, an inline chat
// text run, and the channel-assign player id. A fully-known wire struct.
struct CNetMsg {
    u8 m_0; // +0x00  flag byte (bit7 => "process me")
    char m_pad1[3];
    i32 m_4;     // +0x04  message id (switch tag)
    i32 m_8;     // +0x08  payload word (id / value / timestamp; byte +0x09 = channel)
    char m_c[4]; // +0x0c  chat text start / channel payload (byte +0x0d)
    i32 m_10;    // +0x10
    i32 m_14;    // +0x14  player id (channel-assign path)
};

#endif // GRUNTZ_NET_NETPACKETS_H
