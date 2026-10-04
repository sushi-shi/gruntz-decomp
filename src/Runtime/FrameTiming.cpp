#ifndef GRUNTZ_PORTABLE_TEST
#include <StdAfx.h>
#endif
#include <Runtime/FrameTiming.h>
#include <limits.h>

#if UINT_MAX != 0xffffffffU
#error Frame timing requires a 32-bit unsigned int
#endif

FrameTiming::FrameTiming() {
    reset(0);
}

void FrameTiming::reset(u32 nowMs) {
    resetFrameTime(nowMs);
    m_timerPeriodMs = 100;
    m_timerRemainingMs = 100;
    m_targetFps = 0;
    m_frameBudgetMs = 0;
    m_fpsSampleStartMs = nowMs;
    m_fpsSampleFrameCount = 0;
    m_fps = -1;
}

void FrameTiming::resetFrameTime(u32 nowMs) {
    m_nowMs = nowMs;
    m_deltaMs = 0;
    m_pacingEpochMs = 0;
    m_hasPacingEpoch = false;
}

void FrameTiming::beginFrame(u32 nowMs) {
    m_deltaMs = nowMs - m_nowMs;
    m_nowMs = nowMs;
    // Expiry is visible for one frame. Rearming consumes no elapsed time.
    if (m_timerRemainingMs == 0) {
        m_timerRemainingMs = m_timerPeriodMs;
    } else if (m_deltaMs >= m_timerRemainingMs) {
        m_timerRemainingMs = 0;
    } else {
        m_timerRemainingMs -= m_deltaMs;
    }
}

u32 FrameTiming::pacingDelay(u32 nowMs) const {
    if (m_targetFps <= 0 || !m_hasPacingEpoch) return 0;
    const u32 elapsed = nowMs - m_pacingEpochMs;
    return elapsed < m_frameBudgetMs ? m_frameBudgetMs - elapsed : 0;
}

void FrameTiming::finishPacing(u32 nowMs) {
    if (m_targetFps > 0) {
        m_pacingEpochMs = nowMs;
        m_hasPacingEpoch = true;
    }
    ++m_fpsSampleFrameCount;
    // Preserve the two-second FPS counter even after a long host pause.
    if (m_nowMs - m_fpsSampleStartMs >= 2000) {
        m_fps = m_fpsSampleFrameCount / 2;
        m_fpsSampleFrameCount = 0;
        m_fpsSampleStartMs = nowMs;
    }
}

void FrameTiming::setFrameRate(i32 fps) {
    m_targetFps = fps > 0 ? fps : 0;
    m_frameBudgetMs = fps > 0 ? 1000U / static_cast<u32>(fps) : 0;
}
