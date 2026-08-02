#include <Dsndmgr/DirectSoundMgr.h>
#include <Dsndmgr/SoundVoiceList.h>
#include <Dsndmgr/DSoundVoice.h>
#include <Dsndmgr/SoundDevice.h>
#include <Rez/RezMgr.h>
#include <Win32.h>
#include <mmsystem.h>
#include <dsound.h>
#include <Dsndmgr/WaveFormatPtr.h>
#include <rva.h>
#include <Pix16.h>
#include <math.h>
#include <stdio.h>
#include <io.h>
#include <string.h>

#include <Wap32/Wap32.h>

DATA(0x001ef6b0)
const double c_acosNorm = 2.0;

DATA(0x001ef6a8)
const double c_powExp = 10.0;

DATA(0x001ef6a0)
const double c_volNum = 1.0;

DATA(0x001ef698)
const double c_volScale = 100.0;

#define DSNDMGR_FILE "C:\\Proj\\Dsndmgr\\DSNDMGR.CPP"

typedef enum DSoundDx5Magic {
    DSB_RETAIL_LOOPBIT = 0x02,
    DSBUFFERDESC_SIZE = 0x14,
} DSoundDx5Magic;

VTBL(DirectSoundMgr, 0x001ef6b8);
VTBL(DSoundCloneInst, 0x001ef6bc);
VTBL(DSoundBaseSub, 0x001ef6c0);
VTBL(SoundDevice, 0x001ef6c4);
VTBL(PureSoundElem, 0x001ef6c8);
VTBL(DSoundVoice, 0x001ef6d0);
DATA(0x00253ab8)
i32 g_volumeTable[100];
DATA(0x00253c48)
i32 g_panTable[8];

DATA(0x0020b668)
const char s_rb[] = "rb";

#pragma optimize("", off)

RVA(0x001350b0, 0x5d)
i32 SoundDevice::VolumeToAttenuation(i32 value) {
    if (value == 100) {
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
        return 100;
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
    for (i32 i = 0; i <= 100; i++) {
        g_volumeTable[i] = VolumeToAttenuation(i);
    }
}

RVA(0x001351d0, 0x109)
DirectSoundMgr::DirectSoundMgr(IDirectSoundBuffer* buf, SoundDevice* owner) {

    m_buffer = buf;
    m_owner = owner;
    m_playFlags = 0;
    m_durationMs = 0;
    m_reacquireCb = 0;
    m_reacquireCtx = 0;
    m_rateBase = 0;
    m_sampleRate = 0;
    if (buf == 0) {
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
    if (m_reacquireCb != 0) {
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
    if (voice == 0) {
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
        return 100 - ConvertVolumeToPercent(-pan);
    }
    return ConvertVolumeToPercent(pan) - 100;
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

RVA(0x00135920, 0x80)
i32 DirectSoundMgr::SetFrequencyOffsetPercent(i32 percentOffset) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 v = percentOffset * static_cast<i32>(m_freq) / 100 + static_cast<i32>(m_freq);
    if (static_cast<u32>(v) >= 0x186a0) {
        v = 0x1869f;
    }
    if (static_cast<u32>(v) <= 0x64) {
        v = 0x65;
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
i32 DirectSoundMgr::Unlock(void* audioPtr1, u32 audioBytes1, void* audioPtr2, u32 audioBytes2) {
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
i32 DirectSoundMgr::GetFormat(void* fmt, u32 size, DWORD* written) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 hr = m_buffer->GetFormat(static_cast<LPWAVEFORMATEX>(fmt), size, written) != 0;
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
    while (m_cloneList.m_head != 0) {
        RemoveClone(static_cast<CloneNode*>(m_cloneList.m_head)->m_inst);
    }
}

RVA(0x00135c20, 0xf6)
DirectSoundMgr* DSoundCloneInst::Clone(i32 a) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    DSoundBaseSub* clone = new DSoundBaseSub(m_buffer, m_owner, this);
    if (clone == 0) {
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
        clone->m_buffer = 0;
    }
    ((&m_cloneList))->Unlink(&clone->m_cloneNode);
    if (clone != this) {
        delete clone;
    }
}

// @early-stop
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
        found = 0;
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
        if (!found) {
            return found;
        }
    }
    ((&m_cloneList))->Unlink(&found->m_cloneNode);
    ((&m_cloneList))->InsertTail(&found->m_cloneNode);
    return found;
}

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

    void* audioPtr1;
    DWORD audioBytes1;
    void* audioPtr2;
    DWORD audioBytes2;
    i32 hr = m_buffer->Lock(
        0,
        bytes,
        &audioPtr1,
        &audioBytes1,
        &audioPtr2,
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
i32 DirectSoundMgr::LockConvert(void* src, u32 lockBytes, u32 convert) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }

    void* audioPtr1;
    void* audioPtr2;
    DWORD audioBytes1;
    DWORD audioBytes2;
    i32 hr = m_buffer->Lock(
                 0,
                 lockBytes,
                 &audioPtr1,
                 &audioBytes1,
                 &audioPtr2,
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
            memcpy(audioPtr2, static_cast<char*>(src) + audioBytes1, audioBytes2);
        }
    } else {

        if (audioBytes1 > 0) {
            char* d = static_cast<char*>(audioPtr1);
            i16* s = static_cast<i16*>(src);
            char* end = static_cast<char*>(audioPtr1) + audioBytes1;
            while (d < end) {
                *d = static_cast<char>((static_cast<u32>((*s + 0x8000)) >> 8));
                ++s;
                ++d;
            }
        }
        if (audioBytes2 > 0) {
            char* d = static_cast<char*>(audioPtr2);

            Pix16Ptr half2;
            half2.m_chars = (static_cast<char*>(src) + audioBytes1);
            i16* s = half2.m_swords;
            char* end = static_cast<char*>(audioPtr2) + audioBytes2;
            while (d < end) {
                *d = static_cast<char>((static_cast<u32>((*s + 0x8000)) >> 8));
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
    for (CloneNode* node = static_cast<CloneNode*>(m_cloneList.m_head); node != 0;
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
    void** audioPtr1,
    DWORD* audioBytes1,
    void** audioPtr2,
    DWORD* audioBytes2,
    u32 flags
) {
    if (m_owner->m_initialized == 0) {
        return 0;
    }
    i32 hr = m_buffer->Lock(off, bytes, audioPtr1, audioBytes1, audioPtr2, audioBytes2, flags) != 0;
    if (hr != 0) {

        if (hr == DSERR_BUFFERLOST) {
            if (m_reacquireOwner->ReacquireBuffer() != 0) {
                hr = m_buffer
                         ->Lock(off, bytes, audioPtr1, audioBytes1, audioPtr2, audioBytes2, flags)
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
    m_primaryBuffer = 0;
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
i32 SoundDevice::Create(void* hwnd, u32 level, u32 flags) {
    i32 created = DirectSoundCreate(0, &m_device, 0) != 0;
    if (created) {
        return 0;
    }
    i32 hr = m_device->SetCooperativeLevel(static_cast<HWND>(hwnd), level) != 0;
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

RVA(0x001365f0, 0x57)
i32 SoundDevice::SetCooperativeLevel(void* hwnd, u32 level) {
    if (m_initialized == 0) {
        return 0;
    }
    i32 hr = m_device->SetCooperativeLevel(static_cast<HWND>(hwnd), level) != 0;
    if (hr) {
        DirectSoundMgr::GetErrorString(DSNDMGR_FILE, 0x3cf, hr);
        return 0;
    }
    m_coopLevel = level;
    return 1;
}

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

// @early-stop
RVA(0x001366f0, 0x168)
DSoundCloneInst* SoundDevice::CreateBuffer(WaveFormatX* fmt, u32 bytes, u32 flags) {
    WaveFormatX wf;
    IDirectSoundBuffer* out;
    DSBUFFERDESC desc;
    i32 hr;

    DSoundCloneInst* voice = 0;

    if (m_initialized == 0) {
        goto done;
    }
    if (bytes == 0) {
        goto done;
    }
    if (fmt == 0) {
        goto done;
    }
    if (fmt->wFormatTag != 1) {
        goto done;
    }

    wf.m_formatWord = fmt->m_formatWord;
    wf.nSamplesPerSec = fmt->nSamplesPerSec;
    wf.nAvgBytesPerSec = fmt->nAvgBytesPerSec;
    wf.m_blockWord = fmt->m_blockWord;
    wf.cbSize = fmt->cbSize;

    out = 0;

    memset(&desc, 0, sizeof(DSBUFFERDESC));
    desc.dwSize = DSBUFFERDESC_SIZE;
    desc.dwFlags = flags;
    desc.dwBufferBytes = bytes;
    WaveFormatPtr fmtPtr;
    fmtPtr.m_rec = &wf;
    desc.lpwfxFormat = fmtPtr.m_sdk;

    wf.cbSize = 0;

    hr = m_device->CreateSoundBuffer(&desc, &out, 0) != 0;
    if (hr) {
        DirectSoundMgr::GetErrorString(DSNDMGR_FILE, 0x422, hr);
        goto done;
    }
    if (out == 0) {
        goto done;
    }

    voice = new DSoundCloneInst(out, this);
    voice->m_freq = wf.m_formatWord;
    m_bufferList.InsertHead(voice ? &voice->m_link : 0);
    voice->m_rateBase = fmt->nAvgBytesPerSec;
    voice->m_sampleRate = fmt->nAvgBytesPerSec;
    voice->m_sampleCount = bytes;
    voice->ComputeDuration();
done:
    return voice;
}

RVA(0x00136860, 0xa9)
DSoundCloneInst* SoundDevice::AcquireFile(char* path, u32 flags, u32 loadOpts) {
    if (m_initialized == 0) {
        return 0;
    }
    FILE* fp = fopen(path, s_rb);
    if (fp == 0) {
        return 0;
    }
    u32 size = _filelength(fp->_file);
    void* buf = operator new(size);
    if (fread(buf, size, 1, fp) != 1) {
        fclose(fp);
        operator delete(buf);
        return 0;
    }
    fclose(fp);
    DSoundCloneInst* wrapper = Acquire(buf, flags, loadOpts);
    operator delete(buf);
    return wrapper;
}

RVA(0x00136910, 0x119)
DSoundCloneInst* SoundDevice::Acquire(void* riff, u32 flags, u32 loadOpts) {
    if (m_initialized == 0) {
        return 0;
    }
    if (riff == 0) {
        return 0;
    }

    void* data;
    u32 size;
    WaveFormatX* fmt;
    fmt = 0;
    data = 0;
    size = 0;
    if (ParseWaveChunks(riff, &fmt, &data, &size) == 0) {
        return 0;
    }

    i32 cvt = 0;
    if (m_force8Bit != 0 || (loadOpts & 1) == 1) {
        cvt = 1;
    }
    if (fmt->wBitsPerSample != 0x10 || fmt->wFormatTag != 1) {
        cvt = 0;
    }
    if (cvt) {
        size >>= 1;
        fmt->wBitsPerSample = 8;
        fmt->nAvgBytesPerSec >>= 1;
        fmt->nBlockAlign >>= 1;
    }

    DSoundCloneInst* wrapper = CreateBuffer(fmt, size, flags);
    if (wrapper == 0) {
        return 0;
    }
    if (wrapper->LockConvert(data, size, cvt) == 0) {
        RemoveBuffer(wrapper);
        return 0;
    }
    return wrapper;
}

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
    void* data = LockResource(hRes);
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
    if (fmt == 0) {
        return 0;
    }
    if (fmt->wFormatTag != 1) {
        return 0;
    }
    return buf->Restore() != 0;
}

RVA(0x00136b00, 0xc2)
i32 SoundDevice::ReloadFile(DirectSoundMgr* buf, char* path, u32 loadOpts) {
    if (m_initialized == 0) {
        return 0;
    }
    if (buf->IsLooping() == 0) {
        return 1;
    }
    FILE* fp = fopen(path, s_rb);
    if (fp == 0) {
        return 0;
    }
    u32 size = _filelength(fp->_file);
    void* data = operator new(size);
    if (fread(data, size, 1, fp) != 1) {
        fclose(fp);
        operator delete(data);
        return 0;
    }
    fclose(fp);
    i32 r = ReloadRiff(buf, data, loadOpts);
    operator delete(data);
    return r;
}

RVA(0x00136bd0, 0x110)
i32 SoundDevice::ReloadRiff(DirectSoundMgr* buf, void* riff, u32 loadOpts) {
    if (m_initialized == 0) {
        return 0;
    }
    if (riff == 0) {
        return 0;
    }
    if (buf->IsLooping() == 0) {
        return 1;
    }

    void* data;
    u32 size;
    WaveFormatX* fmt;
    fmt = 0;
    data = 0;
    size = 0;
    if (ParseWaveChunks(riff, &fmt, &data, &size) == 0) {
        return 0;
    }

    i32 cvt = 0;
    if (m_force8Bit != 0 || (loadOpts & 1) == 1) {
        cvt = 1;
    }
    if (fmt->wBitsPerSample != 0x10 || fmt->wFormatTag != 1) {
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
    void* data = LockResource(hRes);
    if (!data) {
        return 0;
    }
    return ReloadRiff(probe, data, loadOpts);
}

RVA(0x00136d80, 0x56)
void SoundDevice::RemoveBuffer(DirectSoundMgr* node) {
    if (m_initialized) {

        m_voiceList.RemoveMatching(node, 0xffff);
        if (node->m_buffer) {
            node->m_buffer->Release();
            node->m_buffer = 0;
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
    if (e == 0) {
        return 0;
    }
    if (time == -1) {
        time = static_cast<i32>(::timeGetTime());
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

// @early-stop
RVA(0x00136ed0, 0x72)
i32 SoundDevice::FreeSamples() {
    if (m_initialized == 0) {
        return 0;
    }
    DSoundElem* node = elemOf<DSoundElem>(m_voiceList.m_head);
    while (node) {
        DSoundLink* n = node->m_link.m_next;
        DSoundElem* next = elemOf<DSoundElem>(n);
        node->Stop();
        m_voiceList.Unlink(node ? &node->m_link : 0);
        if (node) {

            PureSoundElem* pure = node;
            delete pure;
        }
        node = next;
    }
    return 1;
}

RVA(0x00136f60, 0x74)
void DSoundList::RemoveMatching(DirectSoundMgr* key, u32 tag) {
    DSoundElem* e = elemOf<DSoundElem>(m_head);
    while (e) {
        DSoundLink* node = &e->m_link;
        DSoundLink* n = e->m_link.m_next;
        DSoundElem* next = elemOf<DSoundElem>(n);
        if (tag != 0xffff && e->m_tag != tag) {
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
    m_rampStartVolume = pct;
    m_rampEndVolume = key;
    m_rampDurationMs = mode;
    m_rampStartTime = (stamp == -1) ? ::timeGetTime() : stamp;
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
i32 ParseWaveChunks(void* riff, WaveFormatX** fmtOut, void** dataOut, u32* sizeOut) {

    RiffCursor p;
    p.m_bytes = static_cast<char*>(riff) + 4;
    u32 riffSize = *p.m_words;
    p.m_words++;
    u32 waveTag = *p.m_words;
    p.m_words++;
    char* end = p.m_bytes + riffSize - 4;
    if (*static_cast<u32*>(riff) != mmioFOURCC('R', 'I', 'F', 'F')) {
        return 0;
    }
    if (waveTag != mmioFOURCC('W', 'A', 'V', 'E')) {
        return 0;
    }
    *fmtOut = 0;
    *dataOut = 0;
    while (p.m_bytes < end) {
        u32 id = *p.m_words++;
        u32 size = *p.m_words++;
        if (id == mmioFOURCC('f', 'm', 't', ' ')) {
            *fmtOut = p.m_format;
        } else if (id == mmioFOURCC('d', 'a', 't', 'a')) {
            *dataOut = p.m_words;
            *sizeOut = size;
            return *fmtOut != 0;
        }

        p.m_bytes += ((size + 1) & ~1);
    }
    return 0;
}

RVA(0x001371a0, 0x5a)
i32 SoundDevice::SetPrimaryFormat(void* fmt) {
    if (m_initialized == 0) {
        return 0;
    }
    if (CreatePrimaryBuffer() == 0) {
        return 0;
    }
    i32 hr = m_primaryBuffer->SetFormat(static_cast<LPWAVEFORMATEX>(fmt)) != 0;
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
    if (m_primaryBuffer == 0) {
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
