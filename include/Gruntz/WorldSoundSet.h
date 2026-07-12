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

// One active sound channel hanging off a list node. Polymorphic: the teardown /
// retune paths dispatch through its vtable (scalar-deleting dtor at slot 0, a
// 3-arg retune at slot 3). The class + its vtable live in another TU, so it is
// modeled as a small typed shell whose virtual calls reloc-mask by slot.
struct CSoundChannel {
    virtual ~CSoundChannel(); // slot 1 (deleting dtor -> cl-emitted ??_G)
    virtual void Slot1();
    virtual void Slot2();
    virtual void Retune(i32 a1, i32 a2, i32 a3); // slot 3 -> call [vptr+0xc]

    void Recompute(i32 frame); // 0x00bf10  level-scale -> SetVolByIdx on m_voice

    DirectSoundMgr* m_voice; // +0x04  DirectSound handle (StopAndRewind / SetVolByIdx)
    i32 m_level;             // +0x08  level multiplier (scaled by the frame in Recompute)
    i32 m_lastFrame;         // +0x0c  last frame fed to Recompute (early-out compare)
    i32 m_multiplier;        // +0x10  secondary multiplier (>0 gate, percent)
    i32 m_14;                // +0x14  cleared on stop/retune
};

// The embedded list of channels (+0x08) is the REAL MFC CObList (vtable
// ??_7CObList@@6B@ @0x1ed4b4; AddTail 0x1b4991 / RemoveAll 0x1b48a6 / ~CObList
// 0x1b48c6 all reloc-mask). The raw node walks read its m_pNodeHead directly via
// GetHeadPosition() (byte-identical to the old m_head field read). Its CNode is
// modeled here as CSoundNode (next at +0x00, prev +0x04, channel payload +0x08).
struct CSoundNode {
    CSoundNode* m_next;    // +0x00
    CSoundNode* m_prev;    // +0x04
    CSoundChannel* m_data; // +0x08
};

// The sound-channel objects the Create* factories allocate + add to the list are
// the REAL RTTI channel classes (<Gruntz/AmbientSound.h> / RandomAmbientSound.h);
// forward-declared here so the factory signatures carry the real types without
// fattening this header's includers. (The old SoundChannelNew shell view of the
// same family is dissolved.)
class CAmbientSound;
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
    void Retune(i32 pan, i32 vol); // 0x00bd60
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
    CObList m_list;               // +0x08  MFC CObList (head at +0x0c)
    i32 m_active;                 // +0x24  active flag
    i32 m_pan;                    // +0x28  pan
    i32 m_vol;                    // +0x2c  vol
};

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

#endif // GRUNTZ_CWORLDSOUNDSET_H
