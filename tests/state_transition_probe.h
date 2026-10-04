#ifndef GRUNTZ_TEST_STATE_TRANSITION_PROBE_H
#define GRUNTZ_TEST_STATE_TRANSITION_PROBE_H
#include <Runtime/StateTransition.h>
#include <cassert>
#include <string>

struct TransitionProbeState {
    explicit TransitionProbeState(int& destroyed) : destroyed(destroyed) {}
    ~TransitionProbeState() { ++destroyed; }
    int& destroyed;
};

class TransitionProbe : public StateTransitionHost {
public:
    TransitionProbe() : destroyed(0), current(new TransitionProbeState(destroyed)),
        fail(0), departureWait(0), arrivalWait(0), cancelDuring(0), replaceDuring(false), sequence(0) {}
    ~TransitionProbe() { delete current; }
    virtual bool BeginDeparture() {
        calls += 'B';
        assert(current && destroyed == 0);
        delay.start(departureWait);
        interrupt('B');
        return fail != 'B';
    }
    virtual TransitionProgress AdvanceDeparture(u32 deltaMs) {
        calls += 'D';
        assert(current && destroyed == 0);
        interrupt('D');
        if (fail == 'D') return TransitionFailed;
        return delay.advance(deltaMs) ? TransitionComplete : TransitionPending;
    }
    virtual bool InstallDestination() {
        calls += 'I';
        assert(current && destroyed == 0);
        delete current;
        current = new TransitionProbeState(destroyed);
        interrupt('I');
        return fail != 'I';
    }
    virtual TransitionProgress AdvanceArrival(u32) {
        calls += 'A';
        assert(current && destroyed == 1);
        interrupt('A');
        if (fail == 'A') return TransitionFailed;
        if (arrivalWait) { --arrivalWait; return TransitionPending; }
        return TransitionComplete;
    }
    void interrupt(char phase) {
        if (cancelDuring == phase) {
            sequence->cancel();
            if (replaceDuring) assert(sequence->request());
        }
    }
    int destroyed;
    TransitionProbeState* current;
    std::string calls;
    char fail;
    u32 departureWait, arrivalWait;
    char cancelDuring;
    bool replaceDuring;
    StateTransition* sequence;
    TransitionDelay delay;
};
#endif
