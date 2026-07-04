// NetCmdMgr.cpp - CNetSessHost::SelectColor (0x0c4b60, __thiscall): claim the
// color slot `idx` for player `pid`. When the live-session flag (g_64bd5c->m_isHost)
// is set, a color already taken (ChannelSlots_Get==0) rejects with the chat error;
// otherwise the prior owner is released and the new one flagged. Slots are the
// session's 0x238-byte command buffers (matcher-4's CNetCmdBuf); the selection
// field is at +0x158. All callees/globals are external (reloc-masked). Field names
// are placeholders; only offsets + code bytes are load-bearing.
#include <Ints.h>
#include <Net/NetMgr.h>   // shared CNetCmdBuf / CColorSlot (0x238-byte command buffer)
#include <Gruntz/Multi.h> // the g_64bd5c singleton is a CMulti (xref-proven)
#include <rva.h>

struct CNetSessHost {
    char m_pad0[0x5c];
    CNetCmdBuf* m_cmdBuffers; // +0x5c  base of the per-player command-buffer array

    i32 SelectColor(i32 colorIndex, i32 playerId); // 0xc4b60
};
SIZE_UNKNOWN(CNetSessHost); // session-host view (only +0x5c pinned); size TBD

// The multiplayer game-state singleton (g_64bd5c) is a CMulti: its +0x528 is-host
// latch and the ReportVersionMsg chat-error method (0xb7e30) reached off it are both
// CMulti's (xref: the CNetMgr::ShowError label was a heuristic mis-attribution).
DATA(0x0024bd5c)
extern CMulti* g_64bd5c;

// The shared channel/color-slot array accessors (ChannelSlots.cpp, __cdecl). The
// multiplayer color picker reads/stamps a player's slot through them.
i32 ChannelSlots_Get(i32 i);       // 0xdb2d0  (color-taken read)
void ChannelSlots_Set(i32 i, i32 v); // 0xdb2b0  (color-flag stamp)

RVA(0x000c4b60, 0x77)
i32 CNetSessHost::SelectColor(i32 colorIndex, i32 playerId) {
    CColorSlot* colorSlot = &m_cmdBuffers[colorIndex].m_sel;
    if (g_64bd5c->m_isHost != 0) {
        i32 r = ChannelSlots_Get(playerId);
        if (r == 0) {
            g_64bd5c->ReportVersionMsg("Someone has already selected that color.", r);
            return 0;
        }
        ChannelSlots_Set(colorSlot->m_currentOwnerPlayerId, 1);
        ChannelSlots_Set(playerId, 0);
    }
    colorSlot->m_currentOwnerPlayerId = playerId;
    return 1;
}
