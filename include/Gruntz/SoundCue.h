// SoundCue.h - the positional-sound cue subsystem (C:\Proj\Dsndmgr). A named-cue
// registry (CSndFinder::Lookup @0x1b8438) embedded in a host (CSndHost) hung off a
// sub-manager (CSndSubMgr); each looked-up cue (CSndEmitter) carries a per-emitter
// cooldown gate and drives a CSoundCueMgr (ConfigureItem @0x1360d0, SoundCueMgr.h)
// to actually play the sound.
//
// Shared verbatim by the multiplayer menu-select handler (Net/NetMgrMenuSelect.cpp,
// reached via CNetMgr+0xc) and the lightning-strike hazard (Gruntz/PathHazard.cpp,
// reached via the game-registry's +0x30 world holder) - identical layouts + RVAs in
// both (xref-confirmed: Lookup 0x1b8438, ConfigureItem 0x1360d0). Only offsets + code
// bytes are load-bearing; field names are placeholders (the finder/emitter/host/
// sub-mgr classes are non-polymorphic, so RTTI gives no retail names).
#ifndef GRUNTZ_SOUNDCUE_H
#define GRUNTZ_SOUNDCUE_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/SoundCueMgr.h> // CSoundCueMgr (the play-object; ConfigureItem @0x1360d0)

// A looked-up cue: +0x10 the CSoundCueMgr that plays it, +0x14 last-play clock,
// +0x18 cooldown interval (an unsigned `(now - m_14) >= m_18` gate).
struct CSndEmitter {
    char m_pad00[0x10];
    CSoundCueMgr* m_10; // +0x10  the play-object
    u32 m_14;           // +0x14  last-play clock
    u32 m_18;           // +0x18  cooldown interval
};
SIZE_UNKNOWN(CSndEmitter);

// The named-cue registry embedded at CSndHost+0x10; Lookup fills an out-param.
struct CSndFinder {
    void Lookup(const char* name, CSndEmitter** out); // 0x1b8438 (__thiscall)
};
SIZE_UNKNOWN(CSndFinder);

// Holds the finder (+0x10 embedded) and an emit gate (+0x30, must be 0 to emit).
struct CSndHost {
    char m_pad00[0x10];
    CSndFinder m_10;           // +0x10  embedded finder
    char m_pad11[0x30 - 0x11]; // -> +0x30
    i32 m_30;                  // +0x30  gate (must be 0 to emit)
};
SIZE_UNKNOWN(CSndHost);

// The cue sub-manager: +0x28 the host.
struct CSndSubMgr {
    char m_pad00[0x28];
    CSndHost* m_28; // +0x28
};
SIZE_UNKNOWN(CSndSubMgr);

#endif // GRUNTZ_SOUNDCUE_H
