#include <StdAfx.h>

#include <Ints.h>

#include <Dsndmgr/SoundBuffer.h>

#include <ComOutRef.h>
#include <Dsndmgr/SoundDevice.h>
#include <Dsndmgr/SoundVolumeRamp.h>
#include <Dsndmgr/VolumeScale.h>
#include <Enums.h>
#include <Lith/BaseList.h>
#include <Pix16.h>
#include <Rez/RezMgr.h>
#include <SafeDelete.h>
#include <Utils/MillisPer.h>
#include <Wap32/Wap32.h>

#include <dsound.h>
#include <io.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

const double g_volumePercentScale = 100.0;

const double g_volumeCurveUnit = 1.0;

const double g_decibelScale = 10.0;

const double g_attenuationBase = 2.0;

#define DSNDMGR_FILE "C:\\Proj\\Dsndmgr\\DSNDMGR.CPP"

typedef enum DSoundDx5Magic {
    DSBUFFERDESC_SIZE = 0x14,
} DSoundDx5Magic;

i32 g_volumeTable[VOLUME_PCT_MAX + 1];

#pragma optimize("", off)

i32 SoundDevice::VolumeToAttenuation(i32 volumePct) {
    if (volumePct == VOLUME_PCT_MAX) {
        return DSBVOLUME_MAX;
    }
    if (volumePct == 0) {
        return DSBVOLUME_MIN;
    }

    double ratio = acos(pow(g_volumeCurveUnit / (volumePct / g_volumePercentScale), g_decibelScale))
                   / acos(g_attenuationBase);
    return static_cast<i32>((-(ratio * g_volumePercentScale)));
}

i32 ConvertVolumeToPercent(i32 attenuation) {
    if (attenuation == 0) {
        return VOLUME_PCT_MAX;
    }
    double decibels;
    if (attenuation < 0) {
        decibels = static_cast<double>((-attenuation / 100));
    } else {
        decibels = static_cast<double>((attenuation / 100));
    }
    double volume = g_volumePercentScale
                    - (g_volumeCurveUnit - pow(g_attenuationBase, -decibels / g_decibelScale))
                          * g_volumePercentScale;
    if (attenuation < 0) {
        return static_cast<i32>(volume);
    }
    return static_cast<i32>((-volume));
}

#pragma optimize("", on)

void SoundDevice::BuildVolumeTable() {
    for (i32 i = 0; i <= VOLUME_PCT_MAX; i++) {
        g_volumeTable[i] = VolumeToAttenuation(i);
    }
}

SoundBuffer::SoundBuffer(IDirectSoundBuffer* buffer, SoundDevice* owner) {

    m_buffer = buffer;
    m_owner = owner;
    m_playFlags = 0;
    m_durationMs = 0;
    m_reacquireCb = NULL;
    m_reacquireArg = 0;
    m_baseSampleRate = 0;
    m_sampleRate = 0;
    if (buffer == NULL) {
        return;
    }

    DSBCAPS caps;
    caps.dwSize = sizeof(DSBCAPS);
    if (buffer->GetCaps(&caps) == 0) {
        m_caps = caps.dwFlags;
    } else {
        m_caps = 0;
    }

    if ((m_caps & DSBCAPS_CTRLFREQUENCY) == DSBCAPS_CTRLFREQUENCY) {
        b32 hr = buffer->GetFrequency(&m_baseFrequency) != 0;
        if (hr) {
            ReportError(DSNDMGR_FILE, 0x58, hr);
        }
    }
    m_frequency = m_baseFrequency;

    if ((m_caps & DSBCAPS_CTRLPAN) == DSBCAPS_CTRLPAN) {
        b32 hr = buffer->GetPan(&m_pan) != 0;
        if (hr) {
            ReportError(DSNDMGR_FILE, 0x60, hr);
        }
    } else {
        m_pan = DSBPAN_CENTER;
    }

    if ((m_caps & DSBCAPS_CTRLVOLUME) == DSBCAPS_CTRLVOLUME) {
        b32 hr = buffer->GetVolume(&m_volume) != 0;
        if (hr) {
            ReportError(DSNDMGR_FILE, 0x68, hr);
        }
    } else {
        m_volume = DSBVOLUME_MAX;
    }
}

SoundBuffer::~SoundBuffer() {}

i32 SoundBuffer::Restore() {
    b32 hr = m_buffer->Restore() != 0;
    if (hr) {
        ReportError(DSNDMGR_FILE, 0x7b, hr);
        return 0;
    }
    return 1;
}

i32 SoundBuffer::ReacquireBuffer() {
    if (m_owner->m_initialized == false) {
        return 0;
    }
    if (m_reacquireCb != NULL) {
        if (m_reacquireCb(this, m_reacquireArg) != 0) {
            return 1;
        }
    }
    return m_owner->ReacquireViaCallback();
}

i32 SoundBuffer::StopAndRewind() {
    if (m_owner->m_initialized == false) {
        return 0;
    }
    b32 hr = m_buffer->Stop() != 0;
    if (hr) {
        ReportError(DSNDMGR_FILE, 0x99, hr);
        return 0;
    }
    hr = m_buffer->SetCurrentPosition(0) != 0;
    if (hr) {
        ReportError(DSNDMGR_FILE, 0x9e, hr);
    }
    return 1;
}

i32 SoundBuffer::IsPlaying() {
    if (m_owner->m_initialized == false) {
        return 0;
    }
    DWORD status;
    b32 hr = m_buffer->GetStatus(&status) != 0;
    if (hr) {
        ReportError(DSNDMGR_FILE, 0xac, hr);
        return 0;
    }
    if ((status & DSBSTATUS_PLAYING) == DSBSTATUS_PLAYING) {
        return 1;
    }
    return 0;
}

i32 SoundBuffer::IsLooping() {
    if (m_owner->m_initialized == false) {
        return 0;
    }
    DWORD status;
    b32 hr = m_buffer->GetStatus(&status) != 0;
    if (hr) {
        ReportError(DSNDMGR_FILE, 0xbb, hr);
        return 0;
    }
    if ((status & DSBSTATUS_BUFFERLOST) == DSBSTATUS_BUFFERLOST) {
        return 1;
    }
    return 0;
}

i32 SoundBuffer::IsInHardware() {
    if (m_owner->m_initialized == false) {
        return 0;
    }
    DSBCAPS caps;
    memset(&caps, 0, sizeof(caps));
    caps.dwSize = sizeof(DSBCAPS);
    b32 hr = m_buffer->GetCaps(&caps) != 0;
    if (hr) {
        ReportError(DSNDMGR_FILE, 0xcc, hr);
        return 0;
    }
    if ((caps.dwFlags & DSBCAPS_LOCHARDWARE) == DSBCAPS_LOCHARDWARE) {
        return 1;
    }
    return 0;
}

void SoundBuffer::SetLooping(b32 enabled) {
    if (m_owner->m_initialized == false) {
        return;
    }
    if (enabled) {
        m_playFlags |= DSBPLAY_LOOPING;
    } else {
        m_playFlags &= ~DSBPLAY_LOOPING;
    }
}

i32 SoundBuffer::IsLoopingEnabled() {
    if (m_owner->m_initialized == false) {
        return 0;
    }
    if ((m_playFlags & DSBPLAY_LOOPING) == DSBPLAY_LOOPING) {
        return 1;
    }
    return 0;
}

i32 SoundBuffer::SetVolume(i32 attenuation) {
    if (m_owner->m_initialized == false) {
        return 0;
    }
    if ((m_caps & DSBCAPS_CTRLVOLUME) != DSBCAPS_CTRLVOLUME) {
        return 0;
    }
    b32 hr = m_buffer->SetVolume(attenuation) != 0;
    if (hr) {
        ReportError(DSNDMGR_FILE, 0xf6, hr);
        return 0;
    }
    return 1;
}

i32 SoundBuffer::SetVolumePercent(i32 volumePct) {
    if (m_owner->m_initialized == false) {
        return 0;
    }
    return SetVolume(g_volumeTable[volumePct]);
}

i32 SoundBuffer::GetVolume() {
    if (m_owner->m_initialized == false) {
        return 0;
    }
    long attenuation;
    b32 hr = m_buffer->GetVolume(&attenuation) != 0;
    if (hr) {
        ReportError(DSNDMGR_FILE, 0x10e, hr);
        return 0;
    }
    return attenuation;
}

i32 SoundBuffer::GetVolumePercent() {
    if (m_owner->m_initialized == false) {
        return 0;
    }
    return ConvertVolumeToPercent(GetVolume());
}

i32 SoundBuffer::RampVolumeTo(i32 targetVolumePct, i32 durationMs, b32 stopAndRewind) {
    SoundDevice* owner = m_owner;
    if (owner->m_initialized == false) {
        return 0;
    }
    owner->m_volumeRamps.RemoveMatching(this, SOUND_TASK_TAG_VOLUME_RAMP);

    if (durationMs == 0) {
        SetVolumePercent(targetVolumePct);
        return 1;
    }

    SoundVolumeRamp* ramp = new SoundVolumeRamp(
        targetVolumePct,
        GetVolumePercent(),
        durationMs,
        this,
        stopAndRewind,
        -1
    );
    if (ramp == NULL) {
        return 0;
    }
    m_owner->m_volumeRamps.InsertFirst(ramp);
    return 1;
}

i32 SoundBuffer::SetPan(i32 pan) {
    if (m_owner->m_initialized == false) {
        return 0;
    }
    if ((m_caps & DSBCAPS_CTRLPAN) != DSBCAPS_CTRLPAN) {
        return 0;
    }
    b32 hr = m_buffer->SetPan(pan) != 0;
    if (hr) {
        ReportError(DSNDMGR_FILE, 0x141, hr);
        return 0;
    }
    return 1;
}

i32 SoundBuffer::SetPanPercent(i32 panPct) {
    if (m_owner->m_initialized == false) {
        return 0;
    }
    if (panPct >= 0) {
        return SetPan(-g_volumeTable[VOLUME_PCT_MAX - panPct]);
    }
    return SetPan(g_volumeTable[VOLUME_PCT_MAX + panPct]);
}

i32 SoundBuffer::GetPan() {
    if (m_owner->m_initialized == false) {
        return 0;
    }
    long pan;
    b32 hr = m_buffer->GetPan(&pan) != 0;
    if (hr) {
        ReportError(DSNDMGR_FILE, 0x15e, hr);
        return 0;
    }
    return pan;
}

i32 SoundBuffer::GetPanPercent() {
    if (m_owner->m_initialized == false) {
        return 0;
    }
    i32 pan = GetPan();
    if (pan == 0) {
        return 0;
    }
    if (pan > 0) {
        return VOLUME_PCT_MAX - ConvertVolumeToPercent(-pan);
    }
    return ConvertVolumeToPercent(pan) - VOLUME_PCT_MAX;
}

i32 SoundBuffer::SetFrequency(u32 frequency) {
    if (m_owner->m_initialized == false) {
        return 0;
    }
    if ((m_caps & DSBCAPS_CTRLFREQUENCY) != DSBCAPS_CTRLFREQUENCY) {
        return 0;
    }
    i32 hr = m_buffer->SetFrequency(frequency);
    if (hr) {
        ReportError(DSNDMGR_FILE, 0x180, hr);
        return 0;
    }
    m_frequency = frequency;
    return 1;
}

u32 SoundBuffer::GetFrequency() {
    if (m_owner->m_initialized == false) {
        return 0;
    }
    return m_frequency;
}

u32 SoundBuffer::GetBaseFrequency() {
    if (m_owner->m_initialized == false) {
        return 0;
    }
    return m_baseFrequency;
}

i32 SoundBuffer::SetFrequencyOffsetPercent(i32 percentOffset) {
    if (m_owner->m_initialized == false) {
        return 0;
    }
    i32 frequency =
        percentOffset * static_cast<i32>(m_baseFrequency) / 100 + static_cast<i32>(m_baseFrequency);
    if (static_cast<u32>(frequency) >= DSBFREQUENCY_MAX) {
        frequency = DSBFREQUENCY_MAX - 1;
    }
    if (static_cast<u32>(frequency) <= DSBFREQUENCY_MIN) {
        frequency = DSBFREQUENCY_MIN + 1;
    }
    i32 result = SetFrequency(frequency);
    m_sampleRate = percentOffset * m_baseSampleRate / 100 + m_baseSampleRate;
    UpdateDuration();
    return result;
}

void SoundBuffer::UpdateDuration() {
    m_durationMs = m_sampleCount * MILLIS_PER_SECOND / m_sampleRate;
}

i32 SoundBuffer::Unlock(u8* audioPtr1, u32 audioBytes1, u8* audioPtr2, u32 audioBytes2) {
    if (m_owner->m_initialized == false) {
        return 0;
    }
    b32 hr = m_buffer->Unlock(audioPtr1, audioBytes1, audioPtr2, audioBytes2) != 0;
    if (hr) {
        ReportError(DSNDMGR_FILE, 0x1bb, hr);
        return 0;
    }
    return 1;
}

i32 SoundBuffer::GetCurrentPosition(DWORD* playCursor, DWORD* writeCursor) {
    if (m_owner->m_initialized == false) {
        return 0;
    }
    b32 hr = m_buffer->GetCurrentPosition(playCursor, writeCursor) != 0;
    if (hr) {
        ReportError(DSNDMGR_FILE, 0x1c8, hr);
        return 0;
    }
    return 1;
}

i32 SoundBuffer::SetCurrentPosition(u32 position) {
    if (m_owner->m_initialized == false) {
        return 0;
    }
    b32 hr = m_buffer->SetCurrentPosition(position) != 0;
    if (hr) {
        ReportError(DSNDMGR_FILE, 0x1d5, hr);
        return 0;
    }
    return 1;
}

i32 SoundBuffer::GetFormat(WAVEFORMATEX* outFormat, u32 formatBytes, DWORD* writtenBytes) {
    if (m_owner->m_initialized == false) {
        return 0;
    }
    b32 hr = m_buffer->GetFormat(outFormat, formatBytes, writtenBytes) != 0;
    if (hr) {
        ReportError(DSNDMGR_FILE, 0x1e2, hr);
        return 0;
    }
    return 1;
}

SoundSample::SoundSample(IDirectSoundBuffer* buffer, SoundDevice* owner)
    : SoundBufferInstance(buffer, owner) {

    ((&m_instances))->InsertFirst(&m_instanceNode);
    m_reusable = true;
}

SoundSample::~SoundSample() {
    while (m_instances.GetFirst() != NULL) {
        DestroyInstance(m_instances.GetFirst()->m_buffer);
    }
}

SoundBuffer* SoundSample::CreateInstance(b32 reusable) {
    if (m_owner->m_initialized == false) {
        return NULL;
    }
    SoundBufferInstance* instance = new SoundBufferInstance(m_buffer, m_owner, this);
    if (instance == NULL) {
        return NULL;
    }
    IDirectSound* device = m_owner->GetDirectSound();
    b32 hr = device->DuplicateSoundBuffer(m_buffer, &instance->m_buffer) != 0;
    if (hr) {
        ReportError(DSNDMGR_FILE, 0x217, hr);
        return NULL;
    }
    ((&m_instances))->InsertFirst(&instance->m_instanceNode);
    instance->m_reusable = reusable;
    return instance;
}

void SoundSample::DestroyInstance(SoundBuffer* instance) {
    if (m_owner->m_initialized == false) {
        return;
    }
    if (instance != this) {
        IDirectSoundBuffer* buffer = instance->m_buffer;
        buffer->Release();
        instance->m_buffer = NULL;
    }
    ((&m_instances))->Delete(&instance->m_instanceNode);
    if (instance != this) {
        delete instance;
    }
}

SoundBuffer* SoundSample::AcquireInstance() {
    if (!m_owner->m_initialized) {
        return NULL;
    }
    SoundBufferNode* node = m_instances.GetFirst();
    if (node) {
        while (true) {
            if (node->m_buffer->m_reusable && node->m_buffer->IsPlaying() == 0) {
                break;
            }
            node = static_cast<SoundBufferNode*>(node->Next());
            if (!node) {
                break;
            }
        }
    }
    SoundBuffer* instance;
    if (!node) {
        instance = NULL;
    } else {
        instance = node->m_buffer;
    }
    if (instance) {
        instance->SetVolume(m_volume);
        instance->SetPan(m_pan);
        instance->SetFrequency(m_baseFrequency);
    }
    if (!instance) {
        instance = CreateInstance(true);
    }
    if (instance) {
        m_instances.Delete(&instance->m_instanceNode);
        m_instances.InsertLast(&instance->m_instanceNode);
    }
    return instance;
}

i32 SoundBuffer::LoadFromFile(FILE* file, u32 bytes, i32 offset) {
    if (m_owner->m_initialized == false) {
        return 0;
    }
    if (offset != -1) {
        if (fseek(file, offset, SEEK_SET) != 0) {
            return 0;
        }
    }

    u8* audioPtr1;
    DWORD audioBytes1;
    u8* audioPtr2;
    DWORD audioBytes2;
    i32 hr = m_buffer->Lock(
        0,
        bytes,
        PtrOut(&audioPtr1),
        &audioBytes1,
        PtrOut(&audioPtr2),
        &audioBytes2,
        DSBLOCK_FROMWRITECURSOR
    );
    if (hr != 0) {
        ReportError(DSNDMGR_FILE, 0x27c, hr);
        return 0;
    }

    if (audioBytes1 > 0) {
        if (fread(audioPtr1, audioBytes1, 1, file) != 1) {
            return 0;
        }
    }
    if (audioBytes2 > 0) {
        if (fread(audioPtr2, audioBytes2, 1, file) != 1) {
            return 0;
        }
    }

    hr = m_buffer->Unlock(audioPtr1, audioBytes1, audioPtr2, audioBytes2);
    if (hr != 0) {
        ReportError(DSNDMGR_FILE, 0x295, hr);
        return 0;
    }
    return 1;
}

i32 SoundBuffer::LockConvert(u8* sourceAudio, u32 lockBytes, b32 convert16To8) {
    if (m_owner->m_initialized == false) {
        return 0;
    }

    u8* audioPtr1;
    u8* audioPtr2;
    DWORD audioBytes1;
    DWORD audioBytes2;
    i32 hr = m_buffer->Lock(
                 0,
                 lockBytes,
                 PtrOut(&audioPtr1),
                 &audioBytes1,
                 PtrOut(&audioPtr2),
                 &audioBytes2,
                 DSBLOCK_ENTIREBUFFER
             )
             != 0;
    if (hr) {
        ReportError(DSNDMGR_FILE, 0x2bd, hr);
        return 0;
    }

    if (convert16To8 == false) {

        if (audioBytes1 > 0) {
            memcpy(audioPtr1, sourceAudio, audioBytes1);
        }
        if (audioBytes2 > 0) {
            memcpy(audioPtr2, sourceAudio + audioBytes1, audioBytes2);
        }
    } else {

        if (audioBytes1 > 0) {
            u8* dst = audioPtr1;
            Pix16Ptr samples;
            samples.m_bytes = sourceAudio;
            i16* srcSample = samples.m_swords;
            u8* end = audioPtr1 + audioBytes1;
            while (dst < end) {
                *dst = static_cast<u8>((static_cast<u32>((*srcSample + 0x8000)) >> 8));
                ++srcSample;
                ++dst;
            }
        }
        if (audioBytes2 > 0) {
            u8* dst = audioPtr2;

            Pix16Ptr samples;
            samples.m_bytes = sourceAudio + audioBytes1;
            i16* srcSample = samples.m_swords;
            u8* end = audioPtr2 + audioBytes2;
            while (dst < end) {
                *dst = static_cast<u8>((static_cast<u32>((*srcSample + 0x8000)) >> 8));
                ++srcSample;
                ++dst;
            }
        }
    }

    hr = m_buffer->Unlock(audioPtr1, audioBytes1, audioPtr2, audioBytes2) != 0;
    if (hr) {
        ReportError(DSNDMGR_FILE, 0x2e1, hr);
        return 0;
    }
    return 1;
}

i32 SoundSample::Play() {
    if (m_owner->m_initialized == false) {
        return 0;
    }
    SoundBuffer* item = AcquireInstance();
    if (item == NULL) {
        return 0;
    }
    return item->Play();
}

i32 SoundSample::AcquireAndPlay(i32 volumePct, i32 panPct, i32 frequencyOffsetPct, b32 looping) {
    if (!m_owner->m_initialized) {
        return 0;
    }
    SoundBuffer* instance = AcquireInstance();
    if (!instance) {
        return 0;
    }
    b32 ok = true;
    if (!instance->SetVolumePercent(volumePct)) {
        ok = false;
    }
    if (!instance->SetPanPercent(panPct)) {
        ok = false;
    }
    if (!instance->SetFrequencyOffsetPercent(frequencyOffsetPct)) {
        ok = false;
    }
    instance->SetLooping(looping);
    if (!instance->Play()) {
        ok = false;
    }
    return ok;
}

void SoundSample::StopAllInstances() {
    if (m_owner->m_initialized == false) {
        return;
    }
    for (SoundBufferNode* node = m_instances.GetFirst(); node != NULL;
         node = static_cast<SoundBufferNode*>(node->Next())) {
        node->m_buffer->StopAndRewind();
    }
}

SoundBufferInstance::SoundBufferInstance(
    IDirectSoundBuffer* buffer,
    SoundDevice* owner,
    SoundBuffer* original
)
    : SoundBuffer(buffer, owner) {
    m_instanceNode.m_buffer = this;
    m_restoreSource = original;
    m_reusable = true;
    m_sampleCount = original->m_sampleCount;
    m_reacquireCb = original->m_reacquireCb;
    m_reacquireArg = original->m_reacquireArg;
    m_sampleRate = original->m_sampleRate;
    m_baseSampleRate = original->m_baseSampleRate;
    UpdateDuration();
}

SoundBufferInstance::SoundBufferInstance(IDirectSoundBuffer* buffer, SoundDevice* owner)
    : SoundBuffer(buffer, owner) {

    m_instanceNode.m_buffer = this;
    m_restoreSource = this;
    m_reusable = true;
}

SoundBufferInstance::~SoundBufferInstance() {}

i32 SoundBuffer::Play() {
    if (m_owner->m_initialized == false) {
        return 0;
    }
    b32 hr = m_buffer->Play(0, 0, m_playFlags) != 0;
    if (hr != false) {
        if (hr == DSERR_BUFFERLOST) {
            if (m_restoreSource->ReacquireBuffer() != 0) {
                b32 hr2 = m_buffer->Play(0, 0, m_playFlags) != 0;
                if (hr2 == false) {
                    return 1;
                }
                ReportError(DSNDMGR_FILE, 0x34c, hr2);
                return 0;
            }
        } else {
            ReportError(DSNDMGR_FILE, 0x356, hr);
        }
        return 0;
    }
    return 1;
}

i32 SoundBuffer::ApplyAndPlay(i32 volumePct, i32 panPct, i32 frequencyOffsetPct, b32 looping) {
    if (m_owner->m_initialized == false) {
        return 0;
    }
    b32 ok = true;
    if (SetVolumePercent(volumePct) == 0) {
        ok = false;
    }
    if (SetPanPercent(panPct) == 0) {
        ok = false;
    }
    if (SetFrequencyOffsetPercent(frequencyOffsetPct) == 0) {
        ok = false;
    }
    SetLooping(looping);
    if (Play() == 0) {
        ok = false;
    }
    return ok;
}

i32 SoundBuffer::Lock(
    u32 offset,
    u32 bytes,
    u8** audioPtr1,
    DWORD* audioBytes1,
    u8** audioPtr2,
    DWORD* audioBytes2,
    u32 flags
) {
    if (m_owner->m_initialized == false) {
        return 0;
    }
    i32 hr = m_buffer->Lock(
                 offset,
                 bytes,
                 PtrOut(audioPtr1),
                 audioBytes1,
                 PtrOut(audioPtr2),
                 audioBytes2,
                 flags
             )
             != 0;
    if (hr != 0) {

        if (hr == DSERR_BUFFERLOST) {
            if (m_restoreSource->ReacquireBuffer() != 0) {
                hr = m_buffer->Lock(
                         offset,
                         bytes,
                         PtrOut(audioPtr1),
                         audioBytes1,
                         PtrOut(audioPtr2),
                         audioBytes2,
                         flags
                     )
                     != 0;
                if (hr != 0) {
                    ReportError(DSNDMGR_FILE, 0x37c, hr);
                    return 0;
                }
            } else {
                return 0;
            }
        } else {
            ReportError(DSNDMGR_FILE, 0x386, hr);
            return 0;
        }
    }
    return 1;
}

SoundDevice::SoundDevice() {

    m_initialized = false;
    BuildVolumeTable();
    m_reacquireProc = NULL;
    m_primaryBuffer = NULL;
    m_cooperativeLevel = 0;
    m_bufferFlags = 0;
    m_force8Bit = false;
}

SoundDevice::~SoundDevice() {

    if (m_initialized) {
        Shutdown();
    }
}

i32 SoundDevice::Initialize(HWND hwnd, u32 cooperativeLevel, u32 bufferFlags) {
    b32 createFailed = DirectSoundCreate(NULL, &m_device, NULL) != 0;
    if (createFailed) {
        return 0;
    }
    b32 hr = m_device->SetCooperativeLevel(hwnd, cooperativeLevel) != 0;
    if (hr) {
        SoundBuffer::ReportError(DSNDMGR_FILE, 0x3b0, hr);
        m_device->Release();
        return 0;
    }
    m_cooperativeLevel = cooperativeLevel;
    m_bufferFlags = bufferFlags;
    m_lastRampTickMs = 0;
    m_initialized = true;
    return 1;
}

i32 SoundDevice::ReacquireViaCallback() {
    if (m_reacquireProc != NULL) {
        return (this->*m_reacquireProc)();
    }
    return 0;
}

i32 SoundDevice::SetCooperativeLevel(HWND hwnd, u32 cooperativeLevel) {
    if (m_initialized == false) {
        return 0;
    }
    b32 hr = m_device->SetCooperativeLevel(hwnd, cooperativeLevel) != 0;
    if (hr) {
        SoundBuffer::ReportError(DSNDMGR_FILE, 0x3cf, hr);
        return 0;
    }
    m_cooperativeLevel = cooperativeLevel;
    return 1;
}

i32 SoundDevice::Compact() {
    if (m_initialized == false) {
        return 0;
    }
    b32 hr = m_device->Compact() != 0;
    if (hr) {
        SoundBuffer::ReportError(DSNDMGR_FILE, 0x3dc, hr);
        return 0;
    }
    return 1;
}

void SoundDevice::Shutdown() {
    if (m_initialized) {
        SoundSample* node = m_samples.GetFirst();
        while (node) {
            DestroyBuffer(node);
            node = m_samples.GetFirst();
        }
        if (m_primaryBuffer) {
            m_primaryBuffer->Release();
        }
        m_device->Release();
    }
    m_initialized = false;
}

SoundSample* SoundDevice::CreateSample(WAVEFORMATEX* format, u32 bytes, u32 flags) {
    WAVEFORMATEX bufferFormat;
    IDirectSoundBuffer* directSoundBuffer;
    DSBUFFERDESC bufferDesc;
    i32 hr;
    SoundSample* result;

    if (m_initialized == false) {
        result = NULL;
        goto done;
    }
    if (bytes == 0) {
        result = NULL;
        goto done;
    }
    if (format == NULL) {
        result = NULL;
        goto done;
    }
    if (format->wFormatTag != WAVE_FORMAT_PCM) {
        result = NULL;
        goto done;
    }

    bufferFormat = *format;
    bufferFormat.cbSize = 0;

    memset(&bufferDesc, 0, sizeof(DSBUFFERDESC));
    bufferDesc.dwSize = DSBUFFERDESC_SIZE;
    bufferDesc.dwFlags = flags;
    bufferDesc.dwBufferBytes = bytes;
    bufferDesc.lpwfxFormat = &bufferFormat;

    hr = m_device->CreateSoundBuffer(&bufferDesc, &directSoundBuffer, NULL) != 0;
    if (hr) {
        SoundBuffer::ReportError(DSNDMGR_FILE, 0x422, hr);
        result = NULL;
        goto done;
    }
    if (directSoundBuffer == NULL) {
        result = NULL;
        goto done;
    }

    {
        SoundSample* sample = new SoundSample(directSoundBuffer, this);
        sample->m_baseFrequency = bufferFormat.nSamplesPerSec;
        m_samples.InsertFirst(sample);
        sample->m_baseSampleRate = format->nAvgBytesPerSec;
        sample->m_sampleRate = format->nAvgBytesPerSec;
        sample->m_sampleCount = bytes;
        sample->UpdateDuration();
        result = sample;
    }
done:
    return result;
}

SoundSample* SoundDevice::LoadSampleFile(char* path, u32 flags, u32 loadOptions) {
    if (m_initialized == false) {
        return NULL;
    }
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        return NULL;
    }
    u32 size = _filelength(file->_file);
    u8* bytes = new u8[size];
    if (fread(bytes, size, 1, file) != 1) {
        fclose(file);
        delete[] bytes;
        return NULL;
    }
    fclose(file);
    RecordBytes<RiffWaveHeader> riff;
    riff.m_bytes = bytes;
    SoundSample* sample = LoadSample(riff.m_rec, flags, loadOptions);
    delete[] bytes;
    return sample;
}

SoundSample* SoundDevice::LoadSample(RiffWaveHeader* riff, u32 flags, u32 loadOptions) {
    if (m_initialized == false) {
        return NULL;
    }
    if (riff == NULL) {
        return NULL;
    }

    u8* data;
    u32 dataBytes;
    WAVEFORMATEX* format;
    format = NULL;
    data = NULL;
    dataBytes = 0;
    if (ParseWaveChunks(riff, &format, &data, &dataBytes) == 0) {
        return NULL;
    }

    b32 convert16To8 = false;
    if (m_force8Bit != false || (loadOptions & 1) == 1) {
        convert16To8 = true;
    }
    if (format->wBitsPerSample != sizeof(i16) * 8 || format->wFormatTag != WAVE_FORMAT_PCM) {
        convert16To8 = false;
    }
    if (convert16To8) {
        dataBytes >>= 1;
        format->wBitsPerSample = 8;
        format->nAvgBytesPerSec >>= 1;
        format->nBlockAlign >>= 1;
    }

    SoundSample* sample = CreateSample(format, dataBytes, flags);
    if (sample == NULL) {
        return NULL;
    }
    if (sample->LockConvert(data, dataBytes, convert16To8) == 0) {
        DestroyBuffer(sample);
        return NULL;
    }
    return sample;
}

SoundSample* SoundDevice::LoadSampleResource(const char* name, u32 flags, u32 loadOptions) {
    if (m_initialized == false) {
        return NULL;
    }

    HINSTANCE mod1 = AfxGetInstanceHandle();
    HRSRC hRsrc = FindResourceA(mod1, name, "WAVE");
    if (!hRsrc) {
        return NULL;
    }
    HINSTANCE mod2 = AfxGetInstanceHandle();
    HGLOBAL hRes = LoadResource(mod2, hRsrc);
    if (!hRes) {
        return NULL;
    }
    RiffWaveHeader* data = static_cast<RiffWaveHeader*>(LockResource(hRes));
    if (!data) {
        return NULL;
    }
    return LoadSample(data, flags, loadOptions);
}

i32 SoundDevice::ValidateRestore(SoundBuffer* buffer, WAVEFORMATEX* format, u32 formatBytes) {
    if (m_initialized == false) {
        return 0;
    }
    if (formatBytes == 0) {
        return 0;
    }
    if (format == NULL) {
        return 0;
    }
    if (format->wFormatTag != WAVE_FORMAT_PCM) {
        return 0;
    }
    return buffer->Restore() != 0;
}

i32 SoundDevice::ReloadFile(SoundBuffer* buffer, char* path, u32 loadOptions) {
    if (m_initialized == false) {
        return 0;
    }
    if (buffer->IsLooping() == 0) {
        return 1;
    }
    FILE* fp = fopen(path, "rb");
    if (fp == NULL) {
        return 0;
    }
    u32 size = _filelength(fp->_file);
    u8* data = new u8[size];
    if (fread(data, size, 1, fp) != 1) {
        fclose(fp);
        delete[] data;
        return 0;
    }
    fclose(fp);
    RecordBytes<RiffWaveHeader> riff;
    riff.m_bytes = data;
    i32 result = ReloadRiff(buffer, riff.m_rec, loadOptions);
    delete[] data;
    return result;
}

i32 SoundDevice::ReloadRiff(SoundBuffer* buffer, RiffWaveHeader* riff, u32 loadOptions) {
    if (m_initialized == false) {
        return 0;
    }
    if (riff == NULL) {
        return 0;
    }
    if (buffer->IsLooping() == 0) {
        return 1;
    }

    u8* data;
    u32 dataBytes;
    WAVEFORMATEX* format;
    format = NULL;
    data = NULL;
    dataBytes = 0;
    if (ParseWaveChunks(riff, &format, &data, &dataBytes) == 0) {
        return 0;
    }

    b32 convert16To8 = false;
    if (m_force8Bit != false || (loadOptions & 1) == 1) {
        convert16To8 = true;
    }
    if (format->wBitsPerSample != sizeof(i16) * 8 || format->wFormatTag != WAVE_FORMAT_PCM) {
        convert16To8 = false;
    }
    if (convert16To8) {
        dataBytes >>= 1;
        format->wBitsPerSample = 8;
        format->nAvgBytesPerSec >>= 1;
        format->nBlockAlign >>= 1;
    }

    if (ValidateRestore(buffer, format, dataBytes) == 0) {
        return 0;
    }
    return buffer->LockConvert(data, dataBytes, convert16To8) != 0;
}

i32 SoundDevice::ReloadResource(SoundBuffer* buffer, const char* name, u32 loadOptions) {
    if (m_initialized == false) {
        return 0;
    }
    if (buffer->IsLooping() == 0) {
        return 1;
    }

    HINSTANCE mod1 = AfxGetInstanceHandle();
    HRSRC hRsrc = FindResourceA(mod1, name, "WAVE");
    if (!hRsrc) {
        return 0;
    }
    HINSTANCE mod2 = AfxGetInstanceHandle();
    HGLOBAL hRes = LoadResource(mod2, hRsrc);
    if (!hRes) {
        return 0;
    }
    RiffWaveHeader* data = static_cast<RiffWaveHeader*>(LockResource(hRes));
    if (!data) {
        return 0;
    }
    return ReloadRiff(buffer, data, loadOptions);
}

void SoundDevice::DestroyBuffer(SoundBuffer* buffer) {
    if (m_initialized) {

        m_volumeRamps.RemoveMatching(buffer, SOUND_TASK_TAG_ALL);
        SAFE_RELEASE(buffer->m_buffer);
        m_samples.Delete(buffer);
        if (buffer) {
            delete buffer;
        }
    }
}

void SoundDevice::StopAllBuffers() {
    if (m_initialized) {
        SoundSample* node = m_samples.GetFirst();
        while (node) {
            node->StopAndRewind();
            node->StopAllInstances();
            node = static_cast<SoundSample*>(node->Next());
        }
    }
}

i32 SoundDevice::TickVolumeRamps(i32 timestampMs) {
    if (m_initialized == false) {
        return 0;
    }
    SoundTask* ramp = m_volumeRamps.GetFirst();
    if (ramp == NULL) {
        return 0;
    }
    if (timestampMs == -1) {
        timestampMs = static_cast<i32>(timeGetTime());
    }
    if (static_cast<u32>(timestampMs) <= static_cast<u32>(m_lastRampTickMs)) {
        return 1;
    }
    m_lastRampTickMs = timestampMs;
    do {
        CBaseListItem* nextLink = ramp->Next();
        SoundTask* next = static_cast<SoundTask*>(nextLink);
        if (ramp->Tick(timestampMs) == 0) {
            m_volumeRamps.Delete(ramp);
            if (ramp) {
                SoundTask* task = ramp;
                delete task;
            }
        }
        ramp = next;
    } while (ramp);
    return 1;
}

i32 SoundDevice::ClearVolumeRamps() {
    if (m_initialized == false) {
        return 0;
    }
    SoundTask* node = m_volumeRamps.GetFirst();
    if (node == NULL) {
        return 1;
    }
    do {
        CBaseListItem* nextLink = node->Next();
        SoundTask* next = static_cast<SoundTask*>(nextLink);
        node->Stop();
        m_volumeRamps.Delete(node);
        if (node) {

            SoundTask* task = node;
            delete task;
        }
        node = next;
    } while (node);
    return 1;
}

i32 SoundDeviceReturnFalse() {
    return 0;
}

void SoundTaskList::RemoveMatching(SoundBuffer* buffer, u32 tag) {
    SoundTask* task = static_cast<SoundTask*>(m_pFirst);
    while (task) {
        CBaseListItem* link = task;
        CBaseListItem* nextLink = task->Next();
        SoundTask* next = static_cast<SoundTask*>(nextLink);
        if (tag != SOUND_TASK_TAG_ALL && task->m_tag != tag) {
            continue;
        }
        if (task->m_buffer == buffer) {

            Delete(task);
            if (task) {
                SoundTask* removed = task;
                delete removed;
            }
        }
        task = next;
    }
}

SoundVolumeRamp::SoundVolumeRamp(
    i32 targetVolumePct,
    i32 initialVolumePct,
    i32 durationMs,
    SoundBuffer* buffer,
    b32 stopAndRewind,
    i32 startTime
)
    : SoundTask(SOUND_TASK_TAG_VOLUME_RAMP, buffer, stopAndRewind) {
    m_targetVolumePct = targetVolumePct;
    m_initialVolumePct = initialVolumePct;
    m_rampDurationMs = durationMs;
    m_rampStartTime = (startTime == -1) ? timeGetTime() : startTime;
}

i32 SoundVolumeRamp::Tick(i32 timestampMs) {
    b32 done = false;
    timestampMs -= m_rampStartTime;
    if (static_cast<u32>(timestampMs) >= static_cast<u32>(m_rampDurationMs)) {
        timestampMs = m_rampDurationMs;
        done = true;
    }
    if (m_buffer->IsPlaying() == 0) {
        done = true;
    } else {
        i32 volumePct = (m_targetVolumePct - m_initialVolumePct) * timestampMs / m_rampDurationMs
                        + m_initialVolumePct;
        m_buffer->SetVolumePercent(volumePct);
    }
    if (done && m_stopAndRewind != false) {
        m_buffer->StopAndRewind();
    }
    return done == false;
}

i32 SoundVolumeRamp::Stop() {
    if (m_buffer->IsPlaying() != 0) {
        if (m_stopAndRewind != false) {
            m_buffer->StopAndRewind();
            return 1;
        }
        m_buffer->SetVolumePercent(m_targetVolumePct);
    }
    return 1;
}

i32 ParseWaveChunks(
    RiffWaveHeader* riff,
    WAVEFORMATEX** outFormat,
    u8** outData,
    u32* outDataBytes
) {
    RecordBytes<RiffWaveHeader> cursor;
    cursor.m_rec = riff;
    u32 riffTag = static_cast<u32>(*cursor.m_dwords);
    cursor.m_bytes += 4;
    u32 riffSize = static_cast<u32>(*cursor.m_dwords);
    cursor.m_bytes += 4;
    u32 waveTag = static_cast<u32>(*cursor.m_dwords);
    cursor.m_bytes += 4;
    u8* end = cursor.m_bytes + riffSize - 4;
    if (riffTag != mmioFOURCC('R', 'I', 'F', 'F')) {
        return 0;
    }
    if (waveTag != mmioFOURCC('W', 'A', 'V', 'E')) {
        return 0;
    }
    *outFormat = NULL;
    *outData = NULL;
    while (cursor.m_bytes < end) {
        u32 id = static_cast<u32>(*cursor.m_dwords);
        cursor.m_bytes += 4;
        u32 size = static_cast<u32>(*cursor.m_dwords);
        cursor.m_bytes += 4;
        if (id == mmioFOURCC('f', 'm', 't', ' ')) {
            RecordBytes<WAVEFORMATEX> fmtView;
            fmtView.m_bytes = cursor.m_bytes;
            *outFormat = fmtView.m_rec;
        } else if (id == mmioFOURCC('d', 'a', 't', 'a')) {
            *outData = cursor.m_bytes;
            *outDataBytes = size;
            return *outFormat != NULL;
        }
        cursor.m_bytes += (size + 1) & ~1;
    }
    return 0;
}

i32 SoundDevice::SetPrimaryFormat(WAVEFORMATEX* format) {
    if (m_initialized == false) {
        return 0;
    }
    if (CreatePrimaryBuffer() == 0) {
        return 0;
    }
    b32 hr = m_primaryBuffer->SetFormat(format) != 0;
    if (hr) {
        SoundBuffer::ReportError(DSNDMGR_FILE, 0x678, hr);
        return 0;
    }
    return 1;
}

i32 SoundDevice::StartPrimaryBuffer() {
    if (m_initialized == false) {
        return 0;
    }
    if (CreatePrimaryBuffer() == 0) {
        return 0;
    }
    b32 hr = m_primaryBuffer->Play(0, 0, DSBPLAY_LOOPING) != 0;
    if (hr) {
        SoundBuffer::ReportError(DSNDMGR_FILE, 0x68b, hr);
        return 0;
    }
    return 1;
}

i32 SoundDevice::CreatePrimaryBuffer() {
    if (m_initialized == false) {
        return 0;
    }

    if (m_cooperativeLevel == DSSCL_NORMAL) {
        return 0;
    }
    if (m_primaryBuffer == NULL) {
        DSBUFFERDESC desc;
        memset(&desc, 0, sizeof(desc));
        desc.dwSize = sizeof(DSBUFFERDESC);
        desc.dwFlags = m_bufferFlags | DSBCAPS_PRIMARYBUFFER;
        b32 hr = m_device->CreateSoundBuffer(&desc, &m_primaryBuffer, NULL) != 0;
        if (hr) {
            SoundBuffer::ReportError(DSNDMGR_FILE, 0x6ab, hr);
            return 0;
        }
    }
    return 1;
}

IDirectSoundBuffer* SoundDevice::GetPrimary() {
    if (m_initialized == false) {
        return NULL;
    }
    if (CreatePrimaryBuffer() == 0) {
        return NULL;
    }
    return m_primaryBuffer;
}
