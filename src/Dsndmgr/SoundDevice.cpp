// SoundDevice.cpp - the DirectSound *device* manager (C:\Proj\Dsndmgr\DSNDMGR.CPP,
// vftable 0x5ef6c4; distinct class from the buffer wrapper DirectSoundMgr 0x5ef6b8).
// Owns a SoundBuf list (@+0x04, +4-biased links), the IDirectSound device (+0x14) and
// the primary buffer (+0x84); teardown releases each buffer then the primary + device.
#include <Dsndmgr/SoundDevice.h>
#include <Rez/RezMgr.h> // RezAlloc/RezFree - the engine heap allocator/deallocator
#include <math.h>       // acos / pow (intrinsic __CIacos / __CIpow) in VolumeToAttenuation
#include <rva.h>
#include <stdio.h> // FILE - the CRT stream Eng_fopen returns (its _file fd feeds Eng_filelength)
#include <Globals.h>

// The __FILE__ string the device passes to GetErrorString (the shared DSNDMGR.CPP
// $SG pooled constant, 0x619ef8).
#define DSNDMGR_FILE "C:\\Proj\\Dsndmgr\\DSNDMGR.CPP"

// DSBUFFERDESC.dwSize (the 0x14-byte sound-buffer descriptor).
#define DSBUFFERDESC_SIZE 0x14

// A little-endian RIFF FourCC as a u32 (compile-time constant -> the same immediate
// the retail chunk-tag compares use).
#define WAVE_FOURCC(a, b, c, d)                                                                    \
    ((u32)(u8)(a) | ((u32)(u8)(b) << 8) | ((u32)(u8)(c) << 16) | ((u32)(u8)(d) << 24))

// RIFF/WAVE chunk parser 0x137110 (__cdecl): scan a RIFF blob for `fmt `/`data`,
// write fmt (*out), PCM ptr (*dataOut) + len (*sizeOut); nonzero if `fmt ` found.
extern "C" i32 ParseWaveChunks(void* riff, ParseFmt* out, void** dataOut, u32* sizeOut);

// Volume->attenuation table (0x653ab8), filled by BuildVolumeTable(i=0..100); indexed by
// SetVolumeByIndex. INCLUSIVE loop -> the 101st store lands on g_panTable[0] (retail).
DATA(0x00653ab8)
i32 g_volumeTable[100];

// ---------------------------------------------------------------------------
// VolumeToAttenuation (static __cdecl, x87): 0..100 volume -> centi-dB attenuation.
// 100->0, 0->-10000, else -acos(pow(1/(v/100),10))/acos(2)*100, floored via __ftol.
// @early-stop
// x87-fp-stack-schedule wall (docs/patterns/x87-fp-stack-schedule.md, 58%): retail spills
// `ratio` to [ebp-8] (push ebp frame + shared jmp epilogue); MSVC5 keeps it in st0
// (frameless) + an extra fxch. Not source-steerable; logic complete.
RVA(0x001350b0, 0x5d)
i32 SoundDevice::VolumeToAttenuation(i32 value) {
    if (value == 100) {
        return 0;
    }
    if (value == 0) {
        return -10000;
    }
    double t = (double)value / c_volScale;
    double ratio = acos(pow(c_volNum / t, c_powExp)) / acos(c_acosNorm);
    return (i32)(-(ratio * c_volScale));
}

// ---------------------------------------------------------------------------
// BuildVolumeTable: g_volumeTable[i] = VolumeToAttenuation(i), i=0..100 (101 stores).
RVA(0x001351a0, 0x23)
void SoundDevice::BuildVolumeTable() {
    for (i32 i = 0; i <= 100; i++) {
        g_volumeTable[i] = VolumeToAttenuation(i);
    }
}

// ---------------------------------------------------------------------------
// ctor (/GX EH frame): zero the two list members, stamp vptr, clear init flag,
// BuildVolumeTable, zero device/primary state. SoundStream derives -> base call here.
// @early-stop
// eh-dtor-needs-base-subobject wall (docs/patterns/eh-dtor-needs-base-subobject.md):
// retail's /GX frame comes from the fully-constructed object registering ~SoundDevice
// for unwind; MSVC5 emits a frameless body. Body faithful; same family as ~SoundDevice.
RVA(0x00136440, 0x74)
SoundDevice::SoundDevice() {
    // cl auto-stamps ??_7SoundDevice@@6B@ (0x5ef6c4).
    m_bufferList.m_head = 0;
    m_bufferList.m_tail = 0;
    m_voiceList.m_head = 0;
    m_voiceList.m_tail = 0;
    m_initialized = 0;
    BuildVolumeTable();
    m_reacquireProc = 0;
    m_primaryBuffer = 0;
    m_coopLevel = 0;
    m_bufferFlags = 0;
    m_force8Bit = 0;
}

// ---------------------------------------------------------------------------
// ~SoundDevice (/GX EH frame): cl resets vptr, then if init runs the teardown.
// @early-stop
// eh-dtor-needs-base-subobject wall (docs/patterns/eh-dtor-needs-base-subobject.md):
// body byte-exact, but retail's /GX frame comes from the fully-constructed base
// subobject; MSVC5's dtor is frameless. Same wall as ~DirectSoundMgr.
RVA(0x00136500, 0x43)
SoundDevice::~SoundDevice() {
    // cl auto-resets the vptr to ??_7SoundDevice@@6B@ (0x5ef6c4).
    if (m_initialized) {
        Shutdown();
    }
}

// Slot-0 scalar-deleting dtor (??_G) MSVC synthesizes from the virtual dtor; no source
// body -> pin by mangled name.
// @rva-symbol: ??_GSoundDevice@@UAEPAXI@Z 0x001364c0 0x1e

// ---------------------------------------------------------------------------
// Shutdown: RemoveBuffer each owned buffer, then Release the primary + device, clear
// the flag. The `head ? node-4 : 0` biased-pointer recovery is language-forced.
RVA(0x00136690, 0x58)
void SoundDevice::Shutdown() {
    if (m_initialized) {
        SoundBuf* node = elemOf<SoundBuf>(m_bufferList.m_head);
        while (node) {
            RemoveBuffer(node);
            node = elemOf<SoundBuf>(m_bufferList.m_head);
        }
        if (m_primaryBuffer) {
            m_primaryBuffer->Release();
        }
        m_device->Release();
    }
    m_initialized = 0;
}

// ---------------------------------------------------------------------------
// CreateBuffer (/GX EH frame): validate PCM fmt, CreateSoundBuffer, RezAlloc+BaseInit a
// SoundBuf, thread on the +0x04 list, seed fmt/avg-bytes/byte-count + duration.
// @early-stop
// RezAlloc+placement-new EH-frame wall (docs/patterns/rezalloc-placement-new-no-eh-frame.md):
// body byte-exact, but retail's `new`-with-RezAlloc-operator-new emits a /GX
// ctor-in-flight frame the RezAlloc+BaseInit path can't reproduce. Same wall as
// SoundStream::CreateStreamBuffer.
RVA(0x001366f0, 0x168)
DirectSoundMgr* SoundDevice::CreateBuffer(WaveFormatX* fmt, u32 bytes, u32 flags) {
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

    // The 16-byte WAVEFORMATEX copy: retail moves it as dword@0, dword@4, dword@8,
    // dword@0xc, word@0x10 (verified). The two u16-pair fields (wFormatTag|nChannels
    // and nBlockAlign|wBitsPerSample) must be punned to a single dword store -
    // field-by-field would emit two 16-bit moves. Language-forced (verified by disasm).
    WaveFormatX wf;
    *(u32*)&wf.wFormatTag = *(u32*)&fmt->wFormatTag;
    wf.nSamplesPerSec = fmt->nSamplesPerSec;
    wf.nAvgBytesPerSec = fmt->nAvgBytesPerSec;
    *(u32*)&wf.nBlockAlign = *(u32*)&fmt->nBlockAlign;
    wf.cbSize = fmt->cbSize;

    IDirectSoundBufferZ* out = 0;
    DSBUFFERDESC desc;
    desc.dwSize = DSBUFFERDESC_SIZE;
    desc.dwFlags = flags;
    desc.dwBufferBytes = bytes;
    desc.dwReserved = 0;
    desc.lpwfxFormat = &wf;

    i32 hr = m_device->CreateSoundBuffer(&desc, &out, 0) != 0;
    if (hr) {
        DirectSoundMgr::GetErrorString(DSNDMGR_FILE, 0x422, hr);
        return 0;
    }
    if (out == 0) {
        return 0;
    }

    // RezAlloc the 0x60B leaf, then BaseInit (external ctor 0x135b10) stamps its vptr.
    SoundBuf* voice = (SoundBuf*)RezAlloc(0x60);
    if (voice) {
        voice->BaseInit(out, this);
    }
    voice->m_formatWord = *(u32*)&wf.wFormatTag;
    m_bufferList.InsertHead(voice ? &voice->m_link : 0);
    voice->m_avgBytesPerSec = fmt->nAvgBytesPerSec;
    voice->m_avgBytesPerSecDivisor = fmt->nAvgBytesPerSec;
    voice->m_byteCount = bytes;
    voice->ComputeDuration();
    // Bridge SoundBuf -> the DirectSoundMgr wrapper (same object; FIXME unify the types).
    return (DirectSoundMgr*)voice;
}

// Engine fopen 0x11f870 (CRT FILE*) + file-size query 0x18c480 (reads FILE._file fd).
extern "C" FILE* Eng_fopen(const char* path, const char* mode); // 0x11f870
extern "C" u32 Eng_filelength(i32 fd);                          // 0x18c480

// "rb" open-mode string the loader passes fopen (.data @ 0x60b668).
DATA(0x0020b668)
extern const char s_rb[];

// ---------------------------------------------------------------------------
// AcquireFile: gated on init; fopen "rb", slurp whole file into a new'd buffer, Acquire
// the RIFF blob, free + close. Returns the wrapper (0 on any I/O failure).
RVA(0x00136860, 0xa9)
DirectSoundMgr* SoundDevice::AcquireFile(char* path, u32 flags, u32 reserved) {
    if (m_initialized == 0) {
        return 0;
    }
    FILE* fp = Eng_fopen(path, s_rb);
    if (fp == 0) {
        return 0;
    }
    u32 size = Eng_filelength(fp->_file);
    void* buf = operator new(size);
    if (RezFRead(buf, size, 1, fp) != 1) {
        RezFClose(fp);
        operator delete(buf);
        return 0;
    }
    RezFClose(fp);
    DirectSoundMgr* wrapper = Acquire(buf, flags, reserved);
    operator delete(buf);
    return wrapper;
}

// ---------------------------------------------------------------------------
// Acquire: parse RIFF/WAVE fmt+data, optionally 16->8 downconvert (m_force8Bit or parse
// flag), CreateBuffer, LockConvert the PCM in; RemoveBuffer on load failure.
// @early-stop
// frame-homing-area-reuse wall (docs/patterns/stack-buffer-size-drives-frame.md, 99.8%):
// every code byte IDENTICAL; retail overlays the ParseFmt out-struct onto dead arg-home
// slots (sub esp,8) while MSVC5 gives fresh stack (sub esp,0x18). Not source-steerable.
RVA(0x00136910, 0x119)
DirectSoundMgr* SoundDevice::Acquire(void* riff, u32, u32) {
    if (m_initialized == 0) {
        return 0;
    }
    if (riff == 0) {
        return 0;
    }

    ParseFmt po;
    void* data;
    u32 size;
    po.m_reservedC = 0;
    po.m_flags = 0;
    po.m_reservedA = 0;
    if (ParseWaveChunks(riff, &po, &data, &size) == 0) {
        return 0;
    }

    i32 cvt = 0;
    if (m_force8Bit != 0 || (po.m_flags & 1) == 1) {
        cvt = 1;
    }
    if (po.m_fmt->wBitsPerSample != 0x10 || po.m_fmt->wFormatTag != 1) {
        cvt = 0;
    }
    if (cvt) {
        size >>= 1;
        po.m_fmt->wBitsPerSample = 8;
        po.m_fmt->nAvgBytesPerSec >>= 1;
        po.m_fmt->nBlockAlign >>= 1;
    }

    DirectSoundMgr* wrapper = CreateBuffer(po.m_fmt, size, po.m_flags);
    if (wrapper == 0) {
        return 0;
    }
    if (wrapper->LockConvert(data, size, cvt) == 0) {
        // Bridge DirectSoundMgr -> the SoundBuf list node RemoveBuffer unlinks (FLAG:
        // same object as CreateBuffer's return; unifying the types drops this cast).
        RemoveBuffer((SoundBuf*)wrapper);
        return 0;
    }
    return wrapper;
}

// ---------------------------------------------------------------------------
// ValidateRestore: gated on init; require size + fmt (non-null) + wFormatTag==1, then
// Restore the buffer and return its 0/1 success.
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

// ---------------------------------------------------------------------------
// ReloadRiff: re-load a RIFF into an EXISTING buffer (Acquire sibling). Gate on init +
// non-null RIFF + buffer looping; parse, optional 16->8 downconvert, Restore, LockConvert.
// @early-stop
// frame-homing-area-reuse wall (docs/patterns/stack-buffer-size-drives-frame.md): same
// family as Acquire (fresh stack vs retail's arg-home overlay). Not source-steerable.
RVA(0x00136bd0, 0x110)
i32 SoundDevice::ReloadRiff(DirectSoundMgr* buf, void* riff, u32 /*reserved*/) {
    if (m_initialized == 0) {
        return 0;
    }
    if (riff == 0) {
        return 0;
    }
    if (buf->IsLooping() == 0) {
        return 1;
    }

    ParseFmt po;
    void* data;
    u32 size;
    po.m_reservedC = 0;
    po.m_flags = 0;
    po.m_reservedA = 0;
    if (ParseWaveChunks(riff, &po, &data, &size) == 0) {
        return 0;
    }

    i32 cvt = 0;
    if (m_force8Bit != 0 || (po.m_flags & 1) == 1) {
        cvt = 1;
    }
    if (po.m_fmt->wBitsPerSample != 0x10 || po.m_fmt->wFormatTag != 1) {
        cvt = 0;
    }
    if (cvt) {
        size >>= 1;
        po.m_fmt->wBitsPerSample = 8;
        po.m_fmt->nAvgBytesPerSec >>= 1;
        po.m_fmt->nBlockAlign >>= 1;
    }

    if (ValidateRestore(buf, po.m_fmt, size) == 0) {
        return 0;
    }
    return buf->LockConvert(data, size, cvt) != 0;
}

// ---------------------------------------------------------------------------
// RemoveBuffer: reap the buffer's voices (keyed by its address), Release its COM
// buffer, unlink from the owned-buffer list, then scalar-delete it.
RVA(0x00136d80, 0x56)
void SoundDevice::RemoveBuffer(SoundBuf* node) {
    if (m_initialized) {
        // The voices carry the owning buffer's address as their reap key.
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

// ---------------------------------------------------------------------------
// StopAll: StopAndRewind + StopAllClones each owned buffer.
RVA(0x00136de0, 0x3c)
void SoundDevice::StopAll() {
    if (m_initialized) {
        SoundBuf* node = elemOf<SoundBuf>(m_bufferList.m_head);
        while (node) {
            node->StopAndRewind();
            node->StopAllClones();
            node = elemOf<SoundBuf>(node->m_link.m_next);
        }
    }
}

// ---------------------------------------------------------------------------
// FreeSamples: walk the voice list; per node run its slot-1 stop, unlink, then
// `delete (PureSoundElem*)node` (pure-base teardown + RezFree). Returns 1.
// @early-stop
// regalloc/early-out scheduling wall (77%): retail reserves all 4 callee-saved regs and
// runs the early-out in-frame; MSVC emits a leaner frameless early-out. Loop byte-exact.
RVA(0x00136ed0, 0x72)
i32 SoundDevice::FreeSamples() {
    if (m_initialized == 0) {
        return 0;
    }
    DSoundElem* node = elemOf<DSoundElem>(m_voiceList.m_head);
    while (node) {
        DSoundLink* n = node->m_link.m_next;
        DSoundElem* next = elemOf<DSoundElem>(n);
        node->Stop(); // slot 1: stop the element before freeing it
        m_voiceList.Unlink(node ? &node->m_link : 0);
        if (node) {
            // pure-base teardown: reset vptr to ??_7PureSoundElem (0x5ef6c8) + RezFree.
            PureSoundElem* pure = node;
            delete pure;
        }
        node = next;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// ParseWaveChunks (__cdecl): verify 'RIFF'/'WAVE', walk even-aligned chunks, record the
// 'fmt ' payload into out->m_fmt and 'data' ptr/len into *dataOut/*sizeOut; nonzero when
// 'fmt ' seen before 'data'.
// @early-stop
// add-fold scheduling wall (98.2%): byte-identical except the per-chunk cursor advance -
// source `p += 2` -> two `add $4` (retail) vs one `add $8` (MSVC5 /O2 fold). Not steerable.
RVA(0x00137110, 0x8d)
SYMBOL(_ParseWaveChunks)
extern "C" i32 ParseWaveChunks(void* riff, ParseFmt* out, void** dataOut, u32* sizeOut) {
    u32* p = (u32*)((char*)riff + 4);
    u32 riffSize = *p;
    p++;
    u32 waveTag = *p;
    p++;
    char* end = (char*)p + riffSize - 4;
    if (*(u32*)riff != WAVE_FOURCC('R', 'I', 'F', 'F')) {
        return 0;
    }
    if (waveTag != WAVE_FOURCC('W', 'A', 'V', 'E')) {
        return 0;
    }
    out->m_fmt = 0;
    *dataOut = 0;
    while ((char*)p < end) {
        u32 id = p[0];
        u32 size = p[1];
        p += 2;
        if (id == WAVE_FOURCC('f', 'm', 't', ' ')) {
            out->m_fmt = (WaveFormatX*)p;
        } else if (id == WAVE_FOURCC('d', 'a', 't', 'a')) {
            *dataOut = p;
            *sizeOut = size;
            return out->m_fmt != 0;
        }
        p = (u32*)((char*)p + ((size + 1) & ~1));
    }
    return 0;
}

// ---------------------------------------------------------------------------
// SetPrimaryFormat: ensure the primary buffer exists, then SetFormat; report + bail.
RVA(0x001371a0, 0x5a)
i32 SoundDevice::SetPrimaryFormat(void* fmt) {
    if (m_initialized == 0) {
        return 0;
    }
    if (CreatePrimaryBuffer() == 0) {
        return 0;
    }
    i32 hr = m_primaryBuffer->SetFormat(fmt) != 0;
    if (hr) {
        DirectSoundMgr::GetErrorString(DSNDMGR_FILE, 0x678, hr);
        return 0;
    }
    return 1;
}
