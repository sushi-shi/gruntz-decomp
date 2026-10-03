#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/WorldSoundSet.h>

#include <Dsndmgr/SoundStream.h>
#include <Globals.h>
#include <Gruntz/AmbientSound.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/RandomAmbientSound.h>
#include <Gruntz/RandomRange.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SoundCueRegistryInline.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/UserLogic.h>
#include <Lith/BDefs.h>
#include <Rez/FrameCountdown.h>
#include <Rez/RezMgr.h>
#include <Utils/MapTyped.h>
#include <Wap32/CoordUnset.h>
#include <Wwd/WwdGameObjectFamily.h>

#include <math.h>
#include <new>

i32 g_posSoundReq;

i32 CWorldSoundSet::Init(SoundCueRegistry* cueRegistry, i32 masterVolume) {
    if (cueRegistry == NULL) {
        return 0;
    }
    m_cueRegistry = cueRegistry;
    m_masterVolume = masterVolume;
    m_enabled = true;
    m_listenerX = 0;
    m_listenerY = 0;
    return 1;
}

void CWorldSoundSet::Deactivate() {
    if (m_cueRegistry != NULL && m_cueRegistry->m_soundStream != NULL) {
        m_cueRegistry->m_soundStream->ClearVolumeRamps();
    }
    Teardown();
    m_cueRegistry = NULL;
}

void CWorldSoundSet::Teardown() {
    std::list<CAmbientSound*>::iterator pos = m_list.begin();
    while (pos != m_list.end()) {
        CAmbientSound* sound = static_cast<CAmbientSound*>(*(pos++));
        if (sound != NULL) {
            delete sound;
        }
    }
    m_list.clear();
}

CAmbientSound* CWorldSoundSet::CreateAmbientFromKey(
    const char* key,
    i32 volumeLevel,
    RECT* region,
    i32 volumeScale,
    i32 unused
) {
    CAmbientSound* obj = new CAmbientSound;
    if (obj == NULL) {
        return NULL;
    }
    if (obj->InitFromKey(m_cueRegistry, key, volumeLevel, m_masterVolume, region, volumeScale)
        == 0) {
        delete obj;
        return NULL;
    }
    obj->m_listNode = m_list.insert(m_list.end(), obj);
    obj->m_inSoundSet = true;
    return obj;
}

CAmbientSound* CWorldSoundSet::CreateAmbientFromSound(
    SoundBuffer* sound,
    i32 volumeLevel,
    RECT* region,
    i32 volumeScale,
    i32 unused
) {
    CAmbientSound* obj = new CAmbientSound;
    if (obj == NULL) {
        return NULL;
    }
    if (obj->InitFromSound(sound, volumeLevel, m_masterVolume, region, volumeScale) == 0) {
        delete obj;
        return NULL;
    }
    obj->m_listNode = m_list.insert(m_list.end(), obj);
    obj->m_inSoundSet = true;
    return obj;
}

CAmbientPosSound* CWorldSoundSet::CreatePositionedFromKey(
    const char* key,
    i32 volumeLevel,
    AmbientPoint* position,
    i32 volumeScale,
    i32 unused
) {
    CAmbientPosSound* obj = new CAmbientPosSound;
    if (obj == NULL) {
        return NULL;
    }
    if (obj->InitFromKey(m_cueRegistry, key, volumeLevel, m_masterVolume, position, volumeScale)
        == 0) {
        delete obj;
        return NULL;
    }
    obj->m_listNode = m_list.insert(m_list.end(), obj);
    obj->m_inSoundSet = true;
    return obj;
}

CAmbientPosSound* CWorldSoundSet::CreatePositionedFromSound(
    SoundBuffer* sound,
    i32 volumeLevel,
    AmbientPoint* position,
    i32 volumeScale,
    i32 unused
) {
    CAmbientPosSound* obj = new CAmbientPosSound;
    if (obj == NULL) {
        return NULL;
    }
    if (obj->InitFromSound(sound, volumeLevel, m_masterVolume, position, volumeScale) == 0) {
        delete obj;
        return NULL;
    }
    obj->m_listNode = m_list.insert(m_list.end(), obj);
    obj->m_inSoundSet = true;
    return obj;
}

CRandomAmbientSound* CWorldSoundSet::CreateRandomFromKey(
    const char* key,
    i32 volumeLevel,
    RECT* region,
    i32 volumeScale,
    i32 playDurationMin,
    i32 playDurationMax,
    i32 silenceDurationMin,
    i32 silenceDurationMax,
    i32 unused
) {
    if (static_cast<u32>(playDurationMax) < static_cast<u32>(playDurationMin)) {
        return NULL;
    }
    if (static_cast<u32>(silenceDurationMax) < static_cast<u32>(silenceDurationMin)) {
        return NULL;
    }
    CRandomAmbientSound* obj = new CRandomAmbientSound;
    if (obj == NULL) {
        return NULL;
    }
    if (obj->InitFromKey(m_cueRegistry, key, volumeLevel, m_masterVolume, region, volumeScale)
        == 0) {
        delete obj;
        return NULL;
    }
    obj->InitCycleTiming(playDurationMin, playDurationMax, silenceDurationMin, silenceDurationMax);
    obj->m_listNode = m_list.insert(m_list.end(), obj);
    obj->m_inSoundSet = true;
    return obj;
}

CRandomAmbientSound* CWorldSoundSet::CreateRandomFromSound(
    SoundBuffer* sound,
    i32 volumeLevel,
    RECT* region,
    i32 volumeScale,
    i32 playDurationMin,
    i32 playDurationMax,
    i32 silenceDurationMin,
    i32 silenceDurationMax,
    i32 unused
) {
    CRandomAmbientSound* obj = new CRandomAmbientSound;
    if (obj == NULL) {
        return NULL;
    }
    if (obj->InitFromSound(sound, volumeLevel, m_masterVolume, region, volumeScale) == 0) {
        delete obj;
        return NULL;
    }
    obj->InitCycleTiming(playDurationMin, playDurationMax, silenceDurationMin, silenceDurationMax);
    obj->m_listNode = m_list.insert(m_list.end(), obj);
    obj->m_inSoundSet = true;
    return obj;
}

void CWorldSoundSet::SetMasterVolume(i32 masterVolume) {
    m_masterVolume = masterVolume;
    if (m_cueRegistry->m_soundStream != NULL) {
        m_cueRegistry->m_soundStream->ClearVolumeRamps();
    }
    std::list<CAmbientSound*>::iterator pos = m_list.begin();
    while (pos != m_list.end()) {
        CAmbientSound* sound = static_cast<CAmbientSound*>(*(pos++));
        if (sound != NULL) {
            sound->ApplyMasterVolume(masterVolume);
        }
    }
}

void CWorldSoundSet::Stop() {
    if (m_cueRegistry != NULL && m_cueRegistry->m_soundStream != NULL) {
        m_cueRegistry->m_soundStream->ClearVolumeRamps();
    }
    std::list<CAmbientSound*>::iterator pos = m_list.begin();
    while (pos != m_list.end()) {
        CAmbientSound* sound = static_cast<CAmbientSound*>(*(pos++));
        if (sound != NULL && sound->m_sound != NULL) {
            sound->m_sound->StopAndRewind();
            sound->m_isPlaying = false;
        }
    }
}

void CWorldSoundSet::Resume() {
    std::list<CAmbientSound*>::iterator pos = m_list.begin();
    while (pos != m_list.end()) {
        CAmbientSound* sound = static_cast<CAmbientSound*>(*(pos++));
        if (sound != NULL) {
            sound->m_isPlaying = false;
            sound->Update(m_listenerX, m_listenerY, true);
        }
    }

    m_cueRegistry->TickVolumeRamps();
}

void CWorldSoundSet::SetListenerPosition(i32 x, i32 y) {
    m_listenerX = x;
    m_listenerY = y;
    std::list<CAmbientSound*>::iterator pos = m_list.begin();
    while (pos != m_list.end()) {
        CAmbientSound* sound = static_cast<CAmbientSound*>(*(pos++));
        if (sound != NULL) {
            sound->Update(x, y, false);
        }
    }

    m_cueRegistry->TickVolumeRamps();
}

i32 CAmbientSound::InitFromKey(
    SoundCueRegistry* cueRegistry,
    const char* key,
    i32 volumeLevel,
    i32 masterVolume,
    RECT* region,
    i32 volumeScale
) {
    SoundCue* cue = cueRegistry->FindCue(key);
    if (cue == NULL) {
        return 0;
    }
    return InitFromSound(cue->m_sound, volumeLevel, masterVolume, region, volumeScale);
}

i32 CAmbientSound::InitFromSound(
    SoundBuffer* sound,
    i32 volumeLevel,
    i32 masterVolume,
    RECT* region,
    i32 volumeScale
) {
    if (sound == NULL) {
        return 0;
    }
    m_sound = sound;
    m_volumeLevel = volumeLevel;
    m_masterVolume = masterVolume;
    m_volumeScale = volumeScale;
    m_panPercent = 0;
    m_isPlaying = false;
    RECT* p = &m_primaryRegion;
    if (region != NULL) {
        *p = *region;
    } else {
        p->left = COORD_UNSET;
    }
    if (p->left == 0 && m_primaryRegion.top == 0 && m_primaryRegion.right == 0
        && m_primaryRegion.bottom == 0) {
        p->left = COORD_UNSET;
    }
    m_secondaryRegion.left = COORD_UNSET;
    return 1;
}

void CAmbientSound::ApplyMasterVolume(i32 masterVolume) {
    if (m_masterVolume == masterVolume) {
        return;
    }
    i32 mult = m_volumeLevel;
    m_masterVolume = masterVolume;
    i32 v = ScaleVolume(mult);
    m_sound->SetVolumePercent(v);
}

void CAmbientSound::StartPlayback() {
    SoundBuffer* sound = m_sound;
    i32 pos = m_volumeLevel;
    if (sound == NULL) {
        return;
    }
    if (m_isPlaying != false) {
        return;
    }
    if (g_gameReg->m_soundEnabled == false) {
        return;
    }
    if (g_gameReg->m_worldSounds->m_enabled == false) {
        return;
    }
    m_sound->ApplyAndPlay(1, m_panPercent, 0, true);
    m_volumeLevel = pos;
    i32 v = ScaleVolume(pos);
    m_sound->SetVolumePercent(v);
    m_volumeLevel = pos;
    m_isPlaying = true;
}

void CAmbientSound::Update(i32 x, i32 y, b32 immediate) {
    i32 inRange;
    if (m_primaryRegion.left == COORD_UNSET) {

        if (m_isPlaying != false) {
            return;
        }
        SoundBuffer* sound = m_sound;
        i32 lvl = m_volumeLevel;
        if (sound == NULL) {
            return;
        }
        if (lvl == 0) {
            return;
        }
        if (g_gameReg->m_soundEnabled == false) {
            return;
        }
        if (g_gameReg->m_worldSounds->m_enabled == false) {
            return;
        }
        sound->ApplyAndPlay(1, m_panPercent, 0, true);
        SetVolumeLevel(0x64, 0, false);
        m_volumeLevel = 0x64;
        m_isPlaying = true;
        return;
    }

    if (x > m_primaryRegion.left && x < m_primaryRegion.right && y > m_primaryRegion.top
        && y < m_primaryRegion.bottom) {
        inRange = 1;
    } else if (m_secondaryRegion.left != COORD_UNSET && x > m_secondaryRegion.left
               && x < m_secondaryRegion.right && y > m_secondaryRegion.top
               && y < m_secondaryRegion.bottom) {
        inRange = 1;
    } else {
        inRange = 0;
    }

    if (m_isPlaying == false) {

        if (inRange == 0) {
            return;
        }
        if (g_gameReg->m_soundEnabled == false) {
            return;
        }
        if (g_gameReg->m_worldSounds->m_enabled == false) {
            return;
        }
        if (immediate != false) {
            if (m_sound == NULL) {
                return;
            }
            m_sound->ApplyAndPlay(1, m_panPercent, 0, true);
            SetVolumeLevel(0x64, 0, false);
            m_volumeLevel = 0x64;
            m_isPlaying = true;
        } else {
            FadePlayback(true, 0x64, 0x3e8);
        }
    } else {

        if (inRange != 0) {
            return;
        }
        FadePlayback(false, 0, 0x3e8);
    }
}

i32 CAmbientSound::SetVolumeLevel(i32 volumeLevel, i32 rampMs, b32 stopAndRewind) {
    m_volumeLevel = volumeLevel;
    i32 v = ScaleVolume(volumeLevel);
    if (rampMs == 0) {
        return m_sound->SetVolumePercent(v);
    }
    return m_sound->RampVolumeTo(v, rampMs, stopAndRewind);
}

void CAmbientSound::FadePlayback(b32 startPlaying, i32 volumeLevel, i32 rampMs) {
    if (m_sound == NULL) {
        return;
    }
    if (startPlaying != false) {

        if (m_isPlaying != false) {
            return;
        }
        if (g_gameReg->m_soundEnabled == false) {
            return;
        }
        if (g_gameReg->m_worldSounds->m_enabled == false) {
            return;
        }
        if (rampMs == 0) {
            m_sound->ApplyAndPlay(1, m_panPercent, 0, true);
            m_volumeLevel = volumeLevel;
            i32 v = ScaleVolume(volumeLevel);
            m_sound->SetVolumePercent(v);
        } else {
            m_sound->ApplyAndPlay(1, m_panPercent, 0, true);
            m_volumeLevel = volumeLevel;
            i32 v = ScaleVolume(volumeLevel);
            m_sound->RampVolumeTo(v, rampMs, false);
        }
        m_volumeLevel = volumeLevel;
        m_isPlaying = true;
        return;
    }

    if (m_isPlaying == false) {
        return;
    }
    if (rampMs == 0) {
        m_sound->StopAndRewind();
        m_isPlaying = false;
        return;
    }
    m_volumeLevel = 0;
    i32 v = ScaleVolume(m_volumeLevel);
    m_sound->RampVolumeTo(v, rampMs, true);
    m_isPlaying = false;
}

i32 CAmbientPosSound::InitFromKey(
    SoundCueRegistry* cueRegistry,
    const char* key,
    i32 volumeLevel,
    i32 masterVolume,
    AmbientPoint* position,
    i32 volumeScale
) {
    SoundCue* cue = cueRegistry->FindCue(key);
    if (cue == NULL) {
        return 0;
    }
    return InitFromSound(cue->m_sound, volumeLevel, masterVolume, position, volumeScale);
}

i32 CAmbientPosSound::InitFromSound(
    SoundBuffer* sound,
    i32 volumeLevel,
    i32 masterVolume,
    AmbientPoint* position,
    i32 volumeScale
) {
    if (sound == NULL) {
        return 0;
    }
    if (position == NULL) {
        return 0;
    }
    m_sound = sound;
    m_volumeLevel = volumeLevel;
    m_masterVolume = masterVolume;
    m_panPercent = 0;
    m_volumeScale = volumeScale;
    m_isPlaying = false;
    m_position = *position;
    return 1;
}

void CAmbientPosSound::Update(i32 x, i32 y, b32 immediate) {
    i32 dx = abs(m_position.m_x - x);
    i32 dy = abs(m_position.m_y - y);
    i32 dist2 = SQR(dx) + SQR(dy);
    if (dx > 0x280 || dy > 0x280) {
        if (m_sound != NULL && m_isPlaying != false) {
            m_sound->StopAndRewind();
            m_isPlaying = false;
        }
        return;
    }

    i32 dist = static_cast<i32>(sqrt(static_cast<double>(dist2)));
    i32 vol = 0x64 - dist / 12;
    if (vol > 0x64) {
        vol = 0x64;
    } else if (vol < 0) {
        vol = 0;
    }
    i32 pan = dx / 4;
    if (pan > 0x64) {
        pan = 0x64;
    } else if (pan < 0) {
        pan = 0;
    }
    if (m_position.m_x < x) {
        pan = -pan;
    }

    {
        m_volumeLevel = vol;
        i32 v = ScaleVolume(vol);
        m_sound->SetVolumePercent(v);
    }
    m_panPercent = pan;
    m_sound->SetPanPercent(pan);

    if (m_isPlaying != false) {
        return;
    }
    if (m_sound == NULL) {
        return;
    }
    if (g_gameReg->m_soundEnabled == false) {
        return;
    }
    if (g_gameReg->m_worldSounds->m_enabled == false) {
        return;
    }
    m_sound->ApplyAndPlay(1, m_panPercent, 0, true);
    {
        m_volumeLevel = vol;
        i32 v = ScaleVolume(vol);
        m_sound->SetVolumePercent(v);
    }
    m_volumeLevel = vol;
    m_isPlaying = true;
}

i32 DispatchGlobalAmbientSoundLogic(CGameObject* obj) {
    g_posSoundReq = 1;
    return DispatchAmbientSoundLogic(obj);
}

i32 DispatchAmbientSoundLogic(CGameObject* obj) {
    CLogicRecord* record = obj->m_logicRecord;
    CWwdSpriteObject* sprite = static_cast<CWwdSpriteObject*>(obj);
    if (record->m_eventCode == 0) {
        obj->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_SKIP_COLLISION);
        obj->m_stateFlags |= SPRITE_STATE_HIDDEN;
        if (record->m_dispatch == DispatchGlobalAmbientSoundLogic) {
            obj->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE);
        } else {
            obj->m_flags &= ~IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE);
        }
        SoundCue* layer = sprite->m_soundCue;
        if (layer && g_gameReg) {
            RECT rc;
            CopyRect(&rc, &obj->m_area);
            if (record->m_minX > 0 || record->m_maxX > 0) {
                SetRect(&rc, record->m_minX, record->m_minY, record->m_maxX, record->m_maxY);
            }
            if (g_gameReg->m_worldSounds) {
                CAmbientSound* placed;
                if (obj->m_extent.top > 0) {
                    placed = g_gameReg->m_worldSounds->CreateRandomFromSound(
                        layer->m_sound,
                        0x64,
                        &rc,
                        obj->m_damage,
                        obj->m_extent.left,
                        obj->m_extent.top,
                        obj->m_extent.right,
                        obj->m_extent.bottom,
                        0
                    );
                } else {
                    placed =
                        g_gameReg->m_worldSounds
                            ->CreateAmbientFromSound(layer->m_sound, 0x64, &rc, obj->m_damage, 0);
                }
                if (placed && obj->m_switchRect.top > 0) {
                    placed->m_secondaryRegion = obj->m_switchRect;
                }
            }
        }
        obj->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
        record->SetEventCode(5);
    }
    return 1;
}

i32 DispatchAmbientPosSoundLogic(CGameObject* obj) {
    g_posSoundReq = 2;
    return DispatchSpotAmbientSoundLogic(obj);
}

i32 DispatchSpotAmbientSoundLogic(CGameObject* obj) {
    CLogicRecord* record = obj->m_logicRecord;
    CWwdSpriteObject* sprite = static_cast<CWwdSpriteObject*>(obj);
    i32 state = record->m_eventCode;
    if (state != 0) {
        if (state != AMBIENT_SOUND_ACTIVE) {
            return 1;
        }
        CAmbientPosSound* sound = record->m_positionedSound;
        if (sound == NULL) {
            return 1;
        }

        CWorldSoundSet* set = g_gameReg->m_worldSounds;
        if (sound->m_sound != NULL) {
            sound->m_sound->StopAndRewind();
            sound->m_isPlaying = false;
        }
        if (sound->m_inSoundSet) {
            set->m_list.erase(sound->m_listNode);
            sound->m_inSoundSet = false;
            delete sound;
        }
        record->m_positionedSound = NULL;
        record->SetEventCode(0);
        return 1;
    }

    obj->m_stateFlags |= SPRITE_STATE_HIDDEN;
    obj->m_flags =
        (obj->m_flags & ~IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE))
        | IDX(
            WWD_GAME_OBJECT_FLAG_SKIP_COLLISION | WWD_GAME_OBJECT_FLAG_DISPATCH_LEAVE_ACTIVE_REGION
        );
    record->m_positionedSound = NULL;
    SoundCue* layer = sprite->m_soundCue;
    if (layer != NULL && g_gameReg != NULL) {

        CWorldSoundSet* set = g_gameReg->m_worldSounds;
        if (set != NULL) {
            AmbientPoint pt;
            pt.m_x = obj->m_screenX;
            pt.m_y = obj->m_screenY;

            CAmbientPosSound* v =
                set->CreatePositionedFromSound(layer->m_sound, 0x64, &pt, obj->m_damage, 0);
            if (v != NULL) {
                record->m_positionedSound = v;
            }
        }
    }
    record->SetEventCode(5);
    return 1;
}

void CRandomAmbientSound::Update(i32 x, i32 y, b32 immediate) {

    i32 firstBoxLeft = m_primaryRegion.left;
    i32 inBox = 0;
    if (firstBoxLeft == COORD_UNSET) {
        inBox = 1;
    } else if (x > firstBoxLeft && x < m_primaryRegion.right && y > m_primaryRegion.top
               && y < m_primaryRegion.bottom) {
        inBox = 1;
    } else {
        i32 secondBoxLeft = m_secondaryRegion.left;
        if (secondBoxLeft != COORD_UNSET && x > secondBoxLeft && x < m_secondaryRegion.right
            && y > m_secondaryRegion.top && y < m_secondaryRegion.bottom) {
            inBox = 1;
        }
    }

    if (inBox == 0) {
        if (m_isPlaying != false && m_sound != NULL) {
            SetVolumeLevel(0, 0x3e8, true);
            m_isPlaying = false;
        }
        m_playPhase = false;
        return;
    }

    if (immediate != false && m_playPhase != false && m_isPlaying != false) {
        return;
    }

    CountDown(m_countdownMs, g_frameDelta);
    if (m_countdownMs != 0) {
        return;
    }

    m_playPhase ^= 1;
    if (m_playPhase != false) {
        i32 r = RandRange(g_gameReg, m_playDuration.GetMin(), m_playDuration.GetMax());
        m_countdownMs = r;
        i32 half = Min(0x3e8, static_cast<i32>(static_cast<u32>(r) >> 1));
        FadePlayback(true, 0x64, half);
    } else {
        i32 r = RandRange(g_gameReg, m_silenceDuration.GetMin(), m_silenceDuration.GetMax());
        m_countdownMs = r;
        i32 half = Min(0x3e8, static_cast<i32>(static_cast<u32>(r) >> 1));
        FadePlayback(false, 0x64, half);
    }
}

i32 CGruntzMgr::Rand() {
    i32 seed;
    return GetRandomNumber();
}

void CRandomAmbientSound::InitCycleTiming(
    i32 playDurationMin,
    i32 playDurationMax,
    i32 silenceDurationMin,
    i32 silenceDurationMax
) {
    m_playDuration.Set(playDurationMin, playDurationMax);
    m_silenceDuration.Set(silenceDurationMin, silenceDurationMax);
    i32 countdown = RandRange(playDurationMin, playDurationMax);
    m_playPhase = true;
    m_countdownMs = countdown;
}
