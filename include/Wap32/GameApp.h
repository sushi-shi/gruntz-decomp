#ifndef GRUNTZ_WAP32_GAMEAPP_H
#define GRUNTZ_WAP32_GAMEAPP_H

#include <rva.h>

#include <Enums.h>
#include <Utils/MillisPer.h>

GZ_ENUM_CONST_BEGIN(GameAppTiming)
    GAMEAPP_PERIODIC_TIMER_MS = 100,
    GAMEAPP_FPS_UNAVAILABLE = -1,
    GAMEAPP_FPS_SAMPLE_SECONDS = 2,
    GAMEAPP_FPS_SAMPLE_INTERVAL_MS = GAMEAPP_FPS_SAMPLE_SECONDS * MILLIS_PER_SECOND
GZ_ENUM_CONST_END(GameAppTiming)

GZ_ENUM_CONST_BEGIN(AsyncKeyStateMask)
    ASYNC_KEYSTATE_DOWN = 0x80000000
GZ_ENUM_CONST_END(AsyncKeyStateMask)

extern i32 g_wap32Now;
extern i32 g_wap32FrameDelta;
extern i32 g_wap32ClockReset;
extern i32 g_gameAppTimerRemainingMs;
extern i32 g_gameAppTimerPeriodMs;
#endif // GRUNTZ_WAP32_GAMEAPP_H
