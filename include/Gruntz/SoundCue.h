// SoundCue.h - the positional-sound cue subsystem (C:\Proj\Dsndmgr). A named-cue
// registry (CSndFinder::Lookup @0x1b8438) embedded in a host (CSndHost) hung off a
// sub-manager (CSndSubMgr); each looked-up cue (LeafCue) carries a per-emitter
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

struct LeafCue; // folded CSndEmitter

#include <Ints.h>
#include <rva.h>
#include <Gruntz/SoundCueMgr.h> // CSoundCueMgr (the play-object; ConfigureItem @0x1360d0)

struct CSprite; // the frame-data value the +0x10 map ALSO yields (Sprite.h); the +0x28
                // registry's CMapStringToOb stores both cue emitters and sprites by key.
                // Fwd-declared (not #included) to keep this light header (pulled into the
                // ~60-TU GameRegistry.h) free of the sprite graph.

// A looked-up cue: +0x10 the CSoundCueMgr that plays it, +0x14 last-play clock,
// +0x18 cooldown interval (an unsigned `(now - m_14) >= m_18` gate). This IS the
// former StatusBarCueHolder.h CueObj (identical value layout + role) - folded here.
SIZE_UNKNOWN(LeafCue);

// The DirectSound stream hung off CSndHost+0x2c; Stop() halts it. Its base
// SoundDevice sub-object supplies PurgeVoiceList (0x136e20, the per-tick voice
// reaper the level-preview audio-kill path calls). Both reloc-masked __thiscall.
struct StreamVoice; // fwd (DestroyVoice/OpenStream arg+ret)
class CParseSource;
struct SoundStream {
    void Stop();                       // 0x137a80 ?Stop@SoundStream@@QAEXXZ (__thiscall)
    void PurgeVoiceList(i32 time);     // 0x136e20 (SoundDevice base method; voice reap)
    void DestroyVoice(StreamVoice* v); // 0x1379d0
    StreamVoice* OpenStream(CParseSource* p, i32 a, i32 b, i32 c, i32 d, i32 e); // 0x137900
};
SIZE_UNKNOWN(SoundStream);

// The named-cue registry embedded at CSndHost+0x10 (the engine CMapStringToOb, Lookup
// @0x1b8438); Lookup fills an out-param. The map stores CObject-derived values, so the
// out-ptr overloads type the found value cast-free per key: a cue emitter (the sound
// cues) or a frame-data sprite (the status-bar HUD cues, formerly CStatusBarHolder's
// CSpriteHashTable view). Same reloc-masked call either way (`add ecx,0x10`).
struct CSndFinder {
    void Lookup(const char* name, LeafCue** out); // 0x1b8438 (__thiscall)
    void Lookup(const char* name, CSprite** out); // 0x1b8438 (sprite-value overload)
};
SIZE_UNKNOWN(CSndFinder);

// Holds the finder (+0x10 embedded), a DirectSound stream (+0x2c) and an emit gate
// (+0x30, must be 0 to emit). This IS the +0x28 status-bar cue holder every HUD/
// preview/finish-level driver reaches through the world holder (== the former
// StatusBarCueHolder.h CStatusBarHolder, folded here): the name->object map (+0x10),
// the audio-kill sound stream (+0x2c) and the live-surface/emit gate (+0x30).
struct CSndHost {
    char m_pad00[0x10];
    CSndFinder m_10;           // +0x10  embedded finder (name->cue/sprite CMapStringToOb)
    char m_pad11[0x2c - 0x11]; // -> +0x2c
    SoundStream* m_2c;         // +0x2c  DirectSound stream (Stop / audio-kill PurgeVoiceList)
    i32 m_emitGate;            // +0x30  live-surface / emit gate (must be 0 to emit)
    // Resolve a cue name to its emitter via the +0x10 finder (@0x05b7e0, thunk
    // 0x2cca): a real CSndHost __thiscall - `push ecx` out-slot, `add ecx,0x10`,
    // tail into CSndFinder's map Lookup (0x1b8438), return the emitter (0 on
    // miss). Reloc-masked (no body). Was mis-modeled as an extern "C" __stdcall
    // free fn (the bytes matched only because ecx was coincidentally the host at
    // every call site).
    // Register a level asset-namespace key into the finder (@0x1580b0; reloc-masked;
    // called by CGruntzMgr's level-asset-key register step with a null / prefix key).
    i32 RegisterKey(const char* key); // 0x1580b0
};
SIZE_UNKNOWN(CSndHost);

// The cue sub-manager: +0x28 the host.
struct CSndSubMgr {
    char m_pad00[0x28];
    CSndHost* m_28; // +0x28
};
SIZE_UNKNOWN(CSndSubMgr);

#endif // GRUNTZ_SOUNDCUE_H
