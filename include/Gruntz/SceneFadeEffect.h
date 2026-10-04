#ifndef GRUNTZ_SCENEFADEEFFECT_H
#define GRUNTZ_SCENEFADEEFFECT_H
#include <Runtime/FadePlayback.h>
#include <Gruntz/FaderSubtypes.h>

// Surfaces are borrowed until playback ends; state resource/mode changes cancel first.
class SceneFadeEffect : public FadeEffect {
public:
    SceneFadeEffect(CDDSurface* target, CDDSurface* source, i32 intensityPercent);
    virtual u32 frameCount();
    virtual bool begin();
    virtual FadeRenderResult render(u32 frame);
    virtual void end();
private:
    CFaderSine m_fader;
    bool m_valid;
};
#endif
