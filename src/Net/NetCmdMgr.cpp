// NetCmdMgr.cpp - CNetSessHost::SelectColor (0x0c4b60, __thiscall): claim the
// color slot `idx` for player `pid`. When the live-session flag (g_64bd5c->m_528)
// is set, a color already taken (CheckColorTaken==0) rejects with the chat error;
// otherwise the prior owner is released and the new one flagged. Slots are the
// session's 0x238-byte command buffers (matcher-4's CNetCmdBuf); the selection
// field is at +0x158. All callees/globals are external (reloc-masked). Field names
// are placeholders; only offsets + code bytes are load-bearing.
#include <Ints.h>
#include <rva.h>

// The per-color selection sub-object living at CNetCmdBuf+0x150 (only its +0x8
// "current owner" field is touched here).
struct CColorSlot {
    char m_0[8]; // +0x150 region
    i32 m_8;     // +0x158  current owner player id
};

// One session command buffer (0x238). idx*0x238 == idx*71*8 -> the lea/shl/sub.
struct CNetCmdBuf {
    char m_pad0[0x150];
    CColorSlot m_sel; // +0x150
    char m_pad158[0x238 - 0x150 - 12];
};

// The session/lobby manager singleton (g_64bd5c); only its +0x528 "in session"
// flag and its chat-error method are reached.
struct CNetLobbyMgr {
    char m_pad0[0x528];
    i32 m_528; // +0x528  session-active flag
    void ShowError(const char* msg, i32 code); // 0xb7e30 (__thiscall, external)
};

struct CNetSessHost {
    char m_pad0[0x5c];
    CNetCmdBuf* m_5c; // +0x5c  base of the per-player command-buffer array

    i32 SelectColor(i32 idx, i32 pid); // 0xc4b60
};

DATA(0x0024bd5c)
extern CNetLobbyMgr* g_64bd5c;

extern i32 CheckColorTaken(i32 pid);      // 0xdb2d0 (__cdecl, external)
extern void SetColorFlag(i32 a, i32 b);   // 0xdb2b0 (__cdecl, external)

RVA(0x000c4b60, 0x77)
i32 CNetSessHost::SelectColor(i32 idx, i32 pid) {
    CColorSlot* s = &m_5c[idx].m_sel;
    if (g_64bd5c->m_528 != 0) {
        i32 r = CheckColorTaken(pid);
        if (r == 0) {
            g_64bd5c->ShowError("Someone has already selected that color.", r);
            return 0;
        }
        SetColorFlag(s->m_8, 1);
        SetColorFlag(pid, 0);
    }
    s->m_8 = pid;
    return 1;
}
