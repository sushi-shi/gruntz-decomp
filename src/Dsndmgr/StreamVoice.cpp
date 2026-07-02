// StreamVoice.cpp - the per-stream voice wrapper (Dsndmgr module,
// C:\Proj\Dsndmgr\DSndMgSR.CPP, retail vftable 0x5ef6d8 + feeder-override
// 0x5ef6e0). It is the 0xb0-byte DirectSoundMgr-derived buffer wrapper RezAlloc'd
// + constructed by SoundStream::CreateStreamBuffer (ctor 0x1375b0): +0x10 holds
// the owning SoundStream (m_owner), +0x6c is the embedded streaming feeder
// sub-object (whose vptr the voice overrides to 0x5ef6e0), and the standard
// DirectSoundMgr base fields (m_3c the avg-bytes divisor) come from below +0x6c.
//
// The trace conflated this with the StreamFeeder ctor at 0x1375b0 ("MallocCtor_
// 1375b0") and grouped four sibling methods under it; the distinct vtable
// (0x5ef6d8 voice / 0x5ef6e0 feeder-override) proves the voice is its own class.
// Field names are placeholders; offsets + emitted bytes are load-bearing.
#include <Dsndmgr/StreamVoice.h>
#include <Win32.h>
#include <rva.h>

// ALL-VTABLES phase: StreamVoice is now a REAL polymorphic class (declared-only
// slot-0 virtual, see StreamVoice.h). cl manages the vptr - it auto-stamps its own
// ??_7StreamVoice at ctor entry and auto-resets it at dtor entry (was the manual
// voice-vptr DIR32 stores). No VTBL is attached: retail's voice vtable 0x5ef6d8
// is a 1-slot scalar-deleting-dtor pointer packed INSIDE DSoundVoice's vtable storage
// (0x5ef6d0..0x5ef6dc), so cl cannot emit it as a standalone class vtable. The voice
// is also a FLAT class (m_60/m_64/m_68 overlap the DirectSoundMgr clone base padding)
// whose vptr retail stamps AFTER the manual BaseInit - a vptr-override-after-base that
// cl's implicit vptr-first store cannot reproduce (the ctor/dtor EH wall below).

// ---------------------------------------------------------------------------
// StreamVoice::StreamVoice (__thiscall, /GX EH frame). Construct the
// DirectSoundMgr base + the embedded feeder, restamp both vptrs to the voice's
// own tables, cache the two ctor args (m_64/m_60) and clear m_68.
// @early-stop
// EH-ctor wall (docs/patterns/eh-ctor-vptr-store-plateau.md): the base-ctor +
// feeder-ctor calls, both vptr restamps, and the field stores are byte-exact, but
// the /GX ctor-in-flight EH frame retail emits for the non-trivial base subobject
// is unreachable while the class is modeled non-polymorphically (manual vptr
// stamps, no real base/member dtors). Defer to the final sweep once the whole
// Dsndmgr class family is modeled.
RVA(0x001375b0, 0x77)
StreamVoice::StreamVoice(IDirectSoundBufferZ* buf, DirectSoundMgr* owner, i32 a, i32 b) {
    // cl auto-stamps ??_7StreamVoice at ctor entry (was a manual voice-vptr store).
    BaseInit(buf, owner);
    m_64 = a;
    // m_feeder (StreamVoiceFeeder) is cl-constructed (0x5ef6f0 then 0x5ef6e0) as a
    // member before this body - the manual feeder-override store is gone.
    m_60 = b;
    m_68 = 0;
}

// ---------------------------------------------------------------------------
// StreamVoice::SetSource (__thiscall, 1 arg). Ask the owning
// SoundStream (m_owner) to parse the RIFF/WAVE source into a scratch
// WAVEFORMATEX + data (off, len), then arm the embedded feeder's window over it.
RVA(0x001374c0, 0x5d)
i32 StreamVoice::SetSource(StreamSource* src) {
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
// StreamVoice::ComputeRatio (__thiscall, no args). m_feeder.m_windowLength * 1000 / m_3c
// (a position->time ratio; the *1000 is open-coded as *5*5*5*8).
RVA(0x00137590, 0x18)
u32 StreamVoice::ComputeRatio() {
    return m_feeder.m_windowLength * 1000 / m_3c;
}

// ---------------------------------------------------------------------------
// StreamVoice::~StreamVoice (__thiscall, /GX EH frame). Restamp the
// voice vptr, reset + tear down the embedded feeder, then run the DirectSoundMgr
// base destructor.
// @early-stop
// EH-dtor wall (docs/patterns/eh-dtor-needs-base-subobject.md): the vptr restamp,
// feeder FeederReset(0)/Cleanup, and ~base call are byte-exact, but the /GX EH
// frame retail emits for the non-trivial feeder member + base subobject is
// unreachable while the class is modeled non-polymorphically. Sibling of
// ~DirectSoundMgr (0x135bb0); defer to the final sweep.
RVA(0x00137650, 0x64)
StreamVoice::~StreamVoice() {
    // cl auto-resets the vptr to ??_7StreamVoice at dtor entry (was the manual store).
    m_feeder.FeederReset(0);
    m_feeder.Cleanup();
    BaseDtor();
}
