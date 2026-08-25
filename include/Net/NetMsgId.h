#ifndef GRUNTZ_NET_NETMSGID_H
#define GRUNTZ_NET_NETMSGID_H

#include <Enums.h>

// The multiplayer lobby/session message id (CNetCmdPacket::m_msgId). Every name
// below is read off what its arm in CMulti's dispatch DOES - the handler it
// calls or the flag it sets - so the domain is recovered from behaviour, not
// invented. Ids the dispatch does not handle are deliberately absent.
GZ_ENUM_BEGIN(NetMsgId)
    NETMSG_ALL_PLAYERS_READY = 0x3e8,   // m_allPlayersReady = 1
    NETMSG_PLAYER_LEFT = 0x3ea,         // OnPlayerLeft + ResetPlayerCommands
    NETMSG_DROP_PLAYER = 0x3ed,         // RecordDropAcknowledgement
    NETMSG_CHAT_LINE = 0x3f0,           // AppendEditLine
    NETMSG_LOBBY_TICK = 0x3f6,          // handled, no side effect
    NETMSG_REQUEST_CHANNELS = 0x3f7,    // BroadcastChannelTable (host)
    NETMSG_CHANNEL_TABLE = 0x3f8,       // ApplyChannelTable
    NETMSG_REGISTER_PLAYER = 0x3f9,     // RegisterChannelFromPacket
    NETMSG_SWAP_CHANNEL = 0x3fa,        // SwapChannel, sets m_colorIndex
    NETMSG_REMOVED_BY_HOST = 0x3fb,     // m_removedByHost
    NETMSG_VERIFY_CUSTOM_LEVEL = 0x3fc, // m_customLevelVerificationPending
    NETMSG_GAME_CLOSED = 0x3fd,         // m_gameClosed
    NETMSG_GAME_FULL = 0x3fe,           // m_gameFull
    NETMSG_WAIT_DIALOG_REPLY = 0x402,   // m_lastSenderId, m_waitDialogReplyReceived
    NETMSG_OUT_OF_SYNC_REPORT = 0x403,  // SendStatFlag + OnOutOfSync
    NETMSG_OUT_OF_SYNC = 0x404,         // OnOutOfSync
    NETMSG_PAUSE = 0x407,               // OnMultiPause
    NETMSG_DROP_TIMEOUT = 0x40c,        // CheckDropTimeout notifies peers before OnDropPlayer
    NETMSG_SAVE_CONFIG = 0x415,         // SaveConfig
    NETMSG_LOAD_CONFIG = 0x416,         // LoadConfig, sets m_connectAccepted
    NETMSG_VERSION_CHECK = 0x417,       // HandleVersionCheck
    NETMSG_PLAYER_NAME = 0x418,         // GetName + Format
    NETMSG_COLOR_REJECTED = 0x419,      // m_colorSelectionRejected
    NETMSG_DROP_PLAYER_ACK = 0x410,     // AckDropPlayer
    NETMSG_POLL_ABORT = 0x411,          // m_pollAbort, ReportVersionMsg
    NETMSG_LEVEL_CHECKSUM = 0x41c,      // records sender's level checksum
    NETMSG_VERIFY_OK = 0x41d,           // m_verifyDone = 1
    NETMSG_VERIFY_FAILED = 0x41e,       // m_levelVerifyResult = 0
    NETMSG_STAT_REQUEST = 0x41f,        // replies with SendStatValue(..., 0x420)
    NETMSG_STAT_VALUE = 0x420,          // the reply: sets m_count / m_avg
    NETMSG_STAT_DONE = 0x421,
    NETMSG_OPTIONS_PRESENT = 0x422, // OnMultiOptions, m_presenceCounted
    NETMSG_OPTIONS_ABSENT = 0x423,  // m_presenceCounted, no OnMultiOptions

    // The SEND-side spelling of the same ids, lifted from the TU-local enum
    // in Multi.cpp. Value-verified aliases, not a second domain.
    STAT_DROP_ANNOUNCE = NETMSG_ALL_PLAYERS_READY,
    STAT_CHAT = NETMSG_CHAT_LINE,
    STAT_CHANNEL_TABLE = NETMSG_CHANNEL_TABLE,
    STAT_REGISTER_PLAYER = NETMSG_REGISTER_PLAYER,
    STAT_CHANNEL_ONE = NETMSG_SWAP_CHANNEL,
    STAT_CHANNEL_LEFT = NETMSG_REMOVED_BY_HOST,
    STAT_PAUSE = NETMSG_PAUSE,
    STAT_PLAYERLEFT = NETMSG_DROP_PLAYER_ACK,
    STAT_PLAYERLEFT_LOCAL = NETMSG_POLL_ABORT,
    STAT_CONNECTING = NETMSG_SAVE_CONFIG,
    STAT_CONFIG = NETMSG_LOAD_CONFIG,
    STAT_VERSIONPACKET = NETMSG_VERSION_CHECK,
    STAT_VERSIONMISMATCH = NETMSG_PLAYER_NAME,
    STAT_ACKLATENCY = NETMSG_STAT_DONE,
    STAT_LEVEL_CHECKSUM = NETMSG_LEVEL_CHECKSUM,
    STAT_VERIFY_AGREE = NETMSG_VERIFY_OK,
    STAT_VERIFY_DISAGREE = NETMSG_VERIFY_FAILED
GZ_ENUM_END(NetMsgId)

#endif // GRUNTZ_NET_NETMSGID_H
