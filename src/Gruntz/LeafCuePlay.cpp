#include <Ints.h>
#include <Rez/FrameClock.h> // frame-clock band (g_frameDelta/g_frameTime/g_killCueClock/g_engineFrameDelta)
#include <Gruntz/SoundState.h> // g_sndEnabled/g_sndCueTag
#include <rva.h>
#include <Gruntz/LeafCue.h>         // LeafCue (the sound-cue leaf)
#include <Dsndmgr/DirectSoundMgr.h> // DSoundCloneInst::ConfigureItem (0x1360d0)


// @early-stop
// 66% -- split-epilogue wall (twin of RefreshAsset_114120's 100% idiom, but 4-arg):
// the gate/interval guards, the wrap-safe clock compare, the clock restamp, and the
// identical 4-arg push sequence into ConfigureItem all match. MSVC5 here MERGES the
// two guard-failure `return 0` exits into one shared `pop esi; ret 0x10` tail
// (je/jb to a shared epilogue) where retail emits each failure as its own inline
// `xor eax; pop esi; ret 0x10` (jne/jae split). The ConfigureItem callee is
// reloc-masked (CStatusBarMgr vs LeafCuePlayer at the same 0x1360d0). Not source-
// steerable (tried flat + nested guard forms). Logic complete.
RVA(0x0001f940, 0x4c)
i32 LeafCue::PlayIfElapsed(i32 a0, i32 a1, i32 a2, i32 a3) {
    if (g_sndEnabled == 0) {
        return 0;
    }
    if (g_killCueClock - static_cast<u32>(m_14) >= static_cast<u32>(m_18)) {
        m_14 = g_killCueClock;
        return m_10->ConfigureItem(a0, a1, a2, a3);
    }
    return 0;
}
