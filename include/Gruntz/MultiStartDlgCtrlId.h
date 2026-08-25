#ifndef GRUNTZ_GRUNTZ_MULTISTARTDLGCTRLID_H
#define GRUNTZ_GRUNTZ_MULTISTARTDLGCTRLID_H

#include <Enums.h>

// Controls unique to the multiplayer start dialog. The shared player-row
// controls live in DialogCtrlId; their message-map and control-operation roles
// prove player type, name, maximum Gruntz, color, and ready semantics.
GZ_ENUM_BEGIN(MultiStartDlgCtrlId)
    IDC_MULTI_CUSTOM_WORLD = 0x42b, // OnCustomWorld
    IDC_MULTI_CHAT_INPUT = 0x42d,   // OnChatSend reads its text
    IDC_MULTI_ECHO_LATENCY = 0x4e9, // EchoLatencySettings
    IDC_MULTI_WORLD = 0x4ff,        // CommitWorldSelection
    IDC_MULTI_CHAT_LOG = 0x511,     // AppendChatLine appends to it
    IDC_MULTI_GAME_NAME = 0x512,    // DoDataExchange writes GameName()
    IDC_MULTI_LATENCY = 0x527
// CommitLatencySelection
GZ_ENUM_END(MultiStartDlgCtrlId)

#endif // GRUNTZ_GRUNTZ_MULTISTARTDLGCTRLID_H
