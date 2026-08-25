#ifndef GRUNTZ_CAMBIENTSOUND_H
#define GRUNTZ_CAMBIENTSOUND_H

#include <rva.h>

#include <Enums.h>

GZ_ENUM_CONST_BEGIN(AmbientSoundActState)
    AMBIENT_SOUND_ACTIVE = 0x1e
GZ_ENUM_CONST_END(AmbientSoundActState)

#include <Mfc.h>

#include <Dsndmgr/SoundBuffer.h>
#include <Enums.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/UserLogic.h>
#include <Ints.h>

struct AmbientPoint {
    i32 x;
    i32 y;
};

struct SoundCueRegistry;

class CAmbientSound : public CUserBase {
public:
    CAmbientSound() {
        m_sound = NULL;
        m_volumeLevel = 0x64;
        m_isPlaying = 0;
        m_listNode = NULL;
    }

    virtual ~CAmbientSound() OVERRIDE {
        m_sound = NULL;
        m_listNode = NULL;
    }

    i32 ScaleVolume(i32 volumeLevel) {
        i32 scale = m_masterVolume;
        if (scale > 5) {
            scale -= 0xf;
        }
        i32 volume = (scale * volumeLevel) / 100;
        if (m_volumeScale > 0) {
            volume = (volume * m_volumeScale) / 100;
        }
        if (volume < 0) {
            return 0;
        }
        if (volume > 0x64) {
            return 0x64;
        }
        return volume;
    }

    i32 SetVolumeLevel(i32 volumeLevel, i32 rampMs, i32 stopAndRewind);

    virtual void Update(i32 x, i32 y, i32 immediate);

    void FadePlayback(i32 startPlaying, i32 volumeLevel, i32 rampMs);

    void StartPlayback();

    void ApplyMasterVolume(i32 masterVolume);

    i32 InitFromKey(
        SoundCueRegistry* cueRegistry,
        const char* key,
        i32 volumeLevel,
        i32 masterVolume,
        RECT* region,
        i32 volumeScale
    );
    i32 InitFromSound(
        SoundBuffer* sound,
        i32 volumeLevel,
        i32 masterVolume,
        RECT* region,
        i32 volumeScale
    );

    SoundBuffer* m_sound;
    i32 m_volumeLevel;
    i32 m_masterVolume;
    i32 m_volumeScale;
    i32 m_isPlaying;
    RECT m_primaryRegion;
    RECT m_secondaryRegion;
    i32 m_panPercent;
    POSITION m_listNode;
};

class CAmbientPosSound : public CAmbientSound {
public:
    CAmbientPosSound() {}

    virtual ~CAmbientPosSound() OVERRIDE {}
    virtual void Update(i32 x, i32 y, i32 immediate) OVERRIDE;

    i32 InitFromKey(
        SoundCueRegistry* cueRegistry,
        const char* key,
        i32 volumeLevel,
        i32 masterVolume,
        AmbientPoint* position,
        i32 volumeScale
    );
    i32 InitFromSound(
        SoundBuffer* sound,
        i32 volumeLevel,
        i32 masterVolume,
        AmbientPoint* position,
        i32 volumeScale
    );

    AmbientPoint m_position;
};

#endif // GRUNTZ_CAMBIENTSOUND_H
