#ifndef GRUNTZ_CWORLDSOUNDSET_H
#define GRUNTZ_CWORLDSOUNDSET_H

#include <Mfc.h>
#include <rva.h>

#include <Dsndmgr/SoundDevice.h> // real SoundDevice (also pulls DirectSoundMgr)

class CAmbientSound;

class CAmbientPosSound;
class CRandomAmbientSound;
struct AmbientPoint;

struct CRandomAmbientWorld {
    char m_pad00[0x10];
    CMapStringToPtr m_map;   // +0x10  cue key -> AmbSoundRecord* table (the Init6 lookup)
    SoundDevice* m_soundDev; // +0x2c  the world's SoundDevice sub-object
};
SIZE_UNKNOWN();

enum {
    kSoundVolumeMax = 100
}; // 0x64 - full volume on the 0-100 "Sound Volume" slider

class CWorldSoundSet {
public:
    i32 Init(void* world, i32 volume); // 0x00b5e0
    void Teardown();                   // 0x00b660
    void Restart(i32 volume);          // 0x00bc30
    void Stop();                       // 0x00bc80
    void Resume();                     // 0x00bcf0
    void Retune(i32 x, i32 y);         // 0x00bd60  push the listener position to every channel
    void Deactivate();                 // 0x00b620
    CWorldSoundSet();  // inline: m_list(0xa), m_world=0, m_04=0x64 (::operator new = RezAlloc)
    ~CWorldSoundSet(); // 0x085ed0

    // Factories: allocate + seed a sound channel (the real RTTI channel classes),
    // run its one-time Init, and (on success) append it to m_list. The `this` is
    // this CWorldSoundSet owner (the rtti-vptr heuristic once mislabeled them onto
    // the channel classes whose vtables their inlined ctors stamp).
    // `level`/`scaleB` land in the channel's m_level / m_scaleB (m_scaleA gets this
    // set's own m_volume, i.e. the master); the trailing arg is read by no body.
    CAmbientSound* CreateAmbient6(const char* key, i32 level, RECT* box, i32 scaleB, i32 unused);
    CAmbientSound*
    CreateAmbient5(DirectSoundMgr* mgr, i32 level, RECT* box, i32 scaleB, i32 unused);
    CAmbientPosSound*
    CreatePos6(const char* key, i32 level, AmbientPoint* pos, i32 scaleB, i32 unused);
    CAmbientPosSound*
    CreatePos5(DirectSoundMgr* mgr, i32 level, AmbientPoint* pos, i32 scaleB, i32 unused);
    // The four interval args feed CRandomAmbientSound::Init2(lo, hi, lo2, hi2).
    CRandomAmbientSound* CreateRandom(
        DirectSoundMgr* mgr,
        i32 level,
        RECT* box,
        i32 scaleB,
        i32 intervalLoA,
        i32 intervalHiA,
        i32 intervalLoB,
        i32 intervalHiB,
        i32 unused
    );
    // 0xba00: the key-resolving CreateRandom twin; rejects either INVERTED interval
    // range (hi < lo, unsigned) before allocating. ::operator new.
    CRandomAmbientSound* CreateRandomBox(
        const char* key,
        i32 level,
        RECT* box,
        i32 scaleB,
        i32 intervalLoA,
        i32 intervalHiA,
        i32 intervalLoB,
        i32 intervalHiB,
        i32 unused
    );

    CRandomAmbientWorld* m_world; // +0x00
    i32 m_volume; // +0x04  sound volume (0-100) threaded to each channel (ctor default kSoundVolumeMax)
    CPtrList m_list; // +0x08  MFC CPtrList (head at +0x0c)
    i32 m_active;    // +0x24  active flag
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

extern i32 g_posSoundReq;

struct PosSoundObj; // <Gruntz/PosSound.h>

// File-scope prototypes moved from the .cpp: an unqualified
// declaration at file scope has EXTERNAL linkage, so it belongs in
// the owner header.
i32 SpawnPosSound(PosSoundObj* obj);

#endif // GRUNTZ_CWORLDSOUNDSET_H
