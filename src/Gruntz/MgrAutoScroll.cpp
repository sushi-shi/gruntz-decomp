#include <DDrawMgr/DDrawSubMgrPages.h> // the m_drawTarget pages (full def)
#include <Rez/FrameClock.h> // frame-clock band (g_frameDelta/g_frameTime/g_killCueClock/g_engineFrameDelta)
#include <Gruntz/GameRegMfcPtr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h> // m_imageRegistry (full def)
#include <Bute/ButeMgr.h>                 // canonical CButeMgr (one shape); pulls <Mfc.h> afx-first
#include <Gruntz/GruntzMgr.h> // canonical CGruntzMgr (game-manager singleton; one true shape)
#include <Gruntz/GameLevel.h> // canonical CGameLevel (m_world->m_level; its m_mainPlane)
#include <Wwd/WwdFile.h>      // canonical CDDrawWorkerHost (== CDDrawWorkerHost) - the scroll plane
#include <Ints.h>
#include <rva.h>
#include <Gruntz/StatusBarMgr.h>  // the status-bar mgr (bar->m_position gates the h-center)
#include <Gruntz/ScrollState.h>   // ex Globals.h transitive
#include <Gruntz/MgrAutoScroll.h> // own exported globals (ex Globals.h)

DATA(0x002452a4)
i32 g_jitterX;
DATA(0x002452cc)
i32 g_jitterY;
DATA(0x00245508)
i32 g_panMinX;
DATA(0x0024550c)
i32 g_panMaxX;

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
// Serializer-only dwords (MapSerializeCurve is their ONLY reader/writer - the xref
// chase in ScrollState.h dead-ends); the names are POSITIONAL, asserting no meaning.
DATA(0x0024cfc8)
i32 g_scrollSave18;
DATA(0x0024cfcc)
i32 g_scrollSave1c;
DATA(0x0024cfd0)
i32 g_lastScrollX; // 0x64cfd0
DATA(0x0024cfd4)
i32 g_lastScrollY; // 0x64cfd4

static i32 RandRange(i32 lo, i32 hi) {
    i32 range = hi - lo + 1;
    if (range == 0) {
        return (GameGetTime() & 1) ? lo : hi;
    }
    return static_cast<i32>(GameGetTime()) % range + lo;
}

// @early-stop
// ~60% regalloc wall: logic + control flow are byte-isomorphic to retail (the
// int64 ScrollTime compare, the x87 view/back-plane stores, the [BackPlane]
// ScrollDist plumbing and the m_scrollBoundL..0x148 bound writes all match instruction for
// instruction), but retail pins `pm` in ebx (loaded early between the pushes) and
// keeps scrollY in its stack home while this cl pins `pm` in ebp / scrollY in ebx;
// the register/stack-slot assignment is not source-steerable here.
RVA(0x000ebd70, 0x366)
void UpdateMgrScroll(CGruntzMgr* pm, class CStatusBarMgr* bar, i32 snapFlag) {
    CDDrawWorkerHost* v = pm->m_world->m_level->m_mainPlane;
    i32 scrollX = v->m_snappedX;
    i32 scrollY = v->m_snappedY;

    if (g_scrollClock > g_frameTime) {
        if (g_frameDelta < g_scrollTimer) {
            g_scrollTimer -= g_frameDelta;
        } else {
            g_scrollTimer = 0;
        }
        if (g_scrollTimer == 0) {
            g_scrollTimer = RandRange(g_panMinX, g_panMaxX);
            scrollX += RandRange(-g_jitterX, g_jitterX);
            scrollY += RandRange(-g_jitterY, g_jitterY);
        }
    }

    i32 cx = g_gameReg->m_modeW / 2;
    i32 cy = g_gameReg->m_modeH / 2;
    if (bar->m_position != 2) {
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

    i32 deltaY = scrollY - g_lastScrollY;
    i32 deltaX = scrollX - g_lastScrollX;
    g_lastScrollX = scrollX;
    g_lastScrollY = scrollY;

    CDDrawWorkerHost* v3 = pm->m_world->m_level->m_mainPlane;
    {
        float sx = static_cast<float>(scrollX);
        float sy = static_cast<float>(scrollY);
        if (!(v3->m_flags & 1)) {
            sx = sx * v3->m_scaleX;
            sy = sy * v3->m_scaleY;
        }
        v3->m_scaledX = sx;
        v3->m_scaledY = sy;
    }
    RecomputePlaneCoords();

    CDDrawWorkerHost* gm = g_backView;
    if (gm != 0) {
        i32 nx = gm->m_snappedX;
        i32 ny = gm->m_snappedY;
        if (deltaX != 0 || deltaY != 0) {
            nx = static_cast<i32>((static_cast<float>(nx) - static_cast<float>(deltaX) * -0.05f));
            ny = static_cast<i32>((static_cast<float>(ny) - static_cast<float>(deltaY) * -0.05f));
        }
        if (static_cast<i64>(g_frameTime) - g_scrollAccum >= g_scrollLimit) {
            nx += g_buteMgr.GetDword("BackPlane", "ScrollDistX");
            ny += g_buteMgr.GetDword("BackPlane", "ScrollDistY");
            CDDrawWorkerHost* g2 = g_backView;
            float fx = static_cast<float>(nx);
            float fy = static_cast<float>(ny);
            if (!(g2->m_flags & 1)) {
                fx = fx * g2->m_scaleX;
                fy = fy * g2->m_scaleY;
            }
            g2->m_scaledX = fx;
            g2->m_scaledY = fy;
            RecomputePlaneCoords();
            g_scrollLimit = g_buteMgr.GetDword("BackPlane", "ScrollTime");
            g_scrollAccum = g_frameTime;
        }
    }

    CDDrawSurfaceMgr* o = pm->m_world;
    pm->m_viewOriginL = o->m_level->m_mainPlane->m_originX - 0x60;
    pm->m_viewOriginT = o->m_level->m_mainPlane->m_originY - 0x60;
    pm->m_viewOriginR = o->m_level->m_mainPlane->m_extentX + 0x60;
    pm->m_viewOriginB = o->m_level->m_mainPlane->m_extentY + 0x60;
}
