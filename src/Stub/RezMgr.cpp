#include <rva.h>
#include <Win32.h> // WINMM timeGetTime (frame clock); guarded, harmless re-include
// RezMgr.cpp - engine-label stubs for RezMgr (reloc-correlation).

// The engine frame-clock globals (mangled C++ ints, declared in GameApp.cpp's
// cluster). UpdateClock advances g_wap32Now and derives the per-frame delta /
// run-state countdown from them; reloc-masked DATA() refs.
extern i32 g_wap32Now;        // ?g_wap32Now@@3HA
extern i32 g_wap32FrameDelta; // ?g_wap32FrameDelta@@3HA
DATA(0x00253c78)
// Unsigned tick stamp: the `!= 0` gate emits an unsigned `test;jbe` (matches
// retail); the @@3IA mangling differs from GameApp.cpp's int decl but the DATA
// reloc operand is masked in objdiff, so the scoring is unaffected.
extern u32 g_wap32ClockReset; // ?g_wap32ClockReset@@3IA  (last clock-reset tick)
extern i32 g_wap32Run7c;      // ?g_wap32Run7c@@3HA  (run-state countdown)
extern i32 g_wap32Run80;      // ?g_wap32Run80@@3HA  (run-state reload value)

// RezMgr carries the WAP32 game-clock fields (same layout as CGameMgr; the retail
// symbol mangles this method's class as `RezMgr`). Only the offsets + the two
// __thiscall callees (the spin-wait limiter @0x13dec0 and InitTimeFields @0x13de70,
// reloc-masked) are load-bearing.
class RezMgr {
public:
    i32 UpdateClock();

private:
    // External no-body __thiscall callees (call rel32 -> reloc-masked).
    void SpinWaitUntil(i32 target); // @0x13dec0  busy-waits ~target ms past now
    void InitTimeFields(i32 reset); // @0x13de70  zero m_20, sample m_24, arm m_18

    char m_pad00[0x18];
    i32 m_18; // +0x18  smoothed frame count (m_20/2 every ~2s window)
    i32 m_1c; // +0x1c  active gate (>0 enables the per-frame pacing)
    i32 m_20; // +0x20  frame counter (incremented each tick)
    i32 m_24; // +0x24  window-start tick
    i32 m_28; // +0x28  target ms-per-frame (the pacing budget)
};
// @confidence: med
// @source: reloc-correlation (1 caller)
RVA(0x0013ddc0, 0xaa)
i32 RezMgr::UpdateClock() {
    u32 now = timeGetTime();
    u32 delta = now - (u32)g_wap32Now;
    g_wap32Now = now;
    g_wap32FrameDelta = delta;
    u32 run7c = g_wap32Run7c;
    if (run7c == 0) {
        g_wap32Run7c = g_wap32Run80;
    } else if (delta >= run7c) {
        g_wap32Run7c = 0;
    } else {
        g_wap32Run7c = run7c - delta;
    }

    if (m_1c > 0) {
        if (g_wap32ClockReset > 0) {
            u32 elapsed = timeGetTime() - g_wap32ClockReset;
            if (elapsed < (u32)m_28) {
                SpinWaitUntil(m_28 - elapsed);
            }
        }
        g_wap32ClockReset = timeGetTime();
    }

    u32 count = m_20 + 1;
    m_20 = count;
    if ((u32)g_wap32Now - (u32)m_24 >= 0x7d0) {
        m_18 = count >> 1;
        InitTimeFields(0);
    }
    return 1;
}
