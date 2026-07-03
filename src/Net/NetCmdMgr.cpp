// NetCmdMgr.cpp - CNetSessHost::SelectColor (0x0c4b60, __thiscall): claim the
// color slot `idx` for player `pid`. When the live-session flag (g_64bd5c->m_isHost)
// is set, a color already taken (CheckColorTaken==0) rejects with the chat error;
// otherwise the prior owner is released and the new one flagged. Slots are the
// session's 0x238-byte command buffers (matcher-4's CNetCmdBuf); the selection
// field is at +0x158. All callees/globals are external (reloc-masked). Field names
// are placeholders; only offsets + code bytes are load-bearing.
#include <Ints.h>
#include <Net/NetMgr.h>    // shared CNetCmdBuf / CColorSlot (0x238-byte command buffer)
#include <Gruntz/CMulti.h> // the g_64bd5c singleton is a CMulti (xref-proven)
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

extern i32 CheckColorTaken(i32 pid);    // 0xdb2d0 (__cdecl, external)
extern void SetColorFlag(i32 a, i32 b); // 0xdb2b0 (__cdecl, external)

RVA(0x000c4b60, 0x77)
i32 CNetSessHost::SelectColor(i32 colorIndex, i32 playerId) {
    CColorSlot* colorSlot = &m_cmdBuffers[colorIndex].m_sel;
    if (g_64bd5c->m_isHost != 0) {
        i32 r = CheckColorTaken(playerId);
        if (r == 0) {
            g_64bd5c->ReportVersionMsg("Someone has already selected that color.", r);
            return 0;
        }
        SetColorFlag(colorSlot->m_currentOwnerPlayerId, 1);
        SetColorFlag(playerId, 0);
    }
    colorSlot->m_currentOwnerPlayerId = playerId;
    return 1;
}
