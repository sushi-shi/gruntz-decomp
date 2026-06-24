// CRandomAmbientSound.h - a Gruntz ambient-sound game object (trace-discovered
// CRandomAmbientSound, an eyecandy/sound object in C:\Proj\Gruntz). It owns a
// world/level back-pointer (+0x00), a second back-pointer seeded at init (+0x04),
// an embedded MFC list of active sound channels (+0x08, head at +0x0c), an
// "active" flag (+0x24), and a pending pan/volume pair (+0x28/+0x2c).
//
// None of the matched methods touch a vptr on `this` (offset 0 is the world
// pointer, not a vftable), so the class is modeled non-polymorphic - the embedded
// CObject-derived bases / engine vtable live in another TU and are never used
// here. Field names are placeholders (m_<hexoffset>); only the OFFSETS + emitted
// code bytes are load-bearing (campaign doctrine).
#ifndef GRUNTZ_CRANDOMAMBIENTSOUND_H
#define GRUNTZ_CRANDOMAMBIENTSOUND_H

#include <Mfc.h>
#include <rva.h>

// The engine handles the sound pokes through. These are minimal __thiscall shells
// declared under the SAME mangled names the retail objects carry (the names the
// delinker assigned at the call targets), so each reloc-masked call pairs by name:
//   MinervaInner::MinervaInner_136ed0   @ 0x136ed0
//   DirectSoundMgr::StopAndRewind       @ 0x135380
//   DirectSoundMgr::winapi_136e20_timeGetTime @ 0x136e20
// (The world's +0x2c handle is poked both as a MinervaInner and as a DirectSoundMgr
// - the engine overlays both views on the one sub-object; the binary proves it.)
class MinervaInner {
public:
    void MinervaInner_136ed0(); // 0x136ed0  (thiscall, no args)
};

class DirectSoundMgr {
public:
    int StopAndRewind();                // 0x135380  (thiscall, no args)
    int winapi_136e20_timeGetTime(int); // 0x136e20  (thiscall, 1 arg)
};

// One active sound channel hanging off a list node. Polymorphic: the teardown /
// retune paths dispatch through its vtable (scalar-deleting dtor at slot 0, a
// 3-arg retune at slot 3). The class + its vtable live in another TU, so it is
// modeled as a small typed shell whose virtual calls reloc-mask by slot.
struct CSoundChannel {
    virtual void ScalarDtor(int flag); // slot 0  -> call [vptr]
    virtual void Slot1();
    virtual void Slot2();
    virtual void Retune(int a1, int a2, int a3); // slot 3 -> call [vptr+0xc]

    void Recompute(int frame); // 0x00bf10  (non-virtual, defined elsewhere)

    DirectSoundMgr* m_04; // +0x04  DirectSound handle (StopAndRewind target)
    char m_pad08[0x14 - 0x08];
    int m_14; // +0x14  cleared on stop/retune
};

// MFC CPtrList node, walked raw: next at +0x00, the channel payload at +0x08.
struct CSoundNode {
    CSoundNode* m_next;    // +0x00
    CSoundNode* m_prev;    // +0x04
    CSoundChannel* m_data; // +0x08
};

// The embedded MFC CPtrList of channels (the +0x08 sub-object). Modeled minimally:
// the raw node walks read m_head directly; RemoveAll + the destructor are engine
// externs (reloc-masked __thiscall on the sub-object address).
struct CSoundChannelList {
    void* m_vptr;       // +0x00 (== object +0x08) list vftable slot
    CSoundNode* m_head; // +0x04 (== object +0x0c)
    CSoundNode* m_tail; // +0x08
    void* m_free;       // +0x0c
    void* m_blocks;     // +0x10
    int m_blockSize;    // +0x14
    int m_count;        // +0x18

    void RemoveAll();     // 0x1b48a6
    ~CSoundChannelList(); // 0x1b48c6
};

// The world/level object the sound hangs off. Only its +0x2c slot is read here
// (a DirectSoundMgr-ish sub-object), then poked via the engine helpers.
struct CRandomAmbientWorld {
    char m_pad00[0x2c];
    DirectSoundMgr* m_2c; // +0x2c  sub-object handle (also viewed as MinervaInner)
};

class CRandomAmbientSound {
public:
    int Init(void* world, void* a2); // 0x00b5e0
    void Teardown();                 // 0x00b660
    void Restart(void* a1);          // 0x00bc30
    void Stop();                     // 0x00bc80
    void Resume();                   // 0x00bcf0
    void Retune(int pan, int vol);   // 0x00bd60
    void Deactivate();               // 0x00b620  (sibling, defined elsewhere)
    ~CRandomAmbientSound();          // 0x085ed0

    CRandomAmbientWorld* m_world; // +0x00
    void* m_04;                   // +0x04
    CSoundChannelList m_list;     // +0x08  head at +0x0c
    int m_24;                     // +0x24  active flag
    int m_28;                     // +0x28  pan
    int m_2c;                     // +0x2c  vol
};

#endif // GRUNTZ_CRANDOMAMBIENTSOUND_H
