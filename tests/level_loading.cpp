#include "state_transition_probe.h"
#include <Runtime/LevelLoading.h>
#include <Runtime/FrameScheduler.h>
#include <vector>

class LoadingProbe : public LevelLoadingHost {
public:
    LoadingProbe() : sequence(0), fail(-1), cancelAt(-1), replace(false), titleFrames(2),
        batches(0), modeFinished(false), namespacesFinished(false), presented(false) {}
    virtual TransitionProgress AdvanceLevelStep(LevelLoadStep step, u32 deltaMs) {
        calls.push_back(step);
        if (step == fail) return TransitionFailed;
        if (step == cancelAt) {
            sequence->cancel();
            if (replace) assert(sequence->request(false));
            return TransitionComplete;
        }
        if (step == LoadTitle && titleFrames) { --titleFrames; return TransitionPending; }
        if (step >= LoadArea && step <= LoadMusic) ++batches;
        if (step == LoadMusic) delay.start(100);
        if (step == LoadPresentation) {
            if (!delay.advance(deltaMs)) return TransitionPending;
            presented = true;
        }
        if (step == LoadModeCompletion) {
            assert(presented && !modeFinished);
            modeFinished = true;
        }
        if (step == LoadNamespaceCompletion) {
            assert(modeFinished && !namespacesFinished);
            namespacesFinished = true;
        }
        return TransitionComplete;
    }
    LevelLoading* sequence;
    int fail, cancelAt;
    bool replace;
    unsigned titleFrames, batches;
    bool modeFinished, namespacesFinished, presented;
    TransitionDelay delay;
    std::vector<LevelLoadStep> calls;
};

class LoadingTransitionProbe : public TransitionProbe {
public:
    explicit LoadingTransitionProbe(bool bootstrap) : bootstrap(bootstrap) {}
    virtual bool InstallDestination() {
        assert(TransitionProbe::InstallDestination());
        assert(loading.request(bootstrap));
        return true;
    }
    virtual TransitionProgress AdvanceInstallation(u32 deltaMs) {
        calls += 'L';
        assert(current && destroyed == 1);
        return loading.advance(assets, deltaMs);
    }
    virtual bool BeginArrival() {
        assert(!loading.active() && assets.modeFinished && assets.presented);
        assert(assets.namespacesFinished == bootstrap);
        return TransitionProbe::BeginArrival();
    }
    bool bootstrap;
    LevelLoading loading;
    LoadingProbe assets;
};

static void completeLoad(bool bootstrap) {
    StateTransition sequence;
    LoadingTransitionProbe host(bootstrap);
    assert(sequence.request());
    for (unsigned tick = 0; tick < 100 && sequence.active(); ++tick) {
        const unsigned batches = host.assets.batches;
        const std::size_t calls = host.assets.calls.size();
        assert(sequence.advance(host, 50) != TransitionFailed);
        assert(host.assets.batches <= batches + 1);
        assert(host.assets.calls.size() <= calls + 1);
        if (host.loading.active()) assert(host.calls.find('E') == std::string::npos);
    }
    assert(!sequence.active());
    assert(host.assets.batches == LoadMusic - LoadArea + 1);
    assert(host.assets.modeFinished && host.assets.namespacesFinished == bootstrap);
    assert(host.calls.substr(host.calls.size()-2) == "EA");
    const std::size_t count = host.assets.calls.size();
    assert(host.loading.advance(host.assets, 100) == TransitionComplete);
    assert(host.assets.calls.size() == count);
}

static void failuresAndCancellation() {
    for (int step = LoadTitle; step <= LoadNamespaceCompletion; ++step) {
        LevelLoading sequence;
        LoadingProbe host;
        host.fail = step;
        assert(sequence.request(true));
        TransitionProgress result = TransitionPending;
        for (unsigned tick = 0; tick < 100 && result == TransitionPending; ++tick)
            result = sequence.advance(host, 100);
        assert(result == TransitionFailed && !sequence.active());
        assert(host.calls.back() == step);
        const std::size_t count = host.calls.size();
        assert(sequence.advance(host, 100) == TransitionComplete);
        assert(host.calls.size() == count);
        for (unsigned restart = 0; restart < 2; ++restart) {
            LoadingProbe interrupted;
            interrupted.sequence = &sequence;
            interrupted.cancelAt = step;
            interrupted.replace = restart != 0;
            assert(sequence.request(true));
            assert(!sequence.request(false));
            while (interrupted.calls.empty() || interrupted.calls.back() != step)
                assert(sequence.advance(interrupted, 100) == TransitionPending);
            assert(sequence.active() == (restart != 0));
            if (restart) assert(sequence.step() == LoadTitle);
            sequence.cancel();
        }
    }
}

static void presentationClock() {
    LevelLoading sequence;
    LoadingProbe host;
    host.titleFrames = 0;
    assert(sequence.request(false));
    while (sequence.step() != LoadPresentation)
        assert(sequence.advance(host, 0xffffffffU) == TransitionPending);
    FrameScheduler frames;
    frames.reset(0xfffffff0U);
    assert(frames.poll(0xfffffff0U));
    assert(sequence.advance(host, frames.timing().deltaMs()) == TransitionPending);
    assert(!host.presented);
    assert(frames.poll(0x22));
    assert(sequence.advance(host, frames.timing().deltaMs()) == TransitionPending);
    frames.suspend();
    assert(frames.poll(1000000));
    assert(sequence.advance(host, frames.timing().deltaMs()) == TransitionPending);
    assert(!host.presented);
    assert(frames.poll(1000050));
    assert(sequence.advance(host, frames.timing().deltaMs()) == TransitionPending);
    assert(host.presented && !host.modeFinished);
    assert(sequence.advance(host, 0) == TransitionComplete);
}

void testLevelLoading() {
    completeLoad(false);
    completeLoad(true);
    failuresAndCancellation();
    presentationClock();
}
