#ifndef GRUNTZ_RUNTIME_LEVELLOADING_H
#define GRUNTZ_RUNTIME_LEVELLOADING_H
#include <Runtime/StateTransition.h>

enum LevelLoadStep {
    LoadTitle, LoadArea, LoadActionTiles, LoadAreaImages, LoadCommonImages,
    LoadImageKeys, LoadAreaSounds, LoadCommonSounds, LoadGruntSounds,
    LoadAreaAnimations, LoadCommonAnimations, LoadAnimationKeys, LoadWorld,
    LoadMap, LoadPlayers, LoadActors, LoadMusic, LoadPresentation,
    LoadModeCompletion, LoadNamespaceCompletion
};

class LevelLoadingHost {
public:
    virtual ~LevelLoadingHost() {}
    virtual TransitionProgress AdvanceLevelStep(LevelLoadStep step, u32 deltaMs) = 0;
};

// One asset batch or presentation tick per host callback. Namespace completion
// belongs only to a newly created state; a round reload runs mode completion only.
class LevelLoading {
public:
    LevelLoading() : m_active(false), m_bootstrap(false), m_step(LoadTitle), m_generation(0) {}
    bool request(bool bootstrap);
    TransitionProgress advance(LevelLoadingHost& host, u32 deltaMs);
    void cancel() { m_active = false; ++m_generation; }
    bool active() const { return m_active; }
    LevelLoadStep step() const { return m_step; }
private:
    bool m_active, m_bootstrap;
    LevelLoadStep m_step;
    u32 m_generation;
};
#endif
