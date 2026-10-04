#include <StdAfx.h>
#include <Wap32/Wap32.h>
#include <cassert>
#include <cstdio>

int main() {
    {
        CGameApp app;
        assert(app.Step(100) == 0xffffffffU);
        assert(!app.IsQuitPending() && !app.TakeCloseRequest());
        app.RequestQuit(0);
        assert(app.Step(100) == 0);
        assert(!app.m_running && app.TakeCloseRequest());
        assert(!app.TakeCloseRequest() && app.Step(101) == 0xffffffffU);
    }
    {
        CGameApp app;
        app.m_gameMgr = new CGameMgr;
        app.m_gameMgr->ResetFrameTiming(100);
        app.m_appActive = true;
        app.m_running = true;
        assert(app.Step(100) == 0);
        assert(app.Step(110) == 0 && app.m_gameMgr->Timing().nowMs() == 110);
        app.RequestQuit(500);
        assert(app.Step(120) == 500);
        assert(app.m_gameMgr->Timing().nowMs() == 110);
        app.m_appActive = false;
        app.m_running = false;
        assert(app.Step(200) == 420);
        app.RequestQuit(5000);
        assert(app.Step(619) == 1 && !app.TakeCloseRequest());
        assert(app.Step(620) == 0 && app.TakeCloseRequest());
        assert(app.m_gameMgr->Timing().nowMs() == 110);
        assert(!app.TakeCloseRequest() && app.Step(700) == 0xffffffffU);
    }
    {
        CGameApp app;
        app.RequestQuit(32);
        assert(app.Step(0xfffffff0U) == 32);
        assert(app.Step(0) == 16);
        assert(app.Step(16) == 0 && app.TakeCloseRequest());
    }
    puts("Production application shutdown callback tests passed.");
    return 0;
}
