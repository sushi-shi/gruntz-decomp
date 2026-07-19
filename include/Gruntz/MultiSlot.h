// MultiSlot.h - CMultiSlot, a player-slot record in the multiplayer-start dialog's
// m_host slot array (0x238 stride).
//
// PROVEN-FOLD (deferred): CMultiSlot IS a view of the canonical GruntzPlayer (ex CFocusSlot - now folded)
// (GameRegistry.h, the g_gameReg->m_options[4] element MultiStartDlgRoster.cpp
// already indexes as (CFocusSlot*)m_host) - m_16c IS CFocusSlot::m_16c. The +0x154
// CString player-name member is the blocker: CGameRegistry holds CFocusSlot by value,
// so adding it injects CString ctors/dtors into CGameRegistry's ctor/dtor - a
// correctness-sensitive change to a hot class that must be reconciled against the retail
// CGameRegistry ctor first (GameRegistry lane). Kept as a view until then. See also the
// twin ChannelSlot view (<Gruntz/ChannelSlot.h>).
//
// Only offsets + code bytes are load-bearing; names are placeholders.
#ifndef GRUNTZ_MULTISLOT_H
#define GRUNTZ_MULTISLOT_H

#include <rva.h>
#include <Mfc.h> // full CString (the +0x154 player-name value member)

struct CMultiSlot {
    char m_pad00[0x154];
    CString m_154; // +0x154  player-name edit contents (DoDataExchange save pass)
    char m_pad158[0x16c - 0x158];
    i32 m_16c; // +0x16c  occupancy field (== CFocusSlot::m_16c)
    char m_pad170[0x238 - 0x170];
};
SIZE_UNKNOWN(CMultiSlot);

#endif // GRUNTZ_MULTISLOT_H
