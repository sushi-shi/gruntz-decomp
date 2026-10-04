#ifndef GRUNTZ_PORTABLE_TEST
#include <StdAfx.h>
#endif
#include <Runtime/StateTransition.h>

bool StateTransition::request() {
    if (active()) return false;
    m_phase = Begin;
    ++m_generation;
    return true;
}

TransitionProgress StateTransition::advance(StateTransitionHost& host, u32 deltaMs) {
    const u32 generation = m_generation;
    Phase next = m_phase;
    TransitionProgress result = TransitionPending;
    switch (m_phase) {
    case Idle: return TransitionComplete;
    case Begin:
        if (!host.BeginDeparture()) result = TransitionFailed;
        else next = Depart;
        break;
    case Depart:
        result = host.AdvanceDeparture(deltaMs);
        if (result == TransitionComplete) { next = Install; result = TransitionPending; }
        break;
    case Install:
        if (!host.InstallDestination()) result = TransitionFailed;
        else next = Arrive;
        break;
    case Arrive:
        result = host.AdvanceArrival(deltaMs);
        break;
    }
    // Host loading/restoration can pump messages and cancel or replace this request.
    if (generation != m_generation) return TransitionPending;
    m_phase = result == TransitionPending ? next : Idle;
    return result;
}
