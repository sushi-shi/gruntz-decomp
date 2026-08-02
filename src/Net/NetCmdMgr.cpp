#include <rva.h>

#include <Gruntz/Dialogs.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/Multi.h>
#include <Ints.h>
#include <Net/NetMgr.h>

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
        ChannelSlots_Set(colorSlot->m_colorIndex, 1);
        ChannelSlots_Set(playerId, 0);
    }
    colorSlot->m_colorIndex = playerId;
    return 1;
}
