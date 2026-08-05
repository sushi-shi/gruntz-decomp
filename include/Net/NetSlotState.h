#ifndef NET_NETSLOTSTATE_H
#define NET_NETSLOTSTATE_H

#include <Enums.h>

// Where a multiplayer command slot is in its life, as carried by
// CNetCmdSlot::m_state.
//
// Read off the three things that write it and the sixteen that test it:
//
//   0  EMPTY   what CNetCmdSlot's constructor and ResetAll() leave behind.
//   1  INACTIVE/DONE  assigned to an unoccupied roster entry at creation and
//                     after a participant leaves or is dropped.
//   2  LOCAL          assigned when the roster entry's slot key is the host id.
//   3  REMOTE         assigned to other live human participants; these are the
//                     slots whose latency accrues and whose incoming commands
//                     are processed.
GZ_ENUM_BEGIN(NetSlotState)
    NETSLOT_EMPTY = 0,
    NETSLOT_INACTIVE = 1,
    NETSLOT_DONE = NETSLOT_INACTIVE,
    NETSLOT_LOCAL = 2,
    NETSLOT_REMOTE = 3,
    NETSLOT_ACTIVE = NETSLOT_REMOTE
GZ_ENUM_END(NetSlotState)

// How many command slots a session has. CNetSession::m_slots is declared
// [NET_SLOT_COUNT], every walk over it counts down from the same number, and
// CNetCmdSlot::m_ackFlags is the same width because there is one ack per slot.
GZ_ENUM_CONST_BEGIN(NetSlotCount)
    NET_SLOT_COUNT = 4
GZ_ENUM_CONST_END(NetSlotCount)

#endif // NET_NETSLOTSTATE_H
