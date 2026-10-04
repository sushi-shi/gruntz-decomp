#include "state_transition_probe.h"
#include <Runtime/FrameScheduler.h>
#include <cstdio>

static void orderAndLifetime() {
    StateTransition sequence;
    TransitionProbe host;
    host.departureWait = 500;
    host.arrivalWait = 1;
    assert(sequence.request());
    assert(!sequence.request());
    assert(host.calls.empty() && host.destroyed == 0);
    assert(sequence.advance(host, 9999) == TransitionPending && host.calls == "B");
    assert(sequence.advance(host, 9999) == TransitionPending && host.destroyed == 0);
    assert(sequence.advance(host, 499) == TransitionPending && host.destroyed == 0);
    assert(sequence.advance(host, 1) == TransitionPending && host.destroyed == 0);
    assert(sequence.advance(host, 0) == TransitionPending && host.destroyed == 1);
    assert(sequence.advance(host, 0) == TransitionPending);
    assert(sequence.advance(host, 0) == TransitionComplete && !sequence.active());
    assert(host.calls == "BDDDIAA");
    assert(sequence.advance(host, 0) == TransitionComplete && host.calls == "BDDDIAA");
}

static void failuresAndCancellation() {
    const char phases[] = {'B', 'D', 'I', 'A'};
    for (unsigned int i = 0; i < sizeof(phases); ++i) {
        StateTransition sequence;
        TransitionProbe host;
        host.fail = phases[i];
        assert(sequence.request());
        for (unsigned int step = 0; step < i; ++step)
            assert(sequence.advance(host, 0) == TransitionPending);
        assert(sequence.advance(host, 0) == TransitionFailed && !sequence.active());
        assert(host.calls.back() == phases[i]);
        assert(host.destroyed == (i < 2 ? 0 : 1));
    }
    for (unsigned int i = 0; i < sizeof(phases); ++i) {
        StateTransition sequence;
        TransitionProbe host;
        host.sequence = &sequence;
        host.cancelDuring = phases[i];
        assert(sequence.request());
        for (unsigned int step = 0; step <= i; ++step)
            assert(sequence.advance(host, 0) == TransitionPending);
        assert(!sequence.active());
        const std::string calls = host.calls;
        assert(sequence.advance(host, 0) == TransitionComplete && host.calls == calls);
    }
    StateTransition sequence;
    TransitionProbe host;
    host.sequence = &sequence;
    host.cancelDuring = 'B'; host.replaceDuring = true;
    assert(sequence.request());
    assert(sequence.advance(host, 0) == TransitionPending && sequence.active());
    host.cancelDuring = 0;
    assert(sequence.advance(host, 0) == TransitionPending && host.calls == "BB");
    sequence.cancel();
    assert(host.destroyed == 0);
}

static void delayAndSuspension() {
    TransitionDelay delay;
    delay.start(0xffffffffU);
    assert(!delay.advance(0xffffffffU));
    assert(!delay.advance(0xfffffffeU));
    assert(delay.advance(0xffffffffU));
    delay.start(10); delay.cancel(); assert(delay.advance(0));
    FrameScheduler frames;
    frames.reset(0xfffffff0U);
    delay.start(40);
    assert(frames.poll(0xfffffff0U)); assert(!delay.advance(frames.timing().deltaMs()));
    assert(frames.poll(4)); assert(!delay.advance(frames.timing().deltaMs()));
    frames.suspend();
    assert(frames.poll(500000)); assert(!delay.advance(frames.timing().deltaMs()));
    assert(frames.poll(500020)); assert(delay.advance(frames.timing().deltaMs()));
}

int main() {
    orderAndLifetime(); failuresAndCancellation(); delayAndSuspension();
    std::puts("Returning state transition tests passed.");
}
