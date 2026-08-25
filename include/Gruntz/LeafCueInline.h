#ifndef GRUNTZ_LEAFCUEINLINE_H
#define GRUNTZ_LEAFCUEINLINE_H

#include <Dsndmgr/SoundBuffer.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/SoundState.h>
#include <Rez/FrameClock.h>

// Opt-in inline visibility for LeafCue::PlayIfElapsed (out of line at 0x1f940
// in BootyStateActivate.cpp, which wraps this helper).  A WORKAROUND for
// caller-side modelling error, not a proven era structure - no dev writes a
// per-TU visibility header, and no dev writes the member as a free function
// beside itself.  Measured 2026-08-22 by collapsing to ONE out-of-class inline
// LeafCue::PlayIfElapsed in LeafCue.h carrying the RVA pin, the wrapper deleted
// and all seven helper calls rewritten to the member:
//   * 0x1f940 kept an emitter (rehomed bootystateactivate -> sbi_rectonly) and
//     stayed 100.00 EXACT, so "an in-class body loses the RVA" is false here too;
//   * the ~15 TUs retail CALLS it from expanded it instead: CRezImage::FillRectAt
//     100.00 -> 66.44, CSBI_MenuItem::Render 100.00 -> 74.04, CSBI_Image::Render
//     100.00 -> 74.07, CGruntzMgr::HandleCommand 98.57 -> 78.98 and more
//     (-221 total, -12 exact).
// REMOVAL CONDITION: model those callers accurately enough that cl 5.0 declines
// the cue on its own budget where retail called; then one visible body in
// LeafCue.h reproduces the split and both this header and the wrapper collapse.
inline i32 PlayLeafCueIfElapsed(
    LeafCue* cue,
    i32 volumePercent,
    i32 panPercent,
    i32 frequencyOffsetPercent,
    i32 looping
) {
    if (g_soundEnabled == 0) {
        return 0;
    }
    if (g_soundCueTimeMs - static_cast<u32>(cue->m_lastPlayTimeMs)
        < static_cast<u32>(cue->m_replayDelayMs)) {
        return 0;
    }
    cue->m_lastPlayTimeMs = g_soundCueTimeMs;
    return cue->m_sound->AcquireAndPlay(volumePercent, panPercent, frequencyOffsetPercent, looping);
}

#endif // GRUNTZ_LEAFCUEINLINE_H
