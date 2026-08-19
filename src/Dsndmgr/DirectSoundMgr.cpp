#include <rva.h>

#include <Dsndmgr/DirectSoundMgr.h>

#include <Mfc.h>

#include <ComOutRef.h>
#include <Dsndmgr/DSoundVoice.h>
#include <Dsndmgr/SoundDevice.h>
#include <Dsndmgr/SoundVoiceList.h>
#include <Dsndmgr/VolumeScale.h>
#include <Dsndmgr/WaveFormatSdk.h>
#include <Enums.h>
#include <Pix16.h>
#include <Rez/RezMgr.h>
#include <Wap32/Wap32.h>

#include <dsound.h>
#include <io.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

DATA(0x001ef698)
const double c_volScale = 100.0;
DATA(0x001ef6a0)
const double c_volNum = 1.0;
DATA(0x001ef6a8)
const double c_powExp = 10.0;
DATA(0x001ef6b0)
const double c_acosNorm = 2.0;

#define DSNDMGR_FILE "C:\\Proj\\Dsndmgr\\DSNDMGR.CPP"

typedef enum DSoundDx5Magic {
    DSB_RETAIL_LOOPBIT = 0x02,
    DSBUFFERDESC_SIZE = 0x14,
} DSoundDx5Magic;

DATA(0x00253ab8)
i32 g_volumeTable[VOLUME_PCT_MAX];
// NOT a one-element table - a BACKWARD CURSOR into g_volumeTable's tail.
// g_volumeTable[100] ends exactly here (0x253ab8 + 400 = 0x253c48), and
// SetPanByIndex reads g_panTable[-idx] for idx in 0..100, i.e.
// g_volumeTable[100 - idx] - the pan attenuation reuses the volume curve read
// from the top down. The `[1]` is the anchor slot only (it aliases the
// documented g_volumeTable[100] overrun, see VolumeScale.h); the storage read
// is g_volumeTable's. `[1]` is the CORRECT reserved size, PROVEN not assumed:
// the next retail symbol is _g_ssLogEnabled at 0x253c4c, exactly 4 bytes on, so
// retail's own cl emitted a 4-byte .bss slot here too. Widening it would
// double-reserve g_volumeTable's aliased slots AND overlap _g_ssLogEnabled.
// This is the ONE undercount case that is genuinely byte-neutral (a too-small
// .bss array with its OWN forward storage is NOT neutral - it shifts every
// symbol after it). Flagged as the `undercount` class by
// gruntz.audit.data_access_map, kept as-is with this alias documented.
DATA(0x00253c48)
i32 g_panTable[1];

#pragma optimize("", off)

RVA(0x001350b0, 0x5d)
i32 SoundDevice::VolumeToAttenuation(i32 value) {
    if (value == VOLUME_PCT_MAX) {
        return 0;
    }
    if (value == 0) {
        return -10000;
    }

    double ratio = acos(pow(c_volNum / (value / c_volScale), c_powExp)) / acos(c_acosNorm);
    return static_cast<i32>((-(ratio * c_volScale)));
}

RVA(0x00135110, 0x8e)
i32 ConvertVolumeToPercent(i32 v) {
    if (v == 0) {
        return VOLUME_PCT_MAX;
    }
    double d;
    if (v < 0) {
        d = static_cast<double>((-v / 100));
    } else {
        d = static_cast<double>((v / 100));
    }
    double r = c_volScale - (c_volNum - pow(c_acosNorm, -d / c_powExp)) * c_volScale;
    if (v < 0) {
        return static_cast<i32>(r);
    }
    return static_cast<i32>((-r));
}

#pragma optimize("", on)

RVA(0x001351a0, 0x23)
void SoundDevice::BuildVolumeTable() {
    for (i32 i = 0; i <= VOLUME_PCT_MAX; i++) {
        g_volumeTable[i] = VolumeToAttenuation(i);
    }
}

RVA(0x001351d0, 0x109)
DirectSoundMgr::DirectSoundMgr(IDirectSoundBuffer* buf, SoundDevice* owner) {

    m_buffer = buf;
    m_owner = owner;
    m_playFlags = 0;
    m_durationMs = 0;
    m_reacquireCb = NULL;
    m_reacquireCtx = 0;
    m_rateBase = 0;
    m_sampleRate = 0;
    if (buf == NULL) {
        return;
    }

    DSBCAPS caps;
    caps.dwSize = sizeof(DSBCAPS);
    if (buf->GetCaps(&caps) == 0) {
        m_caps = caps.dwFlags;
    } else {
        m_caps = 0;
    }

    if ((m_caps & DSBCAPS_CTRLFREQUENCY) == DSBCAPS_CTRLFREQUENCY) {
        i32 hr = buf->GetFrequency(&m_freq) != 0;
        if (hr) {
            GetErrorString(DSNDMGR_FILE, 0x58, hr);
        }
    }
    m_setFreq = m_freq;

    if ((m_caps & DSBCAPS_CTRLPAN) == DSBCAPS_CTRLPAN) {
        i32 hr = buf->GetPan(&m_pan) != 0;
        if (hr) {
            GetErrorString(DSNDMGR_FILE, 0x60, hr);
        }
    } else {
        m_pan = 0;
    }

    if ((m_caps & DSBCAPS_CTRLVOLUME) == DSBCAPS_CTRLVOLUME) {
        i32 hr = buf->GetVolume(&m_volume) != 0;
        if (hr) {
            GetErrorString(DSNDMGR_FILE, 0x68, hr);
        }
    } else {
        m_volume = 0;
    }
}

RVA_COMPGEN(0x001352e0, 0x1e, ??_GDirectSoundMgr@@UAEPAXI@Z)

RVA(0x00135300, 0x7)
DirectSoundMgr::~DirectSoundMgr() {}

RVA(0x00135310, 0x2a)
i32 DirectSoundMgr::Restore() {
    i32 hr = m_buffer->Restore() != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x7b, hr);
        return 0;
    }
    return 1;
}

RVA(0x00135340, 0x37)
i32 DirectSoundMgr::ReacquireBuffer() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    if (m_reacquireCb != NULL) {
        if (m_reacquireCb(this, m_reacquireCtx) != 0) {
            return 1;
        }
    }
    return m_owner->ReacquireViaCallback();
}

RVA(0x00135380, 0x66)
i32 DirectSoundMgr::StopAndRewind() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 hr = m_buffer->Stop() != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x99, hr);
        return 0;
    }
    hr = m_buffer->SetCurrentPosition(0) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x9e, hr);
    }
    return 1;
}

RVA(0x001353f0, 0x4b)
i32 DirectSoundMgr::IsPlaying() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    DWORD status;
    i32 hr = m_buffer->GetStatus(&status) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0xac, hr);
        return 0;
    }
    if ((status & DSBSTATUS_PLAYING) == DSBSTATUS_PLAYING) {
        return 1;
    }
    return 0;
}

RVA(0x00135440, 0x4d)
i32 DirectSoundMgr::IsLooping() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    DWORD status;
    i32 hr = m_buffer->GetStatus(&status) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0xbb, hr);
        return 0;
    }
    if ((status & DSB_RETAIL_LOOPBIT) == DSB_RETAIL_LOOPBIT) {
        return 1;
    }
    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00135490, 0x73)
i32 DirectSoundMgr::IsInHardware() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    DSBCAPS caps;
    memset(&caps, 0, sizeof(caps));
    caps.dwSize = sizeof(DSBCAPS);
    i32 hr = m_buffer->GetCaps(&caps) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0xcc, hr);
        return 0;
    }
    if ((caps.dwFlags & DSBCAPS_LOCHARDWARE) == DSBCAPS_LOCHARDWARE) {
        return 1;
    }
    return 0;
}

RVA(0x00135510, 0x25)
void DirectSoundMgr::SetLooping(i32 enabled) {
    if (m_owner->m_initialized == 0) {
        return;
    }
    if (enabled) {
        m_playFlags |= 1;
    } else {
        m_playFlags &= ~1;
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00135540, 0x1a)
i32 DirectSoundMgr::IsLoopingEnabled() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    if ((m_playFlags & 1) == 1) {
        return 1;
    }
    return 0;
}

RVA(0x00135560, 0x58)
i32 DirectSoundMgr::SetVolume(i32 vol) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    if ((m_caps & DSBCAPS_CTRLVOLUME) != DSBCAPS_CTRLVOLUME) {
        return 0;
    }
    i32 hr = m_buffer->SetVolume(vol) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0xf6, hr);
        return 0;
    }
    return 1;
}

RVA(0x001355c0, 0x23)
i32 DirectSoundMgr::SetVolumeByIndex(i32 idx) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    return SetVolume(g_volumeTable[idx]);
}

RVA(0x001355f0, 0x42)
i32 DirectSoundMgr::GetVolume() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    long vol;
    i32 hr = m_buffer->GetVolume(&vol) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x10e, hr);
        return 0;
    }
    return vol;
}

RVA(0x00135640, 0x1c)
i32 DirectSoundMgr::GetVolumePercent() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    return ConvertVolumeToPercent(GetVolume());
}

RVA(0x00135660, 0xe0)
i32 DirectSoundMgr::CloneAndPlay(i32 key, i32 mode, i32 slot) {
    SoundDevice* owner = m_owner;
    if (owner->m_initialized == 0) {
        return 0;
    }
    owner->m_voiceList.RemoveMatching(this, 1);

    if (mode == 0) {
        SetVolumeByIndex(key);
        return 1;
    }

    DSoundVoice* voice = new DSoundVoice(key, GetVolumePercent(), mode, this, slot, -1);
    if (voice == NULL) {
        return 0;
    }
    m_owner->m_voiceList.InsertHead(&voice->m_link);
    return 1;
}

RVA(0x00135740, 0x55)
i32 DirectSoundMgr::SetPan(i32 pan) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    if ((m_caps & DSBCAPS_CTRLPAN) != DSBCAPS_CTRLPAN) {
        return 0;
    }
    i32 hr = m_buffer->SetPan(pan) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x141, hr);
        return 0;
    }
    return 1;
}

RVA(0x001357a0, 0x42)
i32 DirectSoundMgr::SetPanByIndex(i32 idx) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    if (idx >= 0) {
        return SetPan(-g_panTable[-idx]);
    }
    return SetPan(g_panTable[idx]);
}

RVA(0x001357f0, 0x42)
i32 DirectSoundMgr::GetPan() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    long pan;
    i32 hr = m_buffer->GetPan(&pan) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x15e, hr);
        return 0;
    }
    return pan;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00135840, 0x3b)
i32 DirectSoundMgr::GetPanPercent() {
    if (m_owner->m_initialized == 0) {
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

RVA(0x00135880, 0x60)
i32 DirectSoundMgr::SetFrequency(u32 freq) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    if ((m_caps & DSBCAPS_CTRLFREQUENCY) != DSBCAPS_CTRLFREQUENCY) {
        return 0;
    }
    i32 hr = m_buffer->SetFrequency(freq);
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x180, hr);
        return 0;
    }
    m_setFreq = freq;
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001358e0, 0x11)
u32 DirectSoundMgr::GetFrequency() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    return m_setFreq;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00135900, 0x11)
u32 DirectSoundMgr::GetBaseFrequency() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    return m_freq;
}

RVA(0x00135920, 0x80)
i32 DirectSoundMgr::SetFrequencyOffsetPercent(i32 percentOffset) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 v = percentOffset * static_cast<i32>(m_freq) / 100 + static_cast<i32>(m_freq);
    if (static_cast<u32>(v) >= DSOUND_FREQUENCY_MAX) {
        v = DSOUND_FREQUENCY_MAX - 1;
    }
    if (static_cast<u32>(v) <= DSOUND_FREQUENCY_MIN) {
        v = DSOUND_FREQUENCY_MIN + 1;
    }
    i32 r = SetFrequency(v);
    m_sampleRate = percentOffset * m_rateBase / 100 + m_rateBase;
    ComputeDuration();
    return r;
}

RVA(0x001359a0, 0x18)
void DirectSoundMgr::ComputeDuration() {
    m_durationMs = m_sampleCount * 1000 / m_sampleRate;
}

RVA(0x001359c0, 0x54)
i32 DirectSoundMgr::Unlock(u8* audioPtr1, u32 audioBytes1, u8* audioPtr2, u32 audioBytes2) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 hr = m_buffer->Unlock(audioPtr1, audioBytes1, audioPtr2, audioBytes2) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x1bb, hr);
        return 0;
    }
    return 1;
}

RVA(0x00135a20, 0x4a)
i32 DirectSoundMgr::GetCurrentPosition(DWORD* play, DWORD* write) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 hr = m_buffer->GetCurrentPosition(play, write) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x1c8, hr);
        return 0;
    }
    return 1;
}

RVA(0x00135a70, 0x45)
i32 DirectSoundMgr::SetCurrentPosition(u32 pos) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 hr = m_buffer->SetCurrentPosition(pos) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x1d5, hr);
        return 0;
    }
    return 1;
}

RVA(0x00135ac0, 0x4f)
i32 DirectSoundMgr::GetFormat(WaveFormatX* fmt, u32 size, DWORD* written) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 hr = m_buffer->GetFormat(WaveFormatSdk(fmt), size, written) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x1e2, hr);
        return 0;
    }
    return 1;
}

RVA(0x00135b10, 0x6b)
DSoundCloneInst::DSoundCloneInst(IDirectSoundBuffer* buf, SoundDevice* owner)
    : DSoundBaseSub(buf, owner) {

    ((&m_cloneList))->InsertHead(&m_cloneNode);
    m_playKey = 1;
}

RVA_COMPGEN(0x00135b80, 0x1e, ??_GDSoundCloneInst@@UAEPAXI@Z)

RVA(0x00135bb0, 0x63)
DSoundCloneInst::~DSoundCloneInst() {
    while (m_cloneList.m_head != NULL) {
        RemoveClone(static_cast<CloneNode*>(m_cloneList.m_head)->m_inst);
    }
}

RVA(0x00135c20, 0xf6)
DirectSoundMgr* DSoundCloneInst::Clone(i32 a) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    DSoundBaseSub* clone = new DSoundBaseSub(m_buffer, m_owner, this);
    if (clone == NULL) {
        return 0;
    }
    IDirectSound* dev = m_owner->m_device;
    i32 hr = dev->DuplicateSoundBuffer(m_buffer, &clone->m_buffer) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x217, hr);
        return 0;
    }
    ((&m_cloneList))->InsertHead(&clone->m_cloneNode);
    clone->m_playKey = a;
    return clone;
}

RVA(0x00135d20, 0x47)
void DSoundCloneInst::RemoveClone(DirectSoundMgr* clone) {
    if (m_owner->m_initialized == 0) {
        return;
    }
    if (clone != this) {
        IDirectSoundBuffer* buf = clone->m_buffer;
        buf->Release();
        clone->m_buffer = NULL;
    }
    ((&m_cloneList))->Unlink(&clone->m_cloneNode);
    if (clone != this) {
        delete clone;
    }
}

RVA(0x00135d70, 0x92)
DirectSoundMgr* DSoundCloneInst::GetItem() {
    if (!m_owner->m_initialized) {
        return 0;
    }
    CloneNode* node = static_cast<CloneNode*>(m_cloneList.m_head);
    if (node) {
        while (1) {
            if (node->m_inst->m_playKey && node->m_inst->IsPlaying() == 0) {
                break;
            }
            node = static_cast<CloneNode*>(node->m_next);
            if (!node) {
                break;
            }
        }
    }
    DirectSoundMgr* found;
    if (!node) {
        found = NULL;
    } else {
        found = node->m_inst;
    }
    if (found) {
        found->SetVolume(m_volume);
        found->SetPan(m_pan);
        found->SetFrequency(m_freq);
    }
    if (!found) {
        found = Clone(1);
    }
    if (found) {
        m_cloneList.Unlink(&found->m_cloneNode);
        m_cloneList.InsertTail(&found->m_cloneNode);
    }
    return found;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00135e10, 0x124)
i32 DirectSoundMgr::LoadFromFile(FILE* fp, u32 bytes, i32 offset) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    if (offset != -1) {
        if (fseek(fp, offset, SEEK_SET) != 0) {
            return 0;
        }
    }

    u8* audioPtr1 = NULL;
    DWORD audioBytes1;
    u8* audioPtr2 = NULL;
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
        GetErrorString(DSNDMGR_FILE, 0x27c, hr);
        return 0;
    }

    if (audioBytes1 > 0) {
        if (fread(audioPtr1, audioBytes1, 1, fp) != 1) {
            return 0;
        }
    }
    if (audioBytes2 > 0) {
        if (fread(audioPtr2, audioBytes2, 1, fp) != 1) {
            return 0;
        }
    }

    hr = m_buffer->Unlock(audioPtr1, audioBytes1, audioPtr2, audioBytes2);
    if (hr != 0) {
        GetErrorString(DSNDMGR_FILE, 0x295, hr);
        return 0;
    }
    return 1;
}

RVA(0x00135f40, 0x169)
i32 DirectSoundMgr::LockConvert(u8* src, u32 lockBytes, u32 convert) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }

    u8* audioPtr1 = NULL;
    u8* audioPtr2 = NULL;
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
        GetErrorString(DSNDMGR_FILE, 0x2bd, hr);
        return 0;
    }

    if (convert == 0) {

        if (audioBytes1 > 0) {
            memcpy(audioPtr1, src, audioBytes1);
        }
        if (audioBytes2 > 0) {
            memcpy(audioPtr2, src + audioBytes1, audioBytes2);
        }
    } else {

        if (audioBytes1 > 0) {
            u8* d = audioPtr1;
            Pix16Ptr samples;
            samples.m_bytes = src;
            i16* s = samples.m_swords;
            u8* end = audioPtr1 + audioBytes1;
            while (d < end) {
                *d = static_cast<u8>((static_cast<u32>((*s + 0x8000)) >> 8));
                ++s;
                ++d;
            }
        }
        if (audioBytes2 > 0) {
            u8* d = audioPtr2;

            Pix16Ptr samples;
            samples.m_bytes = src + audioBytes1;
            i16* s = samples.m_swords;
            u8* end = audioPtr2 + audioBytes2;
            while (d < end) {
                *d = static_cast<u8>((static_cast<u32>((*s + 0x8000)) >> 8));
                ++s;
                ++d;
            }
        }
    }

    hr = m_buffer->Unlock(audioPtr1, audioBytes1, audioPtr2, audioBytes2) != 0;
    if (hr) {
        GetErrorString(DSNDMGR_FILE, 0x2e1, hr);
        return 0;
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001360b0, 0x1e)
i32 DSoundCloneInst::Play() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    DirectSoundMgr* item = GetItem();
    if (item == NULL) {
        return 0;
    }
    return item->Play();
}

RVA(0x001360d0, 0x7e)
i32 DSoundCloneInst::ConfigureItem(i32 vol, i32 pan, i32 freqPct, i32 loop) {
    if (!m_owner->m_initialized) {
        return 0;
    }
    DirectSoundMgr* item = GetItem();
    if (!item) {
        return 0;
    }
    i32 ok = 1;
    if (!item->SetVolumeByIndex(vol)) {
        ok = 0;
    }
    if (!item->SetPanByIndex(pan)) {
        ok = 0;
    }
    if (!item->SetFrequencyOffsetPercent(freqPct)) {
        ok = 0;
    }
    item->SetLooping(loop);
    if (!item->Play()) {
        ok = 0;
    }
    return ok;
}

RVA(0x00136150, 0x22)
void DSoundCloneInst::StopAllClones() {
    if (m_owner->m_initialized == 0) {
        return;
    }
    for (CloneNode* node = static_cast<CloneNode*>(m_cloneList.m_head); node != NULL;
         node = static_cast<CloneNode*>(node->m_next)) {
        node->m_inst->StopAndRewind();
    }
}

RVA(0x00136180, 0x86)
DSoundBaseSub::DSoundBaseSub(IDirectSoundBuffer* buf, SoundDevice* owner, DirectSoundMgr* original)
    : DirectSoundMgr(buf, owner) {
    m_cloneNode.m_inst = this;
    m_reacquireOwner = original;
    m_playKey = 1;
    m_sampleCount = original->m_sampleCount;
    m_reacquireCb = original->m_reacquireCb;
    m_reacquireCtx = original->m_reacquireCtx;
    m_sampleRate = original->m_sampleRate;
    m_rateBase = original->m_rateBase;
    ComputeDuration();
}

RVA_COMPGEN(0x00136210, 0x1e, ??_GDSoundBaseSub@@UAEPAXI@Z)

RVA(0x00136230, 0x2d)
DSoundBaseSub::DSoundBaseSub(IDirectSoundBuffer* buf, SoundDevice* owner)
    : DirectSoundMgr(buf, owner) {

    m_cloneNode.m_inst = this;
    m_reacquireOwner = this;
    m_playKey = 1;
}

RVA(0x00136260, 0xb)
DSoundBaseSub::~DSoundBaseSub() {}

RVA(0x00136270, 0x8b)
i32 DirectSoundMgr::Play() {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 hr = m_buffer->Play(0, 0, m_playFlags) != 0;
    if (hr != 0) {
        if (hr == DSERR_BUFFERLOST) {
            if (m_reacquireOwner->ReacquireBuffer() != 0) {
                i32 hr2 = m_buffer->Play(0, 0, m_playFlags) != 0;
                if (hr2 == 0) {
                    return 1;
                }
                GetErrorString(DSNDMGR_FILE, 0x34c, hr2);
                return 0;
            }
        } else {
            GetErrorString(DSNDMGR_FILE, 0x356, hr);
        }
        return 0;
    }
    return 1;
}

RVA(0x00136300, 0x6f)
i32 DirectSoundMgr::ApplyAndPlay(i32 vol, i32 pan, i32 freqPct, i32 loop) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 ok = 1;
    if (SetVolumeByIndex(vol) == 0) {
        ok = 0;
    }
    if (SetPanByIndex(pan) == 0) {
        ok = 0;
    }
    if (SetFrequencyOffsetPercent(freqPct) == 0) {
        ok = 0;
    }
    SetLooping(loop);
    if (Play() == 0) {
        ok = 0;
    }
    return ok;
}

RVA(0x00136370, 0xcc)
i32 DirectSoundMgr::Lock(
    u32 off,
    u32 bytes,
    u8** audioPtr1,
    DWORD* audioBytes1,
    u8** audioPtr2,
    DWORD* audioBytes2,
    u32 flags
) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 hr = m_buffer->Lock(
                 off,
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
            if (m_reacquireOwner->ReacquireBuffer() != 0) {
                hr = m_buffer->Lock(
                         off,
                         bytes,
                         PtrOut(audioPtr1),
                         audioBytes1,
                         PtrOut(audioPtr2),
                         audioBytes2,
                         flags
                     )
                     != 0;
                if (hr != 0) {
                    GetErrorString(DSNDMGR_FILE, 0x37c, hr);
                    return 0;
                }
            } else {
                return 0;
            }
        } else {
            GetErrorString(DSNDMGR_FILE, 0x386, hr);
            return 0;
        }
    }
    return 1;
}

RVA(0x00136440, 0x74)
SoundDevice::SoundDevice() {

    m_initialized = 0;
    BuildVolumeTable();
    m_reacquireProc = 0;
    m_primaryBuffer = NULL;
    m_coopLevel = 0;
    m_bufferFlags = 0;
    m_force8Bit = 0;
}

RVA_COMPGEN(0x001364c0, 0x1e, ??_GSoundDevice@@UAEPAXI@Z)

RVA(0x00136500, 0x43)
SoundDevice::~SoundDevice() {

    if (m_initialized) {
        Shutdown();
    }
}

RVA(0x00136550, 0x8c)
i32 SoundDevice::Create(HWND hwnd, u32 level, u32 flags) {
    i32 created = DirectSoundCreate(0, &m_device, 0) != 0;
    if (created) {
        return 0;
    }
    i32 hr = m_device->SetCooperativeLevel(hwnd, level) != 0;
    if (hr) {
        DirectSoundMgr::GetErrorString(DSNDMGR_FILE, 0x3b0, hr);
        m_device->Release();
        return 0;
    }
    m_coopLevel = level;
    m_bufferFlags = flags;
    m_createFlag = 0;
    m_initialized = 1;
    return 1;
}

RVA(0x001365e0, 0xf)
i32 SoundDevice::ReacquireViaCallback() {
    if (m_reacquireProc != 0) {
        return (this->*m_reacquireProc)();
    }
    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001365f0, 0x57)
i32 SoundDevice::SetCooperativeLevel(HWND hwnd, u32 level) {
    if (m_initialized == 0) {
        return 0;
    }
    i32 hr = m_device->SetCooperativeLevel(hwnd, level) != 0;
    if (hr) {
        DirectSoundMgr::GetErrorString(DSNDMGR_FILE, 0x3cf, hr);
        return 0;
    }
    m_coopLevel = level;
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00136650, 0x37)
i32 SoundDevice::Compact() {
    if (m_initialized == 0) {
        return 0;
    }
    i32 hr = m_device->Compact() != 0;
    if (hr) {
        DirectSoundMgr::GetErrorString(DSNDMGR_FILE, 0x3dc, hr);
        return 0;
    }
    return 1;
}

RVA(0x00136690, 0x58)
void SoundDevice::Shutdown() {
    if (m_initialized) {
        DSoundCloneInst* node = elemOf<DSoundCloneInst>(m_bufferList.m_head);
        while (node) {
            RemoveBuffer(node);
            node = elemOf<DSoundCloneInst>(m_bufferList.m_head);
        }
        if (m_primaryBuffer) {
            m_primaryBuffer->Release();
        }
        m_device->Release();
    }
    m_initialized = 0;
}

RVA(0x001366f0, 0x168)
DSoundCloneInst* SoundDevice::CreateBuffer(WaveFormatX* fmt, u32 bytes, u32 flags) {
    WaveFormatX wf;
    IDirectSoundBuffer* out;
    DSBUFFERDESC desc;
    i32 hr;
    DSoundCloneInst* result;

    if (m_initialized == 0) {
        result = NULL;
        goto done;
    }
    if (bytes == 0) {
        result = NULL;
        goto done;
    }
    if (fmt == NULL) {
        result = NULL;
        goto done;
    }
    if (fmt->wFormatTag != 1) {
        result = NULL;
        goto done;
    }

    wf = *fmt;
    wf.cbSize = 0;

    memset(&desc, 0, sizeof(DSBUFFERDESC));
    desc.dwSize = DSBUFFERDESC_SIZE;
    desc.dwFlags = flags;
    desc.dwBufferBytes = bytes;
    desc.lpwfxFormat = WaveFormatSdk(&wf);

    hr = m_device->CreateSoundBuffer(&desc, &out, 0) != 0;
    if (hr) {
        DirectSoundMgr::GetErrorString(DSNDMGR_FILE, 0x422, hr);
        result = NULL;
        goto done;
    }
    if (out == NULL) {
        result = NULL;
        goto done;
    }

    {
        DSoundCloneInst* voice = new DSoundCloneInst(out, this);
        // Retail 0x136808 `mov edx,[esp+0x14]` reads wf+4, NOT wf+0: the pUnkOuter
        // `push esi` at 0x13674b is consumed by the CreateSoundBuffer call, so esp is
        // 4 higher here than in the copy block above. m_freq is the buffer's base
        // playback rate - see docs/patterns/early-arg-push-reaims-an-esp-displacement.md
        voice->m_freq = wf.nSamplesPerSec;
        m_bufferList.InsertHead(voice ? &voice->m_link : 0);
        voice->m_rateBase = fmt->nAvgBytesPerSec;
        voice->m_sampleRate = fmt->nAvgBytesPerSec;
        voice->m_sampleCount = bytes;
        voice->ComputeDuration();
        result = voice;
    }
done:
    return result;
}

RVA(0x00136860, 0xa9)
DSoundCloneInst* SoundDevice::AcquireFile(char* path, u32 flags, u32 loadOpts) {
    if (m_initialized == 0) {
        return 0;
    }
    FILE* fp = fopen(path, "rb");
    if (fp == NULL) {
        return 0;
    }
    u32 size = _filelength(fp->_file);
    u8* buf = new u8[size];
    if (fread(buf, size, 1, fp) != 1) {
        fclose(fp);
        delete[] buf;
        return 0;
    }
    fclose(fp);
    RecordBytes<RiffWaveHeader> riff;
    riff.m_bytes = buf;
    DSoundCloneInst* wrapper = Acquire(riff.m_rec, flags, loadOpts);
    delete[] buf;
    return wrapper;
}

RVA(0x00136910, 0x119)
DSoundCloneInst* SoundDevice::Acquire(RiffWaveHeader* riff, u32 flags, u32 loadOpts) {
    if (m_initialized == 0) {
        return 0;
    }
    if (riff == NULL) {
        return 0;
    }

    u8* data;
    u32 size;
    WaveFormatX* fmt;
    fmt = NULL;
    data = NULL;
    size = 0;
    if (ParseWaveChunks(riff, &fmt, &data, &size) == 0) {
        return 0;
    }

    i32 cvt = 0;
    if (m_force8Bit != 0 || (loadOpts & 1) == 1) {
        cvt = 1;
    }
    if (fmt->wBitsPerSample != sizeof(i16) * 8 || fmt->wFormatTag != WAVE_FORMAT_PCM) {
        cvt = 0;
    }
    if (cvt) {
        size >>= 1;
        fmt->wBitsPerSample = 8;
        fmt->nAvgBytesPerSec >>= 1;
        fmt->nBlockAlign >>= 1;
    }

    DSoundCloneInst* wrapper = CreateBuffer(fmt, size, flags);
    if (wrapper == NULL) {
        return 0;
    }
    if (wrapper->LockConvert(data, size, cvt) == 0) {
        RemoveBuffer(wrapper);
        return 0;
    }
    return wrapper;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00136a30, 0x76)
DSoundCloneInst* SoundDevice::AcquireResource(const char* name, u32 flags, u32 loadOpts) {
    if (m_initialized == 0) {
        return 0;
    }

    HINSTANCE mod1 = AfxGetModuleState()->m_hCurrentInstanceHandle;
    HRSRC hRsrc = FindResourceA(mod1, name, "WAVE");
    if (!hRsrc) {
        return 0;
    }
    HINSTANCE mod2 = AfxGetModuleState()->m_hCurrentInstanceHandle;
    HGLOBAL hRes = LoadResource(mod2, hRsrc);
    if (!hRes) {
        return 0;
    }
    RiffWaveHeader* data = static_cast<RiffWaveHeader*>(LockResource(hRes));
    if (!data) {
        return 0;
    }
    return Acquire(data, flags, loadOpts);
}

RVA(0x00136ab0, 0x41)
i32 SoundDevice::ValidateRestore(DirectSoundMgr* buf, WaveFormatX* fmt, u32 size) {
    if (m_initialized == 0) {
        return 0;
    }
    if (size == 0) {
        return 0;
    }
    if (fmt == NULL) {
        return 0;
    }
    if (fmt->wFormatTag != 1) {
        return 0;
    }
    return buf->Restore() != 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00136b00, 0xc2)
i32 SoundDevice::ReloadFile(DirectSoundMgr* buf, char* path, u32 loadOpts) {
    if (m_initialized == 0) {
        return 0;
    }
    if (buf->IsLooping() == 0) {
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
    i32 r = ReloadRiff(buf, riff.m_rec, loadOpts);
    delete[] data;
    return r;
}

RVA(0x00136bd0, 0x110)
i32 SoundDevice::ReloadRiff(DirectSoundMgr* buf, RiffWaveHeader* riff, u32 loadOpts) {
    if (m_initialized == 0) {
        return 0;
    }
    if (riff == NULL) {
        return 0;
    }
    if (buf->IsLooping() == 0) {
        return 1;
    }

    u8* data;
    u32 size;
    WaveFormatX* fmt;
    fmt = NULL;
    data = NULL;
    size = 0;
    if (ParseWaveChunks(riff, &fmt, &data, &size) == 0) {
        return 0;
    }

    i32 cvt = 0;
    if (m_force8Bit != 0 || (loadOpts & 1) == 1) {
        cvt = 1;
    }
    if (fmt->wBitsPerSample != sizeof(i16) * 8 || fmt->wFormatTag != WAVE_FORMAT_PCM) {
        cvt = 0;
    }
    if (cvt) {
        size >>= 1;
        fmt->wBitsPerSample = 8;
        fmt->nAvgBytesPerSec >>= 1;
        fmt->nBlockAlign >>= 1;
    }

    if (ValidateRestore(buf, fmt, size) == 0) {
        return 0;
    }
    return buf->LockConvert(data, size, cvt) != 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00136ce0, 0x92)
i32 SoundDevice::ReloadResource(DirectSoundMgr* probe, const char* name, u32 loadOpts) {
    if (m_initialized == 0) {
        return 0;
    }
    if (probe->IsLooping() == 0) {
        return 1;
    }

    HINSTANCE mod1 = AfxGetModuleState()->m_hCurrentInstanceHandle;
    HRSRC hRsrc = FindResourceA(mod1, name, "WAVE");
    if (!hRsrc) {
        return 0;
    }
    HINSTANCE mod2 = AfxGetModuleState()->m_hCurrentInstanceHandle;
    HGLOBAL hRes = LoadResource(mod2, hRsrc);
    if (!hRes) {
        return 0;
    }
    RiffWaveHeader* data = static_cast<RiffWaveHeader*>(LockResource(hRes));
    if (!data) {
        return 0;
    }
    return ReloadRiff(probe, data, loadOpts);
}

RVA(0x00136d80, 0x56)
void SoundDevice::RemoveBuffer(DirectSoundMgr* node) {
    if (m_initialized) {

        m_voiceList.RemoveMatching(node, SOUND_VOICE_TAG_ALL);
        if (node->m_buffer) {
            node->m_buffer->Release();
            node->m_buffer = NULL;
        }
        m_bufferList.Unlink(node ? &node->m_link : 0);
        if (node) {
            delete node;
        }
    }
}

RVA(0x00136de0, 0x3c)
void SoundDevice::StopAll() {
    if (m_initialized) {
        DSoundCloneInst* node = elemOf<DSoundCloneInst>(m_bufferList.m_head);
        while (node) {
            node->StopAndRewind();
            node->StopAllClones();
            node = elemOf<DSoundCloneInst>(node->m_link.m_next);
        }
    }
}

RVA(0x00136e20, 0xa8)
i32 SoundDevice::PurgeVoiceList(i32 time) {
    if (m_initialized == 0) {
        return 0;
    }
    DSoundLink* head = m_voiceList.m_head;
    DSoundVoice* e = elemOf<DSoundVoice>(head);
    if (e == NULL) {
        return 0;
    }
    if (time == -1) {
        time = static_cast<i32>(timeGetTime());
    }
    if (static_cast<u32>(time) <= static_cast<u32>(m_createFlag)) {
        return 1;
    }
    m_createFlag = time;
    do {
        DSoundLink* n = e->m_link.m_next;
        DSoundVoice* next = elemOf<DSoundVoice>(n);
        if (e->Tick(time) == 0) {
            m_voiceList.Unlink(e ? &e->m_link : 0);
            if (e) {
                PureSoundElem* pure = e;
                delete pure;
            }
        }
        e = next;
    } while (e);
    return 1;
}

RVA(0x00136ed0, 0x72)
i32 SoundDevice::FreeSamples() {
    if (m_initialized == 0) {
        return 0;
    }
    DSoundElem* node = elemOf<DSoundElem>(m_voiceList.m_head);
    if (node == NULL) {
        return 1;
    }
    do {
        DSoundLink* n = node->m_link.m_next;
        DSoundElem* next = elemOf<DSoundElem>(n);
        node->Stop();
        m_voiceList.Unlink(node ? &node->m_link : 0);
        if (node) {

            PureSoundElem* pure = node;
            delete pure;
        }
        node = next;
    } while (node);
    return 1;
}

// @identity-TODO No surviving call, address-taking site, or receiver access
// distinguishes the original owner/signature of this false-return leaf.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00136f50, 0x3)
i32 SoundDeviceReturnFalse() {
    return 0;
}

RVA(0x00136f60, 0x74)
void DSoundList::RemoveMatching(DirectSoundMgr* key, u32 tag) {
    DSoundElem* e = elemOf<DSoundElem>(m_head);
    while (e) {
        DSoundLink* node = &e->m_link;
        DSoundLink* n = e->m_link.m_next;
        DSoundElem* next = elemOf<DSoundElem>(n);
        if (tag != SOUND_VOICE_TAG_ALL && e->m_tag != tag) {
            continue;
        }
        if (e->m_key == key) {

            Unlink(e ? &e->m_link : 0);
            if (e) {
                PureSoundElem* pure = e;
                delete pure;
            }
        }
        e = next;
    }
}

// @early-stop
RVA(0x00136fe0, 0x7b)
DSoundVoice::DSoundVoice(i32 key, i32 pct, i32 mode, DirectSoundMgr* owner, i32 slot, i32 stamp) {
    m_live = 1;
    m_buffer = owner;
    m_stopAndRewind = slot;
    m_rampEndVolume = key;
    m_rampStartVolume = pct;
    m_rampDurationMs = mode;
    m_rampStartTime = (stamp == -1) ? timeGetTime() : stamp;
}

// @early-stop
RVA(0x00137060, 0x6b)
i32 DSoundVoice::Tick(i32 now) {
    i32 done = 0;
    i32 elapsed = now - m_rampStartTime;
    if (static_cast<u32>(elapsed) >= static_cast<u32>(m_rampDurationMs)) {
        elapsed = m_rampDurationMs;
        done = 1;
    }
    if (m_buffer->IsPlaying() == 0) {
        done = 1;
    } else {
        i32 vol =
            (m_rampEndVolume - m_rampStartVolume) * elapsed / m_rampDurationMs + m_rampStartVolume;
        m_buffer->SetVolumeByIndex(vol);
    }
    if (done && m_stopAndRewind != 0) {
        m_buffer->StopAndRewind();
    }
    return done == 0;
}

RVA(0x001370d0, 0x38)
i32 DSoundVoice::Stop() {
    if (m_buffer->IsPlaying() != 0) {
        if (m_stopAndRewind != 0) {
            m_buffer->StopAndRewind();
            return 1;
        }
        m_buffer->SetVolumeByIndex(m_rampEndVolume);
    }
    return 1;
}

RVA(0x00137110, 0x8d)
i32 ParseWaveChunks(RiffWaveHeader* riff, WaveFormatX** fmtOut, u8** dataOut, u32* sizeOut) {
    u32 riffSize = riff->m_riffSize;
    u32 waveTag = riff->m_waveTag;
    u8* cursor = riff->m_chunks;
    u8* end = cursor + riffSize - 4;
    if (riff->m_riffTag != mmioFOURCC('R', 'I', 'F', 'F')) {
        return 0;
    }
    if (waveTag != mmioFOURCC('W', 'A', 'V', 'E')) {
        return 0;
    }
    *fmtOut = NULL;
    *dataOut = NULL;
    while (cursor < end) {
        RecordBytes<RiffChunkHeader> chunkView;
        chunkView.m_bytes = cursor;
        RiffChunkHeader* chunk = chunkView.m_rec;
        if (chunk->m_id == mmioFOURCC('f', 'm', 't', ' ')) {
            RecordBytes<WaveFormatX> formatView;
            formatView.m_bytes = chunk->m_data;
            *fmtOut = formatView.m_rec;
        } else if (chunk->m_id == mmioFOURCC('d', 'a', 't', 'a')) {
            *dataOut = chunk->m_data;
            *sizeOut = chunk->m_size;
            return *fmtOut != NULL;
        }
        cursor = chunk->m_data + ((chunk->m_size + 1) & ~1);
    }
    return 0;
}

RVA(0x001371a0, 0x5a)
i32 SoundDevice::SetPrimaryFormat(WaveFormatX* fmt) {
    if (m_initialized == 0) {
        return 0;
    }
    if (CreatePrimaryBuffer() == 0) {
        return 0;
    }
    i32 hr = m_primaryBuffer->SetFormat(WaveFormatSdk(fmt)) != 0;
    if (hr) {
        DirectSoundMgr::GetErrorString(DSNDMGR_FILE, 0x678, hr);
        return 0;
    }
    return 1;
}

RVA(0x00137200, 0x53)
i32 SoundDevice::StartPrimary() {
    if (m_initialized == 0) {
        return 0;
    }
    if (CreatePrimaryBuffer() == 0) {
        return 0;
    }
    i32 hr = m_primaryBuffer->Play(0, 0, DSBPLAY_LOOPING) != 0;
    if (hr) {
        DirectSoundMgr::GetErrorString(DSNDMGR_FILE, 0x68b, hr);
        return 0;
    }
    return 1;
}

RVA(0x00137260, 0x95)
i32 SoundDevice::CreatePrimaryBuffer() {
    if (m_initialized == 0) {
        return 0;
    }

    if (m_coopLevel == DSSCL_NORMAL) {
        return 0;
    }
    if (m_primaryBuffer == NULL) {
        DSBUFFERDESC desc;
        memset(&desc, 0, sizeof(desc));
        desc.dwSize = sizeof(DSBUFFERDESC);
        desc.dwFlags = m_bufferFlags | DSBCAPS_PRIMARYBUFFER;
        i32 hr = m_device->CreateSoundBuffer(&desc, &m_primaryBuffer, 0) != 0;
        if (hr) {
            DirectSoundMgr::GetErrorString(DSNDMGR_FILE, 0x6ab, hr);
            return 0;
        }
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00137300, 0x23)
IDirectSoundBuffer* SoundDevice::GetPrimary() {
    if (m_initialized == 0) {
        return 0;
    }
    if (CreatePrimaryBuffer() == 0) {
        return 0;
    }
    return m_primaryBuffer;
}
