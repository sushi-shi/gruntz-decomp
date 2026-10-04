#ifndef GRUNTZ_RUNTIME_FADEPLAYBACK_H
#define GRUNTZ_RUNTIME_FADEPLAYBACK_H
#include <Ints.h>

enum FadeRenderResult { FadeRendered, FadeRetry, FadeRenderFailed };

class FadeEffect {
public:
    virtual ~FadeEffect() {}
    virtual u32 frameCount() = 0;
    virtual bool begin() = 0;
    virtual FadeRenderResult render(u32 frame) = 0;
    virtual void end() = 0;
};

enum FadeProgress { FadeIdle, FadeRunning, FadeFinished, FadeFailed };

class FadePlayback {
public:
    FadePlayback();
    ~FadePlayback();
    // Takes ownership, including on failure. Replacing a fade cancels it first.
    bool start(FadeEffect* effect, u32 durationMs, u32 leadMs, bool finalOnly);
    // At most one rendered frame per callback. Pass the admitted gameplay delta;
    // suspension/resume must contribute zero. The first callback establishes the epoch.
    FadeProgress advance(u32 deltaMs);
    void cancel();
    bool active() const { return m_effect != 0; }
private:
    FadePlayback(const FadePlayback&);
    FadePlayback& operator=(const FadePlayback&);
    FadeEffect* m_effect;
    u32 m_count, m_frame, m_durationMs, m_elapsedMs, m_leadMs;
    bool m_begun, m_first, m_finalOnly;
};
#endif
