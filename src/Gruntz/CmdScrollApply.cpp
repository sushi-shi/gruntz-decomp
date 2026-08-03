#include <rva.h>

#include <Gruntz/MgrAutoScroll.h>
#include <Gruntz/ScrollState.h>
#include <Ints.h>
#include <Rez/FrameClock.h>

RVA(0x000ebd30, 0x21)
void Cmd_ResetScroll() {
    g_scrollClock = 0;
    g_scrollTimer = 0;
    g_scrollAccum = 0;
    g_scrollLimit = 0;
}
RVA(0x000ec1c0, 0x43)
void Cmd_ApplyScrollParams(i32 durationMs, i32 jitterX, i32 jitterY, i32 panMinX, i32 panMaxX) {
    i32 t = durationMs + g_frameTime;
    if (g_scrollClock <= static_cast<u32>(t)) {
        g_scrollClock = t;
    }
    g_jitterX = jitterX;
    g_jitterY = jitterY;
    g_panMinX = panMinX;
    g_panMaxX = panMaxX;
}
