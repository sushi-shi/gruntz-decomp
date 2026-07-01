// NetMgrWait.cpp - the CNetMgr level-verify / lobby wait loops (C:\Proj\NetMgr).
//
// Two register-pinned wait loops split out of the big NetMgr.cpp so they don't
// perturb its neighbours: Poll (0xbba10, the verify-vote poll VerifyCustomLevel
// drives through g_648cf8) and WaitForOtherPlayers (0xbb700, /GX). Both pump the
// DirectPlay session (PollSession) on a Sleep(50) beat and time out on the WINMM
// timeGetTime clock; the engine callees (SendNetStat/SendStatFlag/PollSession/
// AckJoinFailure) are reached __thiscall through incremental-link thunks and are
// declared no-body in <Net/NetMgr.h> so their `call rel32` reloc-mask.
#include <Net/NetMgr.h>
#include <rva.h>

// The global game-settings/registry singleton (_g_mgrSettings @0x64556c). Poll
// reads its four per-session player records (a 0x238-stride table based at +0x170)
// through raw dword offsets so the `mov edx,ds:0x64556c` + record walk reloc-mask.
extern "C" char* g_mgrSettings; // 0x64556c

// The frame-clock base stamp WaitForOtherPlayers republishes on exit (0x648ce8).
extern "C" i32 g_648ce8; // 0x648ce8

// The engine text renderer (__cdecl, 0x115440): obj, string, rect + six style args.
void EngStr_DrawText(
    void* obj,
    const CString* str,
    const RECT* rc,
    i32 a3,
    i32 a4,
    i32 a5,
    i32 a6,
    i32 a7,
    i32 a8
); // 0x115440

// --- WaitForOtherPlayers CMulti-family sub-objects (this object carries a lobby /
// vote layout CNetMgr models differently at these offsets; reached through views so
// the this-relative loads + reloc-masked thiscall/DIR32 referents fall out). ---
struct WaitDWordArr {               // this+0x604 (a CDWordArray of per-slot votes)
    void SetSize(i32 n, i32 grow);  // 0x1b4bad
    void SetAtGrow(i32 idx, i32 v); // 0x1b4d7c
    char m_pad0[8];
    i32 m_8; // +0x8
};
struct WaitReportGate { // m_peer (+0x524)
    char m_pad0[0x60];
    i32 m_60; // +0x60  peer-ready gate
};
struct WaitSoundZ {                       // m_4->m_48
    i32 Play(const char* name, i32 flag); // 0x138840
};
struct WaitLogic { // this->m_4
    char m_pad0[0x48];
    WaitSoundZ* m_48; // +0x48  ambient sound sub-mgr
};
struct WaitSettings { // g_mgrSettings (0x64556c)
    char m_pad0[0x14];
    i32 m_14; // +0x14  ambient-enabled gate
    char m_pad18[0x30 - 0x18];
    void* m_30; // +0x30  status-text render surface
    char m_pad34[0x8c - 0x34];
    i32 m_8c; // +0x8c  screen width
    i32 m_90; // +0x90  screen height
};

// ---------------------------------------------------------------------------
// CNetMgr::WaitForOtherPlayers (0xbb700, /GX) - after clearing the per-slot vote
// scratch, if the peer is ready (or no slot is still in state 3) latch m_534 and
// return 1. Otherwise announce the wait (stat 0x3ed), draw the "Waiting for other
// playerz..." status string, and spin (Sleep(50) + PollSession) with a 5s resend
// timer (AckJoinFailure + re-announce) and a 120s abort timer (DropTimeout), while
// accumulating elapsed time onto each state-3 slot, until m_534 latches or ESC is
// pressed. On exit republish the frame clock and, if ambient sound is enabled, play
// the "AMBIENT%d" cue. The "Waiting..." CString is the /GX frame's destructible.
// @early-stop
// Complete, structurally-faithful reconstruction (~75%); parks below 100% on three
// compounding codegen walls, NOT reloc artifacts (verified base-vs-target with
// llvm-objdump -dr - every REL32 callee / DIR32 data referent is named/masked):
//   (1) tail-merge/block-layout: retail folds the two `m_534=1; return 1` early
//       exits (peer-ready and no-state-3-slot) into one shared epilogue block both
//       sites `je`; MSVC5 duplicates the epilogue inline at each site. Not steerable
//       (same family as the sibling Poll 0xbba10 @early-stop epilogue tail-dup wall).
//   (2) status-text SetRect/EngStr_DrawText arg block: the GruntInfoText register-
//       rotation wall (docs/patterns/select-zero-mask-dest-register.md family) -
//       retail threads modeW/modeH/rect through a register rotation cl won't
//       reproduce and spills a second modeW/modeH pair, shifting the /GX frame by 4
//       bytes (sub 0x5c vs 0x58) so every esp-relative slot below diverges.
//   (3) the resend/abort timers + the reused 0-constant are register-pinned across
//       the wait loop (edi/ebx/ebp) differently than cl colours them - the same
//       zero-register/regalloc wall as Poll. Logic + control flow are byte-faithful.
RVA(0x000bb700, 0x265)
i32 CNetMgr::WaitForOtherPlayers() {
    WaitDWordArr* votes = (WaitDWordArr*)((char*)this + 0x604);
    votes->SetSize(0, -1);
    for (i32 k = 3; k != 0; k--) {
        votes->SetAtGrow(votes->m_8, 0);
    }
    if (((WaitReportGate*)m_peer)->m_60 == 1) {
        m_534 = 1;
        return 1;
    }
    i32 count = 0;
    i32* slot = (i32*)((char*)m_session + 0x20);
    for (i32 j = 4; j != 0; j--) {
        if (slot != 0 && *slot == 3) {
            count++;
        }
        slot = (i32*)((char*)slot + 0x64);
    }
    if (count == 0) {
        m_534 = 1;
        return 1;
    }

    SendStatFlag(0x3ed, 1);
    CString waitStr("Waiting for other playerz...");
    WaitSettings* g = (WaitSettings*)g_mgrSettings;
    RECT rc;
    rc.left = 0;
    rc.top = 0;
    rc.right = g->m_8c;
    rc.bottom = g->m_90;
    EngStr_DrawText(g->m_30, &waitStr, &rc, 0x82, 1, 0xff, 0xff, 0, 1);

    i32 resend = 0x1388;
    i32 abort = 0x1d4c0;
    while (m_534 == 0) {
        u32 start = timeGetTime();
        Sleep(0x32);
        PollSession();
        if (GetAsyncKeyState(0x1b) & 0x80000000) {
            return 0;
        }
        u32 elapsed = timeGetTime() - start;
        if (elapsed >= (u32)resend) {
            resend = 0;
        } else {
            resend -= elapsed;
        }
        if (elapsed >= (u32)abort) {
            abort = 0;
        } else {
            abort -= elapsed;
        }
        for (i32 off = 0; off < 0x190; off += 0x64) {
            i32* s = (i32*)((char*)m_session + 0x20 + off);
            if (*s == 3) {
                *(i32*)((char*)s + 0x10) += elapsed;
            }
        }
        if (abort == 0) {
            DropTimeout();
            abort = 0x1d4c0;
        }
        if (resend == 0) {
            resend = 0x1388;
            AckJoinFailure();
            SendStatFlag(0x3ed, 1);
        }
    }

    g_648ce8 = timeGetTime();
    if (g->m_14 != 0) {
        char buf[0x40];
        wsprintfA(buf, "AMBIENT%d", GetAmbientId());
        ((WaitLogic*)m_4)->m_48->Play(buf, 1);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CNetMgr::Poll (0xbba10, __thiscall) - block (pumping the session) until the
// custom-level verify vote resolves (m_540 latches) or the timer runs out.
//
// Two modes on the is-host latch m_528:
//   guest (m_528==0): ship the verify request (stat 0x41c), spin with a 5s resend
//     timer and a 15s abort timer; on each 5s lapse re-arm (AckJoinFailure) and
//     re-send. Exits 1 once PollSession latches m_540, 0 on the 15s timeout.
//   host  (m_528!=0): clear the per-record ack/vote latches, spin with a 15s abort
//     timer, and each pass scan the four active session records - if every present
//     record has acked (m_544[i]) then vote agree/disagree by whether every ack
//     token (m_554[i]) matches ours, push the result stat (0x41d agree / 0x41e
//     disagree), record it (m_53c) and latch m_540. Exits 1 on resolve, 0 on timeout.
//
// @early-stop
// Real codegen diff (~95.5%, NOT a reloc artifact): the body is byte-exact - both
// timer paths, the array-zero loop, the 4-record scan (0x238 stride, [eax-8]/[eax]/
// [eax-0xc] gates, the m_544/m_554 latch + token vote). objdiff MASKS REL32 call/branch
// reloc target-names (measured: renaming the ILT-thunk callees SendNetStat/PollSession/
// AckJoinFailure/SendStatFlag to the real ?...@CNetMgr@@ symbols moved the score 0.0%),
// so the thunk-routed callees are NOT the cap. The residual is an epilogue
// tail-duplication difference: our base shares one return epilogue (jne/jmp) where
// retail tail-duplicates it (je; mov eax,1; ...; ret 4) - a regalloc/block-layout
// wall, not steerable here. See docs/wall-instructions.md.
RVA(0x000bba10, 0x1fb)
i32 CNetMgr::Poll(i32 token) {
    if (m_useChannelLatency == 0) {
        SendNetStat(0x41c, token, 1);
        i32 resend = 0x1388;
        i32 abort = 0x3a98;
        m_540 = 0;
        do {
            u32 start = timeGetTime();
            Sleep(0x32);
            PollSession();
            u32 elapsed = timeGetTime() - start;
            if (elapsed >= (u32)resend) {
                resend = 0;
            } else {
                resend -= elapsed;
            }
            if (elapsed >= (u32)abort) {
                abort = 0;
            } else {
                abort -= elapsed;
            }
            if (abort == 0) {
                return 0;
            }
            if (resend == 0) {
                resend = 0x1388;
                AckJoinFailure();
                SendNetStat(0x41c, token, 1);
            }
        } while (m_540 == 0);
        return 1;
    }

    i32 abort = 0x3a98;
    m_540 = 0;
    for (i32 i = 0; i < 4; i++) {
        m_544[i] = 0;
        m_554[i] = 0;
    }
    while (m_540 == 0) {
        u32 start = timeGetTime();
        Sleep(0x32);
        PollSession();
        u32 elapsed = timeGetTime() - start;
        if (elapsed >= (u32)abort) {
            abort = 0;
        } else {
            abort -= elapsed;
        }
        if (abort == 0) {
            return 0;
        }

        i32 allAcked = 1;
        i32 allAgree = 1;
        i32* rec = (i32*)(g_mgrSettings + 0x170);
        for (i32 i = 0; i < 4; i++) {
            if (rec[-2] != m_5c0 && rec[0] != 0 && rec[-3] != 0) {
                if (m_544[i] == 0) {
                    allAcked = 0;
                } else if (!(m_554[i] == token && token != 0)) {
                    allAgree = 0;
                }
            }
            rec = (i32*)((char*)rec + 0x238);
        }
        if (allAcked != 0) {
            if (allAgree != 0) {
                SendStatFlag(0x41d, 1);
                m_53c = 1;
                m_540 = 1;
            } else {
                SendStatFlag(0x41e, 1);
                m_53c = 0;
                m_540 = 1;
            }
        }
    }
    return 1;
}
