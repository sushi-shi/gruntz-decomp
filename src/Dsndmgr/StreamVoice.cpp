// StreamVoice.cpp - the per-stream voice wrapper (Dsndmgr module,
// C:\Proj\Dsndmgr\DSndMgSR.CPP, retail vftable 0x5ef6d8 + feeder-override
// 0x5ef6e0). It is the 0xb0-byte DirectSoundMgr-derived buffer wrapper (operator
// new(0xb0)) constructed by SoundStream::CreateStreamBuffer (ctor 0x1375b0): +0x10
// holds the owning SoundStream (m_owner), +0x6c is the embedded streaming feeder
// sub-object (whose vptr the voice overrides to 0x5ef6e0), and the standard
// DirectSoundMgr base fields (m_sampleRate, stream: avg-bytes-per-sec) come from below +0x6c.
#include <Dsndmgr/SoundStream.h> // the owning SoundStream (m_owner): ParseWave + m_initialized
#include <Dsndmgr/StreamVoice.h>
#include <Win32.h>
#include <Win32.h>    // windows.h base types (dsound.h needs them)
#include <mmsystem.h> // WAVEFORMATEX (dsound.h needs it predefined)
#include <dsound.h>   // real DirectSound SDK (IDirectSound/Buffer, DSBUFFERDESC, DSBCAPS)
#include <rva.h>

// StreamVoice is a REAL polymorphic class (its lone virtual is the destructor, see
// StreamVoice.h). cl manages the vptr - it auto-stamps ??_7StreamVoice at ctor
// entry and auto-resets it at dtor entry (was the manual voice-vptr DIR32 stores).

// ---------------------------------------------------------------------------
// StreamVoice::StreamVoice (__thiscall, /GX EH frame). Run the DirectSoundMgr
// base init + construct the embedded feeder, cache the two ctor args (a->+0x60,
// b->+0x64) and clear the position (+0x68).
// @early-stop
// EH-ctor wall (docs/patterns/eh-ctor-vptr-store-plateau.md): the base-ctor +
// feeder-ctor calls, both vptr restamps, and the field stores are byte-exact, but
// the /GX ctor-in-flight EH frame retail emits for the non-trivial base subobject
// is unreachable while the flat class can only model the base run as reloc-masked
// method calls (no real base/member dtors). Defer to the final sweep once the whole
// Dsndmgr class family is modeled.
RVA(0x001375b0, 0x77)
StreamVoice::StreamVoice(IDirectSoundBuffer* buf, SoundStream* owner, i32 a, i32 b) {
    // cl auto-stamps ??_7StreamVoice at ctor entry (was a manual voice-vptr store).
    BaseInit(buf, owner);
    m_retireWhenIdle = b;
    // m_feeder (StreamVoiceFeeder) is cl-constructed (0x5ef6f0 then 0x5ef6e0) as a
    // member before this body - the manual feeder-override store is gone.
    m_stopWhenIdle = a;
    m_active = 0;
}

// ---------------------------------------------------------------------------
// StreamVoice::SetSource (__thiscall, 1 arg). Ask the owning
// SoundStream (m_owner) to parse the RIFF/WAVE source into a scratch
// WAVEFORMATEX + data (off, len), then arm the embedded feeder's window over it.
RVA(0x001374c0, 0x5d)
i32 StreamVoice::SetSource(CParseSource* src) {
    if (src == 0) {
        return 0;
    }
    WaveFormatX wf;
    u32 dataOff;
    u32 dataLen;
    if (m_owner->ParseWave(src, &wf, &dataOff, &dataLen) == 0) {
        return 0;
    }
    m_feeder.SeedWindow(src, dataOff, dataLen);
    return 1;
}

// ---------------------------------------------------------------------------
// StreamVoice::Configure (__thiscall, 4 args). Apply the cached
// volume/pan/frequency indices through the DirectSoundMgr base setters, stash the
// loop flag in m_feeder.m_loop, then resume the embedded feeder - ANDing every step's
// success into the returned flag.
RVA(0x00137520, 0x6e)
i32 StreamVoice::Configure(i32 vol, i32 pan, i32 freq, i32 loop) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 ok = 1;
    if (SetVolumeByIndex(vol) == 0) {
        ok = 0;
    }
    if (SetPanByIndex(pan) == 0) {
        ok = 0;
    }
    if (SetFreqByIndex(freq) == 0) {
        ok = 0;
    }
    m_feeder.m_loop = loop;
    if (m_feeder.Resume() == 0) {
        ok = 0;
    }
    return ok;
}

// ---------------------------------------------------------------------------
// StreamVoice::ComputeRatio (__thiscall, no args).
// m_feeder.m_windowLength * 1000 / m_sampleRate (a position->time ratio; the
// *1000 is open-coded as *5*5*5*8).
RVA(0x00137590, 0x18)
u32 StreamVoice::ComputeRatio() {
    return m_feeder.m_windowLength * 1000 / m_sampleRate;
}

// ---------------------------------------------------------------------------
// StreamVoice::~StreamVoice (__thiscall, /GX EH frame). Reset + tear down the
// embedded feeder, then run the DirectSoundMgr base destructor; cl auto-resets the
// vptr to ??_7StreamVoice at entry.
// @early-stop
// EH-dtor wall (docs/patterns/eh-dtor-needs-base-subobject.md): the vptr reset,
// feeder FeederReset(0)/Cleanup, and ~base call are byte-exact, but the /GX EH
// frame retail emits for the non-trivial feeder member + base subobject is
// unreachable while the flat class models the base dtor as a reloc-masked call.
// Sibling of ~DirectSoundMgr (0x135bb0); defer to the final sweep.
RVA(0x00137650, 0x64)
StreamVoice::~StreamVoice() {
    // cl auto-resets the vptr to ??_7StreamVoice at dtor entry (was the manual store).
    m_feeder.FeederReset(0);
    m_feeder.Cleanup();
    BaseDtor();
}
