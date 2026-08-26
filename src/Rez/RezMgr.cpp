#include <rva.h>

#include <Rez/RezMgr.h>

#include <Gruntz/GameStateId.h>
#include <Gruntz/GruntzMgr.h>
#include <Rez/FrameClock.h>
#include <Rez/RezSync.h>
#include <Wap32/GameApp.h>

#include <stddef.h>

typedef CGameMgr CGameMgrBase;

DATA(0x002464d8)
i32 g_lastNow = 0;
DATA(0x002464dc)
u32 g_frameDelta = 0;

DATA(0x002464e0)
u32 g_frameTime = 0;
DATA(0x002464e4)
i32 g_frameTicks = 0;
DATA(0x002464e8)
i32 g_period50CountdownMs = 0;
DATA(0x002464f0)
i32 g_period200CountdownMs = 0;
DATA(0x002464f4)
i32 g_period400CountdownMs = 0;
DATA(0x002464f8)
i32 g_period500CountdownMs = 0;
DATA(0x002464ec)
i32 g_period100CountdownMs = 0;

RVA(0x0008b660, 0x12d)
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
        if (dt >= v) {
            g_period50CountdownMs = 0;
        } else {
            g_period50CountdownMs = v - dt;
        }
        v = (g_period100CountdownMs == 0) ? FRAME_CLOCK_PERIOD_100_MS : g_period100CountdownMs;
        if (dt >= v) {
            g_period100CountdownMs = 0;
        } else {
            g_period100CountdownMs = v - dt;
        }
        v = (g_period200CountdownMs == 0) ? FRAME_CLOCK_PERIOD_200_MS : g_period200CountdownMs;
        if (dt >= v) {
            g_period200CountdownMs = 0;
        } else {
            g_period200CountdownMs = v - dt;
        }
        v = (g_period400CountdownMs == 0) ? FRAME_CLOCK_PERIOD_400_MS : g_period400CountdownMs;
        if (dt >= v) {
            g_period400CountdownMs = 0;
        } else {
            g_period400CountdownMs = v - dt;
        }
        v = (g_period500CountdownMs == 0) ? FRAME_CLOCK_PERIOD_500_MS : g_period500CountdownMs;
        if (dt >= v) {
            g_period500CountdownMs = 0;
        } else {
            g_period500CountdownMs = v - dt;
        }

        g_frameTicks++;
    }

    if (m_renderGate != false) {
        return 0;
    }

    m_curState->Render();
    return 1;
}
