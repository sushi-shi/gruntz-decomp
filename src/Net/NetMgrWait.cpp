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
// Incremental-link-thunk reloc-naming artifact (~95.5%, code bytes byte-exact):
// verified instruction-for-instruction against retail 0xbba10 - both timer paths,
// the array-zero loop, the 4-record scan (0x238 stride, [eax-8]/[eax]/[eax-0xc]
// gates, the m_544/m_554 latch + token vote) and all three ret-4 epilogues match.
// The residual is purely the four __thiscall callees (SendNetStat/PollSession/
// AckJoinFailure/SendStatFlag) reached through the retail ILT jmp thunks 0x2955/
// 0x2c39/0x35e4/0x2e82: our base obj references ?...@CNetMgr@@ direct while the
// delinked target names the thunk stubs - a REL32-symbol-name mismatch (the
// incremental-link-thunk wall, docs/patterns objdiff-reloc-scoring). Beats every
// sibling wait-loop (WaitForConnect 74%, FrameSyncWait 71%, PollSession 60%).
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
