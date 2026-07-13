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
// The +0x10 cue registry IS the MFC CMapStringToPtr (Lookup @0x1b8438 is the NAFXCW
// library routine itself), so this header needs the real MFC collections. <Mfc.h> is a
// superset of <Win32.h> (afx.h first, then the same windows.h), so the handful of
// Win32-umbrella TUs that reach this header transitively just switch umbrella - the
// documented C1189 'wall' here was only 2 TUs deep, not the ~60 the old comment feared.
#include <Mfc.h>
#include <Gruntz/SoundCueMgr.h> // CSoundCueMgr (the play-object; ConfigureItem @0x1360d0)

struct CSprite; // the frame-data value the +0x10 map ALSO yields (Sprite.h); the +0x28
                // registry's CMapStringToPtr stores both cue emitters and sprites by key.
                // Fwd-declared (not #included) to keep this light header (pulled into the
                // ~60-TU GameRegistry.h) free of the sprite graph.

// A looked-up cue: +0x10 the CSoundCueMgr that plays it, +0x14 last-play clock,
// +0x18 cooldown interval (an unsigned `(now - m_14) >= m_18` gate). This IS the
// former StatusBarCueHolder.h CueObj (identical value layout + role) - folded here.
SIZE_UNKNOWN(LeafCue);

// The DirectSound stream hung off CSndHost+0x2c is a `SoundStream : SoundDevice`;
// Stop() halts it, and PurgeVoiceList (0x136e20) is its SoundDevice base method - the
// per-tick voice reaper the audio-kill paths call. The real SoundDevice base is pulled
// here (include-guarded, so it dedupes with the TUs that already have it via
// WorldSoundSet.h / a real Dsndmgr header), letting PurgeVoiceList carry the retail
// SoundDevice symbol so every caller reaches it cast-free. SoundDevice.h is afx-light
// (it only forward-declares the DirectSound COM). Folded away the per-TU SoundDevice /
// DirectSoundMgr facet views. SoundStream's own Stop/DestroyVoice/OpenStream are declared
// against forward-declared StreamVoice/CParseSource so this stays a light method facet.
// FOLDED: the reduced 3-method `SoundStream : SoundDevice` facet that used to be
// DEFINED here was an ODR duplicate of the REAL class in <Dsndmgr/SoundStream.h> (same
// base, same Stop/DestroyVoice/OpenStream rvas). Pull the real one - it also carries
// TickSubManagers (0x137ac0), the per-frame tick CPlay/CMulti drive on this member.
#include <Dsndmgr/SoundStream.h> // the REAL SoundStream (: SoundDevice)

// The named-cue registry embedded at CSndHost+0x10 (the engine ::CMapStringToPtr, Lookup
// @0x1b8438); Lookup fills an out-param.
//
// IT IS CMapStringToPtr, NOT CMapStringToOb (mfc_class, 2026-07-12).  0x1b8438 lies in
// [0x1b8247, 0x1b85b1) - the .obj whose head ctor 0x1b8247 DIR32s ??_7CMapStringToPtr@@6B@
// (0x1eb014), i.e. the class stamps its own vtable and so names itself.  CMapStringToOb's
// .obj is [0x1b7e17, 0x1b8247) and ITS Lookup is 0x1b8008 - a DIFFERENT body.  There is no
// COMDAT fold: MSVC5 has no /OPT:ICF.  The two classes are merely code-identical, which is
// why every FID row in those bands is AMBIG and the tree had guessed wrong.  The GetAssocAt
// this Lookup tail-calls, 0x1b83de, is in the same CMapStringToPtr band - corroboration.
//     python -m gruntz.analysis.mfc_class 0x1b8438
//
// The value slot is therefore void*, and the (LeafCue*)/(CSprite*) casts at the use sites
// are the devs' own - CMapStringToPtr IS a void* container.
// (The ex-`CSndFinder` view is DISSOLVED: its `Lookup` was a fake alias of the MFC library
// ?Lookup@CMapStringToPtr@@QBEHPBDAAPAX@Z at 0x1b8438 - disasm-proven, it tail-calls
// CMapStringToPtr::GetAssocAt@0x1b83de and reuses the `key` arg slot as the out-nHash
// local - so every call through the view bound to nothing. The member below is the real
// ::CMapStringToPtr, whose 0x1c bytes exactly fill +0x10..+0x2c; callers now get the
// library symbol and link.)

// Holds the finder (+0x10 embedded), a DirectSound stream (+0x2c) and an emit gate
// (+0x30, must be 0 to emit). This IS the +0x28 status-bar cue holder every HUD/
// preview/finish-level driver reaches through the world holder (== the former
// StatusBarCueHolder.h CStatusBarHolder, folded here): the name->object map (+0x10),
// the audio-kill sound stream (+0x2c) and the live-surface/emit gate (+0x30).
struct CSndHost {
    char m_pad00[0x10];
    CMapStringToPtr m_10; // +0x10  the real name->cue/sprite map (0x1c bytes -> +0x2c)
    SoundStream* m_2c;    // +0x2c  DirectSound stream (Stop / audio-kill PurgeVoiceList)
    i32 m_emitGate;       // +0x30  live-surface / emit gate (must be 0 to emit)
    // Resolve a cue name to its emitter via the +0x10 finder (@0x05b7e0, thunk
    // 0x2cca): a real CSndHost __thiscall - `push ecx` out-slot, `add ecx,0x10`,
    // tail into the +0x10 map's Lookup (0x1b8438), return the emitter (0 on
    // miss). Reloc-masked (no body). Was mis-modeled as an extern "C" __stdcall
    // free fn (the bytes matched only because ecx was coincidentally the host at
    // every call site).
    // Register a level asset-namespace key into the finder (@0x1580b0; reloc-masked;
    // called by CGruntzMgr's level-asset-key register step with a null / prefix key).
    i32 RegisterKey(const char* key); // 0x1580b0
};
SIZE_UNKNOWN(CSndHost);

// (The ex-`CSndSubMgr` facet is DISSOLVED (2026-07-13, Fable lane): the "+0x28
// cue host" object is the world holder CSpriteFactoryHolder itself
// (<Gruntz/GameRegistry.h>), whose m_28 is already the CSndHost declared above.)

#endif // GRUNTZ_SOUNDCUE_H
