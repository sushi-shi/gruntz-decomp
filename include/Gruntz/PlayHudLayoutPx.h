#ifndef GRUNTZ_GRUNTZ_PLAYHUDLAYOUTPX_H
#define GRUNTZ_GRUNTZ_PLAYHUDLAYOUTPX_H

#include <Enums.h>

// Screen pixels the play HUD is laid out in.
GZ_ENUM_CONST_BEGIN(PlayHudLayoutPx)
// The docked status bar's width. ResetViewport insets the play viewport by it
// (left dock -> r.left = +0xa0, right dock -> r.right = cx - 0xa1) and
// ScrollClampRect subtracts it from the mode width unless the bar is hidden.
    STATUSBAR_WIDTH_PX = 0xa0,
    // CTimer::LoadTimerSprite stores its two arguments straight into m_baseX/m_baseY,
    // so these are the countdown clock's screen origin, not resource ids.
    TIMER_ORIGIN_X_PX = 0x249,
    // Docking the bar on the right steals the rightmost STATUSBAR_WIDTH_PX, so the
    // clock moves exactly that far left: 0x249 - 0xa0 == 0x1a9.
    TIMER_ORIGIN_X_STATUSBAR_RIGHT_PX = TIMER_ORIGIN_X_PX - STATUSBAR_WIDTH_PX,
    TIMER_ORIGIN_Y_PX = 0x1ca
GZ_ENUM_CONST_END(PlayHudLayoutPx)

#endif // GRUNTZ_GRUNTZ_PLAYHUDLAYOUTPX_H
