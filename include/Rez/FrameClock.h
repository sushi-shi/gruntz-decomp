// FrameClock.h - the per-frame game clock + interval-countdown timer band
// (retail .data 0x245580..0x2455a0), DEFINED in src/Rez/RezMgr.cpp. RezMgr::PerFrameTick
// (0x08b740) is the SOLE writer of the whole band: it samples the clock, accumulates the
// clamped frame delta and decrements the five interval countdown timers; every other TU
// only reads/mirrors these cells. Declared ONCE here (owner header) so consumers stop
// re-`extern`-ing them per-TU under divergent names/linkage.
//
// Linkage: the band is C linkage (extern "C") EXCEPT g_timer100 (0x245594), which is
// canonically C++-linkage (?g_timer100@@3HA) - the LightFxRender / GruntzMgr / Multi
// references all bind to the C++ mangled name (an extern "C" _g_* twin would collide on
// the same RVA). All references are DATA-reloc-masked, so the name/linkage is byte-neutral.
//
// 0x245584 (frame delta, g_frameDelta) and 0x245588 (accumulated clock, g_frameTime) are
// part of the same band but are read tree-wide (~50 TUs) under those already-semantic
// names; they keep their existing decls and are intentionally NOT centralized here.
#ifndef INCLUDE_REZ_FRAMECLOCK_H
#define INCLUDE_REZ_FRAMECLOCK_H
#include <Ints.h>

extern "C" {
    extern i32 g_lastNow;    // 0x245580  last timeGetTime() sample
    extern i32 g_frameTicks; // 0x24558c  per-frame counter (++ each tick)
    extern i32 g_timer32;    // 0x245590  interval countdown, seed 0x32 (50 ms)
    extern i32 g_timer200;   // 0x245598  interval countdown, seed 0xc8 (200 ms)
    extern i32 g_timer400;   // 0x24559c  interval countdown, seed 0x190 (400 ms)
    extern i32 g_timer500;   // 0x2455a0  interval countdown, seed 0x1f4 (500 ms)
}
extern i32 g_timer100; // 0x245594  interval countdown, seed 0x64 (100 ms); C++ linkage

#endif // INCLUDE_REZ_FRAMECLOCK_H
