// SoundCue.h - the positional-sound cue subsystem (C:\Proj\Dsndmgr). A named-cue
// registry (CSndFinder::Lookup @0x1b8438) embedded in a host (CSndHost) hung off a
// sub-manager (CSndSubMgr); each looked-up cue (LeafCue) carries a per-emitter
// cooldown gate and drives a DSoundCloneInst (ConfigureItem @0x1360d0, SoundCueMgr.h)
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
// The +0x10 cue registry IS the MFC CMapStringToPtr (Lookup @0x1b8438 is the NAFXCW
// library routine itself), so this header needs the real MFC collections. <Mfc.h> is a
// superset of <Win32.h> (afx.h first, then the same windows.h), so the handful of
// Win32-umbrella TUs that reach this header transitively just switch umbrella - the
// documented C1189 'wall' here was only 2 TUs deep, not the ~60 the old comment feared.
#include <Mfc.h>
#include <Dsndmgr/DirectSoundMgr.h> // DSoundCloneInst (the play-object; ConfigureItem @0x1360d0)

struct CSprite; // the frame-data value the +0x10 map ALSO yields (Sprite.h); the +0x28
                // registry's CMapStringToPtr stores both cue emitters and sprites by key.
                // Fwd-declared (not #included) to keep this light header (pulled into the
                // ~60-TU GameRegistry.h) free of the sprite graph.

// A looked-up cue: the canonical LeafCue (<Gruntz/LeafCue.h>; ex CSndEmitter /
// StatusBarCueHolder.h CueObj): +0x10 the DSoundCloneInst that plays it, +0x14
// last-play clock, +0x18 cooldown interval (an unsigned `(now-m_14) >= m_18` gate).
#include <Gruntz/LeafCue.h>

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
#include <Dsndmgr/SoundStream.h>          // the REAL SoundStream (: SoundDevice)
#include <DDrawMgr/DDrawSubMgrLeafScan.h> // the canonical class behind CSndHost (see below)

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

// SETTLED (Fable lane, 2026-07-13): CSndHost IS the canonical CDDrawSubMgrLeafScan
// (<DDrawMgr/DDrawSubMgrLeafScan.h>) - PROVEN by shared method RVAs (both classes
// declared Lookup @0x05b7e0 and 0x1580b0 - the latter's reconstructed body is
// SumField_1580b0, a map-count query, so the old "RegisterKey" reading was wrong),
// the same CMapStringToPtr @+0x10, the same +0x2c sound stream and the same +0x30
// emit/busy gate. The class IS polymorphic (LeafScanBase : CObject; the old
// "non-polymorphic so RTTI gives no names" note was wrong - it simply has no RTTI).
// This one class is: the +0x28 world sound/cue registry, the status-bar cue holder,
// AND CDDrawSurfaceMgr's m_leafScan child (new(0x38) @Init, vtbl 0x5efca0).
typedef CDDrawSubMgrLeafScan CSndHost;

// (The ex-`CSndSubMgr` facet is DISSOLVED (Fable lane, 2026-07-13): the "+0x28 cue host"
// object is the world holder CSpriteFactoryHolder itself (<Gruntz/GameRegistry.h>), whose
// m_28 is already the CSndHost typedef'd above. It was never a separate class.)

#endif // GRUNTZ_SOUNDCUE_H
