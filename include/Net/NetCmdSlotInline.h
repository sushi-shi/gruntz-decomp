#ifndef GRUNTZ_NET_NETCMDSLOTINLINE_H
#define GRUNTZ_NET_NETCMDSLOTINLINE_H

#include <Net/NetMgr.h>

inline void ResetNetCmdSlotCommandWindow(CNetCmdSlot* slot) {
    slot->ClearCmds();
    for (i32 i = 0; i < NET_SLOT_COUNT; i++) {
        slot->m_ackFlags[i] = 0;
    }
    slot->ResetTriple(slot->m_rangeA);
    slot->ResetTriple(slot->m_rangeB);
}

#endif // GRUNTZ_NET_NETCMDSLOTINLINE_H
