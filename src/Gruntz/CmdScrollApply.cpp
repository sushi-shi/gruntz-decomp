#include <rva.h>

#include <MfcWin.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/DoubleVector.h>
#include <Gruntz/MgrAutoScroll.h>
#include <Gruntz/ScrollState.h>
#include <Gruntz/StatusBarDock.h>
#include <Gruntz/StatusBarMgr.h>
#include <Ints.h>
#include <MakeRect.h>
#include <Rez/FrameClock.h>
#include <Wwd/WwdFile.h>

#include <stddef.h>

static inline i32 RandRange(CGruntzMgr* mgr, i32 lo, i32 hi) {
    i32 range = hi - lo + 1;
    if (range == 0) {
        return (mgr->Rand() & 1) ? lo : hi;
    }
    return mgr->Rand() % range + lo;
}

RVA(0x000ebd30, 0x21)
void Cmd_ResetScroll() {
    g_scrollClock = 0;
    g_scrollTimer = 0;
    g_scrollPace.m_lastTime = 0;
    g_scrollPace.m_period = 0;
}
// @early-stop
RVA(0x000ebd70, 0x366)
void UpdateMgrScroll(CGruntzMgr* pm, class CStatusBarMgr* bar, b32 snapFlag) {
    CDDrawWorkerHost* v = pm->m_world->m_level->m_mainPlane;
    Coord scrollPosition = v->m_scrollPixel;

    if (g_scrollClock > g_frameTime) {
        if (g_frameDelta >= g_scrollTimer) {
            g_scrollTimer = 0;
        } else {
            g_scrollTimer -= g_frameDelta;
        }
        if (g_scrollTimer == 0) {
            g_scrollTimer = RandRange(pm, g_panMinX, g_panMaxX);
            i32 jitterX = RandRange(pm, -g_jitterX, g_jitterX);
            i32 jitterY = RandRange(pm, -g_jitterY, g_jitterY);
            Coord jitter(jitterX, jitterY);
            scrollPosition += jitter;
        }
    }

    CSize screenSize = g_gameReg->m_modeSize;
    CSize halfViewport(screenSize.cx / 2, screenSize.cy / 2);
    if (bar->m_position != STATUSBAR_HIDDEN) {
        halfViewport.cx -= 0xa0;
    }
    if (snapFlag) {
        halfViewport = CSize(0x60, 0x60);
    }

    CDDrawWorkerHost* v2 = pm->m_world->m_level->m_mainPlane;
    scrollPosition.Max(Coord(halfViewport.cx - 1, halfViewport.cy - 1));
    scrollPosition.Min(
        Coord(v2->m_planePixelSize.cx - halfViewport.cx, v2->m_planePixelSize.cy - halfViewport.cy)
    );

    Coord previousScroll(g_lastScrollX, g_lastScrollY);
    Coord scrollDelta = scrollPosition - previousScroll;
    g_lastScrollX = scrollPosition.m_x;
    g_lastScrollY = scrollPosition.m_y;

    CDDrawWorkerHost* v3 = pm->m_world->m_level->m_mainPlane;
    SET_SCROLL_POSITION_PRODUCT_CAST(v3, scrollPosition.m_x, scrollPosition.m_y);

    CDDrawWorkerHost* gm = g_backView;
    if (gm != NULL) {
        Coord backScroll = gm->m_scrollPixel;
        if (scrollDelta != Coord(0, 0)) {
            FloatVector2 parallax(backScroll);
            parallax -= FloatVector2(scrollDelta) * -0.05f;
            backScroll = parallax.ToCoord();
        }
        if (static_cast<i64>(g_frameTime) - g_scrollPace.m_lastTime >= g_scrollPace.m_period) {
            i32 paceX = g_buteMgr.GetDword("BackPlane", "ScrollDistX");
            i32 paceY = g_buteMgr.GetDword("BackPlane", "ScrollDistY");
            Coord pace(paceX, paceY);
            backScroll += pace;
            CDDrawWorkerHost* g2 = g_backView;
            SET_SCROLL_POSITION_PRODUCT_CAST(g2, backScroll.m_x, backScroll.m_y);
            g_scrollPace.m_period = g_buteMgr.GetDword("BackPlane", "ScrollTime");
            g_scrollPace.m_lastTime = g_frameTime;
        }
    }

    const RECT& planeView = pm->m_world->m_level->m_mainPlane->m_planeViewRect;
    pm->m_viewBounds = MakeRect(
        planeView.left - 0x60,
        planeView.top - 0x60,
        planeView.right + 0x60,
        planeView.bottom + 0x60
    );
}

RVA(0x000ec1c0, 0x43)
void Cmd_ApplyScrollParams(i32 durationMs, i32 jitterX, i32 jitterY, i32 panMinX, i32 panMaxX) {
    i32 t = durationMs + g_frameTime;
    if (g_scrollClock <= static_cast<u32>(t)) {
        g_scrollClock = t;
    }
    g_jitterX = jitterX;
    g_jitterY = jitterY;
    g_panMinX = panMinX;
    g_panMaxX = panMaxX;
}
DATA(0x002452a4)
i32 g_jitterX;

DATA(0x002452cc)
i32 g_jitterY;

DATA(0x0024c27c)
CDDrawWorkerHost* g_backView;

RVA_DYNINIT(0x000ebd00, 0x17, g_scrollPace)
DATA(0x0024cfb0)
ScrollPace g_scrollPace;

DATA(0x0024cfc0)
u32 g_scrollClock;

DATA(0x0024cfc4)
u32 g_scrollTimer;

DATA(0x0024cfc8)
i32 g_serializedScrollReservedFirst;

DATA(0x0024cfcc)
i32 g_serializedScrollReservedSecond;

DATA(0x0024cfd0)
i32 g_lastScrollX;

DATA(0x0024cfd4)
i32 g_lastScrollY;
