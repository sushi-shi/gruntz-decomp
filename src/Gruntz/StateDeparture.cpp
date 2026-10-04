#include <StdAfx.h>
#include <Gruntz/State.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SoundCueRegistryInline.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Dsndmgr/SoundBuffer.h>

bool CState::StartDeparture(GameStateId nextState) {
    CancelDeparture();
    m_departureActive = true;
    m_departureRestoreAttempted = false;
    m_departureTarget = nextState;
    if (!LeaveState(nextState)) { CancelDeparture(); return false; }
    return true;
}

i32 CState::BeginAudioDeparture(SoundCue* cue, u32 minimumDelayMs) {
    m_departureDelay.start(minimumDelayMs);
    m_departureSound = cue ? cue->m_sound : NULL;
    if (m_departureSound && m_departureSound->IsPlaying()) {
        if (!m_departureSound->RampVolumeTo(0, 500, true)) return 0;
        m_departureRamp.start(500);
    } else {
        m_departureSound = NULL;
    }
    return 1;
}

TransitionProgress CState::AdvanceStateDeparture(u32 deltaMs) {
    if (!m_departureActive) return TransitionComplete;
    const bool delayDone = m_departureDelay.advance(deltaMs);
    if (m_departureSound) {
        const bool rampExpired = m_departureRamp.advance(deltaMs);
        m_world->SoundRegistry()->TickVolumeRamps();
        if (!m_departureSound->IsPlaying()) m_departureSound = NULL;
        else if (rampExpired) {
            const bool stopped = m_departureSound->StopAndRewind() != 0;
            m_departureSound = NULL;
            if (!stopped) { CancelDeparture(); return TransitionFailed; }
        }
    }
    if (m_sceneFade.active()) {
        const FadeProgress progress = m_sceneFade.advance(deltaMs);
        if (progress == FadeFailed && !RecoverDeparture()) {
            CancelDeparture();
            return TransitionFailed;
        }
    }
    if (!delayDone || m_departureSound || m_sceneFade.active()) return TransitionPending;
    m_departureActive = false;
    return FinishDeparture(m_departureTarget) ? TransitionComplete : TransitionFailed;
}

i32 CState::RecoverDeparture() {
    if (!m_departureActive || m_departureRestoreAttempted) return 0;
    m_departureRestoreAttempted = true;
    CancelSceneFade();
    return RestoreDeparture();
}

void CState::CancelDeparture() {
    m_departureActive = false;
    m_departureSound = NULL;
    m_departureDelay.cancel();
    m_departureRamp.cancel();
    CancelSceneFade();
}
