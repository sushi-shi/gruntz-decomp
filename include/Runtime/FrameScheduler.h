#ifndef GRUNTZ_RUNTIME_FRAMESCHEDULER_H
#define GRUNTZ_RUNTIME_FRAMESCHEDULER_H

#include <Runtime/FrameTiming.h>

class FrameScheduler {
public:
    FrameScheduler();
    void reset(u32 nowMs);
    void resetFrameTime(u32 nowMs);
    // Cancel a pending frame; the next poll starts from its resume timestamp.
    void suspend();
    // A true result admits one game update. A false result retains the frame
    // start until its pacing deadline; the host returns and calls again later.
    // Timing state is published only when an update is admitted. Canceling a
    // pending frame cannot consume a timer expiry the game has not observed.
    bool poll(u32 nowMs);
    u32 delayMs(u32 nowMs) const;
    FrameTiming& timing() { return m_timing; }
    const FrameTiming& timing() const { return m_timing; }

private:
    FrameTiming m_timing;
    u32 m_frameStartMs;
    bool m_pending;
    bool m_resumePending;
};

#endif
