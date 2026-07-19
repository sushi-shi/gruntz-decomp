// ChannelSlot.h - the per-channel multiplayer roster player-slot record.
//
// PROVEN-FOLD (deferred): ChannelSlot IS the +0x150-shifted tail sub-window of the
// canonical GruntzPlayer (ex CFocusSlot, folded) - CMultiStartDlg::SyncChannelSlot reaches it as
// (m_host + ch*0x238 + 0x150), exactly the CFocusSlot entry stride the roster's
// OnColorSlotN / UpdatePlayers already use via ((CFocusSlot*)m_host)[ch]. So
// ChannelSlot.m_14 (entry+0x164) IS CFocusSlot::m_164 and ChannelSlot.m_ready
// (entry+0x16c) IS CFocusSlot::m_16c.
//
// Fully dissolving it (and its offset-cast at the SyncChannelSlot access) needs 5 more
// CFocusSlot members at +0x150/+0x154(CString)/+0x158/+0x160/+0x170. The +0x154 CString
// is the blocker: CGameRegistry holds `CFocusSlot m_focusSlots[4]` BY VALUE, so adding a
// CString member injects 4 CString ctors/dtors into CGameRegistry's ctor/dtor - a
// correctness-sensitive change to a hot class that must be reconciled against the retail
// CGameRegistry ctor first (GameRegistry lane). Kept as a view until then.
//
// Only offsets + code bytes are load-bearing; names are placeholders.
#ifndef GRUNTZ_CHANNELSLOT_H
#define GRUNTZ_CHANNELSLOT_H

#include <rva.h>
#include <Mfc.h> // full CString (the +0x04 label value member)

struct ChannelSlot {
    i32 m_playerId;  // +0x00 player id
    CString m_label; // +0x04 label
    i32 m_slotIndex; // +0x08 slot index
    char m_pad0c[0x10 - 0x0c];
    i32 m_selectionIndex; // +0x10
    i32 m_14;             // +0x14
    char m_pad18[0x1c - 0x18];
    i32 m_ready;  // +0x1c ready flag
    i32 m_active; // +0x20 active flag
};
SIZE_UNKNOWN(ChannelSlot);

#endif // GRUNTZ_CHANNELSLOT_H
