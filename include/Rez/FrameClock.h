#ifndef INCLUDE_REZ_FRAMECLOCK_H
#define INCLUDE_REZ_FRAMECLOCK_H

#include <Enums.h>
#include <Ints.h>

GZ_ENUM_CONST_BEGIN(FrameClockPeriod)
    FRAME_CLOCK_PERIOD_50_MS = 50,
    FRAME_CLOCK_PERIOD_100_MS = 100,
    FRAME_CLOCK_PERIOD_200_MS = 200,
    FRAME_CLOCK_PERIOD_400_MS = 400,
    FRAME_CLOCK_PERIOD_500_MS = 500
GZ_ENUM_CONST_END(FrameClockPeriod)

extern i32 g_lastNow;

extern u32 g_frameDelta;
extern u32 g_frameTime;
extern i32 g_frameTicks;
extern i32 g_period50CountdownMs;
extern i32 g_period200CountdownMs;
extern i32 g_period400CountdownMs;
extern i32 g_period500CountdownMs;
extern u32 g_engineFrameDelta;
extern u32 g_soundCueTimeMs;
extern i32 g_period100CountdownMs;

#endif // INCLUDE_REZ_FRAMECLOCK_H
