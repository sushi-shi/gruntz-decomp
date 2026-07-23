#include <Ints.h>
#include <Net/NetMgr.h>       // shared CNetCmdBuf / SlotInfo (0x238-byte command buffer)
#include <Gruntz/Multi.h>     // the g_multiState singleton is a CMulti (xref-proven)
#include <Gruntz/Dialogs.h>   // CMultiStartDlg - SelectColor's real receiver (ex CNetSessHost)
#include <Gruntz/GruntzMgr.h> // CGruntzMgr - m_slots (its m_options[] is the slot array)
#include <Gruntz/GruntzPlayer.h>
#include <rva.h>

DATA(0x0024bd5c)
CMulti* g_multiState;

RVA(0x000c4b60, 0x77)
i32 CMultiStartDlg::SelectColor(i32 colorIndex, i32 playerId) {
    GruntzPlayer* colorSlot = &m_host->m_options[colorIndex];
    if (g_multiState->m_isHost != 0) {
        i32 r = ChannelSlots_Get(playerId);
        if (r == 0) {
            g_multiState->ReportVersionMsg("Someone has already selected that color.", r);
            return 0;
        }
        ChannelSlots_Set(colorSlot->m_008, 1);
        ChannelSlots_Set(playerId, 0);
    }
    colorSlot->m_008 = playerId;
    return 1;
}
