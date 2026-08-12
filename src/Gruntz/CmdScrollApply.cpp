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
    g_scrollAccum = 0;
    g_scrollLimit = 0;
}
// @early-stop
RVA(0x000ebd70, 0x366)
void UpdateMgrScroll(CGruntzMgr* pm, class CStatusBarMgr* bar, i32 snapFlag) {
    CDDrawWorkerHost* v = pm->m_world->m_level->m_mainPlane;
    i32 scrollX = v->m_snappedX;
    i32 scrollY = v->m_snappedY;

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
    if (scrollX > v2->m_wrapW - cx) {
        scrollX = v2->m_wrapW - cx;
    }
    if (scrollY < cy - 1) {
        scrollY = cy - 1;
    }
    if (scrollY > v2->m_wrapH - cy) {
        scrollY = v2->m_wrapH - cy;
    }

    i32 deltaX = scrollX - g_lastScrollX;
    i32 deltaY = scrollY - g_lastScrollY;
    g_lastScrollX = scrollX;
    g_lastScrollY = scrollY;

    CDDrawWorkerHost* v3 = pm->m_world->m_level->m_mainPlane;
    if (!(v3->m_flags & 1)) {
        v3->m_scaledX = static_cast<float>(scrollX * v3->m_scaleX);
        v3->m_scaledY = static_cast<float>(scrollY * v3->m_scaleY);
    } else {
        v3->m_scaledX = static_cast<float>(scrollX);
        v3->m_scaledY = static_cast<float>(scrollY);
    }
    v3->RecomputePlaneCoords();

    CDDrawWorkerHost* gm = g_backView;
    if (gm != NULL) {
        i32 nx = gm->m_snappedX;
        i32 ny = gm->m_snappedY;
        if (deltaX != 0 || deltaY != 0) {
            nx = static_cast<i32>(
                (static_cast<float>(nx)
                 - static_cast<float>(deltaX) * DATA_COMPGEN(0x001eab3c, -0.05f))
            );
            ny = static_cast<i32>((static_cast<float>(ny) - static_cast<float>(deltaY) * -0.05f));
        }
        if (static_cast<i64>(g_frameTime) - g_scrollAccum >= g_scrollLimit) {
            nx += g_buteMgr.GetDword("BackPlane", "ScrollDistX");
            ny += g_buteMgr.GetDword("BackPlane", "ScrollDistY");
            CDDrawWorkerHost* g2 = g_backView;
            if (!(g2->m_flags & 1)) {
                g2->m_scaledX = static_cast<float>(nx * g2->m_scaleX);
                g2->m_scaledY = static_cast<float>(ny * g2->m_scaleY);
            } else {
                g2->m_scaledX = static_cast<float>(nx);
                g2->m_scaledY = static_cast<float>(ny);
            }
            g2->RecomputePlaneCoords();
            g_scrollLimit = g_buteMgr.GetDword("BackPlane", "ScrollTime");
            g_scrollAccum = g_frameTime;
        }
    }

    CDDrawSurfaceMgr* o = pm->m_world;
    pm->m_viewBounds.left = o->m_level->m_mainPlane->m_viewRect.left - 0x60;
    pm->m_viewBounds.top = o->m_level->m_mainPlane->m_viewRect.top - 0x60;
    pm->m_viewBounds.right = o->m_level->m_mainPlane->m_viewRect.right + 0x60;
    pm->m_viewBounds.bottom = o->m_level->m_mainPlane->m_viewRect.bottom + 0x60;
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

DATA(0x0024cfb0)
i64 g_scrollAccum;

DATA(0x0024cfb8)
i64 g_scrollLimit;

DATA(0x0024cfc0)
u32 g_scrollClock;

DATA(0x0024cfc4)
u32 g_scrollTimer;

DATA(0x0024cfc8)
i32 g_scrollSave18;

DATA(0x0024cfcc)
i32 g_scrollSave1c;

DATA(0x0024cfd0)
i32 g_lastScrollX;

DATA(0x0024cfd4)
i32 g_lastScrollY;
