#ifndef GRUNTZ_TEST_FADEPROBE_H
#define GRUNTZ_TEST_FADEPROBE_H
#include <Runtime/FadePlayback.h>
#include <vector>

struct FadeProbeLog {
    FadeProbeLog() : begins(0), ends(0), destroyed(0), failBegin(false), failRender(false), retryRender(false), failingFrame(0) {}
    unsigned int begins, ends, destroyed;
    bool failBegin, failRender, retryRender;
    u32 failingFrame;
    std::vector<u32> frames;
};

class FadeProbe : public FadeEffect {
public:
    FadeProbe(FadeProbeLog& log, u32 count) : m_log(log), m_count(count) {}
    virtual ~FadeProbe() { ++m_log.destroyed; }
    virtual u32 frameCount() { return m_count; }
    virtual bool begin() { ++m_log.begins; return !m_log.failBegin; }
    virtual FadeRenderResult render(u32 frame) {
        m_log.frames.push_back(frame);
        if (m_log.failRender && frame == m_log.failingFrame) return FadeRenderFailed;
        if (m_log.retryRender && frame == m_log.failingFrame) return FadeRetry;
        return FadeRendered;
    }
    virtual void end() { ++m_log.ends; }
private:
    FadeProbeLog& m_log;
    u32 m_count;
};
#endif
