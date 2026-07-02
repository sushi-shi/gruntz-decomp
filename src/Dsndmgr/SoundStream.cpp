// SoundStream.cpp - the streaming DirectSound class run from DSndMgSR.CPP
// (C:\Proj\Dsndmgr\DSndMgSR.CPP, retail vftable 0x5ef6ec). SoundStream DERIVES
// from SoundDevice (DSNDMGR.CPP, 0x5ef6c4): the dtor restamps its own vptr then
// tail-calls ~SoundDevice. The streaming half parses a RIFF/WAVE source, asks
// the inherited IDirectSound device for a secondary buffer, wraps it in a
// per-stream StreamVoiceNode (the 0xb0-byte DirectSoundMgr-derived voice, ctor
// 0x1375b0) and threads that voice on the inherited +0x94 instance list.
//
// Trace called all three Dsndmgr classes "MinervaInner"; the distinct vtables
// (device 0x5ef6c4 / buffer 0x5ef6b8 / stream 0x5ef6ec) prove they are separate.
// Field names are placeholders; offsets + emitted bytes are load-bearing.
#include <Dsndmgr/SoundStream.h>
#include <Rez/RezMgr.h> // RezAlloc - the engine heap allocator (reloc-masked)
#include <Win32.h>
#include <rva.h>

// The __FILE__ string DSndMgSR.CPP passes to GetErrorString (the $SG pooled
// constant 0x619f1c).
#define DSNDMGSR_FILE "C:\\Proj\\Dsndmgr\\DSndMgSR.cpp"

// Placement new (construct in place into the RezAlloc'd block); no allocation, so
// it is matching-neutral - it just runs the StreamVoiceNode ctor (0x1375b0) on the
// raw RezAlloc result, exactly as the retail RezAlloc-then-construct does.
inline void* operator new(u32, void* p) {
    return p;
}

// ALL-VTABLES phase: the stream vftable (0x5ef6ec) is now cl-emitted as
// ??_7SoundStream@@6B@ from the real polymorphic SoundStream : SoundDevice (virtual
// dtor override); the ctor auto-stamps it and the dtor auto-resets it + chains
// ~SoundDevice (was the manual g_SoundStreamVtbl stamps).

// The inherited intrusive instance-list helpers on the +0x94 head (same engine
// list family DirectSoundMgr models as the clone list): Insert (0x1390e0, prepend
// one biased-+4 node) and Unlink (0x1391e0, remove one). __thiscall on the head.
struct StreamList {
    void* m_head;
    void* m_tail;
    void Insert(void* node); // 0x1390e0
    void Unlink(void* node); // 0x1391e0
};
SIZE_UNKNOWN(StreamList); // {head,tail} list-head view

// The device's per-voice channel sub-list reap helper (0x136f60, __thiscall on
// the inherited +0x0c head): unlink + free every voice matching (node, mask).
struct StreamVoiceList {
    void* m_head;
    void* m_tail;
    void Reap(void* node, i32 mask); // 0x136f60
};
SIZE_UNKNOWN(StreamVoiceList); // {head,tail} list-head view

// ---------------------------------------------------------------------------
// SoundStream::SoundStream (__thiscall). Run the base ctor, zero the
// two instance words (m_94 list head / m_98), then stamp the stream vptr.
// The 0x5ef6ec stamp + the 0x137710 shared dtor (via the scalar dtor below)
// prove it is SoundStream's.
// @early-stop
// vptr-position wall: retail stamps the 0x5ef6ec vptr AFTER the two zero stores
// (vptr-last), but the ALL-VTABLES real-polymorphic model forces cl's implicit
// vptr-first store at ctor entry (after the SoundDevice base ctor). Body (base ctor
// + two zeros) otherwise matches; the vptr-position divergence is accepted.
RVA(0x001376d0, 0x20)
SoundStream::SoundStream() {
    // cl auto-stamps ??_7SoundStream@@6B@ (0x5ef6ec) here (was the manual store).
    m_94 = 0;
    m_98 = 0;
}

// ---------------------------------------------------------------------------
// SoundStream::ScalarDtor (__thiscall) - the scalar-deleting dtor
// (??_G): run ~SoundStream, then operator delete (the engine RezFree) when the
// low flag bit is set; returns this. Modeled as a plain method (name-independent
// at delink) since SoundStream is kept non-polymorphic (manual vptr stamp).
RVA(0x001376f0, 0x1e)
void* SoundStream::ScalarDtor(i32 flag) {
    this->~SoundStream();
    if (flag & 1) {
        RezFree(this);
    }
    return this;
}

// ---------------------------------------------------------------------------
// SoundStream::~SoundStream (__thiscall). Empty body: cl resets the vptr to
// ??_7SoundStream@@6B@ (0x5ef6ec) then chains ~SoundDevice (the teardown) - was
// the hand-rolled restamp + implicit base-dtor tail.
RVA(0x00137710, 0xb)
SoundStream::~SoundStream() {}

// ---------------------------------------------------------------------------
// SoundStream::CreateStreamBuffer (__thiscall, /GX EH frame). Validate
// the PCM WAVEFORMATEX, build a DSBUFFERDESC and ask the inherited IDirectSound
// device (+0x14) for a secondary buffer, then RezAlloc + construct a StreamVoiceNode
// wrapping it, thread it on the +0x94 list, and seed its duration fields.
// @early-stop
// RezAlloc+placement-new EH-frame wall (docs/patterns/rezalloc-placement-new-no-eh-frame.md):
// the body (fmt copy, CreateSoundBuffer, RezAlloc, ctor, list insert, ComputeDuration)
// is byte-exact, but retail's `new`-with-RezAlloc-operator-new emits a /GX
// ctor-in-flight EH frame (push -1/fs:0 + trylevel) and a single shared `jmp`
// epilogue that MSVC5's placement-new (no placement operator delete) cannot
// reproduce - 47% on a documented EH wall. Defer to the final sweep once the
// StreamVoiceNode ctor (0x1375b0) is modelled and a real `new T` allocator path emits
// the frame.
RVA(0x00137780, 0x171)
StreamVoiceNode* SoundStream::CreateStreamBuffer(WaveFormatX* fmt, u32 bytes, i32 a, i32 b, i32 c) {
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
    *(u32*)&wf.nChannels = *(u32*)&fmt->nChannels;
    wf.nAvgBytesPerSec = fmt->nAvgBytesPerSec;
    *(u32*)&wf.nBlockAlign = *(u32*)&fmt->nBlockAlign;
    wf.cbSize = fmt->cbSize;

    IDirectSoundBufferZ* out = 0;
    DSBUFFERDESC desc;
    desc.dwSize = 0x14;
    desc.dwFlags = a;
    desc.dwBufferBytes = bytes;
    desc.dwReserved = 0;
    desc.lpwfxFormat = &wf;

    i32 hr = m_device->vtbl->CreateSoundBuffer(m_device, &desc, &out, 0) != 0;
    if (hr) {
        DirectSoundMgr::GetErrorString(DSNDMGSR_FILE, 0x678, hr);
        return 0;
    }
    if (out == 0) {
        return 0;
    }

    StreamVoiceNode* voice = (StreamVoiceNode*)RezAlloc(0xb0);
    if (voice) {
        voice = new (voice) StreamVoiceNode(out, this, b, c);
    }
    ((StreamList*)&m_94)->Insert(voice ? &voice->m_link : 0);
    voice->m_avgBytes = fmt->nAvgBytesPerSec;
    voice->m_avgBytesDiv = fmt->nAvgBytesPerSec;
    voice->m_byteLength = bytes;
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
// 90.6% on a documented regalloc wall - logic complete, deferred to the final sweep.
RVA(0x00137900, 0xc6)
StreamVoiceNode*
SoundStream::OpenStream(StreamSource* src, i32 p1, i32 p2, i32 p3, i32 p4, i32 p5) {
    if (src == 0) {
        return 0;
    }
    WaveFormatX wf;
    u32 dataOff;
    u32 dataLen;
    if (ParseWave(src, &wf, &dataOff, &dataLen) == 0) {
        return 0;
    }
    StreamVoiceNode* voice = CreateStreamBuffer(&wf, p1, p3, p4, p5);
    if (voice == 0) {
        return 0;
    }
    StreamFeederView* feeder = &voice->m_feeder;
    feeder->m_dataLen = dataOff;
    feeder->m_dataPtr = (u32)src;
    feeder->m_source = (u32)src;
    feeder->m_30 = 0;
    feeder->m_34 = 0;
    if (feeder->FeederStart(this, &wf, p1, p2, voice, -1) == 0) {
        DestroyVoice(voice);
        return 0;
    }
    return voice;
}

// ---------------------------------------------------------------------------
// SoundStream::DestroyVoice (__thiscall, 1 arg). Reset the voice's
// feeder, reap its queued channel voices, release its IDirectSoundBuffer, unlink
// it from the +0x94 list, then run its scalar-deleting destructor.
RVA(0x001379d0, 0x5f)
void SoundStream::DestroyVoice(StreamVoiceNode* voice) {
    if (m_initialized) {
        voice->m_feeder.FeederReset(0);
        ((StreamVoiceList*)&m_voiceHead)->Reap(voice, 0xffff);
        voice->m_buf0c->vtbl->Release(voice->m_buf0c);
        voice->m_buf0c = 0;
        ((StreamList*)&m_94)->Unlink(voice ? &voice->m_link : 0);
        if (voice) {
            ((DSoundCloneBase*)voice)->ScalarDtor(1);
        }
    }
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
// control flow byte-exact; only the frame size / slot assignment differs - 98.3%,
// the entropy tail. Logic complete, deferred to the final sweep.
RVA(0x00137b70, 0x159)
i32 SoundStream::ParseWave(
    StreamSource* src,
    WaveFormatX* fmtBuf,
    u32* outDataOff,
    u32* outDataLen
) {
    i32 gotFmt = 0;
    i32 gotData = 0;
    src->Seek(0);

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
            src->Seek(next);
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
            src->Seek(src->m_cursor + 1);
        }
    }
    return 0;
}
