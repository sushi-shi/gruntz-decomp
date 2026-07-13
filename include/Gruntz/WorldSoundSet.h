// WorldSoundSet.h - a trace-discovered Gruntz ambient-sound game object
// (C:\Proj\Gruntz; the this-tracer mislabeled it CRandomAmbientSound - that RTTI
// name belongs to the polymorphic class in CRandomAmbientSound.h). It owns a
// world/level back-pointer (+0x00), a second back-pointer seeded at init (+0x04),
// an embedded MFC list of active sound channels (+0x08, head at +0x0c), an
// "active" flag (+0x24), and a pending pan/volume pair (+0x28/+0x2c).
//
// None of the matched methods touch a vptr on `this` (offset 0 is the world
// pointer, not a vftable), so the class is modeled non-polymorphic - the embedded
// CObject-derived bases / engine vtable live in another TU and are never used
// here. Field names are placeholders (m_<hexoffset>); only the OFFSETS + emitted
// code bytes are load-bearing (campaign doctrine).
#ifndef GRUNTZ_CWORLDSOUNDSET_H
#define GRUNTZ_CWORLDSOUNDSET_H

#include <Mfc.h>
#include <rva.h>

// The world's +0x2c sound sub-object is a SoundDevice: both FreeSamples (0x136ed0)
// and PurgeVoiceList (0x136e20) are its methods (the earlier "dual SoundDevice /
// DirectSoundMgr view" was a mis-model - the one handle is a SoundDevice). Each
// channel's +0x04 voice is a DirectSoundMgr (StopAndRewind 0x135380 /
// SetVolumeByIndex 0x1355c0). Both come from the canonical Dsndmgr headers so the
// reloc-masked __thiscall calls pair by symbol.
#include <Dsndmgr/SoundDevice.h> // real SoundDevice (also pulls DirectSoundMgr)

// DISSOLVED (Fable A2, 2026-07-14): the former "CSoundChannel" shell WAS the
// canonical CAmbientSound (<Gruntz/AmbientSound.h>) - the common base of every
// channel the Create* factories append to m_list (CAmbientSound / CAmbientPosSound
// / CRandomAmbientSound, all `: CAmbientSound`). Slot-for-slot: its dtor = the
// family slot-0 scalar dtor; "Slot1/Slot2" = the inherited CUserBase
// SerializeMove/GetTypeTag; "Retune(a1,a2,a3)" = the slot-3 Update(x,y,force)
// (the args are LISTENER COORDS - Play/Multi push the main plane's scroll origin);
// "Recompute" (0xbf10) is now CAmbientSound::Recompute (its m_level/m_scaleA/
// m_scaleB are the same +0x08/+0x0c/+0x10 fields SetLevel scales through).
class CAmbientSound;

// The embedded list of channels (+0x08) is the REAL MFC CPtrList (vtable
// ??_7CObList@@6B@ @0x1ed4b4; AddTail 0x1b4991 / RemoveAll 0x1b48a6 / ~CPtrList
// 0x1b48c6 all reloc-mask). The raw node walks read its m_pNodeHead directly via
// GetHeadPosition() (byte-identical to the old m_head field read). Its CNode is
// modeled here as CSoundNode (next at +0x00, prev +0x04, channel payload +0x08).
struct CSoundNode {
    CSoundNode* m_next;    // +0x00
    CSoundNode* m_prev;    // +0x04
    CAmbientSound* m_data; // +0x08  the channel (family base; see the note above)
};

// The sound-channel objects the Create* factories allocate + add to the list are
// the REAL RTTI channel classes (<Gruntz/AmbientSound.h> / RandomAmbientSound.h);
// forward-declared here so the factory signatures carry the real types without
// fattening this header's includers. (The old SoundChannelNew shell view of the
// same family is dissolved.)
class CAmbientPosSound;
class CRandomAmbientSound;

// The world/level object the sound hangs off. Only its +0x2c slot is read here
// (the SoundDevice sub-object), then poked via FreeSamples / PurgeVoiceList.
struct CRandomAmbientWorld {
    char m_pad00[0x2c];
    SoundDevice* m_soundDev; // +0x2c  the world's SoundDevice sub-object
};

class CWorldSoundSet {
public:
    i32 Init(void* world, i32 a2); // 0x00b5e0
    void Teardown();               // 0x00b660
    void Restart(void* a1);        // 0x00bc30
    void Stop();                   // 0x00bc80
    void Resume();                 // 0x00bcf0
    void Retune(i32 x, i32 y); // 0x00bd60  push the listener position to every channel
    void Deactivate();             // 0x00b620
    ~CWorldSoundSet();             // 0x085ed0

    // Factories: allocate + seed a sound channel (the real RTTI channel classes),
    // run its one-time Init, and (on success) append it to m_list. The `this` is
    // this CWorldSoundSet owner (the rtti-vptr heuristic once mislabeled them onto
    // the channel classes whose vtables their inlined ctors stamp).
    CAmbientSound* CreateAmbient6_b6a0(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4);
    CAmbientSound* CreateAmbient5_b7b0(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4);
    CAmbientPosSound* CreatePos6_b850(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4);
    CAmbientPosSound* CreatePos5_b960(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4);
    CRandomAmbientSound*
    CreateRandom_bb60(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7, i32 a8);
    // 0xba00: CRandomAmbientSound with a validated (x,y) bounding box; ::operator new.
    CRandomAmbientSound*
    CreateRandomBox_ba00(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7, i32 a8);

    CRandomAmbientWorld* m_world; // +0x00
    void* m_04;                   // +0x04
    CPtrList m_list;              // +0x08  MFC CPtrList (head at +0x0c)
    i32 m_active;                 // +0x24  active flag
    // +0x28/+0x2c: the pending LISTENER position (not pan/vol - Play/Multi push the
    // main plane's scroll origin here via Retune; Resume replays it into each
    // channel's Update(x,y,force) slot).
    i32 m_listenerX; // +0x28
    i32 m_listenerY; // +0x2c
};

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

#endif // GRUNTZ_CWORLDSOUNDSET_H
