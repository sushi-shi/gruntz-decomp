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
    void ScalarDtor(i32 flag); // slot 0  -> call [vptr]
    void Slot1();
    void Slot2();
    void Retune(i32 a1, i32 a2, i32 a3); // slot 3 -> call [vptr+0xc]

    void Recompute(i32 frame); // 0x00bf10  level-scale -> SetVolByIdx on m_voice

    DirectSoundMgr* m_voice; // +0x04  DirectSound handle (StopAndRewind / SetVolByIdx)
    i32 m_level;             // +0x08  level multiplier (scaled by the frame in Recompute)
    i32 m_lastFrame;         // +0x0c  last frame fed to Recompute (early-out compare)
    i32 m_multiplier;        // +0x10  secondary multiplier (>0 gate, percent)
    i32 m_14;                // +0x14  cleared on stop/retune
};

// MFC CPtrList node, walked raw: next at +0x00, the channel payload at +0x08.
struct CSoundNode {
    CSoundNode* m_next;    // +0x00
    CSoundNode* m_prev;    // +0x04
    CSoundChannel* m_data; // +0x08
};

// The MFC CPlex block the list's node allocator chains (CPtrList::m_pBlocks);
// only its type identity is needed here (layout owned by MFC, never walked).
struct CSoundBlock;

// The embedded MFC CPtrList of channels (the +0x08 sub-object). Modeled minimally:
// the raw node walks read m_head directly; RemoveAll + the destructor are engine
// externs (reloc-masked __thiscall on the sub-object address).
struct CSoundChannelList {
    void
    VSlot0(); // +0x00 (== object +0x08) list vftable slot  // real polymorphic vptr @+0x00 (was m_vptr)
    CSoundNode* m_head;    // +0x04 (== object +0x0c)  MFC CPtrList::m_pNodeHead
    CSoundNode* m_tail;    // +0x08                    m_pNodeTail
    CSoundNode* m_free;    // +0x0c                    m_pNodeFree (free-node list)
    CSoundBlock* m_blocks; // +0x10                   m_pBlocks (MFC CPlex block chain)
    i32 m_blockSize;       // +0x14
    i32 m_count;           // +0x18

    void* AddTail(void* p); // 0x1b4991  (returns the new node)
    void RemoveAll();       // 0x1b48a6
    ~CSoundChannelList();   // 0x1b48c6
};

// The new sound-channel object the Create* factories allocate + add to the list.
// Modeled minimally: a polymorphic scalar-deleting dtor at vtable slot 0, the
// fields the factories seed (m_voice/m_level/m_14/m_listNode), and the non-virtual one-time
// Init overloads (reached by direct rel32 -> ILT thunk, reloc-masked). The retail
// vtable is referenced only by address when an instance is created.
struct SoundChannelNew {
    void ScalarDtor(i32 flag); // slot 0
    i32 m_voice;               // +0x04  DirectSound handle
    i32 m_level;               // +0x08  level (default 100)
    char m_pad0c[0x14 - 0x0c];
    i32 m_14; // +0x14
    char m_pad18[0x3c - 0x18];
    i32 m_listNode; // +0x3c  the m_list node this channel was added at

    i32 Init6(void* world, i32 a1, i32 a2, void* a3, i32 a4, i32 a5);
    i32 Init5(i32 a0, i32 a1, void* a2, i32 a3, i32 a4);
    void Init2(i32 a0, i32 a1, i32 a2, i32 a3);
};

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
    void Deactivate();             // 0x00b620  (sibling, defined elsewhere)
    ~CWorldSoundSet();             // 0x085ed0

    // Factories: allocate + seed a sound channel, run its one-time Init, and (on
    // success) append it to m_list. The rtti-vptr heuristic mislabeled these as
    // CAmbientSound / CAmbientPosSound / CRandomAmbientSound members (they only
    // reference those classes' vtables); the `this` is this CWorldSoundSet owner.
    SoundChannelNew* CreateAmbient6_b6a0(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4);
    SoundChannelNew* CreateAmbient5_b7b0(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4);
    SoundChannelNew* CreatePos6_b850(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4);
    SoundChannelNew* CreatePos5_b960(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4);
    SoundChannelNew*
    CreateRandom_bb60(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7, i32 a8);

    CRandomAmbientWorld* m_world; // +0x00
    void* m_04;                   // +0x04
    CSoundChannelList m_list;     // +0x08  head at +0x0c
    i32 m_active;                 // +0x24  active flag
    i32 m_pan;                    // +0x28  pan
    i32 m_vol;                    // +0x2c  vol
};

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

#endif // GRUNTZ_CWORLDSOUNDSET_H
