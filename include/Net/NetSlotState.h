#ifndef NET_NETSLOTSTATE_H
#define NET_NETSLOTSTATE_H

#include <Enums.h>

// Where a multiplayer command slot is in its life, as carried by
// CNetCmdSlot::m_state.
//
// Read off the three things that write it and the sixteen that test it:
//
//   0  EMPTY   what CNetCmdSlot's constructor and ResetAll() leave behind.
//   1  DONE    always set together with `m_desc->m_doneFlag = 1`, and always
//              immediately after FullReset() - the player has left or been
//              dropped.
//   3  ACTIVE  the only state the session treats as a live participant. These
//              are the slots whose m_latency accrues each tick, the ones counted
//              before the "Waiting for other playerz..." wait, and the ones a
//              drop turns into DONE.
//
// 2 never appears - nothing writes or tests it.
GZ_ENUM_BEGIN(NetSlotState)
    NETSLOT_EMPTY = 0,
    NETSLOT_DONE = 1,
    NETSLOT_ACTIVE = 3
GZ_ENUM_END(NetSlotState)

// How many command slots a session has. CNetSession::m_slots is declared
// [NET_SLOT_COUNT], every walk over it counts down from the same number, and
// CNetCmdSlot::m_ackFlags is the same width because there is one ack per slot.
GZ_ENUM_CONST_BEGIN(NetSlotCount)
    NET_SLOT_COUNT = 4
GZ_ENUM_CONST_END(NetSlotCount)

#endif // NET_NETSLOTSTATE_H
