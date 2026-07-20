#ifndef DSNDMGR_DSOUNDVOICE_H
#define DSNDMGR_DSOUNDVOICE_H

#include <rva.h>

#include <Dsndmgr/SoundVoiceList.h> // shared DSoundLink intrusive anchor

class DirectSoundMgr; // the cloned DirectSound buffer this voice drives

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
