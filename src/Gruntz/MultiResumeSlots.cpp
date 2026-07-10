// MultiResumeSlots.cpp - CMulti's CState-override slots 9/10 (the multiplayer
// resume/per-frame handlers) homed from the GapFunctions stubs. CMulti : CPlay
// (: CState); these override the base per-frame virtuals and, after chaining the
// base reset + the manager tick, restore the game clock and re-seed the net
// drain/frame timers. The CNetMgr facet is reached through a cast on `this` (the
// documented CMulti/CNetMgr dual-role, per Multi.h). Kept in its own /GX unit so
// the CNetMgr include cannot perturb the matched Multi.cpp regalloc. Only offsets +
// code bytes are load-bearing.
#include <Gruntz/Multi.h>     // CMulti (: CPlay : CState); m_savedClock, m_connected, timer members
#include <Gruntz/GruntzMgr.h> // CGruntzMgr::PerFrameTick (0x8f620, m_4)
#include <Net/NetMgr.h>       // CNetMgr::SendNetStat (0xb9290, this-facet)
#include <Globals.h>          // g_645588 (the running game clock)
#include <rva.h>

// The cached timeGetTime import fn-ptr (0x6c4650); pinned in a callee-saved reg.
extern "C" u32(WINAPI* g_pTimeGetTime)();

// CMulti::Vslot09 (0x0b6330): chain the base ResetForMode gate; on success run the
// manager per-frame tick, restore the saved game clock, and re-seed the net drain /
// frame timer block (two timeGetTime samples), then push a net stat when connected.
// @early-stop
// ~98.6%: logic + every instruction byte-faithful. Residual is a single callee-saved
// register-NAMING swap: retail pins the cached g_pTimeGetTime fn-ptr in ebx and the
// zero in edi; cl picks edi for the fn-ptr and ebx for the zero. Invariant to
// materialization order (tried the tg decl before/after the first zero store) and
// to the permuter - a non-steerable regalloc coin-flip between the two pushed
// callee-saved regs.
RVA(0x000b6330, 0x89)
i32 CMulti::Vslot09(i32 arg) {
    if (ResetForMode(arg) == 0) {
        return 0;
    }
    m_4->CGruntzMgr::PerFrameTick();
    g_645588 = m_savedClock;
    u32(WINAPI * tg)() = g_pTimeGetTime;
    m_drainTimer = 0;
    m_lastTime = tg();
    m_frameDelta = 0;
    m_5ec = 0;
    m_5e8 = 0;
    m_accumTime = 0;
    m_5e4 = tg();
    if (m_connected != 0) {
        ((CNetMgr*)this)->SendNetStat(0x402, 0x4d2, 1);
    }
    return 1;
}
