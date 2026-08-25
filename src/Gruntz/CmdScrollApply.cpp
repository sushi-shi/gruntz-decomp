#include <rva.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/MgrAutoScroll.h>
#include <Gruntz/ScrollState.h>
#include <Gruntz/StatusBarDock.h>
#include <Gruntz/StatusBarMgr.h>
#include <Ints.h>
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
void UpdateMgrScroll(CGruntzMgr* pm, class CStatusBarMgr* bar, i32 snapFlag) {
    CDDrawWorkerHost* v = pm->m_world->m_level->m_mainPlane;
    i32 scrollX = v->m_scrollPixelX;
    i32 scrollY = v->m_scrollPixelY;

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
            scrollX += jitterX;
            scrollY += jitterY;
        }
    }

    tagSIZE screenSize = g_gameReg->m_modeSize;
    i32 cx = screenSize.cx / 2;
    i32 cy = screenSize.cy / 2;
    if (bar->m_position != STATUSBAR_HIDDEN) {
        cx -= 0xa0;
    }
    if (snapFlag) {
        cx = 0x60;
        cy = 0x60;
    }

    if (scrollX < cx - 1) {
        scrollX = cx - 1;
    }
    CDDrawWorkerHost* v2 = pm->m_world->m_level->m_mainPlane;
    if (scrollX > v2->m_planePixelWidth - cx) {
        scrollX = v2->m_planePixelWidth - cx;
    }
    if (scrollY < cy - 1) {
        scrollY = cy - 1;
    }
    if (scrollY > v2->m_planePixelHeight - cy) {
        scrollY = v2->m_planePixelHeight - cy;
    }

    i32 deltaX = scrollX - g_lastScrollX;
    i32 deltaY = scrollY - g_lastScrollY;
    g_lastScrollX = scrollX;
    g_lastScrollY = scrollY;

    CDDrawWorkerHost* v3 = pm->m_world->m_level->m_mainPlane;
    SET_SCROLL_POSITION_PRODUCT_CAST(v3, scrollX, scrollY);

    CDDrawWorkerHost* gm = g_backView;
    if (gm != NULL) {
        i32 nx = gm->m_scrollPixelX;
        i32 ny = gm->m_scrollPixelY;
        if (deltaX != 0 || deltaY != 0) {
            nx = static_cast<i32>((static_cast<float>(nx) - static_cast<float>(deltaX) * -0.05f));
            ny = static_cast<i32>((static_cast<float>(ny) - static_cast<float>(deltaY) * -0.05f));
        }
        if (static_cast<i64>(g_frameTime) - g_scrollPace.m_lastTime >= g_scrollPace.m_period) {
            nx += g_buteMgr.GetDword("BackPlane", "ScrollDistX");
            ny += g_buteMgr.GetDword("BackPlane", "ScrollDistY");
            CDDrawWorkerHost* g2 = g_backView;
            SET_SCROLL_POSITION_PRODUCT_CAST(g2, nx, ny);
            g_scrollPace.m_period = g_buteMgr.GetDword("BackPlane", "ScrollTime");
            g_scrollPace.m_lastTime = g_frameTime;
        }
    }

    CDDrawSurfaceMgr* o = pm->m_world;
    pm->m_viewBounds.left = o->m_level->m_mainPlane->m_planeViewRect.left - 0x60;
    pm->m_viewBounds.top = o->m_level->m_mainPlane->m_planeViewRect.top - 0x60;
    pm->m_viewBounds.right = o->m_level->m_mainPlane->m_planeViewRect.right + 0x60;
    pm->m_viewBounds.bottom = o->m_level->m_mainPlane->m_planeViewRect.bottom + 0x60;
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
