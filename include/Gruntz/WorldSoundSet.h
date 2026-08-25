#ifndef GRUNTZ_CWORLDSOUNDSET_H
#define GRUNTZ_CWORLDSOUNDSET_H

#include <rva.h>

#include <Mfc.h>

#include <Dsndmgr/SoundDevice.h>
#include <Gruntz/SoundCueRegistry.h>

class CAmbientSound;

class CAmbientPosSound;
class CRandomAmbientSound;
struct AmbientPoint;

enum {
    kSoundVolumeMax = 100
};

class CWorldSoundSet {
public:
    i32 Init(SoundCueRegistry* cueRegistry, i32 volume);
    void Teardown();
    void Restart(i32 volume);
    void Stop();
    void Resume();
    void Retune(i32 x, i32 y);
    void Deactivate();
    CWorldSoundSet();
    ~CWorldSoundSet();

    CAmbientSound*
    CreateAmbientFromKey(const char* key, i32 level, RECT* box, i32 scaleB, i32 unused);
    CAmbientSound*
    CreateAmbientFromSound(SoundBuffer* mgr, i32 level, RECT* box, i32 scaleB, i32 unused);
    CAmbientPosSound*
    CreatePositionedFromKey(const char* key, i32 level, AmbientPoint* pos, i32 scaleB, i32 unused);
    CAmbientPosSound* CreatePositionedFromSound(
        SoundBuffer* mgr,
        i32 level,
        AmbientPoint* pos,
        i32 scaleB,
        i32 unused
    );

    CRandomAmbientSound* CreateRandom(
        SoundBuffer* mgr,
        i32 level,
        RECT* box,
        i32 scaleB,
        i32 intervalLoA,
        i32 intervalHiA,
        i32 intervalLoB,
        i32 intervalHiB,
        i32 unused
    );

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

    SoundCueRegistry* m_cueRegistry;
    i32 m_volume;
    CPtrList m_list;
    i32 m_active;

    i32 m_listenerX;
    i32 m_listenerY;
};

inline CWorldSoundSet::CWorldSoundSet() : m_list(0xa) {
    m_cueRegistry = NULL;
    m_volume = kSoundVolumeMax;
}

// Inline in retail: CGruntzMgr::LoadWorldMode expands it (Deactivate + the m_list
// CPtrList dtor) at both of its delete sites, while CGruntzMgr::Close calls the
// COMDAT copy the same object file emits at 0x85ed0.
inline CWorldSoundSet::~CWorldSoundSet() {
    Deactivate();
}

extern i32 g_posSoundReq;

struct CGameObject;

#endif // GRUNTZ_CWORLDSOUNDSET_H
