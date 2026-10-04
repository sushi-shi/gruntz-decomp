#ifndef GRUNTZ_PORTABLE_TEST
#include <StdAfx.h>
#endif
#include <Runtime/FrameScheduler.h>

FrameScheduler::FrameScheduler() : m_frameStartMs(0), m_pending(false), m_resumePending(false) {}

void FrameScheduler::reset(u32 nowMs) {
    m_pending = false;
    m_resumePending = false;
    m_timing.reset(nowMs);
}

void FrameScheduler::resetFrameTime(u32 nowMs) {
    m_pending = false;
    m_resumePending = false;
    m_timing.resetFrameTime(nowMs);
}

void FrameScheduler::suspend() {
    m_pending = false;
    m_resumePending = true;
}

bool FrameScheduler::poll(u32 nowMs) {
    if (m_resumePending) resetFrameTime(nowMs);
    if (!m_pending) {
        m_frameStartMs = nowMs;
        m_pending = true;
    }
    if (m_timing.pacingDelay(nowMs)) return false;
    m_timing.beginFrame(m_frameStartMs);
    m_timing.finishPacing(nowMs);
    m_pending = false;
    return true;
}

u32 FrameScheduler::delayMs(u32 nowMs) const {
    // Sample the next frame start immediately after the preceding game update.
    return m_pending ? m_timing.pacingDelay(nowMs) : 0;
}
