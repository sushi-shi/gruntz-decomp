// MgrAutoScroll.cpp - the CGruntzMgr camera auto-scroll / clamp update (0x0ebd70).
// __cdecl helper called from the render path (CPlay::Render etc., via the ILT
// thunk at 0x2356) with arg0 = g_mgrSettings (the CGruntzMgr singleton). Walks the
// active view (pm->m_30->m_24->m_5c), runs a timeGetTime-driven random auto-pan
// when the countdown timer (g_64cfc4) expires, clamps the scroll to the view
// bounds (centered on g_mgrSettings->m_8c/m_90), publishes the scaled scroll to the
// view (m_10/m_14) + a second "back plane" view (g_64c27c) eased by 0.05*delta plus
// the ButeMgr [BackPlane] ScrollDist offsets on a 64-bit ScrollTime cadence, then
// writes the four scroll-bound fields (pm->m_13c..0x148). Field names are
// placeholders; offsets + code bytes are the load-bearing fact.
#include <Ints.h>
#include <rva.h>

// The active scroll view (pm->m_30->m_24->m_5c, and the back-plane g_64c27c).
struct ScrollView {
    char p00[0x08];
    i32 m_08; // +0x08  flags (&1 => skip the m_18/m_1c scale)
    char p0c[0x10 - 0x0c];
    float m_10; // +0x10  scaled scroll X
    float m_14; // +0x14  scaled scroll Y
    float m_18; // +0x18  X scale
    float m_1c; // +0x1c  Y scale
    char p20[0x30 - 0x20];
    i32 m_30; // +0x30  X clamp extent
    i32 m_34; // +0x34  Y clamp extent
    char p38[0x40 - 0x38];
    i32 m_40; // +0x40  bound source 0
    i32 m_44; // +0x44  bound source 1
    i32 m_48; // +0x48  bound source 2
    i32 m_4c; // +0x4c  bound source 3
    char p50[0x84 - 0x50];
    i32 m_84; // +0x84  current scroll X
    i32 m_88; // +0x88  current scroll Y
};

struct MgrSub2 {
    char p00[0x5c];
    ScrollView* m_5c; // +0x5c
};

struct MgrSub {
    char p00[0x24];
    MgrSub2* m_24; // +0x24
};

struct CGruntzMgr {
    char p00[0x30];
    MgrSub* m_30; // +0x30
    char p34[0x8c - 0x34];
    i32 m_8c; // +0x8c  view half-width source
    i32 m_90; // +0x90  view half-height source
    char p94[0x13c - 0x94];
    i32 m_13c; // +0x13c  scroll bound L
    i32 m_140; // +0x140 scroll bound T
    i32 m_144; // +0x144 scroll bound R
    i32 m_148; // +0x148 scroll bound B
};

// The retail [BackPlane] config reader (CButeMgr::GetDword, 0x172240, __thiscall).
struct CButeMgr {
    u32 GetDword(char* tag, char* key); // 0x172240
};

extern "C" {
// The game frame-clock wrapper (0xcd00, reached via the ILT thunk 0x39ae): returns
// timeGetTime(). Reloc-masked E8 call.
u32 GameGetTime(void);              // 0xcd00
void RecomputePlaneCoords(void);    // 0x161c90
}

// Reloc-masked engine globals (DIR32 data operands).
DATA(0x0024556c)
extern CGruntzMgr* g_mgrSettings; // 0x64556c
DATA(0x000453d8)
extern CButeMgr g_buteMgr; // 0x6453d8
extern ScrollView* g_backView; // 0x64c27c
DATA(0x00245588)
extern u32 g_frameTime; // 0x645588
DATA(0x00245584)
extern u32 g_frameDelta; // 0x645584
DATA(0x0024cfc0)
extern u32 g_scrollClock; // 0x64cfc0
extern u32 g_scrollTimer; // 0x64cfc4
extern i32 g_panMinX; // 0x645508
extern i32 g_panMaxX; // 0x64550c
extern i32 g_jitterX; // 0x6452a4
extern i32 g_jitterY; // 0x6452cc
extern i32 g_lastScrollX; // 0x64cfd0
extern i32 g_lastScrollY; // 0x64cfd4
DATA(0x0024cfb0)
extern i64 g_scrollAccum; // 0x64cfb0 (64-bit)
extern i64 g_scrollLimit; // 0x64cfb8 (64-bit)

// timeGetTime-driven random value in [lo, hi]; inlined three times by retail.
static i32 RandRange(i32 lo, i32 hi) {
    i32 range = hi - lo + 1;
    if (range == 0)
        return (GameGetTime() & 1) ? lo : hi;
    return (i32)GameGetTime() % range + lo;
}

// @early-stop
// ~60% regalloc wall: logic + control flow are byte-isomorphic to retail (the
// int64 ScrollTime compare, the x87 view/back-plane stores, the [BackPlane]
// ScrollDist plumbing and the m_13c..0x148 bound writes all match instruction for
// instruction), but retail pins `pm` in ebx (loaded early between the pushes) and
// keeps scrollY in its stack home while this cl pins `pm` in ebp / scrollY in ebx;
// the register/stack-slot assignment is not source-steerable here.
RVA(0x000ebd70, 0x366)
void UpdateMgrScroll(CGruntzMgr* pm, i32* pMode, i32 snapFlag, i32 unused) {
    ScrollView* v = pm->m_30->m_24->m_5c;
    i32 scrollX = v->m_84;
    i32 scrollY = v->m_88;

    if (g_scrollClock > g_frameTime) {
        if (g_frameDelta < g_scrollTimer)
            g_scrollTimer -= g_frameDelta;
        else
            g_scrollTimer = 0;
        if (g_scrollTimer == 0) {
            g_scrollTimer = RandRange(g_panMinX, g_panMaxX);
            scrollX += RandRange(-g_jitterX, g_jitterX);
            scrollY += RandRange(-g_jitterY, g_jitterY);
        }
    }

    i32 cx = g_mgrSettings->m_8c / 2;
    i32 cy = g_mgrSettings->m_90 / 2;
    if (*pMode != 2)
        cx -= 0xa0;
    if (snapFlag) {
        cx = 0x60;
        cy = 0x60;
    }

    if (scrollX < cx - 1)
        scrollX = cx - 1;
    ScrollView* v2 = pm->m_30->m_24->m_5c;
    if (scrollX > v2->m_30 - cx)
        scrollX = v2->m_30 - cx;
    if (scrollY < cy - 1)
        scrollY = cy - 1;
    if (scrollY > v2->m_34 - cy)
        scrollY = v2->m_34 - cy;

    i32 deltaY = scrollY - g_lastScrollY;
    i32 deltaX = scrollX - g_lastScrollX;
    g_lastScrollX = scrollX;
    g_lastScrollY = scrollY;

    ScrollView* v3 = pm->m_30->m_24->m_5c;
    {
        float sx = (float)scrollX;
        float sy = (float)scrollY;
        if (!(v3->m_08 & 1)) {
            sx = sx * v3->m_18;
            sy = sy * v3->m_1c;
        }
        v3->m_10 = sx;
        v3->m_14 = sy;
    }
    RecomputePlaneCoords();

    ScrollView* gm = g_backView;
    if (gm != 0) {
        i32 nx = gm->m_84;
        i32 ny = gm->m_88;
        if (deltaX != 0 || deltaY != 0) {
            nx = (i32)((float)nx - (float)deltaX * -0.05f);
            ny = (i32)((float)ny - (float)deltaY * -0.05f);
        }
        if ((i64)g_frameTime - g_scrollAccum >= g_scrollLimit) {
            nx += g_buteMgr.GetDword((char*)"BackPlane", (char*)"ScrollDistX");
            ny += g_buteMgr.GetDword((char*)"BackPlane", (char*)"ScrollDistY");
            ScrollView* g2 = g_backView;
            float fx = (float)nx;
            float fy = (float)ny;
            if (!(g2->m_08 & 1)) {
                fx = fx * g2->m_18;
                fy = fy * g2->m_1c;
            }
            g2->m_10 = fx;
            g2->m_14 = fy;
            RecomputePlaneCoords();
            g_scrollLimit = g_buteMgr.GetDword((char*)"BackPlane", (char*)"ScrollTime");
            g_scrollAccum = g_frameTime;
        }
    }

    MgrSub* o = pm->m_30;
    pm->m_13c = o->m_24->m_5c->m_40 - 0x60;
    pm->m_140 = o->m_24->m_5c->m_44 - 0x60;
    pm->m_144 = o->m_24->m_5c->m_48 + 0x60;
    pm->m_148 = o->m_24->m_5c->m_4c + 0x60;
}
