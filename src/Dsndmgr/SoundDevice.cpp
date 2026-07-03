// SoundDevice.cpp - the DirectSound *device* manager's lifecycle/teardown run
// (C:\Proj\Dsndmgr\DSNDMGR.CPP, retail vftable 0x5ef6c4). Owns a list of
// per-buffer SoundBuf wrappers (a DSoundList at +0x04, chained through each
// buffer's +0x04 link with the engine POSITION +4 bias), the IDirectSound device
// (+0x14) and the primary buffer (+0x84). The teardown releases every buffer's
// voices + COM interface, then the primary + device.
//
// The 0x5ef6c4 vtable proves SoundDevice is a distinct class from the buffer
// wrapper DirectSoundMgr (0x5ef6b8). Field names are placeholders; offsets +
// emitted bytes are load-bearing.
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

// The RIFF/WAVE chunk parser (0x137110, __cdecl): scans a RIFF blob for the `fmt `
// and `data` chunks, writing the fmt descriptor (*out), the PCM data pointer
// (*dataOut) and its byte length (*sizeOut); returns nonzero when a `fmt ` chunk
// was found. Reloc-masked rel32; defined below.
extern "C" i32 ParseWaveChunks(void* riff, ParseFmt* out, void** dataOut, u32* sizeOut);

// ALL-VTABLES phase: the device vftable (0x5ef6c4) is cl-emitted as
// ??_7SoundDevice@@6B@ from the real polymorphic SoundDevice (virtual dtor); the
// ctor auto-stamps it and the dtor auto-resets it.

// The volume->attenuation lookup table (0x653ab8), filled at device-ctor time by
// BuildVolumeTable with VolumeToAttenuation(i) for i=0..100. SetVolumeByIndex
// (DirectSoundMgr.cpp) indexes it. The fill loop runs INCLUSIVE to index 100 - the
// 101st store lands on g_panTable[0] (0x653c48), the adjacent pan table, which the
// pan-table init overwrites; that is the retail behaviour.
DATA(0x00653ab8)
i32 g_volumeTable[100];

// The engine global operator new / delete (RezAlloc/RezFree-backed, 0x1b9b46 /
// 0x1b9b82); reloc-masked rel32. The void* is the operator new/delete ABI.
void* operator new(u32);     // 0x1b9b46
void operator delete(void*); // 0x1b9b82

// ---------------------------------------------------------------------------
// SoundDevice::VolumeToAttenuation (static __cdecl, x87). Map a 0..100
// volume to a DSound hundredths-of-dB attenuation: 100 -> 0 (full), 0 -> -10000
// (silence), else an acos(pow(...))/acos(...) transfer scaled by c_volScale (==100),
// floored via __ftol. Constants (measured): c_volScale=100, c_volNum=1, c_powExp=10,
// c_acosNorm=2; w = pow(1/(v/100), 10), evaluated acos(w) first then acos(2).
// @early-stop
// x87-fp-stack-schedule wall (docs/patterns/x87-fp-stack-schedule.md): the integer
// scaffold (the 100/-10000 guards, the fild/fdiv/fdivr u-chain, the two __CIacos +
// __CIpow + __ftol calls with the right constants) is faithful, but retail spills the
// division result `ratio` to [ebp-8] (forcing a push ebp frame + a shared jmp epilogue)
// and loads c_powExp in natural order, while MSVC5 here keeps ratio in st0 (frameless,
// per-path ret) and hoists c_powExp + an extra fxch. The spill/frame and the fxch
// ordering are not steerable from C. 58% - logic complete, deferred to the final sweep.
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
// SoundDevice::BuildVolumeTable (static __cdecl). Fill g_volumeTable with
// VolumeToAttenuation(i), the pointer walking until it reaches g_panTable (0x653c48)
// inclusive (101 stores).
RVA(0x001351a0, 0x23)
void SoundDevice::BuildVolumeTable() {
    for (i32 i = 0; i <= 100; i++) {
        g_volumeTable[i] = VolumeToAttenuation(i);
    }
}

// ---------------------------------------------------------------------------
// SoundDevice::SoundDevice (__thiscall, /GX EH frame). Zero the two intrusive
// list members, stamp the device vptr (cl-implicit), clear the init flag,
// BuildVolumeTable, then zero the device/primary state. SoundStream derives from
// this, so its ctor emits the base call here.
// @early-stop
// eh-dtor-needs-base-subobject wall (docs/patterns/eh-dtor-needs-base-subobject.md):
// retail's /GX frame (push -1 / fs:0 setup + the 0->1 trylevel around BuildVolumeTable)
// comes from the fully-constructed object registering ~SoundDevice for unwind; MSVC5
// here emits a frameless body. Member zeros + vptr stamp + BuildVolumeTable are
// faithful; same family as ~SoundDevice (0x136500). Deferred to the final sweep.
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
// SoundDevice::~SoundDevice (__thiscall, /GX EH frame). cl resets the device
// vptr, then if initialized runs the full teardown.
// @early-stop
// eh-dtor-needs-base-subobject wall (docs/patterns/eh-dtor-needs-base-subobject.md):
// the vptr-reset + `if(m_initialized) Shutdown()` body is byte-exact, but retail's /GX
// EH frame (push -1 / fs:0 / trylevel) comes from the fully-constructed base subobject;
// MSVC5's emitted dtor is frameless. Same wall as DirectSoundMgr::~DirectSoundMgr
// (0x135bb0). Deferred to the final sweep.
RVA(0x00136500, 0x43)
SoundDevice::~SoundDevice() {
    // cl auto-resets the vptr to ??_7SoundDevice@@6B@ (0x5ef6c4).
    if (m_initialized) {
        Shutdown();
    }
}

// The slot-0 scalar-deleting destructor (??_G) MSVC synthesizes from the virtual
// dtor: run ~SoundDevice, then (flag&1) operator delete(this). The thunk has no
// source body to carry an RVA(), so pin it by mangled name.
// @rva-symbol: ??_GSoundDevice@@UAEPAXI@Z 0x001364c0 0x1e

// ---------------------------------------------------------------------------
// SoundDevice::Shutdown (__thiscall). Release every owned buffer (RemoveBuffer
// drains its voices, releases its COM buffer + unlinks + destroys it), then the
// primary buffer and the device, then clear the flag. The `head ? node-4 : 0`
// biased-pointer recovery (docs/patterns/biased-pointer-advance-ternary.md) walks
// the POSITION-biased list head to the owning node - no cast-free container_of
// exists in C++, so the reinterpret is language-forced (same as the shared
// DSoundList home).
RVA(0x00136690, 0x58)
void SoundDevice::Shutdown() {
    if (m_initialized) {
        SoundBuf* node = m_bufferList.m_head ? (SoundBuf*)((char*)m_bufferList.m_head - 4) : 0;
        while (node) {
            RemoveBuffer(node);
            node = m_bufferList.m_head ? (SoundBuf*)((char*)m_bufferList.m_head - 4) : 0;
        }
        if (m_primaryBuffer) {
            m_primaryBuffer->Release();
        }
        m_device->Release();
    }
    m_initialized = 0;
}

// ---------------------------------------------------------------------------
// SoundDevice::CreateBuffer (__thiscall, /GX EH frame). Validate the PCM
// WAVEFORMATEX, build a DSBUFFERDESC and ask the IDirectSound device (+0x14) for a
// sound buffer, then RezAlloc + BaseInit a SoundBuf wrapping it, thread it on the
// +0x04 owned-buffer list, and seed its format/avg-bytes/byte-count + duration.
// @early-stop
// RezAlloc+placement-new EH-frame wall (docs/patterns/rezalloc-placement-new-no-eh-frame.md):
// the body (fmt copy, CreateSoundBuffer, RezAlloc, BaseInit, list insert,
// ComputeDuration) is byte-exact, but retail's `new`-with-RezAlloc-operator-new emits
// a /GX ctor-in-flight EH frame (push -1/fs:0 + trylevel) and a single shared `jmp`
// epilogue that MSVC5's RezAlloc+BaseInit path cannot reproduce. Same wall as the
// sibling SoundStream::CreateStreamBuffer (0x137780). Deferred to the final sweep.
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

    // RezAlloc the 0x60-byte buffer leaf, then BaseInit (the external DSoundCloneInst
    // ctor 0x135b10) stamps its real vptr (0x5ef6bc); the raw-allocator result cast is
    // the usual malloc-style typing.
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
    // Bridge SoundBuf -> the DirectSoundMgr wrapper the callers see (FLAG: same object;
    // unifying SoundBuf into DirectSoundMgr would drop this cast).
    return (DirectSoundMgr*)voice;
}

// The engine fopen (0x11f870, returns a CRT FILE*) + file-size query (0x18c480,
// reads the FILE's _file fd). fread/fclose come from <Rez/RezMgr.h> (RezFRead /
// RezFClose). The whole-file buffer is new'd/deleted through the engine allocator.
extern "C" FILE* Eng_fopen(const char* path, const char* mode); // 0x11f870
extern "C" u32 Eng_filelength(i32 fd);                          // 0x18c480

// The "rb" open mode string the loader passes fopen (.data constant @ 0x60b668).
DATA(0x0020b668)
extern const char s_rb[];

// ---------------------------------------------------------------------------
// SoundDevice::AcquireFile (__thiscall, ret 0xc => 3 args). Gated on init.
// fopen the path "rb", read the whole file into a freshly-new'd buffer (its length
// from the FILE fd), Acquire that RIFF blob, then free the buffer and close the
// file. Returns the acquired buffer wrapper (0 on any I/O failure).
RVA(0x00136860, 0xa9)
DirectSoundMgr* SoundDevice::AcquireFile(char* path, u32 a2, u32 a3) {
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
    DirectSoundMgr* wrapper = Acquire(buf, a2, a3);
    operator delete(buf);
    return wrapper;
}

// ---------------------------------------------------------------------------
// SoundDevice::Acquire (__thiscall). Parse a RIFF/WAVE blob for its PCM fmt +
// data extents, optionally downconvert a 16-bit PCM format to 8-bit (forced by
// m_force8Bit or the parse flag), create a buffer for the format, then load the PCM
// data into it; on a load failure the buffer is removed. Only the first arg is live.
// @early-stop
// frame-homing-area-reuse wall (docs/patterns/stack-buffer-size-drives-frame.md):
// every code byte is IDENTICAL to retail - the only diff is the frame size, retail
// `sub esp,8` overlays the ParseFmt out-struct onto the dead incoming-arg homing slots
// while MSVC5 gives it fresh stack (`sub esp,0x18`), shifting the [esp+N] operands by
// 0x10. MSVC5's /O2 won't home a local into the caller frame, so the pack isn't
// steerable from C. 99.8% - every instruction complete, deferred to the final sweep.
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
// SoundDevice::ValidateRestore (__thiscall, ret 0xc => 3 args). Gated on init.
// Validate the size + fmt (non-null) and the PCM format tag (wFormatTag==1), then
// Restore the buffer and return its normalized 0/1 success.
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
// SoundDevice::ReloadRiff (__thiscall, ret 0xc => 3 args). Re-load a RIFF blob
// into an EXISTING buffer wrapper (the Acquire sibling that reuses a buffer): gate
// on init + a non-null RIFF, only proceed when the buffer is currently looping,
// parse the chunks, optionally downconvert a 16-bit PCM format to 8-bit (forced by
// m_force8Bit or the parse flag), validate+Restore the buffer, then LockConvert the
// PCM data into it. Returns the LockConvert success.
// @early-stop
// frame-homing-area-reuse wall (docs/patterns/stack-buffer-size-drives-frame.md):
// same family as the sibling Acquire (0x136910, 99.8%) - the body is complete and the
// parse/downconvert/validate/lock shape matches, but MSVC5 gives the ParseFmt
// out-struct fresh stack while retail overlays it onto the dead param-homing slots,
// shifting the [esp+N] operands. Not source-steerable; deferred to the final sweep.
RVA(0x00136bd0, 0x110)
i32 SoundDevice::ReloadRiff(DirectSoundMgr* buf, void* riff, u32 a3) {
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
// SoundDevice::RemoveBuffer (__thiscall, 1 arg). Reap the buffer's queued
// voices (keyed by the buffer's identity), release its IDirectSoundBuffer, unlink
// it from the owned-buffer list, then run its scalar-deleting destructor.
RVA(0x00136d80, 0x56)
void SoundDevice::RemoveBuffer(SoundBuf* node) {
    if (m_initialized) {
        // The voices carry the owning buffer's address as their reap key; the pointer
        // stored as a DWORD key is the authentic engine identity (pointer<->DWORD pun).
        m_voiceList.RemoveMatching((u32)node, 0xffff);
        if (node->m_buffer) {
            node->m_buffer->Release();
            node->m_buffer = 0;
        }
        m_bufferList.Unlink(node ? &node->m_link : 0);
        if (node) {
            node->ScalarDtor(1);
        }
    }
}

// ---------------------------------------------------------------------------
// SoundDevice::StopAll (__thiscall). Walk the owned-buffer list,
// StopAndRewind + StopAllClones on each (biased-pointer recovery per Shutdown).
RVA(0x00136de0, 0x3c)
void SoundDevice::StopAll() {
    if (m_initialized) {
        SoundBuf* node = m_bufferList.m_head ? (SoundBuf*)((char*)m_bufferList.m_head - 4) : 0;
        while (node) {
            node->StopAndRewind();
            node->StopAllClones();
            node = node->m_link.m_next ? (SoundBuf*)((char*)node->m_link.m_next - 4) : 0;
        }
    }
}

// ---------------------------------------------------------------------------
// SoundDevice::FreeSamples (__thiscall). Walk the cached-voice list (+0x0c,
// biased +4 links): for each, run its slot-1 "free" virtual, unlink it, restamp its
// vptr to the pure base, and RezFree it. Returns 1.
// @early-stop
// regalloc/early-out scheduling wall: retail reserves all 4 callee-saved regs
// (ebx/ebp/esi/edi) in the prologue and runs the `if(!m_initialized) return 0` early-out
// IN-frame (popping all 4), while MSVC here emits a leaner frameless early-out before
// the saves. The loop body (slot-1 Slot1, neg/sbb/and arg, vptr restamp, RezFree) is
// byte-exact; only the prologue/early-out register scheduling shifts. 77% on a
// documented regalloc wall - logic complete, deferred to the final sweep.
RVA(0x00136ed0, 0x72)
i32 SoundDevice::FreeSamples() {
    if (m_initialized == 0) {
        return 0;
    }
    DSoundElem* node = m_voiceList.m_head ? (DSoundElem*)((char*)m_voiceList.m_head - 4) : 0;
    while (node) {
        DSoundLink* n = node->m_link.m_next;
        DSoundElem* next = n ? (DSoundElem*)((char*)n - 4) : 0;
        node->Slot1(); // slot 1 = the element's "free" virtual
        m_voiceList.Unlink(node ? &node->m_link : 0);
        if (node) {
            // Reset the reaped element's vptr to the pure base (0x5ef6c8) before RezFree
            // - a dead store MSVC's dtor codegen would elide, so it stays explicit (same
            // as the shared DSoundList home).
            *(void**)node = (void*)PureSoundElemVtable;
            RezFree(node);
        }
        node = next;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// ParseWaveChunks (__cdecl). Scan a RIFF/WAVE blob in memory: verify the
// 'RIFF'/'WAVE' magic, then walk the chunk list (each {u32 id; u32 size; payload}
// even-aligned), recording the 'fmt ' chunk payload into out->m_fmt and, on the
// 'data' chunk, the payload pointer/length into *dataOut/*sizeOut. Returns nonzero
// only when a 'fmt ' chunk was already seen by the time 'data' is found.
// @early-stop
// add-fold scheduling wall: the ENTIRE function is byte-identical except the
// per-chunk cursor advance past the 8-byte {id,size} header - retail emits it as two
// `add $4,eax` (the source `p += 2` after reading p[0]/p[1]), while MSVC5 /O2
// strength-reduces consecutive increments into one `add $8,eax`. Not steerable from C.
// 98.2% - every other instruction complete, deferred to the final sweep.
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
// SoundDevice::SetPrimaryFormat (__thiscall, 1 arg). Ensure the primary buffer
// exists, then set its WAVEFORMATEX; report a failing HRESULT and bail.
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
