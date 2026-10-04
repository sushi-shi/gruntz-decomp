#include <Runtime/FrameTiming.h>
#include <cassert>
#include <cstdio>
#include <climits>

static void frame(FrameTiming& timing, u32 start, u32 paced) {
    timing.beginFrame(start);
    timing.finishPacing(paced);
}

static void countdowns() {
    FrameTiming timing;
    assert(timing.nowMs() == 0 && timing.deltaMs() == 0);
    assert(timing.timerRemainingMs() == 100);
    assert(timing.fps() == -1 && timing.targetFps() == 0);
    timing.reset(1000);
    frame(timing, 1030, 1037);
    assert(timing.nowMs() == 1030 && timing.deltaMs() == 30);
    assert(timing.timerRemainingMs() == 70);
    frame(timing, 1100, 1107);
    assert(timing.timerRemainingMs() == 0);
    timing.setTimerPeriod(50);
    frame(timing, 1200, 1207);
    assert(timing.timerRemainingMs() == 50);
    frame(timing, 1220, 1227);
    assert(timing.timerRemainingMs() == 30);
    timing.setTimerPeriod(500);
    assert(timing.timerRemainingMs() == 30);
    frame(timing, 1251, 1251);
    assert(timing.timerRemainingMs() == 0);
    frame(timing, 1251, 1251);
    assert(timing.deltaMs() == 0 && timing.timerRemainingMs() == 500);
    timing.setTimerPeriod(0);
    frame(timing, 2000, 2000);
    frame(timing, 2010, 2010);
    assert(timing.timerRemainingMs() == 0);
}

static void pacing() {
    FrameTiming timing;
    timing.setFrameRate(60);
    assert(timing.targetFps() == 60);
    assert(timing.pacingDelay(0) == 0);
    frame(timing, 0, 0);
    // A timestamp of zero is valid, including when the host clock wraps.
    assert(timing.pacingDelay(0) == 16);
    assert(timing.pacingDelay(5) == 11);
    assert(timing.pacingDelay(16) == 0);
    assert(timing.pacingDelay(1000) == 0);
    frame(timing, 5, 17);
    assert(timing.nowMs() == 5 && timing.deltaMs() == 5);
    assert(timing.pacingDelay(17) == 16);
    frame(timing, 21, 34);
    assert(timing.deltaMs() == 16);
    timing.setFrameRate(100);
    assert(timing.pacingDelay(35) == 9);
    timing.setFrameRate(0);
    assert(timing.targetFps() == 0 && timing.pacingDelay(35) == 0);
    frame(timing, 36, 36);
    timing.setFrameRate(100);
    assert(timing.pacingDelay(36) == 8);
    timing.setFrameRate(-10);
    assert(timing.targetFps() == 0 && timing.pacingDelay(36) == 0);
    timing.setFrameRate(INT_MAX);
    assert(timing.pacingDelay(36) == 0);
    timing.setFrameRate(1);
    assert(timing.pacingDelay(36) == 998);
}

static void wrapAndPauses() {
    FrameTiming timing;
    timing.reset(0xfffffff0U);
    timing.setFrameRate(50);
    frame(timing, 0xfffffff8U, 0xfffffff8U);
    assert(timing.deltaMs() == 8 && timing.timerRemainingMs() == 92);
    assert(timing.pacingDelay(4) == 8);
    frame(timing, 4, 12);
    assert(timing.deltaMs() == 12 && timing.timerRemainingMs() == 80);
    frame(timing, 84, 84);
    assert(timing.timerRemainingMs() == 0);

    timing.reset(0x7ffffff0U);
    frame(timing, 0x80000010U, 0x80000010U);
    assert(timing.deltaMs() == 32 && timing.timerRemainingMs() == 68);
    timing.reset(0);
    frame(timing, 0x80000010U, 0x80000010U);
    assert(timing.deltaMs() == 0x80000010U);
    assert(timing.timerRemainingMs() == 0);
    frame(timing, 0x80000020U, 0x80000020U);
    assert(timing.deltaMs() == 16 && timing.timerRemainingMs() == 100);
    timing.reset(0);
    timing.setFrameRate(60);
    frame(timing, 0, 0);
    assert(timing.pacingDelay(60000) == 0);
    frame(timing, 60000, 60000);
    assert(timing.deltaMs() == 60000 && timing.timerRemainingMs() == 0);
    assert(timing.fps() == 1);
}

static void fpsSamples() {
    FrameTiming timing;
    timing.reset(1000);
    frame(timing, 1500, 1507);
    frame(timing, 2999, 3006);
    assert(timing.fps() == -1);
    frame(timing, 3007, 3014);
    assert(timing.fps() == 1);
    // Completion starts the next sample window; frame start tests its expiry.
    frame(timing, 5013, 5020);
    assert(timing.fps() == 1);
    frame(timing, 5021, 5028);
    assert(timing.fps() == 1);
    frame(timing, 5528, 5528);
    frame(timing, 6028, 6028);
    frame(timing, 6528, 6528);
    frame(timing, 7028, 7028);
    assert(timing.fps() == 2);
    timing.reset(0xfffffff0U);
    frame(timing, 1983, 1983);
    assert(timing.fps() == -1);
    frame(timing, 1984, 1984);
    assert(timing.fps() == 1);
    timing.reset(0);
    for (u32 now = 0; now <= 2000; now += 20) frame(timing, now, now);
    assert(timing.fps() == 50);
}

static void resetAndIsolation() {
    FrameTiming first;
    FrameTiming second;
    first.setFrameRate(50);
    first.setTimerPeriod(25);
    frame(first, 90, 90);
    frame(second, 10, 10);
    assert(first.deltaMs() == 90 && second.deltaMs() == 10);
    assert(first.timerRemainingMs() == 10 && second.timerRemainingMs() == 90);
    first.resetFrameTime(10000);
    assert(first.nowMs() == 10000 && first.deltaMs() == 0);
    assert(first.targetFps() == 50 && first.pacingDelay(10000) == 0);
    assert(first.timerRemainingMs() == 10);
    frame(first, 10005, 10005);
    assert(first.deltaMs() == 5 && first.timerRemainingMs() == 5);
    assert(first.fps() == 1);
    frame(first, 10010, 10010);
    assert(first.timerRemainingMs() == 0);
    frame(first, 10011, 10011);
    assert(first.timerRemainingMs() == 25);
    assert(second.nowMs() == 10 && second.fps() == -1);
    first.reset(20000);
    assert(first.nowMs() == 20000 && first.deltaMs() == 0);
    assert(first.targetFps() == 0 && first.fps() == -1);
    assert(first.timerRemainingMs() == 100);
}

int main() {
    static_assert(sizeof(u32) == 4, "Timing uses 32-bit milliseconds");
#ifdef __EMSCRIPTEN__
    static_assert(sizeof(void*) == 4, "This suite must exercise wasm32");
#endif
    countdowns();
    pacing();
    wrapAndPauses();
    fpsSamples();
    resetAndIsolation();
    std::printf("Frame timing passed (%u-bit pointers).\n",
                static_cast<unsigned int>(sizeof(void*) * 8));
}
