// DSoundVoice.h - the 0x28-byte "playing voice" node of the Dsndmgr module
// (C:\Proj\Dsndmgr\, retail vftable 0x5ef6d0). DirectSoundMgr::CloneAndPlay
// (0x135660) new's one of these per requested play and threads its intrusive
// anchor (m_link @ +0x04) into the owning device's voice list; the voice ctor
// (0x136fe0) stamps the vtable + the play params.
//
// A voice drives a single cloned DirectSound buffer (m_buffer, a DirectSoundMgr)
// over a volume ramp: from m_rampStartVolume -> m_rampEndVolume across
// m_rampDurationMs ms starting at m_rampStartTime, with m_stopAndRewind a
// "stop+rewind when finished" flag. The modeled methods are vtable slots 0
// (Tick, 0x137060) and 1 (Stop, 0x1370d0); both drive the buffer's IsPlaying /
// SetVolumeByIndex / StopAndRewind wrappers.
#ifndef DSNDMGR_DSOUNDVOICE_H
#define DSNDMGR_DSOUNDVOICE_H

#include <rva.h>

#include <Dsndmgr/SoundVoiceList.h> // shared DSoundLink intrusive anchor

class DirectSoundMgr; // the cloned DirectSound buffer this voice drives

// The 0x28-byte voice node (vtable @0x5ef6d0). A PureSoundElem-derived element:
// slots 0/1 override the pure Tick/Stop, slot 2 is a new virtual. Layout from the
// ctor (0x136fe0) + the Tick/Stop methods: an intrusive list-anchor at +0x04, a
// live flag at +0x0c, the cloned buffer at +0x10, then the volume-ramp params.
struct DSoundVoice : public PureSoundElem {
    // Slots 0/1 (Tick/Stop) are defined in DSoundVoice.cpp overriding the pure base,
    // so cl emits ??_7DSoundVoice@@6B@ (0x5ef6d0) referencing them + the external
    // slot 2.
    virtual i32 Tick(i32 now) OVERRIDE; // vtbl slot 0  0x137060
    virtual i32 Stop() OVERRIDE;        // vtbl slot 1  0x1370d0
    virtual ~DSoundVoice(); // vtbl slot 2  (scalar-deleting dtor; defined in the ctor's TU)

    DSoundLink m_link;        // +0x04  intrusive list-anchor {next@+4, prev@+8}
    i32 m_live;               // +0x0c  live flag (set by the ctor)
    DirectSoundMgr* m_buffer; // +0x10  the cloned DirectSound buffer
    i32 m_stopAndRewind;      // +0x14  stop+rewind-when-finished flag
    i32 m_rampEndVolume;      // +0x18
    i32 m_rampStartVolume;    // +0x1c
    i32 m_rampDurationMs;     // +0x20
    i32 m_rampStartTime;      // +0x24

    // 6-arg __thiscall ctor 0x136fe0 (external): stamps the vtable + the play
    // params. A placement/heap new in CloneAndPlay lowers to `call 0x136fe0`.
    DSoundVoice(i32 key, i32 pct, i32 mode, DirectSoundMgr* owner, i32 slot, i32 stamp);
};
SIZE(DSoundVoice, 0x28);       // measured: new(0x28) voice node (ctor 0x136fe0)
VTBL(DSoundVoice, 0x001ef6d0); // cl-emitted ??_7DSoundVoice@@6B@ (Tick/Stop/+137630)

#endif // DSNDMGR_DSOUNDVOICE_H
