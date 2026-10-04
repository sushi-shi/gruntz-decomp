#ifndef GRUNTZ_PORTABLE_TEST
#include <StdAfx.h>
#endif
#include <Runtime/FadePlayback.h>

FadePlayback::FadePlayback() : m_effect(0), m_count(0), m_frame(0), m_durationMs(0),
    m_elapsedMs(0), m_leadMs(0), m_retryTimeoutMs(0), m_retryElapsedMs(0),
    m_begun(false), m_first(false), m_finalOnly(false), m_retrying(false) {}
FadePlayback::~FadePlayback() { cancel(); }

void FadePlayback::cancel() {
    FadeEffect* effect = m_effect;
    const bool begun = m_begun;
    m_effect = 0;
    m_begun = false;
    if (effect) {
        if (begun) effect->end();
        delete effect;
    }
}

bool FadePlayback::start(FadeEffect* effect, u32 durationMs, u32 leadMs, bool finalOnly,
    u32 retryTimeoutMs) {
    cancel();
    m_effect = effect;
    m_count = effect ? effect->frameCount() : 0;
    if (!m_count) { cancel(); return false; }
    m_durationMs = durationMs;
    m_elapsedMs = 0;
    m_leadMs = leadMs;
    m_frame = 0;
    m_first = true;
    m_finalOnly = finalOnly;
    m_retryTimeoutMs = retryTimeoutMs;
    m_retryElapsedMs = 0;
    m_retrying = false;
    m_begun = effect->begin();
    if (!m_begun || effect->render(0) != FadeRendered) { cancel(); return false; }
    return true;
}

FadeProgress FadePlayback::advance(u32 deltaMs) {
    if (!m_effect) return FadeIdle;
    if (m_first) { m_first = false; deltaMs = 0; }
    const u32 lead = deltaMs < m_leadMs ? deltaMs : m_leadMs;
    m_leadMs -= lead;
    deltaMs -= lead;
    if (m_leadMs) return FadeRunning;
    const u32 remaining = m_durationMs - m_elapsedMs;
    m_elapsedMs += deltaMs < remaining ? deltaMs : remaining;
    const u32 frame = m_durationMs
        ? static_cast<u32>(static_cast<u64>(m_elapsedMs) * m_count / m_durationMs) : m_count;
    if (frame != m_frame && (!m_finalOnly || frame == m_count)) {
        const FadeRenderResult result = m_effect->render(frame);
        if (result == FadeRetry) {
            if (m_retryTimeoutMs && m_retrying) {
                const u32 remainingRetry = m_retryTimeoutMs - m_retryElapsedMs;
                m_retryElapsedMs += deltaMs < remainingRetry ? deltaMs : remainingRetry;
                if (m_retryElapsedMs == m_retryTimeoutMs) { cancel(); return FadeFailed; }
            }
            m_retrying = true;
            return FadeRunning;
        }
        if (result != FadeRendered) { cancel(); return FadeFailed; }
        m_retrying = false;
        m_retryElapsedMs = 0;
    }
    m_frame = frame;
    if (m_elapsedMs == m_durationMs) { cancel(); return FadeFinished; }
    return FadeRunning;
}
