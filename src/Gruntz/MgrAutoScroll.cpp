// MgrAutoScroll.cpp - the CGruntzMgr camera auto-scroll / clamp update (0x0ebd70).
// __cdecl helper called from the render path (CPlay::Render etc., via the ILT
// thunk at 0x2356) with arg0 = g_gameReg (the CGruntzMgr singleton). Walks the
// active view (pm->m_world->m_level->m_view), runs a timeGetTime-driven random auto-pan
// when the countdown timer (g_64cfc4) expires, clamps the scroll to the view
// bounds (centered on g_gameReg->m_modeW/m_modeH), publishes the scaled scroll to the
// view (m_scrollX/m_scrollY) + a second "back plane" view (g_64c27c) eased by 0.05*delta plus
// the ButeMgr [BackPlane] ScrollDist offsets on a 64-bit ScrollTime cadence, then
// writes the four scroll-bound fields (pm->m_viewOriginL..0x148). Field names are
// placeholders; offsets + code bytes are the load-bearing fact.
#include <DDrawMgr/DDrawSubMgrPages.h>    // the m_drawTarget pages (full def)
#include <DDrawMgr/DDrawWorkerRegistry.h> // m_imageRegistry (full def)
#include <Bute/ButeMgr.h>                 // canonical CButeMgr (one shape); pulls <Mfc.h> afx-first
#include <Gruntz/GruntzMgr.h> // canonical CGruntzMgr (game-manager singleton; one true shape)
#include <Gruntz/GameLevel.h> // canonical CGameLevel (m_world->m_level; its m_mainPlane)
#include <Wwd/WwdFile.h>      // canonical CLevelPlane (== CDDrawWorkerHost) - the scroll plane
#include <Ints.h>
#include <rva.h>
#include <Globals.h>

// The active scroll "view" (pm->m_world->m_level->m_mainPlane, and the back-plane g_64c27c)
// IS the canonical CLevelPlane (CDDrawWorkerHost): the former MgrSub/MgrSub2/ScrollView
// +0x24 CGameLevel, and every ScrollView field maps to an already-named plane member:
//   m_flags@08, m_scrollX/Y@10/14 -> m_scaledX/Y, m_scaleX/Y@18/1c, m_clampX/Y@30/34 ->
//   m_wrapW/H, m_boundL/T/R/B@40/44/48/4c -> m_originX/originY/extentX/extentY,
//   m_curScrollX/Y@84/88 -> m_snappedX/Y. Reached cast-free through the typed m_mainPlane.

// The retail [BackPlane] config reader (CButeMgr::GetDword, 0x172240, __thiscall)
// is on the canonical CButeMgr (include/Bute/ButeMgr.h).

extern "C" {
    // The game frame-clock wrapper (0xcd00, reached via the ILT thunk 0x39ae): returns
    // timeGetTime(). Reloc-masked E8 call.
    u32 GameGetTime(void);           // 0xcd00
    void RecomputePlaneCoords(void); // 0x161c90
}

// Reloc-masked engine globals (DIR32 data operands).
extern "C" CGruntzMgr* g_gameReg; // 0x64556c
// g_buteMgr (VA 0x6453d8) comes from <Bute/ButeMgr.h>.
// g_frameTime was a SECOND NAME for g_frameTime (0x245588 frame clock) - same address,
// so nothing ever defined it. Unified onto the canonical.
extern "C" u32 g_frameTime;
// g_frameDelta was a SECOND NAME for g_frameDelta (0x245584 per-frame delta) - same address,
// so nothing ever defined it. Unified onto the canonical.
extern "C" u32 g_frameDelta;
DATA(0x0024cfc0)
u32 g_scrollClock;
DATA(0x0024cfb0)
i64 g_scrollAccum;
// Last-frame scroll position (owner-TU defs; .bss, VA 0x64cfd0/0x64cfd4).
DATA(0x0024cfd0)
i32 g_lastScrollX; // 0x64cfd0
DATA(0x0024cfd4)
i32 g_lastScrollY; // 0x64cfd4

// State singletons owned by the camera auto-scroll manager (.bss, zero-init),
// RVA-ascending. Referenced by CGruntzMgr / CmdScrollApply too; the reference
// externs stay in <Globals.h>. (REHOME DD-Drain-1)
DATA(0x002452a4)
i32 g_jitterX;
DATA(0x002452cc)
i32 g_jitterY;
DATA(0x00245508)
i32 g_panMinX;
DATA(0x0024550c)
i32 g_panMaxX;
DATA(0x0024c27c)
CLevelPlane* g_backView;
DATA(0x0024cfb8)
i64 g_scrollLimit;
DATA(0x0024cfc4)
u32 g_scrollTimer;

// timeGetTime-driven random value in [lo, hi]; inlined three times by retail.
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
// __cdecl 3-arg (retail callers push 3; the former 4th `unused` param was spurious).
void UpdateMgrScroll(CGruntzMgr* pm, i32* pMode, i32 snapFlag) {
    CLevelPlane* v = pm->m_world->m_level->m_mainPlane;
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
    if (*pMode != 2) {
        cx -= 0xa0;
    }
    if (snapFlag) {
        cx = 0x60;
        cy = 0x60;
    }

    if (scrollX < cx - 1) {
        scrollX = cx - 1;
    }
    CLevelPlane* v2 = pm->m_world->m_level->m_mainPlane;
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

    CLevelPlane* v3 = pm->m_world->m_level->m_mainPlane;
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

    CLevelPlane* gm = g_backView;
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
            CLevelPlane* g2 = g_backView;
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
