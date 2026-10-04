#ifndef GRUNTZ_RUNTIME_STATETRANSITION_H
#define GRUNTZ_RUNTIME_STATETRANSITION_H
#include <Ints.h>

enum TransitionProgress { TransitionPending, TransitionComplete, TransitionFailed };

class StateTransitionHost {
public:
    virtual ~StateTransitionHost() {}
    virtual bool BeginDeparture() = 0;
    virtual TransitionProgress AdvanceDeparture(u32 deltaMs) = 0;
    virtual bool InstallDestination() = 0;
    virtual TransitionProgress AdvanceArrival(u32 deltaMs) = 0;
};

// The host retains the departing state until InstallDestination. No host action
// runs in request(); a request made by a state cannot delete its own call frame.
class StateTransition {
public:
    StateTransition() : m_phase(Idle), m_generation(0) {}
    bool request();
    TransitionProgress advance(StateTransitionHost& host, u32 deltaMs);
    void cancel() { m_phase = Idle; ++m_generation; }
    bool active() const { return m_phase != Idle; }
private:
    enum Phase { Idle, Begin, Depart, Install, Arrive };
    Phase m_phase;
    u32 m_generation;
};

class TransitionDelay {
public:
    TransitionDelay() : m_remaining(0), m_first(false) {}
    void start(u32 durationMs) { m_remaining = durationMs; m_first = true; }
    void cancel() { m_remaining = 0; m_first = false; }
    bool advance(u32 deltaMs) {
        if (m_first) { m_first = false; deltaMs = 0; }
        m_remaining -= deltaMs < m_remaining ? deltaMs : m_remaining;
        return m_remaining == 0;
    }
private:
    u32 m_remaining;
    bool m_first;
};
#endif
