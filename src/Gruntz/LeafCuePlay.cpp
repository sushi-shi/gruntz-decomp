#include <Gruntz/Multi.h> // C linkage for the definitions below (inherited, not restated)
#include <Ints.h>
#include <Rez/FrameClock.h> // frame-clock band (g_frameDelta/g_frameTime/g_killCueClock/g_engineFrameDelta)
#include <Gruntz/SoundState.h> // g_sndEnabled/g_sndCueTag
#include <rva.h>
#include <Gruntz/LeafCue.h>         // LeafCue (the sound-cue leaf)
#include <Dsndmgr/DirectSoundMgr.h> // DSoundCloneInst::ConfigureItem (0x1360d0)

RVA(0x0001f940, 0x4c)
// The four forwarded words are DSoundCloneInst::ConfigureItem's own (vol, pan, freqPct,
// loop) @0x1360d0 - it hands them to SetVolumeByIndex / SetPanByIndex / SetField2 /
// SetField3 in that order.
// BOTH guards are EARLY RETURNS. Retail gives each failure its own inline
// `xor eax,eax; pop esi; ret 0x10` because each `return 0` is the taken-branch
// fallthrough of its own guard; wrapping the tail in `if (elapsed >= window) {...}`
// with one trailing `return 0` makes cl share a single epilogue instead.
i32 LeafCue::PlayIfElapsed(i32 vol, i32 pan, i32 freqPct, i32 loop) {
    if (g_sndEnabled == 0) {
        return 0;
    }
    if (g_killCueClock - static_cast<u32>(m_14) < static_cast<u32>(m_18)) {
        return 0;
    }
    m_14 = g_killCueClock;
    return m_10->ConfigureItem(vol, pan, freqPct, loop);
}
