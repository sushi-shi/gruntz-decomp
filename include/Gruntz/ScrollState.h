#ifndef GRUNTZ_GRUNTZ_SCROLLSTATE_H
#define GRUNTZ_GRUNTZ_SCROLLSTATE_H

#include <Ints.h>

// The back-plane auto-scroll pace. The two members are ONE object, not two
// globals: retail builds them with a single `.CRT$XC` initializer whose body
// zeroes all four dwords at once (0x000ebd00), and cl 5.0 emits one XC slot per
// constructed object.
struct ScrollPace {
    ScrollPace() {
        m_lastTime = 0;
        m_period = 0;
    }

    // Frame time at which the back plane last stepped.
    i64 m_lastTime;
    // Milliseconds between steps, reloaded from bute BackPlane/ScrollTime.
    i64 m_period;
};

extern ScrollPace g_scrollPace;
extern u32 g_scrollClock;
extern u32 g_scrollTimer;

extern i32 g_serializedScrollReservedFirst;
extern i32 g_serializedScrollReservedSecond;
extern i32 g_lastScrollX;
extern i32 g_lastScrollY;

#endif // GRUNTZ_GRUNTZ_SCROLLSTATE_H
