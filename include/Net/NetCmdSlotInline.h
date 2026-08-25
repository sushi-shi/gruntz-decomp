#ifndef GRUNTZ_NET_NETCMDSLOTINLINE_H
#define GRUNTZ_NET_NETCMDSLOTINLINE_H

#include <Net/NetMgr.h>

inline void ResetNetCmdSlotCommandWindow(CNetCmdSlot* slot) {
    slot->ClearRecords();
    for (i32 i = 0; i < NET_SLOT_COUNT; i++) {
        slot->m_drainAckFlags[i] = 0;
    }
    slot->ClearSequenceSet(slot->m_receivedAhead);
    slot->ClearSequenceSet(slot->m_peerReceivedAhead);
}

#endif // GRUNTZ_NET_NETCMDSLOTINLINE_H
