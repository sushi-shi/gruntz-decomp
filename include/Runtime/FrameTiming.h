#ifndef GRUNTZ_RUNTIME_FRAMETIMING_H
#define GRUNTZ_RUNTIME_FRAMETIMING_H

#include <Ints.h>

class FrameTiming {
public:
    FrameTiming();

    // Timestamps are monotonic milliseconds modulo 2^32. Consecutive samples
    // must be less than one full clock cycle apart. The host owns the clock.
    void reset(u32 nowMs);
    void resetFrameTime(u32 nowMs);
    void beginFrame(u32 nowMs);
    // Call once after host pacing, before updating the game. The frame delta
    // stays based on beginFrame; completion time starts the next pacing window.
    void finishPacing(u32 nowMs);
    u32 pacingDelay(u32 nowMs) const;

    void setFrameRate(i32 fps);
    // Changing the period does not restart the current countdown.
    void setTimerPeriod(u32 periodMs) { m_timerPeriodMs = periodMs; }

    u32 nowMs() const { return m_nowMs; }
    u32 deltaMs() const { return m_deltaMs; }
    u32 timerRemainingMs() const { return m_timerRemainingMs; }
    i32 fps() const { return m_fps; }
    i32 targetFps() const { return m_targetFps; }

private:
    u32 m_nowMs;
    u32 m_deltaMs;
    u32 m_timerPeriodMs;
    u32 m_timerRemainingMs;
    u32 m_pacingEpochMs;
    bool m_hasPacingEpoch;
    i32 m_targetFps;
    u32 m_frameBudgetMs;
    u32 m_fpsSampleStartMs;
    u32 m_fpsSampleFrameCount;
    i32 m_fps;
};

#endif
