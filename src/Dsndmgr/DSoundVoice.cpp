#include <rva.h>
// DSoundVoice.cpp - the 0x28-byte "playing voice" node of the Dsndmgr module
// (C:\Proj\Dsndmgr\, retail vftable 0x5ef6d0). DirectSoundMgr::CloneAndPlay
// (0x135660) new's one of these per requested play and threads its intrusive
// anchor (+0x04) into the owner's voice list; the voice ctor (0x136fe0) stamps
// the vtable + the play params. The trace tagged the node ClassUnknown_55.
//
// A voice drives a single cloned DirectSound buffer (m_10, a DirectSoundMgr) over
// a volume ramp: from m_1c -> m_18 across m_20 ms starting at m_24, with m_14 a
// "stop+rewind when finished" flag. The two methods modeled here are vtable slots
// 0 (Tick, 0x137060) and 1 (Stop, 0x1370d0); both call the buffer's IsPlaying /
// SetVolumeByIndex / StopAndRewind wrappers (all reloc-masked __thiscall externs).
//
// Field names are placeholders; only the OFFSETS + emitted code bytes are
// load-bearing.
#include <Dsndmgr/DirectSoundMgr.h>

// The 0x28-byte voice node (vtable @0x5ef6d0). Layout recovered from the ctor
// (0x136fe0) + these two methods: an intrusive list-anchor at +0x04, a live flag
// at +0x0c, the cloned buffer at +0x10, then the ramp params.
struct DSoundVoice {
    i32 Tick_137060(i32 now); // 0x137060  vtbl slot 0
    i32 Stop_1370d0();        // 0x1370d0  vtbl slot 1

    void* m_vtbl;              // +0x00
    char m_pad04[0x10 - 0x04]; // +0x04  intrusive anchor (+0x04) + live flag (+0x0c)
    DirectSoundMgr* m_10;      // +0x10  the cloned DirectSound buffer
    i32 m_14;                  // +0x14  stop+rewind-when-finished flag
    i32 m_18;                  // +0x18  ramp end volume index
    i32 m_1c;                  // +0x1c  ramp start volume index
    i32 m_20;                  // +0x20  ramp duration (ms)
    i32 m_24;                  // +0x24  ramp start timestamp
};

// ---------------------------------------------------------------------------
// 0x137060: DSoundVoice::Tick (vtbl slot 0, __thiscall, 1 arg = the current
// clock). Clamp the elapsed time to the ramp duration (flagging completion when
// it overruns); if the buffer stopped playing on its own, also flag done.
// Otherwise interpolate the ramp volume index across the elapsed fraction and
// push it through SetVolumeByIndex. On completion, when the stop flag is set,
// stop+rewind the buffer. Returns whether the voice is still live (!done).
// @early-stop
// 95.7% -- regalloc-pinning wall (docs/patterns/zero-register-pinning.md): every
// instruction matches (the unsigned clamp `jb`, the signed idiv interpolation, the
// three buffer calls, the done/!done epilogue). The only residual is the esi<->edi
// coin-flip: retail pins this->esi + elapsed->edi, MSVC5 here pins elapsed->esi +
// this->edi (same values, mirrored register file). SetVolumeByIndex is reloc-masked
// (CStatusBarItem2::SetField0 vs DirectSoundMgr::SetVolumeByIndex at 0x1355c0). Logic complete.
RVA(0x00137060, 0x6b)
i32 DSoundVoice::Tick_137060(i32 now) {
    i32 done = 0;
    i32 elapsed = now - m_24;
    if ((u32)elapsed >= (u32)m_20) {
        elapsed = m_20;
        done = 1;
    }
    if (m_10->IsPlaying() == 0) {
        done = 1;
    } else {
        i32 vol = (m_18 - m_1c) * elapsed / m_20 + m_1c;
        m_10->SetVolumeByIndex(vol);
    }
    if (done && m_14 != 0) {
        m_10->StopAndRewind();
    }
    return done == 0;
}

// ---------------------------------------------------------------------------
// 0x1370d0: DSoundVoice::Stop (vtbl slot 1, __thiscall, no args). If the buffer
// is still playing: when the stop flag is set, stop+rewind it; otherwise snap its
// volume to the ramp end. Always returns 1.
RVA(0x001370d0, 0x38)
i32 DSoundVoice::Stop_1370d0() {
    if (m_10->IsPlaying() != 0) {
        if (m_14 != 0) {
            m_10->StopAndRewind();
            return 1;
        }
        m_10->SetVolumeByIndex(m_18);
    }
    return 1;
}
