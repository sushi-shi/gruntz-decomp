#include "fade_probe.h"
#include <Runtime/FrameScheduler.h>
#include <cassert>
#include <cstdio>

static void playback() {
    FadeProbeLog log;
    FadePlayback fade;
    assert(fade.advance(100) == FadeIdle);
    assert(fade.start(new FadeProbe(log, 100), 20, 10, false));
    assert(log.begins == 1 && log.frames.size() == 1 && log.frames[0] == 0);
    assert(fade.advance(9000) == FadeRunning); // Establish the callback epoch.
    assert(fade.advance(5) == FadeRunning && log.frames.size() == 1);
    assert(fade.advance(10) == FadeRunning && log.frames.back() == 25);
    assert(fade.advance(0) == FadeRunning && log.frames.size() == 2);
    assert(fade.advance(9999) == FadeFinished);
    assert(log.frames.size() == 3 && log.frames.back() == 100);
    assert(!fade.active() && log.ends == 1 && log.destroyed == 1);
    assert(fade.advance(1) == FadeIdle);
    fade.cancel();
    assert(log.ends == 1 && log.destroyed == 1);
}

static void ownershipAndFailure() {
    FadeProbeLog first, second, invalid, failBegin, failFirst, failLater;
    {
        FadePlayback fade;
        assert(fade.start(new FadeProbe(first, 5), 100, 0, false));
        assert(fade.start(new FadeProbe(second, 5), 100, 0, false));
        assert(first.ends == 1 && first.destroyed == 1);
    }
    assert(second.ends == 1 && second.destroyed == 1);
    FadePlayback fade;
    assert(!fade.start(new FadeProbe(invalid, 0), 1, 0, false));
    assert(invalid.begins == 0 && invalid.ends == 0 && invalid.destroyed == 1);
    failBegin.failBegin = true;
    assert(!fade.start(new FadeProbe(failBegin, 1), 1, 0, false));
    assert(failBegin.begins == 1 && failBegin.ends == 0 && failBegin.destroyed == 1);
    failFirst.failRender = true;
    assert(!fade.start(new FadeProbe(failFirst, 1), 1, 0, false));
    assert(failFirst.ends == 1 && failFirst.destroyed == 1);
    failLater.failRender = true; failLater.failingFrame = 1;
    assert(fade.start(new FadeProbe(failLater, 1), 1, 0, false));
    assert(fade.advance(0) == FadeRunning);
    assert(fade.advance(1) == FadeFailed);
    assert(failLater.ends == 1 && failLater.destroyed == 1 && !fade.active());
    assert(fade.advance(1) == FadeIdle);
    assert(!fade.start(0, 1, 0, false));
}

static void boundaries() {
    FadePlayback fade;
    FadeProbeLog immediate, delayed, finalOnly, huge;
    assert(fade.start(new FadeProbe(immediate, 100), 0, 0, false));
    assert(fade.advance(0) == FadeFinished);
    assert(immediate.frames.size() == 2 && immediate.frames.back() == 100);
    assert(fade.start(new FadeProbe(delayed, 100), 0, 5, false));
    assert(fade.advance(50) == FadeRunning);
    assert(fade.advance(4) == FadeRunning && delayed.frames.size() == 1);
    assert(fade.advance(1) == FadeFinished && delayed.frames.back() == 100);
    assert(fade.start(new FadeProbe(finalOnly, 100), 100, 0, true));
    assert(fade.advance(0) == FadeRunning);
    for (int i = 0; i < 9; ++i) assert(fade.advance(10) == FadeRunning);
    assert(finalOnly.frames.size() == 1);
    assert(fade.advance(10) == FadeFinished && finalOnly.frames.size() == 2);
    assert(fade.start(new FadeProbe(huge, 0xffffffffU), 0xffffffffU, 0xffffffffU, false));
    assert(fade.advance(0) == FadeRunning);
    assert(fade.advance(0xffffffffU) == FadeRunning && huge.frames.size() == 1);
    assert(fade.advance(0xfffffffeU) == FadeRunning && huge.frames.back() == 0xfffffffeU);
    assert(fade.advance(1) == FadeFinished && huge.frames.back() == 0xffffffffU);
}

static void deferredSurface() {
    FadeProbeLog log;
    FadePlayback fade;
    assert(fade.start(new FadeProbe(log, 10), 100, 0, false));
    assert(fade.advance(0) == FadeRunning);
    log.retryRender = true; log.failingFrame = 5;
    assert(fade.advance(50) == FadeRunning && log.frames.back() == 5);
    assert(fade.advance(0) == FadeRunning && log.frames.back() == 5);
    log.failingFrame = 10;
    assert(fade.advance(100) == FadeRunning && log.frames.back() == 10);
    assert(fade.active() && !log.ends && !log.destroyed);
    log.retryRender = false;
    assert(fade.advance(0) == FadeFinished && log.frames.back() == 10);
    assert(log.ends == 1 && log.destroyed == 1);
}

static void boundedPresentation() {
    FadePlayback fade;
    FadeProbeLog busy, recovered, huge, intermittent;
    busy.retryRender = true; busy.failingFrame = 1;
    assert(fade.start(new FadeProbe(busy, 1), 0, 0, false, 2000));
    assert(fade.advance(999999) == FadeRunning); // First busy attempt starts the retry epoch.
    assert(fade.advance(1999) == FadeRunning);
    assert(fade.advance(0) == FadeRunning); // Suspended time is not charged.
    assert(busy.ends == 0 && busy.destroyed == 0);
    assert(fade.advance(1) == FadeFailed);
    assert(busy.ends == 1 && busy.destroyed == 1 && !fade.active());
    assert(fade.advance(1) == FadeIdle);

    recovered.retryRender = true; recovered.failingFrame = 1;
    assert(fade.start(new FadeProbe(recovered, 1), 0, 0, false, 2000));
    assert(fade.advance(0) == FadeRunning);
    assert(fade.advance(1999) == FadeRunning);
    recovered.retryRender = false;
    assert(fade.advance(1) == FadeFinished);
    assert(recovered.ends == 1 && recovered.destroyed == 1);

    huge.retryRender = true; huge.failingFrame = 1;
    assert(fade.start(new FadeProbe(huge, 1), 0, 0, false, 0xffffffffU));
    assert(fade.advance(0) == FadeRunning);
    assert(fade.advance(0xfffffffeU) == FadeRunning);
    assert(fade.advance(0xffffffffU) == FadeFailed); // Saturation, not wrap.
    assert(huge.ends == 1 && huge.destroyed == 1);

    assert(fade.start(new FadeProbe(intermittent, 2), 2000, 0, false, 10));
    assert(fade.advance(0) == FadeRunning);
    intermittent.retryRender = true; intermittent.failingFrame = 1;
    assert(fade.advance(1000) == FadeRunning);
    assert(fade.advance(5) == FadeRunning);
    intermittent.retryRender = false;
    assert(fade.advance(0) == FadeRunning); // A successful frame resets the timeout.
    intermittent.retryRender = true; intermittent.failingFrame = 2;
    assert(fade.advance(995) == FadeRunning);
    assert(fade.advance(9) == FadeRunning);
    assert(fade.advance(1) == FadeFailed);
    assert(intermittent.ends == 1 && intermittent.destroyed == 1);
}

static void hostDeltas() {
    FrameScheduler frames;
    frames.reset(0xfffffff0U);
    FadeProbeLog log;
    FadePlayback fade;
    assert(fade.start(new FadeProbe(log, 100), 100, 0, false));
    assert(frames.poll(0xfffffff0U));
    assert(fade.advance(frames.timing().deltaMs()) == FadeRunning);
    assert(frames.poll(4));
    assert(fade.advance(frames.timing().deltaMs()) == FadeRunning && log.frames.back() == 20);
    frames.suspend();
    assert(frames.poll(500000));
    assert(frames.timing().deltaMs() == 0);
    assert(fade.advance(frames.timing().deltaMs()) == FadeRunning && log.frames.back() == 20);
    assert(frames.poll(500080));
    assert(fade.advance(frames.timing().deltaMs()) == FadeFinished && log.frames.back() == 100);
}

void testGameplayPresentations();

int main() {
    testGameplayPresentations();
    playback(); ownershipAndFailure(); boundaries(); deferredSurface(); boundedPresentation(); hostDeltas();
    std::puts("Returning fade playback tests passed.");
}
