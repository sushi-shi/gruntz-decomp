#ifndef GRUNTZ_GRUNTZ_MULTISTARTDLGCTRLID_H
#define GRUNTZ_GRUNTZ_MULTISTARTDLGCTRLID_H

#include <Enums.h>

// Control ids of the multiplayer start dialog. Each one is named from the
// evidence that binds it: the AFX_MSGMAP_ENTRY that routes it to a handler, or
// the member that reads it. The chat SEND button is 0x4c6, already carried by
// <Net/NetLobbyCtrlId.h> as IDC_NETCHAT_SEND.
//
// Deliberately absent, because nothing names them: 0x512 (only feeds a
// SetWindowTextA), and the ids whose handlers are themselves placeholders -
// 0x50a..0x50d (OnEnChange50a..d) and 0x51f/0x523/0x524/0x525 (OnCmd51f etc.).
GZ_ENUM_BEGIN(MultiStartDlgCtrlId)
    IDC_MULTI_CUSTOM_WORLD = 0x42b, // OnCustomWorld
    IDC_MULTI_CHAT_INPUT = 0x42d,   // OnChatSend reads its text
    IDC_MULTI_ECHO_LATENCY = 0x4e9, // EchoLatencySettings
    IDC_MULTI_WORLD = 0x4ff,        // CommitWorldHost
    IDC_MULTI_CHANNEL0 = 0x500,     // ReconcileChannel0
    IDC_MULTI_COLOR0 = 0x501,       // OnColorSlot0
    IDC_MULTI_COLOR1 = 0x503,       // OnColorSlot1
    IDC_MULTI_COLOR2 = 0x505,       // OnColorSlot2
    IDC_MULTI_COLOR3 = 0x507,       // OnColorSlot3
    IDC_MULTI_CONNECT = 0x50e,      // ConnectStep
    IDC_MULTI_CHANNEL2 = 0x50f,     // ReconcileChannel2
    IDC_MULTI_CHANNEL3 = 0x510,     // ReconcileChannel3
    IDC_MULTI_CHAT_LOG = 0x511,     // AppendChatLine appends to it
    IDC_MULTI_SLOT0 = 0x51e,        // OnSlotSelect0
    IDC_MULTI_SLOT1 = 0x520,        // OnSlotSelect1
    IDC_MULTI_SLOT2 = 0x521,        // OnSlotSelect2
    IDC_MULTI_SLOT3 = 0x522,        // OnSlotSelect3
    IDC_MULTI_LATENCY = 0x527
        // CommitLatencyOption
GZ_ENUM_END(MultiStartDlgCtrlId)

#endif // GRUNTZ_GRUNTZ_MULTISTARTDLGCTRLID_H
