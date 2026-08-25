#ifndef GRUNTZ_GRUNTZ_SCROLLSTATE_H
#define GRUNTZ_GRUNTZ_SCROLLSTATE_H

#include <Ints.h>

struct ScrollPace {
    ScrollPace() {
        m_lastTime = 0;
        m_period = 0;
    }

    i64 m_lastTime;
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
