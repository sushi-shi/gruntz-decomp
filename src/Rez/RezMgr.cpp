#include <StdAfx.h>

#include <Ints.h>

#include <Rez/RezMgr.h>

#include <Gruntz/GameStateId.h>
#include <Gruntz/GruntzMgr.h>
#include <Rez/FrameClock.h>
#include <Rez/FrameCountdown.h>
#include <Rez/RezSync.h>
#include <Wap32/GameApp.h>

#include <stddef.h>

typedef CGameMgr CGameMgrBase;

i32 g_lastNow = 0;

u32 g_frameDelta = 0;

u32 g_frameTime = 0;

i32 g_frameTicks = 0;

i32 g_period50CountdownMs = 0;

i32 g_period200CountdownMs = 0;

i32 g_period400CountdownMs = 0;

i32 g_period500CountdownMs = 0;

i32 g_period100CountdownMs = 0;

i32 CGruntzMgr::PerFrameTick() {
    if (m_curState == NULL) {
        return 0;
    }

    CGameMgrBase::PerFrameTick();

    GameStateId r = m_curState->Update();
    if (r != GAMESTATE_MULTI) {
        u32 dt = g_gameAppFrameDeltaMs;
        g_lastNow = g_gameAppNowMs;
        g_frameDelta = dt;
        if (dt > 0x64) {
            dt = 0x64;
            g_frameDelta = 0x64;
        }
        g_frameTime += dt;

        u32 v;
        v = (g_period50CountdownMs == 0) ? FRAME_CLOCK_PERIOD_50_MS : g_period50CountdownMs;
        g_period50CountdownMs = CountdownRemaining(v, dt);
        v = (g_period100CountdownMs == 0) ? FRAME_CLOCK_PERIOD_100_MS : g_period100CountdownMs;
        g_period100CountdownMs = CountdownRemaining(v, dt);
        v = (g_period200CountdownMs == 0) ? FRAME_CLOCK_PERIOD_200_MS : g_period200CountdownMs;
        g_period200CountdownMs = CountdownRemaining(v, dt);
        v = (g_period400CountdownMs == 0) ? FRAME_CLOCK_PERIOD_400_MS : g_period400CountdownMs;
        g_period400CountdownMs = CountdownRemaining(v, dt);
        v = (g_period500CountdownMs == 0) ? FRAME_CLOCK_PERIOD_500_MS : g_period500CountdownMs;
        g_period500CountdownMs = CountdownRemaining(v, dt);

        g_frameTicks++;
    }

    if (m_renderGate != false) {
        return 0;
    }

    m_curState->Render();
    return 1;
}
