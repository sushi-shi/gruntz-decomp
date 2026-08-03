#include <rva.h>

#include <Rez/RezMgr.h>

#include <Gruntz/GameStateId.h>
#include <Gruntz/GruntzMgr.h>
#include <Rez/FrameClock.h>
#include <Rez/RezSync.h>
#include <Wap32/GameApp.h>

typedef CGameMgr CGameMgrBase;

DATA(0x00245580)
i32 g_lastNow = 0;
DATA(0x00245584)
u32 g_frameDelta = 0;

DATA(0x00245588)
u32 g_frameTime = 0;
DATA(0x0024558c)
i32 g_frameTicks = 0;
DATA(0x00245590)
i32 g_timer32 = 0;
DATA(0x00245598)
i32 g_timer200 = 0;
DATA(0x0024559c)
i32 g_timer400 = 0;
DATA(0x002455a0)
i32 g_timer500 = 0;
DATA(0x00245594)
i32 g_timer100 = 0;

RVA(0x0008b740, 0x12d)
i32 CGruntzMgr::PerFrameTick() {
    if (m_curState == 0) {
        return 0;
    }

    CGameMgrBase::PerFrameTick();

    GameStateId r = m_curState->Update();
    if (r != GAMESTATE_MULTI) {
        u32 dt = g_wap32FrameDelta;
        g_lastNow = g_wap32Now;
        g_frameDelta = dt;
        if (dt > 0x64) {
            dt = 0x64;
            g_frameDelta = 0x64;
        }
        g_frameTime += dt;

        u32 v;
        v = (g_timer32 == 0) ? 0x32 : g_timer32;
        if (dt >= v) {
            g_timer32 = 0;
        } else {
            g_timer32 = v - dt;
        }
        v = (g_timer100 == 0) ? 0x64 : g_timer100;
        if (dt >= v) {
            g_timer100 = 0;
        } else {
            g_timer100 = v - dt;
        }
        v = (g_timer200 == 0) ? 0xc8 : g_timer200;
        if (dt >= v) {
            g_timer200 = 0;
        } else {
            g_timer200 = v - dt;
        }
        v = (g_timer400 == 0) ? 0x190 : g_timer400;
        if (dt >= v) {
            g_timer400 = 0;
        } else {
            g_timer400 = v - dt;
        }
        v = (g_timer500 == 0) ? 0x1f4 : g_timer500;
        if (dt >= v) {
            g_timer500 = 0;
        } else {
            g_timer500 = v - dt;
        }

        g_frameTicks++;
    }

    if (m_renderGate != 0) {
        return 0;
    }

    m_curState->Render();
    return 1;
}
