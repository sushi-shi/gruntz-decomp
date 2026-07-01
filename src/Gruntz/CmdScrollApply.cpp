// CmdScrollApply.cpp - the scroll/jitter/pan command applier (0x0ec1c0), a __cdecl
// free function. It raises the auto-scroll clock to a0+g_645588 (only if not already
// past it) and latches the jitter/pan command parameters into the MgrAutoScroll
// globals (already-named data symbols).
#include <Ints.h>
#include <rva.h>

extern "C" i32 g_645588;  // per-frame sync salt
extern u32 g_scrollClock; // 0x64cfc0
extern u32 g_scrollTimer; // 0x64cfc4
extern i64 g_scrollAccum; // 0x64cfb0 (64-bit)
extern i64 g_scrollLimit; // 0x64cfb8 (64-bit)
extern i32 g_jitterX;     // 0x6452a4
extern i32 g_jitterY;     // 0x6452cc
extern i32 g_panMinX;     // 0x645508
extern i32 g_panMaxX;     // 0x64550c

RVA(0x000ec1c0, 0x43)
void Cmd_ApplyScrollParams_0ec1c0(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4) {
    i32 t = a0 + g_645588;
    if (g_scrollClock <= (u32)t) {
        g_scrollClock = t;
    }
    g_jitterX = a1;
    g_jitterY = a2;
    g_panMinX = a3;
    g_panMaxX = a4;
}

// Cmd_ResetScroll - zero the auto-scroll clock/timer + the two 64-bit accumulators
// (ClassUnknown_17; __cdecl free function). Six dword stores.
RVA(0x000ebd30, 0x21)
void Cmd_ResetScroll_0ebd30() {
    g_scrollClock = 0;
    g_scrollTimer = 0;
    g_scrollAccum = 0;
    g_scrollLimit = 0;
}
