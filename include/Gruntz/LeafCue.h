// LeafCue.h - LeafCue, THE 0x1c keyed sound-cue element of the CDDrawSubMgrLeafScan
// (CSndHost) cache: a CLoadable leaf wrapping one acquired DirectSound buffer
// (m_10, the pooled DSoundCloneInst) plus a throttle gate (m_14 last-play clock /
// m_18 cooldown interval). ONE class - the former per-TU readings of the same map
// value (LeafElementObj / LeafScanValue / LeafScanSoundArg / the LeafSumSource
// +0x2c reader / SoundCue.h's "folded CSndEmitter") are all THIS shape.
//
// Retail vtable ??_7 @0x1eff08 (9 slots) == CLoadable's scheme:
//   [0-4] CObject   [5] IsLoaded 0x158650 (`return m_10 != 0`)
//   [6] IsReady   (inherited CLoadable default @0x001c08 - not redeclared)
//   [7] Unload    0x1587c0 (release the acquired buffer; the ex "Release_1587c0")
//   [8] GetClassId (inherited CLoadable default @0x154a00 - not redeclared)
// The ??1 @0x158680 / cl-??_G @0x158660 and the loader bodies live in
// src/DDrawMgr/DDrawSubMgr.cpp (the G obj).
//
// Field names are placeholders; only OFFSETS + emitted code bytes are load-bearing.
#ifndef GRUNTZ_GRUNTZ_LEAFCUE_H
#define GRUNTZ_GRUNTZ_LEAFCUE_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/Loadable.h> // CLoadable : CWapObj : CObject (m_04/m_08/m_0c + reset dtor)

class DSoundCloneInst; // the pooled cue player/buffer (Dsndmgr/DirectSoundMgr.h)
struct CParseSource;   // the parsed draw-source Configure_158760 reads (STRUCT key = the def)

struct LeafCue : public CLoadable {
    // [5] 0x158650: loaded iff the buffer slot is armed (was a Ghidra size-0
    // recovery gap; bytes `8b 51 10 33 c0 85 d2 0f 95 c0 c3`).
    RVA(0x00158650, 0xb)
    virtual i32 IsLoaded() OVERRIDE {
        return m_10 != 0;
    }
    // [7] 0x1587c0 (DDrawSubMgr.cpp): release the acquired buffer through the owner's
    // SoundDevice (reaps voices + releases + unlinks + deletes), clear the slot.
    virtual i32 Unload() OVERRIDE;
    // [1] ??1 @0x158680 (DDrawSubMgr.cpp): Unload (devirt direct 0x1587c0) + the
    // ~CLoadable field resets + grand-base re-stamp. cl-??_G @0x158660.
    virtual ~LeafCue() OVERRIDE;

    // Inline factory ctor (folded into CDDrawSubMgrLeafScan::CreateEntry*): seed the
    // CLoadable header (map count, 0, owner handle) + the zeroed tail, +0x18 first.
    LeafCue(i32 count, i32 handle);

    // The three buffer loaders (all cache into m_10; bodies in DDrawSubMgr.cpp).
    // The owner handle in CLoadable::m_0c is the CDDrawSurfaceMgr whose +0x20
    // m_soundStream is the SoundDevice they acquire through.
    i32 LoadSoundA(void* riff);              // 0x1586e0  SoundDevice::Acquire(riff)
    i32 LoadSoundB(void* src);               // 0x158720  SoundDevice::AcquireFile(path)
    i32 Configure_158760(CParseSource* src); // 0x158760  BeginParse -> Acquire -> EndParse

    // The gated play entry (LeafCuePlay.cpp): when the throttle interval elapsed,
    // restamp the clock and forward the 4 args to the player's ConfigureItem.
    i32 PlayIfElapsed(i32 a0, i32 a1, i32 a2, i32 a3); // 0x1f940 (ret 0x10)

    DSoundCloneInst* m_10; // +0x10  the acquired/pooled DirectSound buffer (0 = unloaded)
    i32 m_14;              // +0x14  last draw-clock (throttle stamp)
    i32 m_18;              // +0x18  cooldown interval (seeded from the cache's m_34)
};
// Seed order mirrors the factory's writes: count, 0, handle (header), then the
// zeroed tail with +0x18 before +0x14. The vptr is cl-auto-stamped (ctor prologue).
inline LeafCue::LeafCue(i32 count, i32 handle) {
    m_04 = count;
    m_flags = 0;
    m_0c = handle;
    m_10 = 0;
    m_18 = 0;
    m_14 = 0;
}

SIZE(LeafCue, 0x1c);
VTBL(LeafCue, 0x001eff08); // ??_7LeafCue (9-slot CLoadable leaf; was g_leafElemVtbl)

#endif // GRUNTZ_GRUNTZ_LEAFCUE_H
