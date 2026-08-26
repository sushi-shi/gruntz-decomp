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
    i32 Init(SoundCueRegistry* cueRegistry, i32 masterVolume);
    void Teardown();
    void SetMasterVolume(i32 masterVolume);
    void Stop();
    void Resume();
    void SetListenerPosition(i32 x, i32 y);
    void Deactivate();
    CWorldSoundSet();
    ~CWorldSoundSet();

    CAmbientSound* CreateAmbientFromKey(
        const char* key,
        i32 volumeLevel,
        RECT* region,
        i32 volumeScale,
        i32 unused
    );
    CAmbientSound* CreateAmbientFromSound(
        SoundBuffer* sound,
        i32 volumeLevel,
        RECT* region,
        i32 volumeScale,
        i32 unused
    );
    CAmbientPosSound* CreatePositionedFromKey(
        const char* key,
        i32 volumeLevel,
        AmbientPoint* position,
        i32 volumeScale,
        i32 unused
    );
    CAmbientPosSound* CreatePositionedFromSound(
        SoundBuffer* sound,
        i32 volumeLevel,
        AmbientPoint* position,
        i32 volumeScale,
        i32 unused
    );

    CRandomAmbientSound* CreateRandomFromSound(
        SoundBuffer* sound,
        i32 volumeLevel,
        RECT* region,
        i32 volumeScale,
        i32 playDurationMin,
        i32 playDurationMax,
        i32 silenceDurationMin,
        i32 silenceDurationMax,
        i32 unused
    );

    CRandomAmbientSound* CreateRandomFromKey(
        const char* key,
        i32 volumeLevel,
        RECT* region,
        i32 volumeScale,
        i32 playDurationMin,
        i32 playDurationMax,
        i32 silenceDurationMin,
        i32 silenceDurationMax,
        i32 unused
    );

    SoundCueRegistry* m_cueRegistry;
    i32 m_masterVolume;
    CPtrList m_list;
    b32 m_enabled;

    i32 m_listenerX;
    i32 m_listenerY;
};

inline CWorldSoundSet::CWorldSoundSet() : m_list(0xa) {
    m_cueRegistry = NULL;
    m_masterVolume = kSoundVolumeMax;
}

inline CWorldSoundSet::~CWorldSoundSet() {
    Deactivate();
}

extern i32 g_posSoundReq;

struct CGameObject;

#endif // GRUNTZ_CWORLDSOUNDSET_H
