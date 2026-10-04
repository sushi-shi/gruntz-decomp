#ifndef GRUNTZ_RUNTIME_SHUTDOWNREQUEST_H
#define GRUNTZ_RUNTIME_SHUTDOWNREQUEST_H
#include <Ints.h>

// Host timestamps are milliseconds modulo 2^32, sampled less than a cycle apart.
// A request starts on the next poll; repeated requests cannot postpone it.
class ShutdownRequest {
public:
    ShutdownRequest();
    void request(u32 delayMs);
    u32 poll(u32 nowMs);
    bool pending() const { return m_pending; }
    bool ready() const { return m_pending && m_started && !m_remainingMs; }
    // The host consumes this once to dispatch its native close operation.
    bool takeCloseRequest();
private:
    u32 m_remainingMs;
    u32 m_previousMs;
    bool m_pending;
    bool m_started;
    bool m_closeDelivered;
};
#endif
