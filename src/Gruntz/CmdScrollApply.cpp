#include <Ints.h>
#include <Rez/FrameClock.h> // frame-clock band (g_frameDelta/g_frameTime/g_killCueClock/g_engineFrameDelta)
#include <rva.h>
#include <Gruntz/ScrollState.h> // g_scrollAccum / g_scrollClock (auto-scroll state block)
#include <Gruntz/MgrAutoScroll.h> // ex Globals.h

RVA(0x000ec1c0, 0x43)
void Cmd_ApplyScrollParams(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4) {
    i32 t = a0 + g_frameTime;
    if (g_scrollClock <= static_cast<u32>(t)) {
        g_scrollClock = t;
    }
    g_jitterX = a1;
    g_jitterY = a2;
    g_panMinX = a3;
    g_panMaxX = a4;
}

RVA(0x000ebd30, 0x21)
void Cmd_ResetScroll() {
    g_scrollClock = 0;
    g_scrollTimer = 0;
    g_scrollAccum = 0;
    g_scrollLimit = 0;
}
