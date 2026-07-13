// MgrAutoScroll.cpp - the CGruntzMgr camera auto-scroll / clamp update (0x0ebd70).
// __cdecl helper called from the render path (CPlay::Render etc., via the ILT
// thunk at 0x2356) with arg0 = g_gameReg (the CGruntzMgr singleton). Walks the
// active view (pm->m_world->m_24->m_view), runs a timeGetTime-driven random auto-pan
// when the countdown timer (g_64cfc4) expires, clamps the scroll to the view
// bounds (centered on g_gameReg->m_modeW/m_modeH), publishes the scaled scroll to the
// view (m_scrollX/m_scrollY) + a second "back plane" view (g_64c27c) eased by 0.05*delta plus
// the ButeMgr [BackPlane] ScrollDist offsets on a 64-bit ScrollTime cadence, then
// writes the four scroll-bound fields (pm->m_viewOriginL..0x148). Field names are
// placeholders; offsets + code bytes are the load-bearing fact.
#include <Bute/ButeMgr.h>     // canonical CButeMgr (one shape); pulls <Mfc.h> afx-first
#include <Gruntz/GruntzMgr.h> // canonical CGruntzMgr (game-manager singleton; one true shape)
#include <Ints.h>
#include <rva.h>
#include <Globals.h>

// The active scroll view (pm->m_world->m_24->m_view, and the back-plane g_64c27c).
// MgrSub/MgrSub2 are the CGruntzMgr::m_world sub-chain (a facet of the world/view
// object) - a SEPARATE consolidation owned by the world/CViewport modeling, so they
// stay local here and are reached by a documented view of the canonical m_world.
struct ScrollView {
    char p00[0x08];
    i32 m_flags; // +0x08  flags (&1 => skip the m_scaleX/m_scaleY scale)
    char p0c[0x10 - 0x0c];
    float m_scrollX; // +0x10  scaled scroll X
    float m_scrollY; // +0x14  scaled scroll Y
    float m_scaleX;  // +0x18  X scale
    float m_scaleY;  // +0x1c  Y scale
    char p20[0x30 - 0x20];
    i32 m_clampX; // +0x30  X clamp extent
    i32 m_clampY; // +0x34  Y clamp extent
    char p38[0x40 - 0x38];
    i32 m_boundL; // +0x40  bound source 0
    i32 m_boundT; // +0x44  bound source 1
    i32 m_boundR; // +0x48  bound source 2
    i32 m_boundB; // +0x4c  bound source 3
    char p50[0x84 - 0x50];
    i32 m_curScrollX; // +0x84  current scroll X
    i32 m_curScrollY; // +0x88  current scroll Y
};

struct MgrSub2 {
    char p00[0x5c];
    ScrollView* m_view; // +0x5c
};

struct MgrSub {
    char p00[0x24];
    MgrSub2* m_sub; // +0x24
};

// The retail [BackPlane] config reader (CButeMgr::GetDword, 0x172240, __thiscall)
// is on the canonical CButeMgr (include/Bute/ButeMgr.h).

extern "C" {
    // The game frame-clock wrapper (0xcd00, reached via the ILT thunk 0x39ae): returns
    // timeGetTime(). Reloc-masked E8 call.
    u32 GameGetTime(void);           // 0xcd00
    void RecomputePlaneCoords(void); // 0x161c90
}

// Reloc-masked engine globals (DIR32 data operands).
DATA(0x0024556c)
extern "C" CGruntzMgr* g_gameReg; // 0x64556c
DATA(0x002453d8)
extern CButeMgr g_buteMgr; // VA 0x6453d8 -> RVA 0x2453d8
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
ScrollView* g_backView;
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
    return (i32)GameGetTime() % range + lo;
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
    ScrollView* v = ((MgrSub*)pm->m_world)->m_sub->m_view;
    i32 scrollX = v->m_curScrollX;
    i32 scrollY = v->m_curScrollY;

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
    ScrollView* v2 = ((MgrSub*)pm->m_world)->m_sub->m_view;
    if (scrollX > v2->m_clampX - cx) {
        scrollX = v2->m_clampX - cx;
    }
    if (scrollY < cy - 1) {
        scrollY = cy - 1;
    }
    if (scrollY > v2->m_clampY - cy) {
        scrollY = v2->m_clampY - cy;
    }

    i32 deltaY = scrollY - g_lastScrollY;
    i32 deltaX = scrollX - g_lastScrollX;
    g_lastScrollX = scrollX;
    g_lastScrollY = scrollY;

    ScrollView* v3 = ((MgrSub*)pm->m_world)->m_sub->m_view;
    {
        float sx = (float)scrollX;
        float sy = (float)scrollY;
        if (!(v3->m_flags & 1)) {
            sx = sx * v3->m_scaleX;
            sy = sy * v3->m_scaleY;
        }
        v3->m_scrollX = sx;
        v3->m_scrollY = sy;
    }
    RecomputePlaneCoords();

    ScrollView* gm = g_backView;
    if (gm != 0) {
        i32 nx = gm->m_curScrollX;
        i32 ny = gm->m_curScrollY;
        if (deltaX != 0 || deltaY != 0) {
            nx = (i32)((float)nx - (float)deltaX * -0.05f);
            ny = (i32)((float)ny - (float)deltaY * -0.05f);
        }
        if ((i64)g_frameTime - g_scrollAccum >= g_scrollLimit) {
            nx += g_buteMgr.GetDword("BackPlane", "ScrollDistX");
            ny += g_buteMgr.GetDword("BackPlane", "ScrollDistY");
            ScrollView* g2 = g_backView;
            float fx = (float)nx;
            float fy = (float)ny;
            if (!(g2->m_flags & 1)) {
                fx = fx * g2->m_scaleX;
                fy = fy * g2->m_scaleY;
            }
            g2->m_scrollX = fx;
            g2->m_scrollY = fy;
            RecomputePlaneCoords();
            g_scrollLimit = g_buteMgr.GetDword("BackPlane", "ScrollTime");
            g_scrollAccum = g_frameTime;
        }
    }

    MgrSub* o = ((MgrSub*)pm->m_world);
    pm->m_viewOriginL = o->m_sub->m_view->m_boundL - 0x60;
    pm->m_viewOriginT = o->m_sub->m_view->m_boundT - 0x60;
    pm->m_viewOriginR = o->m_sub->m_view->m_boundR + 0x60;
    pm->m_viewOriginB = o->m_sub->m_view->m_boundB + 0x60;
}

SIZE_UNKNOWN(MgrSub);
SIZE_UNKNOWN(MgrSub2);
SIZE_UNKNOWN(ScrollView);
