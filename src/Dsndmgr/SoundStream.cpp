#include <rva.h>

#include <Dsndmgr/SoundStream.h>

#include <Mfc.h>

#include <Dsndmgr/StreamFeeder.h>
#include <Dsndmgr/StreamVoice.h>
#include <Dsndmgr/WaveFormatPtr.h>
#include <EmptyString.h>
#include <Rez/RezMgr.h>

#include <dsound.h>
#include <mmsystem.h>
#include <stdio.h>
#include <string.h>

#define DSNDMGSR_FILE "C:\\Proj\\Dsndmgr\\DSndMgSR.cpp"

DATA(0x00253c4c)
i32 g_ssLogEnabled;
DATA(0x00253c50)
i32 g_ssMsgBoxEnabled;
DATA(0x00253c54)
i32 g_ssBeepEnabled;
DATA(0x00253c58)
i32 g_ssThirdEnabled;

RVA_COMPGEN(0x00137330, 0x7, ??1PureSoundElem@@QAE@XZ)

RVA(0x00137340, 0x33)
i32 StreamFeeder::SeedWindow(CParseSource* src, u32 off, u32 len) {
    if (src == NULL) {
        return 0;
    }
    m_source = src;
    m_windowLength = len;
    m_windowStart = off;
    m_sourceOffset = off;
    m_windowEnd = off + len;
    TickPump(-1);
    return 1;
}

RVA(0x00137380, 0x10e)
i32 StreamVoiceFeeder::Feed(void* dst1, u32 n1, u32* got1, void* dst2, u32 n2, u32* got2) {
    if (dst1 != NULL && n1 > 0) {
        u32 want = n1;
        if (m_sourceOffset + n1 > m_windowEnd) {
            want = m_windowEnd - m_sourceOffset;
        }
        *got1 = m_source->Read(dst1, want, m_sourceOffset);
        m_sourceOffset += *got1;
        while (*got1 < n1 && m_loop != 0) {
            m_sourceOffset = m_windowStart;
            want = n1;
            if (m_windowStart + n1 > m_windowEnd) {
                want = m_windowEnd - m_windowStart;
            }
            *got1 = m_source->Read(dst1, want, m_sourceOffset);
            m_sourceOffset += *got1;
        }
    }
    if (dst2 != NULL && n2 > 0) {
        u32 want = n2;
        if (m_sourceOffset + n2 > m_windowEnd) {
            want = m_windowEnd - m_sourceOffset;
        }
        *got2 = m_source->Read(dst2, want, m_sourceOffset);
        m_sourceOffset += *got2;
        while (*got2 < n2 && m_loop != 0) {
            m_sourceOffset = m_windowStart;
            want = n2;
            if (m_windowStart + n2 > m_windowEnd) {
                want = m_windowEnd - m_windowStart;
            }
            *got2 = m_source->Read(dst2, want, m_sourceOffset);
            m_sourceOffset += *got2;
        }
    }
    return 1;
}

RVA(0x00137490, 0x14)
i32 StreamVoiceFeeder::FeedData() {
    m_sourceOffset = m_windowStart;
    m_windowEnd = m_windowStart + m_windowLength;
    return 1;
}

RVA(0x001374b0, 0x1)
void StreamVoiceFeeder::OnDrain() {}

RVA(0x001374c0, 0x5d)
i32 StreamVoice::SetSource(CParseSource* src) {
    if (src == NULL) {
        return 0;
    }
    WaveFormatX wf;
    u32 dataOff;
    u32 dataLen;

    if ((static_cast<SoundStream*>(m_owner))->ParseWave(src, &wf, &dataOff, &dataLen) == 0) {
        return 0;
    }
    m_feeder.SeedWindow(src, dataOff, dataLen);
    return 1;
}

RVA(0x00137520, 0x6e)
i32 StreamVoice::Configure(i32 vol, i32 pan, i32 freq, i32 loop) {
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
    if (SetFrequencyOffsetPercent(freq) == 0) {
        ok = 0;
    }
    m_feeder.m_loop = loop;
    if (m_feeder.Resume() == 0) {
        ok = 0;
    }
    return ok;
}

RVA(0x00137590, 0x18)
u32 StreamVoice::ComputeRatio() {
    return m_feeder.m_windowLength * 1000 / m_sampleRate;
}

RVA(0x001375b0, 0x77)
StreamVoice::StreamVoice(IDirectSoundBuffer* buf, SoundStream* owner, i32 a, i32 b)
    : DSoundCloneInst(buf, owner) {

    m_stopWhenIdle = a;
    m_retireWhenIdle = b;
    m_active = 0;
}

RVA_COMPGEN(0x00137630, 0x1e, ??_GStreamVoice@@UAEPAXI@Z)

RVA(0x00137650, 0x64)
StreamVoice::~StreamVoice() {
    m_feeder.FeederReset(0);
}

RVA_COMPGEN(0x001376c0, 0x5, ??1StreamVoiceFeeder@@QAE@XZ)

RVA(0x001376d0, 0x20)
SoundStream::SoundStream() {}

RVA_COMPGEN(0x001376f0, 0x1e, ??_GSoundStream@@UAEPAXI@Z)

RVA(0x00137710, 0xb)
SoundStream::~SoundStream() {}

RVA(0x00137720, 0x14)
i32 SoundStream::PlaySoundDefaulted(void* hWnd, i32 flag) {
    return Create(hWnd, flag, 0);
}

RVA(0x00137740, 0x3e)
void SoundStream::Free() {
    for (StreamVoice* p = elemOf<StreamVoice>(m_voices.m_head); p != NULL;
         p = elemOf<StreamVoice>(m_voices.m_head)) {
        DestroyVoice(p);
    }
    Shutdown();
}

// @early-stop
RVA(0x00137780, 0x171)
StreamVoice* SoundStream::CreateStreamBuffer(
    WaveFormatX* fmt,
    u32 bytes,
    i32 dsFlags,
    i32 stopWhenIdle,
    i32 retireWhenIdle
) {
    WaveFormatX wf;
    IDirectSoundBuffer* out;
    DSBUFFERDESC desc;
    i32 hr;
    StreamVoice* voice = 0;

    if (m_initialized == 0) {
        return 0;
    }
    if (bytes == 0) {
        return 0;
    }
    if (fmt == NULL) {
        return 0;
    }
    if (fmt->wFormatTag != 1) {
        return 0;
    }

    wf = *fmt;

    out = NULL;

    memset(&desc, 0, sizeof(DSBUFFERDESC));
    desc.dwFlags = dsFlags;
    WaveFormatPtr fmtPtr;
    fmtPtr.m_rec = &wf;
    desc.lpwfxFormat = fmtPtr.m_sdk;

    wf.cbSize = 0;
    desc.dwSize = 0x14;
    desc.dwBufferBytes = bytes;

    hr = m_device->CreateSoundBuffer(&desc, &out, 0) != 0;
    if (hr) {
        DirectSoundMgr::GetErrorString(DSNDMGSR_FILE, 0x678, hr);
        return 0;
    }
    if (out == NULL) {
        return 0;
    }

    voice = new StreamVoice(out, this, stopWhenIdle, retireWhenIdle);
    m_voices.InsertHead(voice ? &voice->m_link : 0);
    voice->m_rateBase = fmt->nAvgBytesPerSec;
    voice->m_sampleRate = fmt->nAvgBytesPerSec;
    voice->m_sampleCount = bytes;
    voice->ComputeDuration();
    return voice;
}

// @early-stop
RVA(0x00137900, 0xc6)
StreamVoice* SoundStream::OpenStream(
    CParseSource* src,
    i32 bytes,
    i32 format,
    i32 dsFlags,
    i32 stopWhenIdle,
    i32 retireWhenIdle
) {
    if (src == NULL) {
        return 0;
    }
    WaveFormatX wf;
    u32 dataOff;
    u32 dataLen;
    if (ParseWave(src, &wf, &dataOff, &dataLen) == 0) {
        return 0;
    }
    StreamVoice* voice = CreateStreamBuffer(&wf, bytes, dsFlags, stopWhenIdle, retireWhenIdle);
    if (voice == NULL) {
        return 0;
    }
    StreamFeeder* feeder = &voice->m_feeder;
    feeder->m_windowStart = dataOff;
    feeder->m_windowLength = dataLen;
    feeder->m_source = src;
    feeder->m_loop = 0;
    feeder->m_sourceOffset = 0;

    if (feeder->FeederStart(this, &wf, bytes, format, voice, -1) == 0) {
        DestroyVoice(voice);
        return 0;
    }
    return voice;
}

RVA(0x001379d0, 0x5f)
void SoundStream::DestroyVoice(StreamVoice* voice) {
    if (m_initialized) {
        voice->m_feeder.FeederReset(0);

        m_voiceList.RemoveMatching(voice, SOUND_VOICE_TAG_ALL);
        voice->m_buffer->Release();
        voice->m_buffer = NULL;
        m_voices.Unlink(voice ? &voice->m_link : 0);
        if (voice) {
            delete voice;
        }
    }
}

RVA(0x00137a30, 0x4b)
StreamVoice* SoundStream::PlayStream(CParseSource* src, i32 bytes, i32 format, i32 dsFlags) {
    StreamVoice* voice = OpenStream(src, bytes, format, dsFlags, 0, 1);
    if (voice == NULL) {
        return 0;
    }
    if (voice->m_feeder.Resume() != 0) {
        return voice;
    }
    DestroyVoice(voice);
    return 0;
}

RVA(0x00137a80, 0x3d)
void SoundStream::Stop() {
    StreamVoice* node = elemOf<StreamVoice>(m_voices.m_head);
    while (node != NULL) {
        node->m_feeder.Pause();
        node = elemOf<StreamVoice>(node->m_link.m_next);
    }
    StopAll();
}

RVA(0x00137ac0, 0xa2)
i32 SoundStream::TickSubManagers(i32 time) {
    if (time == -1) {
        time = static_cast<i32>(timeGetTime());
    }
    DSoundLink* head = m_voices.m_head;
    StreamVoice* o = elemOf<StreamVoice>(head);
    while (o) {
        StreamVoice* next = elemOf<StreamVoice>(o->m_link.m_next);
        o->m_feeder.Tick(time);
        i32 r = o->m_feeder.m_buffer->IsPlaying();
        if (r == 0 && o->m_active != 0) {
            if (o->m_stopWhenIdle != 0) {
                o->m_feeder.TickPump(-1);
            }
            if (o->m_retireWhenIdle != 0) {
                DestroyVoice(o);
                o = NULL;
            }
        }
        if (o) {
            o->m_active = r;
        }
        o = next;
    }
    return 1;
}

// @early-stop
// Frame is 4 bytes bigger than retail's: retail parks one header word in the DEAD
// incoming-argument home (src is copied to esi at entry) and we allocate a fourth
// local slot instead. Dropping the `end` local reaches sub esp,0xc but recolours
// the bound computation; scoping the chunk locals to the function is worse.
RVA(0x00137b70, 0x159)
i32 SoundStream::ParseWave(
    CParseSource* src,
    WaveFormatX* fmtBuf,
    u32* outDataOff,
    u32* outDataLen
) {
    i32 gotFmt = 0;
    i32 gotData = 0;
    src->SetPos(0);

    u32 riffTag;
    u32 riffSize;
    u32 waveTag;
    src->Read(&riffTag, 4, -1);
    src->Read(&riffSize, 4, -1);
    src->Read(&waveTag, 4, -1);
    if (riffTag != mmioFOURCC('R', 'I', 'F', 'F')) {
        return 0;
    }
    if (waveTag != mmioFOURCC('W', 'A', 'V', 'E')) {
        return 0;
    }

    u32 end = src->m_cursor + riffSize - 4;
    if (end > src->m_length) {
        end = src->m_length;
    }
    while (src->m_cursor < end) {
        u32 chunkId;
        u32 chunkSize;
        src->Read(&chunkId, 4, -1);
        src->Read(&chunkSize, 4, -1);
        if (chunkId == mmioFOURCC('f', 'm', 't', ' ')) {
            i32 next = src->m_cursor + chunkSize;

            u32 n = 0x12;
            if (chunkSize < n) {
                n = chunkSize;
            }
            src->Read(fmtBuf, static_cast<i32>(n), -1);
            src->SetPos(next);
            gotFmt = 1;
        } else if (chunkId == mmioFOURCC('d', 'a', 't', 'a')) {
            *outDataOff = src->m_cursor;
            *outDataLen = chunkSize;
            gotData = 1;
        }
        if (gotFmt && gotData) {
            return 1;
        }
        if ((src->m_cursor & 1) == 1) {
            src->SetPos(src->m_cursor + 1);
        }
    }
    return 0;
}

RVA(0x00137cd0, 0x1a)
StreamFeeder::StreamFeeder() {

    m_buffer = NULL;
    m_armed = 0;
    m_bufferCursor = 0;
    m_drained = 0;
    m_lastTickMs = 0;
}

RVA(0x00137cf0, 0x20)
StreamFeeder::~StreamFeeder() {
    if (m_armed != 0) {
        FeederReset(1);
    }
    m_buffer = NULL;
}

// @early-stop
RVA(0x00137d10, 0xab)
i32 StreamFeeder::FeederStart(
    SoundDevice* owner,
    WaveFormatX* fmt,
    u32 len,
    u32 format,
    DirectSoundMgr* buf,
    i32 tickArg
) {
    m_format = format;
    m_owner = owner;
    m_bufferLength = len;
    m_drained = 0;
    if (fmt->wBitsPerSample > 8) {
        m_silenceByte = 0;
    } else {
        m_silenceByte = 0x80;
    }
    if (buf == NULL) {
        m_buffer = owner->CreateBuffer(fmt, len, 0x100e0);
    } else {
        m_buffer = buf;
    }
    if (m_buffer == NULL) {
        return 0;
    }
    m_armed = 1;
    if (FeedData() == 0) {
        FeederReset(1);
        return 0;
    }
    if (TickPump(tickArg) == 0) {
        FeederReset(1);
        return 0;
    }
    return 1;
}

RVA(0x00137dc0, 0x43)
void StreamFeeder::FeederReset(i32 doStop) {
    if (m_armed != 0) {
        if (m_drained != 0) {
            Pause();
        }
        OnDrain();
        if (doStop != 0) {

            m_owner->RemoveBuffer(m_buffer);
        }
        m_buffer = NULL;
        m_armed = 0;
    }
}

RVA(0x00137e10, 0x6)
i32 StreamFeeder::FeedData() {
    return 1;
}

RVA(0x00137e20, 0x1)
void StreamFeeder::OnDrain() {}

RVA(0x00137e30, 0x98)
i32 StreamFeeder::Tick(i32 timestamp) {
    if (!m_drained) {
        return 1;
    }
    i32 t = (timestamp == -1) ? static_cast<i32>(timeGetTime()) : timestamp;
    if (static_cast<u32>(t) <= static_cast<u32>((m_lastTickMs + 0x64))) {
        return 1;
    }
    m_lastTickMs = t;
    DWORD hi, lo;
    if (!m_buffer->GetCurrentPosition(&hi, &lo)) {
        return 0;
    }
    i32 v;
    if (static_cast<u32>(hi) >= static_cast<u32>(m_bufferCursor)) {
        if (hi == m_bufferCursor) {
            v = m_bufferLength;
        } else {
            v = hi - m_bufferCursor;
        }
    } else {
        v = m_bufferLength + hi - m_bufferCursor;
    }
    if (static_cast<u32>(v) < static_cast<u32>(m_format)) {
        return 1;
    }
    return FillBuffer(m_bufferCursor, v) != 0;
}

RVA(0x00137ed0, 0x30)
i32 StreamFeeder::Resume() {
    if (m_drained != 0) {
        return 1;
    }
    m_buffer->SetLooping(1);
    i32 r = m_buffer->Play();
    if (r != 0) {
        m_drained = 1;
    }
    return r;
}

RVA(0x00137f00, 0x26)
i32 StreamFeeder::Pause() {
    if (m_drained == 0) {
        return 1;
    }
    i32 r = m_buffer->StopAndRewind();
    if (r != 0) {
        m_drained = 0;
    }
    return r;
}

RVA(0x00137f30, 0x197)
i32 StreamFeeder::FillBuffer(u32 writePos, u32 bytes) {
    void* lock1;
    DWORD n1;
    void* lock2;
    DWORD n2;
    if (m_buffer->Lock(writePos, bytes, &lock1, &n1, &lock2, &n2, 0) == 0) {
        return 0;
    }
    u32 got1 = 0;
    u32 got2 = 0;
    if (m_pendingBytes == 0) {
        if (Feed(lock1, n1, &got1, lock2, n2, &got2) == 0) {
            m_buffer->Unlock(lock1, n1, lock2, n2);
            return 0;
        }
    } else {
        got1 = 0;
        got2 = 0;
    }
    if (got1 < n1) {
        m_pendingBytes += n1 - got1;

        memset(static_cast<char*>(lock1) + got1, m_silenceByte, n1 - got1);
    }
    if (got2 < n2) {
        m_pendingBytes += n2 - got2;
        memset(static_cast<char*>(lock2) + got2, m_silenceByte, n2 - got2);
    }
    if (m_pendingBytes >= m_bufferLength) {
        Pause();
    }

    if (n2 == 0) {
        m_bufferCursor = writePos + n1;
    } else {
        m_bufferCursor = n2;
    }
    if (m_bufferCursor >= m_bufferLength) {
        m_bufferCursor = 0;
    }
    m_buffer->Unlock(lock1, n1, lock2, n2);
    return 1;
}

RVA(0x001380d0, 0x4e)
i32 StreamFeeder::TickPump(i32 now) {
    i32 t = (now == -1) ? static_cast<i32>(timeGetTime()) : now;
    m_lastTickMs = t;
    m_bufferCursor = 0;
    if (!m_buffer->SetCurrentPosition(0)) {
        return 0;
    }
    m_pendingBytes = 0;
    return FillBuffer(m_bufferCursor, m_bufferLength) != 0;
}

RVA(0x00138120, 0x27)
void SetDSoundReportModes(i32 log, i32 msgBox, i32 beep, i32 third) {
    g_ssLogEnabled = log;
    g_ssMsgBoxEnabled = msgBox;
    g_ssBeepEnabled = beep;
    g_ssThirdEnabled = third;
}

RVA(0x00138150, 0x33b)
void DirectSoundMgr::GetErrorString(char* file, i32 line, i32 hr) {
    char szCode[64];
    char szMsg[256];
    char szLine[512];

    if (g_ssBeepEnabled) {
        MessageBeep(MB_ICONEXCLAMATION);
    }
    if (!g_ssLogEnabled && !g_ssMsgBoxEnabled && !g_ssThirdEnabled) {
        return;
    }

    i32 code = hr & 0xffff;

    strcpy(szMsg, "Unknown Error Message");
    sprintf(szCode, "Unknown Error Code");
    strcpy(szLine, g_emptyString);

    switch (hr) {
        case static_cast<i32>(0x80004001):
            strcpy(szCode, "DSERR_UNSUPPORTED");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x80004005):
            strcpy(szCode, "DSERR_GENERIC");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x80040110):
            strcpy(szCode, "DSERR_NOAGGREGATION");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x8007000e):
            strcpy(szCode, "DSERR_OUTOFMEMORY");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x80070057):
            strcpy(szCode, "DSERR_INVALIDPARAM");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x8878000a):
            strcpy(szCode, "DSERR_ALLOCATED");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x8878001e):
            strcpy(szCode, "DSERR_CONTROLUNAVAIL");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x88780032):
            strcpy(szCode, "DSERR_INVALIDCALL");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x88780046):
            strcpy(szCode, "DSERR_PRIOLEVELNEEDED");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x88780064):
            strcpy(szCode, "DSERR_BADFORMAT");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x88780078):
            strcpy(szCode, "DSERR_NODRIVER");
            strcpy(szMsg, "No message");
            break;
        case DSERR_BUFFERLOST:
            strcpy(szCode, "DSERR_BUFFERLOST");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x887800a0):
            strcpy(szCode, "DSERR_OTHERAPPHASPRIO");
            strcpy(szMsg, "No message");
            break;
        case DS_OK:
            strcpy(szCode, "DS_OK");
            strcpy(szMsg, "No error");
            break;
        default:
            break;
    }

    if (g_ssLogEnabled) {
        if (file == NULL || line <= 0) {
            sprintf(szLine, "%s (%i) - %s\n", szCode, code, szMsg);
        } else {
            sprintf(szLine, "%s, line %i: %s (%i) - %s\n", file, line, szCode, code, szMsg);
        }
        OutputDebugStringA(szLine);
    }
    if (g_ssMsgBoxEnabled) {
        if (file == NULL || line <= 0) {
            sprintf(szLine, "%s (%i)\n\n%s", szCode, code, szMsg);
        } else {
            sprintf(szLine, "%s, line %i\n\n%s (%i)\n\n%s", file, line, szCode, code, szMsg);
        }
        MessageBoxA(static_cast<HWND>(0), szLine, "DirectSoundMgr", MB_ICONEXCLAMATION);
    }
}
