#include <rva.h>

#include <Dsndmgr/SoundStream.h>

#include <Mfc.h>

#include <Dsndmgr/StreamFeeder.h>
#include <Dsndmgr/StreamVoice.h>
#include <Dsndmgr/WaveFormatSdk.h>
#include <Rez/RezMgr.h>
#include <Utils/MillisPer.h>

#include <dsound.h>
#include <stdio.h>
#include <string.h>

#define DSNDMGSR_FILE "C:\\Proj\\Dsndmgr\\DSndMgSR.cpp"

DATA(0x00253c4c)
b32 g_dsoundDebugLog;
DATA(0x00253c50)
b32 g_dsoundErrorDialogs;
DATA(0x00253c54)
b32 g_dsoundErrorBeeps;
DATA(0x00253c58)
b32 g_dsoundFormatErrors;

RVA(0x00137340, 0x33)
i32 StreamFeeder::SeedWindow(CRezItm* source, u32 offset, u32 bytes) {
    if (source == NULL) {
        return 0;
    }
    m_source = source;
    m_windowLength = bytes;
    m_windowStart = offset;
    m_sourceOffset = offset;
    m_windowEnd = offset + bytes;
    PrimeBuffer(-1);
    return 1;
}

RVA(0x00137380, 0x10e)
i32 StreamVoiceFeeder::Feed(
    u8* dst1,
    u32 bytes1,
    u32* filled1,
    u8* dst2,
    u32 bytes2,
    u32* filled2
) {
    if (dst1 != NULL && bytes1 > 0) {
        u32 requestedBytes = bytes1;
        if (m_sourceOffset + bytes1 > m_windowEnd) {
            requestedBytes = m_windowEnd - m_sourceOffset;
        }
        *filled1 = m_source->Read(dst1, requestedBytes, m_sourceOffset);
        m_sourceOffset += *filled1;
        while (*filled1 < bytes1 && m_looping != false) {
            m_sourceOffset = m_windowStart;
            requestedBytes = bytes1;
            if (m_windowStart + bytes1 > m_windowEnd) {
                requestedBytes = m_windowEnd - m_windowStart;
            }
            *filled1 = m_source->Read(dst1, requestedBytes, m_sourceOffset);
            m_sourceOffset += *filled1;
        }
    }
    if (dst2 != NULL && bytes2 > 0) {
        u32 requestedBytes = bytes2;
        if (m_sourceOffset + bytes2 > m_windowEnd) {
            requestedBytes = m_windowEnd - m_sourceOffset;
        }
        *filled2 = m_source->Read(dst2, requestedBytes, m_sourceOffset);
        m_sourceOffset += *filled2;
        while (*filled2 < bytes2 && m_looping != false) {
            m_sourceOffset = m_windowStart;
            requestedBytes = bytes2;
            if (m_windowStart + bytes2 > m_windowEnd) {
                requestedBytes = m_windowEnd - m_windowStart;
            }
            *filled2 = m_source->Read(dst2, requestedBytes, m_sourceOffset);
            m_sourceOffset += *filled2;
        }
    }
    return 1;
}

RVA(0x00137490, 0x14)
i32 StreamVoiceFeeder::ResetSource() {
    m_sourceOffset = m_windowStart;
    m_windowEnd = m_windowStart + m_windowLength;
    return 1;
}

RVA(0x001374b0, 0x1)
void StreamVoiceFeeder::OnReset() {}

RVA(0x001374c0, 0x5d)
i32 StreamVoice::SetSource(CRezItm* source) {
    if (source == NULL) {
        return 0;
    }
    WaveFormatX format;
    u32 dataOffset;
    u32 dataBytes;

    if ((static_cast<SoundStream*>(m_owner))->ParseWave(source, &format, &dataOffset, &dataBytes)
        == 0) {
        return 0;
    }
    m_feeder.SeedWindow(source, dataOffset, dataBytes);
    return 1;
}

RVA(0x00137520, 0x6e)
i32 StreamVoice::Configure(i32 volumePct, i32 panPct, i32 frequencyOffsetPct, b32 looping) {
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
    m_feeder.m_looping = looping;
    if (m_feeder.Resume() == 0) {
        ok = false;
    }
    return ok;
}

RVA(0x00137590, 0x18)
u32 StreamVoice::GetDurationMs() {
    return m_feeder.m_windowLength * MILLIS_PER_SECOND / m_sampleRate;
}

RVA(0x001375b0, 0x77)
StreamVoice::StreamVoice(
    IDirectSoundBuffer* buffer,
    SoundStream* owner,
    i32 reprimeWhenIdle,
    i32 destroyWhenIdle
)
    : SoundSample(buffer, owner) {

    m_reprimeWhenIdle = reprimeWhenIdle;
    m_destroyWhenIdle = destroyWhenIdle;
    m_wasPlaying = false;
}

RVA_COMPGEN(0x00137630, 0x1e, ??_GStreamVoice@@UAEPAXI@Z)

RVA(0x00137650, 0x64)
StreamVoice::~StreamVoice() {
    m_feeder.Reset(0);
}

RVA_COMPGEN(0x001376c0, 0x5, ??1StreamVoiceFeeder@@QAE@XZ)

RVA(0x001376d0, 0x20)
SoundStream::SoundStream() {}

RVA_COMPGEN(0x001376f0, 0x1e, ??_GSoundStream@@UAEPAXI@Z)

RVA(0x00137710, 0xb)
SoundStream::~SoundStream() {}

RVA(0x00137720, 0x14)
i32 SoundStream::InitializeDevice(HWND hwnd, i32 cooperativeLevel) {
    return SoundDevice::Initialize(hwnd, cooperativeLevel, 0);
}

RVA(0x00137740, 0x3e)
void SoundStream::ShutdownStreams() {
    for (StreamVoice* voice = static_cast<StreamVoice*>(m_voices.GetFirst()); voice != NULL;
         voice = static_cast<StreamVoice*>(m_voices.GetFirst())) {
        DestroyVoice(voice);
    }
    Shutdown();
}

RVA(0x00137780, 0x171)
StreamVoice* SoundStream::CreateStreamVoice(
    WaveFormatX* format,
    u32 bufferBytes,
    i32 dsFlags,
    i32 reprimeWhenIdle,
    i32 destroyWhenIdle
) {
    if (m_initialized == false) {
        return NULL;
    }
    if (bufferBytes == 0) {
        return NULL;
    }
    if (format == NULL) {
        return NULL;
    }
    if (format->wFormatTag != 1) {
        return NULL;
    }

    WaveFormatX bufferFormat = *format;
    IDirectSoundBuffer* directSoundBuffer;
    DSBUFFERDESC bufferDesc;
    memset(&bufferDesc, 0, sizeof(DSBUFFERDESC));
    bufferDesc.dwFlags = dsFlags;
    bufferDesc.lpwfxFormat = WaveFormatSdk(&bufferFormat);

    bufferFormat.cbSize = 0;
    bufferDesc.dwSize = 0x14;
    bufferDesc.dwBufferBytes = bufferBytes;

    b32 hr = m_device->CreateSoundBuffer(&bufferDesc, &directSoundBuffer, NULL) != 0;
    if (hr != false) {
        SoundBuffer::ReportError(DSNDMGSR_FILE, 0xe8, hr);
        return NULL;
    }
    if (directSoundBuffer == NULL) {
        return NULL;
    }

    StreamVoice* voice = new StreamVoice(directSoundBuffer, this, reprimeWhenIdle, destroyWhenIdle);
    m_voices.InsertFirst(voice);
    voice->m_baseSampleRate = format->nAvgBytesPerSec;
    voice->m_sampleRate = format->nAvgBytesPerSec;
    voice->m_sampleCount = bufferBytes;
    voice->UpdateDuration();
    return voice;
}

// @early-stop
RVA(0x00137900, 0xc6)
StreamVoice* SoundStream::OpenStream(
    CRezItm* source,
    i32 bufferBytes,
    i32 refillThresholdBytes,
    i32 dsFlags,
    i32 reprimeWhenIdle,
    i32 destroyWhenIdle
) {
    if (source == NULL) {
        return NULL;
    }
    WaveFormatX format;
    u32 dataOffset;
    u32 dataBytes;
    if (ParseWave(source, &format, &dataOffset, &dataBytes) == 0) {
        return NULL;
    }
    StreamVoice* voice =
        CreateStreamVoice(&format, bufferBytes, dsFlags, reprimeWhenIdle, destroyWhenIdle);
    if (voice == NULL) {
        return NULL;
    }
    StreamFeeder* feeder = &voice->m_feeder;
    feeder->ConfigureWindow(source, dataOffset, dataBytes);

    if (feeder->Initialize(this, &format, bufferBytes, refillThresholdBytes, voice, -1) == 0) {
        DestroyVoice(voice);
        return NULL;
    }
    return voice;
}

RVA(0x001379d0, 0x5f)
void SoundStream::DestroyVoice(StreamVoice* voice) {
    if (m_initialized) {
        voice->m_feeder.Reset(0);

        m_volumeRamps.RemoveMatching(voice, SOUND_TASK_TAG_ALL);
        voice->m_buffer->Release();
        voice->m_buffer = NULL;
        m_voices.Delete(voice);
        if (voice) {
            delete voice;
        }
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00137a30, 0x4b)
StreamVoice*
SoundStream::PlayStream(CRezItm* source, i32 bufferBytes, i32 refillThresholdBytes, i32 dsFlags) {
    StreamVoice* voice = OpenStream(source, bufferBytes, refillThresholdBytes, dsFlags, 0, 1);
    if (voice == NULL) {
        return NULL;
    }
    if (voice->m_feeder.Resume() != 0) {
        return voice;
    }
    DestroyVoice(voice);
    return NULL;
}

RVA(0x00137a80, 0x3d)
void SoundStream::StopAllStreams() {
    StreamVoice* node = static_cast<StreamVoice*>(m_voices.GetFirst());
    while (node != NULL) {
        node->m_feeder.Pause();
        node = static_cast<StreamVoice*>(node->Next());
    }
    StopAllBuffers();
}

RVA(0x00137ac0, 0xa2)
i32 SoundStream::TickStreams(i32 timestampMs) {
    if (timestampMs == -1) {
        timestampMs = static_cast<i32>(timeGetTime());
    }
    CBaseListItem* head = m_voices.GetFirst();
    StreamVoice* voice = static_cast<StreamVoice*>(head);
    while (voice) {
        StreamVoice* next = static_cast<StreamVoice*>(voice->Next());
        voice->m_feeder.Tick(timestampMs);
        b32 isPlaying = voice->m_feeder.m_buffer->IsPlaying();
        if (isPlaying == false && voice->m_wasPlaying != false) {
            if (voice->m_reprimeWhenIdle != false) {
                voice->m_feeder.PrimeBuffer(-1);
            }
            if (voice->m_destroyWhenIdle != false) {
                DestroyVoice(voice);
                voice = NULL;
            }
        }
        if (voice) {
            voice->m_wasPlaying = isPlaying;
        }
        voice = next;
    }
    return 1;
}

RVA(0x00137b70, 0x159)
i32 SoundStream::ParseWave(
    CRezItm* source,
    WaveFormatX* outFormat,
    u32* outDataOffset,
    u32* outDataBytes
) {
    i32 foundFormat = 0;
    i32 foundData = 0;
    source->Seek(0);

    u32 riffTag;
    u32 chunkId;
    u32 chunkSize;
    source->Read(&riffTag, 4, -1);
    source->Read(&chunkSize, 4, -1);
    source->Read(&chunkId, 4, -1);
    if (riffTag != mmioFOURCC('R', 'I', 'F', 'F')) {
        return 0;
    }
    if (chunkId != mmioFOURCC('W', 'A', 'V', 'E')) {
        return 0;
    }

    u32 riffEnd = source->GetSeekPos() + chunkSize - 4;
    if (riffEnd > source->GetSize()) {
        riffEnd = source->GetSize();
    }
    while (source->GetSeekPos() < riffEnd) {
        source->Read(&chunkId, 4, -1);
        source->Read(&chunkSize, 4, -1);
        if (chunkId == mmioFOURCC('f', 'm', 't', ' ')) {
            i32 nextChunk = source->GetSeekPos() + chunkSize;

            u32 formatBytes = 0x12;
            if (chunkSize < formatBytes) {
                formatBytes = chunkSize;
            }
            source->Read(outFormat, static_cast<i32>(formatBytes), -1);
            source->Seek(nextChunk);
            foundFormat = 1;
        } else if (chunkId == mmioFOURCC('d', 'a', 't', 'a')) {
            *outDataOffset = source->GetSeekPos();
            *outDataBytes = chunkSize;
            foundData = 1;
        }
        if (foundFormat && foundData) {
            return 1;
        }
        if ((source->GetSeekPos() & 1) == 1) {
            source->Seek(source->GetSeekPos() + 1);
        }
    }
    return 0;
}

RVA(0x00137cd0, 0x1a)
StreamFeeder::StreamFeeder() {

    m_buffer = NULL;
    m_initialized = false;
    m_writeCursor = 0;
    m_playing = false;
    m_lastTickMs = 0;
}

RVA(0x00137cf0, 0x20)
StreamFeeder::~StreamFeeder() {
    if (m_initialized != false) {
        Reset(1);
    }
    m_buffer = NULL;
}

RVA(0x00137d10, 0xab)
i32 StreamFeeder::Initialize(
    SoundDevice* owner,
    WaveFormatX* format,
    u32 bufferBytes,
    u32 refillThresholdBytes,
    SoundBuffer* buffer,
    i32 initialTickMs
) {
    m_owner = owner;
    m_bufferBytes = bufferBytes;
    m_refillThresholdBytes = refillThresholdBytes;
    m_playing = false;
    if (format->wBitsPerSample > 8) {
        m_silenceByte = 0;
    } else {
        m_silenceByte = 0x80;
    }
    if (buffer == NULL) {
        m_buffer = owner->CreateSample(
            format,
            bufferBytes,
            DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLDEFAULT
        );
    } else {
        m_buffer = buffer;
    }
    if (m_buffer == NULL) {
        return 0;
    }
    m_initialized = true;
    if (ResetSource() == 0) {
        Reset(1);
        return 0;
    }
    if (PrimeBuffer(initialTickMs) == 0) {
        Reset(1);
        return 0;
    }
    return 1;
}

RVA(0x00137dc0, 0x43)
void StreamFeeder::Reset(i32 destroyBuffer) {
    if (m_initialized != false) {
        if (m_playing != false) {
            Pause();
        }
        OnReset();
        if (destroyBuffer != 0) {

            m_owner->DestroyBuffer(m_buffer);
        }
        m_buffer = NULL;
        m_initialized = false;
    }
}

RVA(0x00137e10, 0x6)
i32 StreamFeeder::ResetSource() {
    return 1;
}

RVA(0x00137e20, 0x1)
void StreamFeeder::OnReset() {}

RVA(0x00137e30, 0x98)
i32 StreamFeeder::Tick(i32 timestampMs) {
    if (!m_playing) {
        return 1;
    }
    i32 currentTimeMs = (timestampMs == -1) ? static_cast<i32>(timeGetTime()) : timestampMs;
    if (static_cast<u32>(currentTimeMs) <= static_cast<u32>((m_lastTickMs + 0x64))) {
        return 1;
    }
    m_lastTickMs = currentTimeMs;
    DWORD playCursor, writeCursor;
    if (!m_buffer->GetCurrentPosition(&playCursor, &writeCursor)) {
        return 0;
    }
    i32 consumedBytes;
    if (static_cast<u32>(playCursor) >= static_cast<u32>(m_writeCursor)) {
        if (playCursor == m_writeCursor) {
            consumedBytes = m_bufferBytes;
        } else {
            consumedBytes = playCursor - m_writeCursor;
        }
    } else {
        consumedBytes = m_bufferBytes + playCursor - m_writeCursor;
    }
    if (static_cast<u32>(consumedBytes) < static_cast<u32>(m_refillThresholdBytes)) {
        return 1;
    }
    return FillBuffer(m_writeCursor, consumedBytes) != 0;
}

RVA(0x00137ed0, 0x30)
i32 StreamFeeder::Resume() {
    if (m_playing != false) {
        return 1;
    }
    m_buffer->SetLooping(true);
    i32 result = m_buffer->Play();
    if (result != 0) {
        m_playing = true;
    }
    return result;
}

RVA(0x00137f00, 0x26)
i32 StreamFeeder::Pause() {
    if (m_playing == false) {
        return 1;
    }
    i32 result = m_buffer->StopAndRewind();
    if (result != 0) {
        m_playing = false;
    }
    return result;
}

RVA(0x00137f30, 0x197)
i32 StreamFeeder::FillBuffer(u32 writePos, u32 bytes) {
    u8* audioPtr1;
    DWORD audioBytes1;
    u8* audioPtr2;
    DWORD audioBytes2;
    if (m_buffer->Lock(writePos, bytes, &audioPtr1, &audioBytes1, &audioPtr2, &audioBytes2, 0)
        == 0) {
        return 0;
    }
    u32 filled1 = 0;
    u32 filled2 = 0;
    if (m_silenceBytes == 0) {
        if (Feed(audioPtr1, audioBytes1, &filled1, audioPtr2, audioBytes2, &filled2) == 0) {
            m_buffer->Unlock(audioPtr1, audioBytes1, audioPtr2, audioBytes2);
            return 0;
        }
    } else {
        filled1 = 0;
        filled2 = 0;
    }
    if (filled1 < audioBytes1) {
        m_silenceBytes += audioBytes1 - filled1;

        memset(audioPtr1 + filled1, m_silenceByte, audioBytes1 - filled1);
    }
    if (filled2 < audioBytes2) {
        m_silenceBytes += audioBytes2 - filled2;
        memset(audioPtr2 + filled2, m_silenceByte, audioBytes2 - filled2);
    }
    if (m_silenceBytes >= m_bufferBytes) {
        Pause();
    }

    if (audioBytes2 == 0) {
        m_writeCursor = writePos + audioBytes1;
    } else {
        m_writeCursor = audioBytes2;
    }
    if (m_writeCursor >= m_bufferBytes) {
        m_writeCursor = 0;
    }
    m_buffer->Unlock(audioPtr1, audioBytes1, audioPtr2, audioBytes2);
    return 1;
}

RVA(0x001380d0, 0x4e)
i32 StreamFeeder::PrimeBuffer(i32 timestampMs) {
    i32 currentTimeMs = (timestampMs == -1) ? static_cast<i32>(timeGetTime()) : timestampMs;
    m_lastTickMs = currentTimeMs;
    m_writeCursor = 0;
    if (!m_buffer->SetCurrentPosition(0)) {
        return 0;
    }
    m_silenceBytes = 0;
    return FillBuffer(m_writeCursor, m_bufferBytes) != 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00138120, 0x27)
void ConfigureSoundErrorReporting(
    b32 debugLog,
    b32 errorDialogs,
    b32 errorBeeps,
    b32 formatErrors
) {
    g_dsoundDebugLog = debugLog;
    g_dsoundErrorDialogs = errorDialogs;
    g_dsoundErrorBeeps = errorBeeps;
    g_dsoundFormatErrors = formatErrors;
}

RVA(0x00138150, 0x33b)
void SoundBuffer::ReportError(char* file, i32 line, i32 hr) {
    char szCode[64];
    char szMsg[256];
    char szLine[512];

    if (g_dsoundErrorBeeps) {
        MessageBeep(MB_ICONEXCLAMATION);
    }
    if (!g_dsoundDebugLog && !g_dsoundErrorDialogs && !g_dsoundFormatErrors) {
        return;
    }

    i32 code = hr & 0xffff;

    strcpy(szMsg, "Unknown Error Message");
    sprintf(szCode, "Unknown Error Code");
    strcpy(szLine, "");

    switch (hr) {
        case DSERR_UNSUPPORTED:
            strcpy(szCode, "DSERR_UNSUPPORTED");
            strcpy(szMsg, "No message");
            break;
        case DSERR_GENERIC:
            strcpy(szCode, "DSERR_GENERIC");
            strcpy(szMsg, "No message");
            break;
        case DSERR_NOAGGREGATION:
            strcpy(szCode, "DSERR_NOAGGREGATION");
            strcpy(szMsg, "No message");
            break;
        case DSERR_OUTOFMEMORY:
            strcpy(szCode, "DSERR_OUTOFMEMORY");
            strcpy(szMsg, "No message");
            break;
        case DSERR_INVALIDPARAM:
            strcpy(szCode, "DSERR_INVALIDPARAM");
            strcpy(szMsg, "No message");
            break;
        case DSERR_ALLOCATED:
            strcpy(szCode, "DSERR_ALLOCATED");
            strcpy(szMsg, "No message");
            break;
        case DSERR_CONTROLUNAVAIL:
            strcpy(szCode, "DSERR_CONTROLUNAVAIL");
            strcpy(szMsg, "No message");
            break;
        case DSERR_INVALIDCALL:
            strcpy(szCode, "DSERR_INVALIDCALL");
            strcpy(szMsg, "No message");
            break;
        case DSERR_PRIOLEVELNEEDED:
            strcpy(szCode, "DSERR_PRIOLEVELNEEDED");
            strcpy(szMsg, "No message");
            break;
        case DSERR_BADFORMAT:
            strcpy(szCode, "DSERR_BADFORMAT");
            strcpy(szMsg, "No message");
            break;
        case DSERR_NODRIVER:
            strcpy(szCode, "DSERR_NODRIVER");
            strcpy(szMsg, "No message");
            break;
        case DSERR_BUFFERLOST:
            strcpy(szCode, "DSERR_BUFFERLOST");
            strcpy(szMsg, "No message");
            break;
        case DSERR_OTHERAPPHASPRIO:
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

    if (g_dsoundDebugLog) {
        if (file == NULL || line <= 0) {
            sprintf(szLine, "%s (%i) - %s\n", szCode, code, szMsg);
        } else {
            sprintf(szLine, "%s, line %i: %s (%i) - %s\n", file, line, szCode, code, szMsg);
        }
        OutputDebugStringA(szLine);
    }
    if (g_dsoundErrorDialogs) {
        if (file == NULL || line <= 0) {
            sprintf(szLine, "%s (%i)\n\n%s", szCode, code, szMsg);
        } else {
            sprintf(szLine, "%s, line %i\n\n%s (%i)\n\n%s", file, line, szCode, code, szMsg);
        }
        MessageBoxA(static_cast<HWND>(0), szLine, "DirectSoundMgr", MB_ICONEXCLAMATION);
    }
}
