#include <StdAfx.h>
#include <Gruntz/SceneFadeEffect.h>
#include <Gruntz/State.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/FaderSettings.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfacePair.h>

SceneFadeEffect::SceneFadeEffect(CDDSurface* target, CDDSurface* source, i32 intensityPercent) {
    m_fader.SetDefaultSurfaces(NULL, NULL);
    m_fader.SetDeviceManager(NULL);
    CSineFaderConfig config;
    config.m_clearToBlack = false;
    config.m_intensityPercent = intensityPercent;
    config.m_targetSurface = target;
    config.m_sourceSurface = source;
    m_valid = m_fader.ApplyInit(&config) != 0;
}
u32 SceneFadeEffect::frameCount() { return m_valid ? m_fader.GetFrameCount() : 0; }
bool SceneFadeEffect::begin() { m_fader.BeginFade(); return m_valid; }
FadeRenderResult SceneFadeEffect::render(u32 frame) { return m_fader.TryRenderFrame(frame); }
void SceneFadeEffect::end() { m_fader.EndFade(); }

i32 CState::BeginSceneFade(i32 intensityPercent, u32 durationMs, u32 leadMs, bool useOverlay) {
    CancelSceneFade();
    if (!m_world || !m_world->GetDeviceManager() || !m_world->GetDrawTarget()) return 0;
    CDDrawSubMgrPages* pages = m_world->GetDrawTarget();
    CDDrawSurfacePair* source = useOverlay && pages->HasOverlay()
        ? pages->m_overlayPair : pages->GetBackPair();
    if (!source || !pages->GetFrontSurface()) return 0;
    CDDSurface* targetSurface = pages->GetFrontSurface()->GetSurface();
    CDDSurface* sourceSurface = source->GetSurface();
    if (!targetSurface || !sourceSurface || targetSurface == sourceSurface) return 0;
    return m_sceneFade.start(new SceneFadeEffect(targetSurface, sourceSurface, intensityPercent),
        durationMs, g_disableFades ? 0 : leadMs, g_disableFades != false);
}

void CState::CancelSceneFade() { m_sceneFade.cancel(); }

i32 CState::AdvanceSceneFade(u32 deltaMs) {
    const FadeProgress progress = m_sceneFade.advance(deltaMs);
    if (progress == FadeFailed) {
        // Playback released all surface borrows before restoration may replace resources.
        return RestoreAfterSceneFade() ? 1 : -1;
    }
    if (progress == FadeFinished) OnSceneFadeComplete();
    return progress == FadeRunning ? 1 : 0;
}
