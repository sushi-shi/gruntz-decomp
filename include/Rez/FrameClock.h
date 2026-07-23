#ifndef INCLUDE_REZ_FRAMECLOCK_H
#define INCLUDE_REZ_FRAMECLOCK_H
#include <Ints.h>

extern "C" {
    extern i32 g_lastNow;        // 0x245580  last timeGetTime() sample
    extern "C" i32 g_frameDelta; // 0x245584  frame delta (clamped <= 0x64); owner Rez/RezMgr.cpp
    extern "C" u32
        g_frameTime; // 0x245588  running accumulated game clock (unsigned); owner Rez/RezMgr.cpp
    extern i32 g_frameTicks;           // 0x24558c  per-frame counter (++ each tick)
    extern i32 g_timer32;              // 0x245590  interval countdown, seed 0x32 (50 ms)
    extern i32 g_timer200;             // 0x245598  interval countdown, seed 0xc8 (200 ms)
    extern i32 g_timer400;             // 0x24559c  interval countdown, seed 0x190 (400 ms)
    extern i32 g_timer500;             // 0x2455a0  interval countdown, seed 0x1f4 (500 ms)
    extern "C" u32 g_engineFrameDelta; // 0x2bf3bc  draw-delta mirror; owner Gruntz/Play.cpp
    extern "C" u32 g_killCueClock;     // 0x2bf3c0  draw-clock mirror; owner Gruntz/Play.cpp
}
extern i32 g_timer100; // 0x245594  interval countdown, seed 0x64 (100 ms); C++ linkage

#endif // INCLUDE_REZ_FRAMECLOCK_H
