#ifndef GRUNTZ_NET_NETMSGID_H
#define GRUNTZ_NET_NETMSGID_H

#include <Enums.h>

// Marks an application-defined multiplayer packet. DirectPlay system messages
// share the receive buffer but do not carry this bit; every engine packet
// writer sets it and DispatchRecvMsg rejects packets without it.
GZ_ENUM_FLAGS_BEGIN(NetPacketFlags, u8)
    NET_PACKET_APPLICATION = 0x80
GZ_ENUM_FLAGS_END(NetPacketFlags, u8)
GZ_ENUM_FLAGS_OPS(NetPacketFlags)

// The multiplayer lobby/session message id (CNetMsg::m_messageId). Every name
// below is read off what its arm in CMulti's dispatch DOES - the handler it
// calls or the flag it sets - so the domain is recovered from behaviour, not
// invented. Ids the dispatch does not handle are deliberately absent.
GZ_ENUM_BEGIN(NetMsgId)
    NETMSG_ALL_PLAYERS_READY = 0x3e8,    // m_allPlayersReady = 1
    NETMSG_PLAYER_LEFT = 0x3ea,          // OnPlayerLeft + ResetPlayerCommands
    NETMSG_PLAYER_READY = 0x3ed,         // RecordPlayerReady
    NETMSG_CHAT_LINE = 0x3f0,            // AppendEditLine
    NETMSG_KEEP_ALIVE = 0x3f6,           // handled, no side effect
    NETMSG_REQUEST_PLAYER_TABLE = 0x3f7, // BroadcastPlayerTable (host)
    NETMSG_PLAYER_TABLE = 0x3f8,         // ApplyPlayerTable
    NETMSG_REGISTER_PLAYER = 0x3f9,      // RegisterPlayerFromPacket
    NETMSG_UPDATE_PLAYER = 0x3fa,        // TrySetColor, sets m_color
    NETMSG_REMOVED_BY_HOST = 0x3fb,      // m_removedByHost
    NETMSG_VERIFY_CUSTOM_LEVEL = 0x3fc,  // m_customLevelVerificationPending
    NETMSG_GAME_CLOSED = 0x3fd,          // m_gameClosed
    NETMSG_GAME_FULL = 0x3fe,            // m_gameFull
    NETMSG_WAIT_DIALOG_REPLY = 0x402,    // m_lastSenderId, m_waitDialogReplyReceived
    NETMSG_OUT_OF_SYNC_REPORT = 0x403,   // BroadcastPlayerIdMessage + OnOutOfSync
    NETMSG_OUT_OF_SYNC = 0x404,          // OnOutOfSync
    NETMSG_PAUSE = 0x407,                // ShowMultiplayerPauseDialog
    NETMSG_DROP_TIMEOUT = 0x40c,         // notifies peers before ShowDropPlayerDialog
    NETMSG_APPLY_PLAYER_DROP = 0x410,    // ApplyPlayerDrop
    NETMSG_YOU_WERE_DROPPED = 0x411,     // reports removal, sets m_pollAbort
    NETMSG_REQUEST_CONFIG = 0x415,       // host replies via SendGameConfig
    NETMSG_CONFIG = 0x416,               // ApplyGameConfig, sets m_connectAccepted
    NETMSG_VERSION_CHECK = 0x417,        // HandleVersionCheck
    NETMSG_VERSION_MISMATCH = 0x418,     // reports a mismatched peer
    NETMSG_COLOR_REJECTED = 0x419,       // m_colorSelectionRejected
    NETMSG_LEVEL_CHECKSUM = 0x41c,       // records sender's level checksum
    NETMSG_VERIFY_OK = 0x41d,            // m_verifyDone = 1
    NETMSG_VERIFY_FAILED = 0x41e,        // m_levelVerifyResult = 0
    NETMSG_LATENCY_PROBE = 0x41f,        // peer replies with NETMSG_LATENCY_REPLY
    NETMSG_LATENCY_REPLY = 0x420,        // accumulates the peer round-trip time
    NETMSG_ACK_LATENCY_REPORT = 0x421,   // host records the peer's maximum latency
    NETMSG_OPTIONS_OPENED = 0x422,       // tracks peer and opens the wait dialog
    NETMSG_OPTIONS_CLOSED = 0x423,       // removes peer from the options count

    // The SEND-side spelling of the same ids, lifted from the TU-local enum
    // in Multi.cpp. Value-verified aliases, not a second domain.
    STAT_ALL_PLAYERS_READY = NETMSG_ALL_PLAYERS_READY,
    STAT_CHAT = NETMSG_CHAT_LINE,
    STAT_PLAYER_TABLE = NETMSG_PLAYER_TABLE,
    STAT_REGISTER_PLAYER = NETMSG_REGISTER_PLAYER,
    STAT_PLAYER_UPDATE = NETMSG_UPDATE_PLAYER,
    STAT_REMOVED_BY_HOST = NETMSG_REMOVED_BY_HOST,
    STAT_PAUSE = NETMSG_PAUSE,
    STAT_APPLY_PLAYER_DROP = NETMSG_APPLY_PLAYER_DROP,
    STAT_YOU_WERE_DROPPED = NETMSG_YOU_WERE_DROPPED,
    STAT_REQUEST_CONFIG = NETMSG_REQUEST_CONFIG,
    STAT_CONFIG = NETMSG_CONFIG,
    STAT_VERSION_CHECK = NETMSG_VERSION_CHECK,
    STAT_VERSION_MISMATCH = NETMSG_VERSION_MISMATCH,
    STAT_ACK_LATENCY_REPORT = NETMSG_ACK_LATENCY_REPORT,
    STAT_LEVEL_CHECKSUM = NETMSG_LEVEL_CHECKSUM,
    STAT_VERIFY_AGREE = NETMSG_VERIFY_OK,
    STAT_VERIFY_DISAGREE = NETMSG_VERIFY_FAILED
GZ_ENUM_END(NetMsgId)

#endif // GRUNTZ_NET_NETMSGID_H
