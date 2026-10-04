#ifndef GRUNTZ_PORTABLE_TEST
#include <StdAfx.h>
#endif
#include <Runtime/LevelLoading.h>

bool LevelLoading::request(bool bootstrap) {
    if (m_active) return false;
    m_bootstrap = bootstrap;
    m_step = LoadTitle;
    m_active = true;
    ++m_generation;
    return true;
}

TransitionProgress LevelLoading::advance(LevelLoadingHost& host, u32 deltaMs) {
    if (!m_active) return TransitionComplete;
    const u32 generation = m_generation;
    const TransitionProgress result = host.AdvanceLevelStep(m_step, deltaMs);
    if (generation != m_generation) return TransitionPending;
    if (result == TransitionPending) return result;
    if (result == TransitionFailed || m_step == LoadNamespaceCompletion
        || (m_step == LoadModeCompletion && !m_bootstrap)) {
        m_active = false;
        return result;
    }
    m_step = static_cast<LevelLoadStep>(m_step + 1);
    return TransitionPending;
}
