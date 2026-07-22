#include <Dsndmgr/SoundStream.h>
#include <EmptyString.h>          // g_emptyString
#include <Dsndmgr/StreamVoice.h>  // canonical StreamVoice + StreamVoiceFeeder
#include <Dsndmgr/StreamFeeder.h> // the embedded feeder/pump (StreamVoice+0x6c)
#include <Rez/RezMgr.h>           // RezAlloc - the engine heap allocator (reloc-masked)
#include <Win32.h>                // windows.h base types (dsound.h needs them)
#include <mmsystem.h>             // WAVEFORMATEX (dsound.h needs it predefined) + timeGetTime
#include <dsound.h> // real DirectSound SDK (IDirectSound/Buffer, DSBUFFERDESC, DSBCAPS)
#include <rva.h>
#include <stdio.h>  // engine sprintf (reloc-masked) in GetErrorString
#include <string.h> // memset (rep stos) in FillBuffer; inline strcpy in GetErrorString

#define DSNDMGSR_FILE "C:\\Proj\\Dsndmgr\\DSndMgSR.cpp"

extern "C" {
VTBL(StreamVoice, 0x001ef6d8); // 1-slot ??_7StreamVoice (slot 0 = ??_G 0x137630)
VTBL(StreamVoiceFeeder, 0x001ef6e0); // cl-emitted ??_7StreamVoiceFeeder@@6B@ (derived override)
VTBL(SoundStream, 0x001ef6ec); // cl-emitted ??_7SoundStream@@6B@ (virtual dtor override)
VTBL(StreamFeeder, 0x001ef6f0); // cl-emitted ??_7StreamFeeder@@6B@ (3-slot base)
    DATA(0x00253c4c)
    i32 g_ssLogEnabled; // 0x653c4c -> OutputDebugStringA
    DATA(0x00253c50)
    i32 g_ssMsgBoxEnabled; // 0x653c50 -> MessageBox
    DATA(0x00253c54)
    i32 g_ssBeepEnabled; // 0x653c54 -> startup beep
    DATA(0x00253c58)
    i32 g_ssThirdEnabled; // 0x653c58 -> "any output" gate
}

// ---------------------------------------------------------------------------
// 0x137330 - ??1PureSoundElem, the standalone out-of-line COMDAT copy of the
// canonical inline dtor (<Dsndmgr/SoundVoiceList.h>): 7-byte `mov [ecx],
// ??_7PureSoundElem; ret`. The inline `delete (PureSoundElem*)e` sites
// (0x136f60/e20/ed0, DSNDMGR.CPP side) inline the same teardown; retail's DSndMgSR
// obj additionally emits this copy because its EH unwind funclet (0x1e0950) takes
// the dtor's address (an EH shape this TU does not yet reproduce, so the retail fn
// is target-side-named only). First fn of the DSndMgSR.cpp obj (the 0x137330 file
// boundary; see interval-dossiers.md).
RVA_COMPGEN(0x00137330, 0x7, ??1PureSoundElem@@QAE@XZ)

RVA(0x00137340, 0x33)
i32 StreamFeeder::SeedWindow(CParseSource* src, u32 off, u32 len) {
    if (src == 0) {
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
    if (dst1 != 0 && n1 > 0) {
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
    if (dst2 != 0 && n2 > 0) {
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
    if (src == 0) {
        return 0;
    }
    WaveFormatX wf;
    u32 dataOff;
    u32 dataLen;
    // The base m_owner (+0x10, SoundDevice*) IS the creating SoundStream (the ctor
    // stores CreateStreamBuffer's `this`); ParseWave is SoundStream's RIFF parser,
    // so the voice downcasts its owner back to the concrete stream device.
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
    // The three setters are the inherited DirectSoundMgr::SetVolumeByIndex/
    // SetPanByIndex/SetField2 (0x1355c0/0x1357a0/0x135920), reached through the
    // real DSoundCloneInst base - no upcast needed.
    i32 ok = 1;
    if (SetVolumeByIndex(vol) == 0) {
        ok = 0;
    }
    if (SetPanByIndex(pan) == 0) {
        ok = 0;
    }
    if (SetField2(freq) == 0) {
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
    // cl auto-stamps ??_7StreamVoice after the base + member ctors.
    m_stopWhenIdle = a;
    m_retireWhenIdle = b;
    m_active = 0;
}

// 0x137630 - ??_GStreamVoice: the auto-emitted scalar-deleting dtor (slot 0 of the
// 1-slot ??_7StreamVoice @0x5ef6d8; `push esi; call ~StreamVoice; test [esp+8],1;
// conditional operator delete; ret 4`). Was a FID ??_G__non_rtti_object false
// positive in config/library_labels.csv.
RVA_COMPGEN(0x00137630, 0x1e, ??_GStreamVoice@@UAEPAXI@Z)

RVA(0x00137650, 0x64)
StreamVoice::~StreamVoice() {
    m_feeder.FeederReset(0);
}


// 0x1376c0 - ??1StreamVoiceFeeder: cl's auto-generated IMPLICIT dtor for the
// derived feeder (no members to destroy -> a bare 5-byte tail-jmp to ~StreamFeeder
// @0x137cf0, no vptr re-stamp). ~StreamVoice's EH unwind funclet (state 1: destroy
// m_feeder) takes its address, which is why the copy is emitted + kept. Was a FID
// __inc false positive.
RVA_COMPGEN(0x001376c0, 0x5, ??1StreamVoiceFeeder@@QAE@XZ)

// ---------------------------------------------------------------------------
// SoundStream::SoundStream (__thiscall). Run the base ctor, then zero the
// voice-list head/tail (+0x94/+0x98).
// @early-stop
// vptr-position wall: retail stamps the 0x5ef6ec vptr AFTER the two zero stores
// (vptr-last), but the ALL-VTABLES real-polymorphic model forces cl's implicit
// vptr-first store at ctor entry (after the SoundDevice base ctor). Body (base ctor
// + two zeros) otherwise matches; the vptr-position divergence is accepted.
RVA(0x001376d0, 0x20)
SoundStream::SoundStream() {
    // cl auto-stamps ??_7SoundStream@@6B@ (0x5ef6ec) here.
    m_voices.m_head = 0;
    m_voices.m_tail = 0;
}

// 0x1376f0 - ??_GSoundStream: the auto-emitted scalar-deleting dtor (slot 0 of
// ??_7SoundStream @0x5ef6ec). Was a FID ??_G__non_rtti_object false positive.
RVA_COMPGEN(0x001376f0, 0x1e, ??_GSoundStream@@UAEPAXI@Z)

RVA(0x00137710, 0xb)
SoundStream::~SoundStream() {}

// SoundStream::PlaySoundDefaulted (0x137720, __thiscall - `this`/ecx is unused): thin
// wrapper defaulting the 3rd flag arg to 0 over the shared play helper (0x136550). The
// sole caller (CDDrawSurfaceMgr::PlayDefaultSound) dispatches it on m_soundStream
// (`mov ecx,eax; push 1; push [hWnd]; call 0x137720`), so it is a real SoundStream
// method, not the free __stdcall it was modelled as (byte-identical either way since
// ecx is dead; the thiscall name binds the caller's reloc to the real 0x137720).
// @early-stop
// regalloc free-list-pick wall (docs/patterns/select-zero-mask-dest-register.md):
// body byte-exact except retail loads the hWnd arg into edx while cl picks ecx after
// eax is taken by flag - a single free-list register pick, not source-steerable (~98.6%).
i32 __stdcall PlaySound3_136550(i32 a, i32 b, i32 flag); // RVA 0x136550
RVA(0x00137720, 0x14)
i32 SoundStream::PlaySoundDefaulted(void* hWnd, i32 flag) {
    return PlaySound3_136550(reinterpret_cast<i32>(hWnd), flag, 0);
}

RVA(0x00137740, 0x3e)
void SoundStream::Free() {
    for (StreamVoice* p = elemOf<StreamVoice>(m_voices.m_head); p != 0;
         p = elemOf<StreamVoice>(m_voices.m_head)) {
        DestroyVoice(p);
    }
    Shutdown();
}

// ---------------------------------------------------------------------------
// SoundStream::CreateStreamBuffer (__thiscall, /GX EH frame). Validate
// the PCM WAVEFORMATEX, build a DSBUFFERDESC and ask the inherited IDirectSound
// device (+0x14) for a secondary buffer, then `new StreamVoice` wrapping it,
// thread it on the +0x94 list, and seed its duration fields. Carries the
// DSndMgSR.cpp __FILE__ anchor (the 0x678 GetErrorString report). The former
// RezAlloc+placement-new EH-frame wall (docs/patterns/
// rezalloc-placement-new-no-eh-frame.md) fell with the real StreamVoice :
// DSoundCloneInst derivation: plain `new T` over the ::operator-new(==RezAlloc)
// allocator now emits the retail /GX frame (byte-identical prologue).
// @early-stop
// /O2 cross-jump residue: retail tail-merges every early-out `return 0` through
// ONE shared fs:0-restoring epilogue (`xor eax,eax; jmp <epi>`) and pins arg `a`
// in ebp (4 callee-saves); cl here inlines the epilogue at each return and uses 3
// saves. Structure (plain `new`, the proven dev shape) correct; permuter/final-
// sweep territory. See the UPDATE in the pattern doc.
RVA(0x00137780, 0x171)
StreamVoice* SoundStream::CreateStreamBuffer(WaveFormatX* fmt, u32 bytes, i32 a, i32 b, i32 c) {
    if (m_initialized == 0) {
        return 0;
    }
    if (bytes == 0) {
        return 0;
    }
    if (fmt == 0) {
        return 0;
    }
    if (fmt->wFormatTag != 1) {
        return 0;
    }

    WaveFormatX wf;
    wf.wFormatTag = fmt->wFormatTag;
    wf.nChannels = fmt->nChannels;
    wf.nSamplesPerSec = fmt->nSamplesPerSec;
    wf.nAvgBytesPerSec = fmt->nAvgBytesPerSec;
    wf.nBlockAlign = fmt->nBlockAlign;
    wf.wBitsPerSample = fmt->wBitsPerSample;
    wf.cbSize = fmt->cbSize;

    IDirectSoundBuffer* out = 0;
    DSBUFFERDESC desc;
    desc.dwSize = 0x14;
    desc.dwFlags = a;
    desc.dwBufferBytes = bytes;
    desc.dwReserved = 0;
    desc.lpwfxFormat = reinterpret_cast<LPWAVEFORMATEX>(&wf);

    i32 hr = m_device->CreateSoundBuffer(&desc, &out, 0) != 0;
    if (hr) {
        DirectSoundMgr::GetErrorString(DSNDMGSR_FILE, 0x678, hr);
        return 0;
    }
    if (out == 0) {
        return 0;
    }

    // Plain `new StreamVoice` - ::operator new IS RezAlloc (0x1b9b46; reloc-masked
    // same callee), cl emits the null-guard + the /GX delete-on-throw EH state for
    // the now-really-throwing ctor (the real DSoundCloneInst base).
    StreamVoice* voice = new StreamVoice(out, this, b, c);
    m_voices.InsertHead(voice ? &voice->m_link : 0);
    voice->m_rateBase = fmt->nAvgBytesPerSec;
    voice->m_sampleRate = fmt->nAvgBytesPerSec;
    voice->m_sampleCount = bytes;
    voice->ComputeDuration();
    return voice;
}

// ---------------------------------------------------------------------------
// SoundStream::OpenStream (__thiscall). Parse the RIFF/WAVE source,
// create a streaming buffer for it, seed the voice's feeder window, then arm the
// feeder; on a feeder failure, destroy the voice.
// @early-stop
// regalloc wall: retail pins `src` in ebp across the ParseWave/CreateStreamBuffer
// calls; MSVC here pins it in ebx, shifting the prologue load + the callee-saved
// cascade (one register choice, body otherwise exact). See
// docs/patterns/zero-register-pinning.md / pin-local-for-callee-saved-reg.md.
// Logic complete, deferred to the final sweep.
RVA(0x00137900, 0xc6)
StreamVoice* SoundStream::OpenStream(CParseSource* src, i32 p1, i32 p2, i32 p3, i32 p4, i32 p5) {
    if (src == 0) {
        return 0;
    }
    WaveFormatX wf;
    u32 dataOff;
    u32 dataLen;
    if (ParseWave(src, &wf, &dataOff, &dataLen) == 0) {
        return 0;
    }
    StreamVoice* voice = CreateStreamBuffer(&wf, p1, p3, p4, p5);
    if (voice == 0) {
        return 0;
    }
    StreamFeeder* feeder = &voice->m_feeder;
    feeder->m_windowStart = dataOff;
    feeder->m_windowLength = dataLen;
    feeder->m_source = src;
    feeder->m_loop = 0;
    feeder->m_sourceOffset = 0;
    // voice IS-A DirectSoundMgr buffer wrapper (StreamVoice : DSoundCloneInst);
    // the upcast is implicit.
    if (feeder->FeederStart(this, &wf, p1, p2, voice, -1) == 0) {
        DestroyVoice(voice);
        return 0;
    }
    return voice;
}

RVA(0x001379d0, 0x5f)
void SoundStream::DestroyVoice(StreamVoice* voice) {
    if (m_initialized) {
        voice->m_feeder.FeederReset(0);
        // Reap the inherited SoundDevice voice sub-list (m_voiceList @ +0x0c), keyed
        // by this voice's identity.
        m_voiceList.RemoveMatching(voice, 0xffff);
        voice->m_buffer->Release();
        voice->m_buffer = 0;
        m_voices.Unlink(voice ? &voice->m_link : 0);
        if (voice) {
            delete voice;
        }
    }
}

RVA(0x00137a30, 0x4b)
StreamVoice* SoundStream::PlayStream(CParseSource* src, i32 a2, i32 a3, i32 a4) {
    StreamVoice* voice = OpenStream(src, a2, a3, a4, 0, 1);
    if (voice == 0) {
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
    while (node != 0) {
        node->m_feeder.Pause();
        node = elemOf<StreamVoice>(node->m_link.m_next);
    }
    StopAll();
}

RVA(0x00137ac0, 0xa2)
i32 SoundStream::TickSubManagers(i32 time) {
    if (time == -1) {
        time = static_cast<i32>(::timeGetTime());
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
                o = 0;
            }
        }
        if (o) {
            o->m_active = r;
        }
        o = next;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// SoundStream::ParseWave (__thiscall - the implicit `this` is unused).
// Walk the RIFF/WAVE chunks of `src`: verify the RIFF+WAVE magic, then loop the
// fmt/data chunks, copying the 18-byte PCM header into fmtBuf and reporting the
// data chunk's offset+length. Returns 1 only when both fmt and data were seen.
// @early-stop
// local-coalescing wall: retail reuses the incoming arg stack slots as scratch
// (sub esp,0xc) for the RIFF/chunk header reads; MSVC here allocates one extra
// local dword (sub esp,0x10), shifting every [esp+N] local offset by 4. Body +
// control flow byte-exact; only the frame size / slot assignment differs - the
// entropy tail. Logic complete, deferred to the final sweep.
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
    if (riffTag != 0x46464952) {
        return 0;
    }
    if (waveTag != 0x45564157) {
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
        if (chunkId == 0x20746d66) {
            i32 next = src->m_cursor + chunkSize;
            i32 n = chunkSize;
            if (n >= 0x12) {
                n = 0x12;
            }
            src->Read(fmtBuf, n, -1);
            src->SetPos(next);
            gotFmt = 1;
        } else if (chunkId == 0x61746164) {
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
    // cl auto-stamps ??_7StreamFeeder@@6B@ (0x5ef6f0) here.
    m_buffer = 0;
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
    m_buffer = 0;
}

// ---------------------------------------------------------------------------
// StreamFeeder::FeederStart (__thiscall, 6 args, ret 0x18). Arm the
// feeder: record owner/length/format, derive the silence byte from the PCM bit
// depth, create (or adopt) the streaming buffer, arm, FeedData (slot 1) and
// prime via Tick.
// @early-stop
// regalloc eax/edx wall: retail pins the format-pointer (arg2) and the format-value
// (arg4) into a fixed eax/edx pair; MSVC here assigns the opposite pair. Body +
// control flow byte-exact, only the two-arg register choice differs - a
// register-allocation plateau (docs/patterns/zero-register-pinning.md family).
// Logic complete, deferred to the final sweep.
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
    if (buf == 0) {
        m_buffer = owner->CreateBuffer(fmt, len, 0x100e0);
    } else {
        m_buffer = buf;
    }
    if (m_buffer == 0) {
        return 0;
    }
    m_armed = 1;
    if (FeedData() == 0) { // slot 1 (virtual)
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
        OnDrain(); // slot 2 (virtual)
        if (doStop != 0) {
            // m_buffer and RemoveBuffer's param are the same DirectSoundMgr base view now
            // (matcher-6 unified the former SoundBuf dual-view into DirectSoundMgr).
            m_owner->RemoveBuffer(m_buffer);
        }
        m_buffer = 0;
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
    u32 hi, lo;
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
    m_buffer->SetField3(1);
    i32 r = m_buffer->Play();
    if (r != 0) {
        m_drained = 1;
    }
    return r; // fall-through keeps the Play result in eax (no re-zero)
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
    return r; // fall-through keeps the StopAndRewind result in eax (no re-zero)
}

// ---------------------------------------------------------------------------
// StreamFeeder::FillBuffer (__thiscall, 2 args, ret 0x8). Lock the
// streaming secondary buffer at the write window, copy the source window into
// the (possibly wrapped) locked regions, silence-pad the unfilled tail, advance
// the read cursor (m_bufferCursor) with wraparound, and Unlock.
// @early-stop
// local-coalescing / frame-size wall: retail reuses dead local slots for the
// got1/got2 scratch (sub esp,0x10); MSVC here keeps them distinct (sub esp,0x14),
// shifting every Lock out-pointer slot + the [esp+N] local offsets. Instruction
// selection + control flow byte-exact; only the stack-slot assignment differs -
// the documented local-coalescing wall (docs/patterns/stack-buffer-size-drives-frame.md).
// Logic complete, deferred to the final sweep.
RVA(0x00137f30, 0x197)
i32 StreamFeeder::FillBuffer(u32 writePos, u32 bytes) {
    void* p1;
    u32 n1;
    void* p2;
    u32 n2;
    if (m_buffer->Lock(writePos, bytes, &p1, &n1, &p2, &n2, 0) == 0) {
        return 0;
    }
    u32 got1 = 0;
    u32 got2 = 0;
    if (m_pendingBytes == 0) {
        if (Feed(p1, n1, &got1, p2, n2, &got2) == 0) { // slot 0 (virtual)
            m_buffer->Unlock(p1, n1, p2, n2);
            return 0;
        }
    } else {
        got1 = 0;
        got2 = 0;
    }
    if (got1 < n1) {
        m_pendingBytes += n1 - got1;
        // language-forced: p1 is a void* DirectSound-locked region; the +got1
        // pointer arithmetic requires a byte-pointer view. Inlines to rep stos.
        memset(reinterpret_cast<char*>(p1) + got1, m_silenceByte, n1 - got1);
    }
    if (got2 < n2) {
        m_pendingBytes += n2 - got2;
        memset(reinterpret_cast<char*>(p2) + got2, m_silenceByte, n2 - got2);
    }
    if (m_pendingBytes >= m_bufferLength) {
        Pause();
    }
    if (got2 != 0) {
        m_bufferCursor = got2;
    } else {
        m_bufferCursor = writePos + bytes;
    }
    if (m_bufferCursor >= m_bufferLength) {
        m_bufferCursor = 0;
    }
    m_buffer->Unlock(p1, n1, p2, n2);
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
    char szCode[64];  // error-code name
    char szMsg[256];  // description
    char szLine[512]; // formatted output line

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
        case 0:
            strcpy(szCode, "DS_OK");
            strcpy(szMsg, "No error");
            break;
        default:
            break;
    }

    if (g_ssLogEnabled) {
        if (file == 0 || line <= 0) {
            sprintf(szLine, "%s (%i) - %s\n", szCode, code, szMsg);
        } else {
            sprintf(szLine, "%s, line %i: %s (%i) - %s\n", file, line, szCode, code, szMsg);
        }
        OutputDebugStringA(szLine);
    }
    if (g_ssMsgBoxEnabled) {
        if (file == 0 || line <= 0) {
            sprintf(szLine, "%s (%i)\n\n%s", szCode, code, szMsg);
        } else {
            sprintf(szLine, "%s, line %i\n\n%s (%i)\n\n%s", file, line, szCode, code, szMsg);
        }
        MessageBoxA(static_cast<HWND>(0), szLine, "DirectSoundMgr", MB_ICONEXCLAMATION);
    }
}
