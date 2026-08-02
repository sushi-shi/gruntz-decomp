#include <Gruntz/Multi.h>
#include <Ints.h>
#include <Rez/FrameClock.h>
#include <Gruntz/SoundState.h>
#include <rva.h>
#include <Gruntz/LeafCue.h>
#include <Dsndmgr/DirectSoundMgr.h>

RVA(0x0001f940, 0x4c)

i32 LeafCue::PlayIfElapsed(i32 vol, i32 pan, i32 freqPct, i32 loop) {
    if (g_sndEnabled == 0) {
        return 0;
    }
    if (g_killCueClock - static_cast<u32>(m_lastPlayTime) < static_cast<u32>(m_replayDelay)) {
        return 0;
    }
    m_lastPlayTime = g_killCueClock;
    return m_sound->ConfigureItem(vol, pan, freqPct, loop);
}
