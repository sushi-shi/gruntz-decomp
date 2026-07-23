#ifndef GRUNTZ_GRUNTZ_LEAFCUE_H
#define GRUNTZ_GRUNTZ_LEAFCUE_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/Loadable.h> // CLoadable : CWapObj : CObject (m_04/m_08/m_0c + reset dtor)

class DSoundCloneInst; // the pooled cue player/buffer (Dsndmgr/DirectSoundMgr.h)
struct CParseSource;   // the parsed draw-source Configure reads (STRUCT key = the def)

struct LeafCue : public CLoadable {
    // [5] 0x158650: loaded iff the buffer slot is armed (was a Ghidra size-0
    // recovery gap; bytes `8b 51 10 33 c0 85 d2 0f 95 c0 c3`).
    RVA(0x00158650, 0xb)
    virtual i32 IsLoaded() OVERRIDE {
        return m_10 != 0;
    }
    // [7] 0x1587c0 (DDrawSubMgr.cpp): release the acquired buffer through the owner's
    // SoundDevice (reaps voices + releases + unlinks + deletes), clear the slot.
    virtual void Unload() OVERRIDE;
    // [1] ??1 @0x158680 (DDrawSubMgr.cpp): Unload (devirt direct 0x1587c0) + the
    // ~CLoadable field resets + grand-base re-stamp. cl-??_G @0x158660.
    virtual ~LeafCue() OVERRIDE;

    // Inline factory ctor (folded into CDDrawSubMgrLeafScan::CreateEntry*): seed the
    // CLoadable header (map count, 0, owner handle) + the zeroed tail, +0x18 first.
    LeafCue(i32 count, i32 handle);

    // The three buffer loaders (all cache into m_10; bodies in DDrawSubMgr.cpp).
    // The owner handle in CLoadable::m_0c is the CDDrawSurfaceMgr whose +0x20
    // m_soundStream is the SoundDevice they acquire through.
    i32 LoadSoundA(void* riff);       // 0x1586e0  SoundDevice::Acquire(riff)
    i32 LoadSoundB(void* src);        // 0x158720  SoundDevice::AcquireFile(path)
    i32 Configure(CParseSource* src); // 0x158760  BeginParse -> Acquire -> EndParse

    // The gated play entry (LeafCuePlay.cpp): when the throttle interval elapsed,
    // restamp the clock and forward the 4 args to the player's ConfigureItem.
    i32 PlayIfElapsed(i32 a0, i32 a1, i32 a2, i32 a3); // 0x1f940 (ret 0x10)

    DSoundCloneInst* m_10; // +0x10  the acquired/pooled DirectSound buffer (0 = unloaded)
    i32 m_14;              // +0x14  last draw-clock (throttle stamp)
    i32 m_18;              // +0x18  cooldown interval (seeded from the cache's m_34)
};
SIZE(0x1c);
inline LeafCue::LeafCue(i32 count, i32 handle) {
    m_id = count;
    m_flags = 0;
    m_ownerCtx = handle;
    m_10 = 0;
    m_18 = 0;
    m_14 = 0;
}

#endif // GRUNTZ_GRUNTZ_LEAFCUE_H
