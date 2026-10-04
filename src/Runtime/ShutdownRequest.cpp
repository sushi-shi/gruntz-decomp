#ifndef GRUNTZ_PORTABLE_TEST
#include <StdAfx.h>
#endif
#include <Runtime/ShutdownRequest.h>

ShutdownRequest::ShutdownRequest()
    : m_remainingMs(0), m_previousMs(0), m_pending(false), m_started(false), m_closeDelivered(false) {}

void ShutdownRequest::request(u32 delayMs) {
    if (m_pending) return;
    m_pending = true;
    // Keep the host's all-bits-set indefinite-wait value out of real deadlines.
    m_remainingMs = delayMs > 0x7fffffffU ? 0x7fffffffU : delayMs;
}

u32 ShutdownRequest::poll(u32 nowMs) {
    if (!m_pending || m_closeDelivered) return 0xffffffffU;
    if (m_started) {
        const u32 elapsed = nowMs - m_previousMs;
        m_remainingMs = elapsed >= m_remainingMs ? 0 : m_remainingMs - elapsed;
    }
    m_previousMs = nowMs;
    m_started = true;
    return m_remainingMs;
}

bool ShutdownRequest::takeCloseRequest() {
    if (!ready() || m_closeDelivered) return false;
    m_closeDelivered = true;
    return true;
}
