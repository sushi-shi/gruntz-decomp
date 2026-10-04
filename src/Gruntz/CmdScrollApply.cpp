#include <StdAfx.h>

#include <rva.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/MgrAutoScroll.h>
#include <Gruntz/RandomRange.h>
#include <Gruntz/ScrollState.h>
#include <Gruntz/StatusBarDock.h>
#include <Gruntz/StatusBarMgr.h>
#include <Ints.h>
#include <RectMacros.h>
#include <Rez/FrameClock.h>
#include <Rez/FrameCountdown.h>
#include <Wap32/TileGeometry.h>
#include <Wwd/WwdFile.h>

#include <stddef.h>

RVA(0x000ebd30, 0x21)
void Cmd_ResetScroll() {
    g_screenShakeEndTime = 0;
    g_screenShakeDelayRemainingMs = 0;
    g_scrollPace.m_lastTime = 0;
    g_scrollPace.m_period = 0;
}
RVA(0x000ebd70, 0x366)
void UpdateMgrScroll(CGruntzMgr* pm, class CStatusBarMgr* bar, b32 snapFlag) {
    CLevelPlane* v = pm->World()->GetLevel()->m_mainPlane;
    i32 scrollX = v->GetScrollPixelX();
    i32 scrollY = v->GetScrollPixelY();

    if (g_screenShakeEndTime > g_frameTime) {
        CountDown(g_screenShakeDelayRemainingMs, g_frameDelta);
        if (g_screenShakeDelayRemainingMs == 0) {
            g_screenShakeDelayRemainingMs =
                RandRange(pm, g_screenShakeMinDelayMs, g_screenShakeMaxDelayMs);
            i32 jitterX = RandRange(pm, -g_screenShakeAmplitudeX, g_screenShakeAmplitudeX);
            i32 jitterY = RandRange(pm, -g_screenShakeAmplitudeY, g_screenShakeAmplitudeY);
            scrollX += jitterX;
            scrollY += jitterY;
        }
    }

    tagSIZE screenSize = g_gameReg->m_modeSize;
    i32 cx = screenSize.cx / 2;
    i32 cy = screenSize.cy / 2;
    if (bar->GetDockState() != STATUSBAR_HIDDEN) {
        cx -= 0xa0;
    }
    if (snapFlag) {
        cx = 0x60;
        cy = 0x60;
    }

    if (scrollX < cx - 1) {
        scrollX = cx - 1;
    }
    CLevelPlane* boundsPlane = pm->World()->GetLevel()->m_mainPlane;
    CLAMP_UPPER_INPLACE(scrollX, boundsPlane->GetPlanePixelWidth() - cx);
    if (scrollY < cy - 1) {
        scrollY = cy - 1;
    }
    CLAMP_UPPER_INPLACE(scrollY, boundsPlane->GetPlanePixelHeight() - cy);

    i32 deltaX = scrollX - g_lastScrollX;
    i32 deltaY = scrollY - g_lastScrollY;
    g_lastScrollX = scrollX;
    g_lastScrollY = scrollY;

    CLevelPlane* scrollPlane = pm->World()->GetLevel()->m_mainPlane;
    scrollPlane->SetScrollPosition(scrollX, scrollY);

    CLevelPlane* gm = g_backView;
    if (gm != NULL) {
        i32 nx = gm->GetScrollPixelX();
        i32 ny = gm->GetScrollPixelY();
        if (deltaX != 0 || deltaY != 0) {
            nx = static_cast<i32>((static_cast<float>(nx) - static_cast<float>(deltaX) * -0.05f));
            ny = static_cast<i32>((static_cast<float>(ny) - static_cast<float>(deltaY) * -0.05f));
        }
        if (static_cast<i64>(g_frameTime) - g_scrollPace.m_lastTime >= g_scrollPace.m_period) {
            nx += g_buteMgr.GetDword("BackPlane", "ScrollDistX");
            ny += g_buteMgr.GetDword("BackPlane", "ScrollDistY");
            CLevelPlane* g2 = g_backView;
            g2->SetScrollPosition(nx, ny);
            g_scrollPace.m_period = g_buteMgr.GetDword("BackPlane", "ScrollTime");
            g_scrollPace.m_lastTime = g_frameTime;
        }
    }

    CDDrawSurfaceMgr* o = pm->World();
    SET_RECT_COMPONENTS(
        pm->m_viewBounds,
        o->GetLevel()->m_mainPlane->GetPlaneViewRect()->left - 0x60,
        o->GetLevel()->m_mainPlane->GetPlaneViewRect()->top - 0x60,
        o->GetLevel()->m_mainPlane->GetPlaneViewRect()->right + 0x60,
        o->GetLevel()->m_mainPlane->GetPlaneViewRect()->bottom + 0x60
    );
}

RVA(0x000ec1c0, 0x43)
void StartScreenShake(
    i32 durationMs,
    i32 amplitudeX,
    i32 amplitudeY,
    i32 minDelayMs,
    i32 maxDelayMs
) {
    i32 endTime = durationMs + g_frameTime;
    g_screenShakeEndTime = max(g_screenShakeEndTime, static_cast<u32>(endTime));
    g_screenShakeAmplitudeX = amplitudeX;
    g_screenShakeAmplitudeY = amplitudeY;
    g_screenShakeMinDelayMs = minDelayMs;
    g_screenShakeMaxDelayMs = maxDelayMs;
}
DATA(0x002452a4)
i32 g_screenShakeAmplitudeX;

DATA(0x002452cc)
i32 g_screenShakeAmplitudeY;

DATA(0x0024c27c)
CLevelPlane* g_backView;

RVA_DYNINIT(0x000ebd00, 0x17, g_scrollPace)
DATA(0x0024cfb0)
ScrollPace g_scrollPace;

DATA(0x0024cfc0)
u32 g_screenShakeEndTime;

DATA(0x0024cfc4)
u32 g_screenShakeDelayRemainingMs;

DATA(0x0024cfc8)
i32 g_serializedScrollReservedFirst;

DATA(0x0024cfcc)
i32 g_serializedScrollReservedSecond;

DATA(0x0024cfd0)
i32 g_lastScrollX;

DATA(0x0024cfd4)
i32 g_lastScrollY;
