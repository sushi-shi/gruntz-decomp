#ifndef GRUNTZ_CWORLDSOUNDSET_H
#define GRUNTZ_CWORLDSOUNDSET_H

#include <Mfc.h>
#include <rva.h>

#include <Dsndmgr/SoundDevice.h> // real SoundDevice (also pulls DirectSoundMgr)

class CAmbientSound;

struct CSoundNode {
    CSoundNode* m_next;    // +0x00
    CSoundNode* m_prev;    // +0x04
    CAmbientSound* m_data; // +0x08  the channel (family base; see the note above)
};
SIZE_UNKNOWN();

class CAmbientPosSound;
class CRandomAmbientSound;

struct CRandomAmbientWorld {
    char m_pad00[0x2c];
    SoundDevice* m_soundDev; // +0x2c  the world's SoundDevice sub-object
};
SIZE_UNKNOWN();

enum { kSoundVolumeMax = 100 }; // 0x64 - full volume on the 0-100 "Sound Volume" slider

class CWorldSoundSet {
public:
    i32 Init(void* world, i32 a2); // 0x00b5e0
    void Teardown();               // 0x00b660
    void Restart(i32 a1);          // 0x00bc30
    void Stop();                   // 0x00bc80
    void Resume();                 // 0x00bcf0
    void Retune(i32 x, i32 y);     // 0x00bd60  push the listener position to every channel
    void Deactivate();             // 0x00b620
    CWorldSoundSet();              // inline: m_list(0xa), m_world=0, m_04=0x64 (::operator new = RezAlloc)
    ~CWorldSoundSet();             // 0x085ed0

    // Factories: allocate + seed a sound channel (the real RTTI channel classes),
    // run its one-time Init, and (on success) append it to m_list. The `this` is
    // this CWorldSoundSet owner (the rtti-vptr heuristic once mislabeled them onto
    // the channel classes whose vtables their inlined ctors stamp).
    CAmbientSound* CreateAmbient6(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4);
    CAmbientSound* CreateAmbient5(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4);
    CAmbientPosSound* CreatePos6(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4);
    CAmbientPosSound* CreatePos5(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4);
    CRandomAmbientSound*
    CreateRandom(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7, i32 a8);
    // 0xba00: CRandomAmbientSound with a validated (x,y) bounding box; ::operator new.
    CRandomAmbientSound*
    CreateRandomBox(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7, i32 a8);

    CRandomAmbientWorld* m_world; // +0x00
    i32 m_volume;                 // +0x04  sound volume (0-100) threaded to each channel (ctor default kSoundVolumeMax)
    CPtrList m_list;              // +0x08  MFC CPtrList (head at +0x0c)
    i32 m_active;                 // +0x24  active flag
    // +0x28/+0x2c: the pending LISTENER position (not pan/vol - Play/Multi push the
    // main plane's scroll origin here via Retune; Resume replays it into each
    // channel's Update(x,y,force) slot).
    i32 m_listenerX; // +0x28
    i32 m_listenerY; // +0x2c
};
SIZE_UNKNOWN();

inline CWorldSoundSet::CWorldSoundSet() : m_list(0xa) {
    m_world = 0;
    m_volume = kSoundVolumeMax;
}


// TU-local thunk/table names this TU registers (moved from the .cpp; the
// addresses are ILT thunk VAs, reloc-masked at every use).
extern "C" void DefaultActionHandler_2d15(); // LAB_00402d15 (address only)


// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern "C" void* __stdcall PosSoundSpawn(void* layer, i32 a2, void* outPt, i32 a4, i32 a5);

#endif // GRUNTZ_CWORLDSOUNDSET_H
