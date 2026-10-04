#include <Gruntz/GameplayPresentation.h>
#include <Runtime/FadePlayback.h>
#include <Runtime/FrameScheduler.h>
#include "fade_probe.h"
#include <cassert>

static void deferredCompletion(GameplayPresentationKind kind, GameStateId previous) {
    GameplayPresentation presentation;
    GameplayPresentationAction completion;
    FadePlayback fade;
    FadeProbeLog log;
    assert(presentation.request(kind, previous));
    assert(!presentation.request(RestoreGameplay, GAMESTATE_NONE));
    assert(presentation.needsPrepare() && !fade.active() && log.frames.empty());
    assert(!presentation.take(completion));
    assert(fade.start(new FadeProbe(log, 10), 100, 0, false));
    presentation.prepared();
    FrameScheduler frames;
    frames.reset(0xfffffff0U);
    assert(frames.poll(0xfffffff0U));
    assert(fade.advance(frames.timing().deltaMs()) == FadeRunning);
    assert(frames.poll(0x22));
    assert(fade.advance(frames.timing().deltaMs()) == FadeRunning);
    frames.suspend();
    assert(frames.poll(1000000));
    assert(fade.advance(frames.timing().deltaMs()) == FadeRunning);
    assert(presentation.active());
    assert(frames.poll(1000050));
    assert(fade.advance(frames.timing().deltaMs()) == FadeFinished);
    assert(presentation.take(completion));
    assert(completion.kind == kind && completion.previous == previous && !completion.recovery);
    assert(!presentation.active() && !presentation.take(completion));
    assert(fade.advance(9999) == FadeIdle && log.ends == 1 && log.destroyed == 1);
}

static void cancelAndRecover() {
    for (unsigned int playing = 0; playing < 2; ++playing) {
        GameplayPresentation presentation;
        GameplayPresentationAction completion;
        FadePlayback fade;
        FadeProbeLog old;
        assert(presentation.request(EnterGameplayState, GAMESTATE_HELP));
        if (playing) {
            assert(fade.start(new FadeProbe(old, 1), 100, 0, false));
            presentation.prepared();
        }
        // A surface-loss callback preserves the pending caller's action, while
        // explicit cancellation removes it before releasing/replacing surfaces.
        GameplayPresentation recovery = presentation;
        presentation.cancel();
        fade.cancel();
        assert(!presentation.take(completion));
        assert(recovery.recover() && recovery.needsPrepare());
        assert(!recovery.recover());
        assert(!recovery.take(completion));
        FadeProbeLog restored;
        assert(fade.start(new FadeProbe(restored, 1), 0, 0, false, 2000));
        recovery.prepared();
        presentation = recovery;
        restored.retryRender = true; restored.failingFrame = 1;
        assert(fade.advance(0) == FadeRunning);
        assert(fade.advance(1999) == FadeRunning);
        restored.retryRender = false;
        assert(fade.advance(1) == FadeFinished);
        assert(presentation.take(completion));
        assert(completion.kind == EnterGameplayState && completion.previous == GAMESTATE_HELP
            && completion.recovery);
        assert(!presentation.take(completion));
        assert(old.destroyed == playing && restored.destroyed == 1);
    }
    for (int phase = 0; phase < 3; ++phase) {
        GameplayPresentation presentation;
        GameplayPresentationAction completion;
        assert(presentation.request(StartGameplayInput, GAMESTATE_PLAY));
        if (phase) presentation.prepared();
        if (phase == 2) assert(presentation.recover());
        presentation.cancel();
        assert(!presentation.active() && !presentation.take(completion));
        assert(presentation.request(RestoreGameplay, GAMESTATE_NONE));
        assert(!presentation.action().recovery);
        presentation.prepared();
        assert(presentation.take(completion) && completion.kind == RestoreGameplay);
    }
}

static void failureDoesNotComplete() {
    GameplayPresentation presentation;
    GameplayPresentationAction completion;
    FadePlayback fade;
    FadeProbeLog failed;
    assert(presentation.request(EnterGameplayState, GAMESTATE_MENU));
    failed.failBegin = true;
    assert(!fade.start(new FadeProbe(failed, 1), 1, 0, false));
    presentation.cancel();
    assert(!presentation.take(completion));
    assert(presentation.request(StartGameplayInput, GAMESTATE_PLAY));
    FadeProbeLog later;
    later.failRender = true; later.failingFrame = 1;
    assert(fade.start(new FadeProbe(later, 1), 1, 0, false));
    presentation.prepared();
    assert(fade.advance(0) == FadeRunning);
    assert(fade.advance(1) == FadeFailed);
    assert(presentation.recover());
    assert(!presentation.take(completion));
    FadeProbeLog busy;
    busy.retryRender = true; busy.failingFrame = 1;
    assert(fade.start(new FadeProbe(busy, 1), 0, 0, false, 2000));
    presentation.prepared();
    assert(fade.advance(0) == FadeRunning);
    assert(fade.advance(2000) == FadeFailed);
    presentation.cancel();
    assert(!presentation.take(completion));
}

static void stackedArrivalFallback() {
    GameplayPresentation presentation;
    GameplayPresentationAction completion;
    assert(presentation.request(EnterGameplayState, GAMESTATE_HELP));
    presentation.prepared();
    presentation.cancel();
    assert(!presentation.take(completion));
    assert(presentation.request(EnterGameplayState, GAMESTATE_HELP));
    assert(presentation.recover());
    assert(!presentation.recover());
    assert(!presentation.take(completion));
    presentation.prepared();
    assert(presentation.take(completion));
    assert(completion.kind == EnterGameplayState && completion.previous == GAMESTATE_HELP
        && completion.recovery);
    assert(!presentation.take(completion));
}

void testGameplayPresentations() {
    stackedArrivalFallback();
    deferredCompletion(RestoreGameplay, GAMESTATE_NONE);
    deferredCompletion(EnterGameplayState, GAMESTATE_HELP);
    deferredCompletion(EnterGameplayState, GAMESTATE_MENU);
    deferredCompletion(StartGameplayInput, GAMESTATE_PLAY);
    cancelAndRecover();
    failureDoesNotComplete();
}
