// SoundDevice.cpp - the DirectSound *device* manager's lifecycle/teardown run
// (C:\Proj\Dsndmgr\DSNDMGR.CPP, retail vftable 0x5ef6c4). Owns a list of
// per-buffer DirectSoundMgr wrappers (head @ +0x04, chained through each
// wrapper's +0x04 link with the MFC POSITION +4 bias), the IDirectSound device
// (+0x14) and primary buffer (+0x84). The teardown releases every buffer's
// voices + COM interface, then the primary + device.
//
// Trace called this class "MinervaInner"; its 0x5ef6c4 vtable proves it is a
// distinct class from the buffer wrapper DirectSoundMgr (0x5ef6b8). Field names
// are placeholders; offsets + emitted bytes are load-bearing.
#include <Dsndmgr/SoundDevice.h>
#include <Rez/RezMgr.h> // RezAlloc/RezFree - the engine heap allocator/deallocator
#include <math.h>       // acos / pow (intrinsic __CIacos / __CIpow) in VolumeToAttenuation
#include <rva.h>
#include <Globals.h>

// The __FILE__ string the device wrappers pass to GetErrorString (the shared
// DSNDMGR.CPP $SG pooled constant, 0x619ef8).
#define DSNDMGR_FILE "C:\\Proj\\Dsndmgr\\DSNDMGR.CPP"

// DSBUFFERDESC.dwSize (the 0x14-byte sound-buffer descriptor).
#define DSBUFFERDESC_SIZE 0x14

// Placement new (construct in place into the RezAlloc'd block); no allocation, so
// matching-neutral - it just runs the base init (BaseInit, 0x135b10) on the raw
// RezAlloc result, exactly as the retail RezAlloc-then-construct does.
inline void* operator new(u32, void* p) {
    return p;
}

// The plain sound-sample voice CreateBuffer wraps a created IDirectSoundBuffer in:
// a 0x60-byte base DirectSoundMgr-buffer object (retail vtable 0x5ef6bc, stamped
// by BaseInit). It shares the per-buffer this-shape (the +0x04 word doubles as the
// owned-buffer-list link); CreateBuffer seeds its format word (+0x18), avg-bytes
// (+0x38/+0x3c) and byte count (+0x2c) then derives the duration-ms (+0x28) via
// ComputeDuration. BaseInit (0x135b10) + ComputeDuration (0x1359a0) are the shared
// DirectSoundMgr-base helpers (defined in their own TUs); reloc-masked __thiscall.
struct SampleVoice {
    void* m_vtbl;        // +0x00  (retail vtable 0x5ef6bc; virtuals external)
    SampleVoice* m_link; // +0x04  owned-buffer-list link, biased +4 (POSITION)
    char m_pad08[0x18 - 0x08];
    u32 m_formatWord; // +0x18  wFormatTag|nChannels of the WAVEFORMATEX
    char m_pad1c[0x28 - 0x1c];
    u32 m_durationMs; // +0x28  duration-ms (= m_byteCount*1000/m_avgBytesPerSecDivisor)
    u32 m_byteCount;  // +0x2c
    char m_pad30[0x38 - 0x30];
    u32 m_avgBytesPerSec;        // +0x38
    u32 m_avgBytesPerSecDivisor; // +0x3c
    char m_pad40[0x60 - 0x40];

    void BaseInit(IDirectSoundBufferZ* buf, SoundDevice* owner); // 0x135b10
    void ComputeDuration();                                      // 0x1359a0
};
SIZE(SampleVoice, 0x60); // 0x60-byte base DirectSoundMgr-buffer object

// The fmt-chunk descriptor ParseWaveChunks fills (its `out` param). m_fmt is the
// WAVEFORMATEX pointer into the RIFF blob; m_flag is a parse flag Acquire tests
// (force-8-bit). Its address escapes to the parser, so the pre-zeroed fields stay
// live (not constant-folded).
struct ParseFmt {
    WaveFormatX* m_fmt; // +0x00
    u32 m_04;           // +0x04
    u32 m_flag;         // +0x08  (Acquire: bit 0 forces an 8-bit downconvert)
    u32 m_0c;           // +0x0c
    u32 m_10;           // +0x10
};
SIZE_UNKNOWN(ParseFmt); // parser scratch descriptor (address escapes)

// The RIFF/WAVE chunk parser (0x137110, __cdecl): scans a RIFF blob for the `fmt `
// and `data` chunks, writing the fmt descriptor (*out), the PCM data pointer
// (*dataOut) and its byte length (*sizeOut); returns nonzero when a `fmt ` chunk
// was found. Reloc-masked rel32; defined elsewhere.
extern "C" i32 ParseWaveChunks(void* riff, ParseFmt* out, void** dataOut, u32* sizeOut);

// The owned-buffer-list prepend helper (0x1390e0, __thiscall on the +0x04 head):
// link one node (biased +4) at the head. Same engine helper as DSoundList::InsertHead.

// The abstract-base ("pure") vftable (0x5ef6c8) a freed sample's vptr is restamped
// to before RezFree - a transitional reloc-masked DIR32 store.
DATA(0x001ef6c8)
extern void* const g_PureVtbl[];

// The device's voice/channel sub-list reap helper (0x136f60, __thiscall on the
// +0x0c list head): unlink + free every voice matching (arg, mask). Same engine
// helper DirectSoundMgr.cpp models as DSoundVoiceList::Reap.
struct SoundVoiceList {
    void* m_head;
    void* m_tail;
    void Reap(void* node, i32 mask); // 0x136f60
};
SIZE_UNKNOWN(SoundVoiceList); // {head,tail} list-head view

// The intrusive buffer-list unlink helper (0x1391e0, __thiscall on the +0x04
// list head): unlink one node given its anchor. Same as DSoundCloneList::Unlink.
struct SoundBufList {
    void* m_head;
    void* m_tail;
    void Insert(void* anchor); // 0x1390e0  prepend one biased-+4 node
    void Unlink(void* anchor); // 0x1391e0
};
SIZE_UNKNOWN(SoundBufList); // {head,tail} list-head view

// ALL-VTABLES phase: the device vftable (0x5ef6c4) is now cl-emitted as
// ??_7SoundDevice@@6B@ from the real polymorphic SoundDevice (virtual dtor); the
// ctor auto-stamps it and the dtor auto-resets it (was the manual g_SoundDeviceVtbl
// stamps).

// SoundBuf::StopAndRewind (0x135380) / StopAllClones (0x136150) are external
// DirectSoundMgr buffer methods (defined in DirectSoundMgr.cpp); declared with no
// body here so their __thiscall calls reloc-mask as rel32.

// The volume->attenuation lookup table (0x653ab8), filled at device-ctor time by
// BuildVolumeTable with VolumeToAttenuation(i) for i=0..100. SetVolumeByIndex
// (DirectSoundMgr.cpp) indexes it. The fill loop runs INCLUSIVE to index 100 -
// the 101st store lands on g_panTable[0] (0x653c48), the adjacent pan table, which
// the pan-table init overwrites; that is the retail behaviour.
DATA(0x00653ab8)
i32 g_volumeTable[100];

// The x87 transfer-curve constants VolumeToAttenuation reads (.rdata doubles);
// reloc-masked DIR32 operands, named here so the references pair.

// The engine global operator delete (RezFree-backed, 0x1b9b82) the scalar-deleting
// destructor tail-calls; reloc-masked rel32 (also redeclared near AcquireFile).
void operator delete(void*);

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
// ordering are not steerable from C (3 spellings tried: 58% frameless / 43% swapped /
// 57.9% w-spill). 58% - logic complete, deferred to the final sweep.
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
// SoundDevice::SoundDevice (__thiscall, /GX EH frame). Zero the two
// intrusive list members (owned-buffer +0x04/+0x08, voice +0x0c/+0x10), stamp the
// device vptr, clear the init flag, BuildVolumeTable, then zero the device/primary
// state. SoundStream derives from this, so its ctor emits the base call here.
// @early-stop
// eh-dtor-needs-base-subobject wall (docs/patterns/eh-dtor-needs-base-subobject.md):
// retail's /GX frame (push -1 / fs:0 setup + the 0->1 trylevel around BuildVolumeTable)
// comes from the two intrusive list members being real destructible subobjects whose
// dtors register on the unwind; the manual-vptr non-polymorphic model (kept so the 6
// exact siblings + the @early-stop dtor don't regress) emits a frameless body. Body
// (member zeros + vptr stamp + BuildVolumeTable + return this) is faithful; same family
// as ~SoundDevice (0x136500). Deferred to the final sweep when the whole class (ctor +
// dtor + the list members) is modelled as destructible subobjects atomically.
RVA(0x00136440, 0x74)
SoundDevice::SoundDevice() {
    // cl auto-stamps ??_7SoundDevice@@6B@ (0x5ef6c4) here (was the manual store).
    m_bufferHead = 0;
    m_bufferTail = 0;
    m_voiceHead = 0;
    m_voiceTail = 0;
    m_initialized = 0;
    BuildVolumeTable();
    m_80 = 0;
    m_primaryBuffer = 0;
    m_coopLevel = 0;
    m_bufferFlags = 0;
    m_force8Bit = 0;
}

// ---------------------------------------------------------------------------
// SoundDevice::ScalarDtor (__thiscall) - the ??_G vtable slot-0
// scalar-deleting destructor MSVC auto-emits for the virtual dtor: run ~SoundDevice,
// then operator delete (the engine RezFree) when the low flag bit is set; returns
// this. Modeled as a plain method (name-independent at delink) since SoundDevice is
// kept non-polymorphic (manual vptr stamp); same shape as SoundStream::ScalarDtor.
RVA(0x001364c0, 0x1e)
void* SoundDevice::ScalarDtor(i32 flag) {
    this->~SoundDevice();
    if (flag & 1) {
        operator delete(this);
    }
    return this;
}

// ---------------------------------------------------------------------------
// SoundDevice::~SoundDevice (__thiscall, /GX EH frame). Stamp the
// device vptr, then if initialized run the full teardown.
// @early-stop
// eh-dtor-needs-base-subobject wall (docs/patterns/eh-dtor-needs-base-subobject.md):
// the vptr-stamp + `if(m_initialized) Shutdown()` body is byte-exact, but the retail /GX EH
// frame (push -1 / fs:0 setup / trylevel) comes from a non-trivial base subobject
// the manual-vptr non-polymorphic model can't emit. Same wall as DirectSoundMgr::
// ~DirectSoundMgr (0x135bb0). Defer to the final sweep when the full base+vtable
// is modeled; converting now would regress the 3 exact siblings.
RVA(0x00136500, 0x43)
SoundDevice::~SoundDevice() {
    // cl auto-resets the vptr to ??_7SoundDevice@@6B@ (0x5ef6c4) here (was manual).
    if (m_initialized) {
        Shutdown();
    }
}

// ---------------------------------------------------------------------------
// SoundDevice::Shutdown (__thiscall). Release every owned buffer
// wrapper (RemoveBuffer drains its voices, releases its COM buffer + unlinks +
// destroys it), then the primary buffer and the device, then clear the flag.
RVA(0x00136690, 0x58)
void SoundDevice::Shutdown() {
    if (m_initialized) {
        SoundBuf* node = m_bufferHead ? (SoundBuf*)((char*)m_bufferHead - 4) : 0;
        while (node) {
            RemoveBuffer(node);
            node = m_bufferHead ? (SoundBuf*)((char*)m_bufferHead - 4) : 0;
        }
        if (m_primaryBuffer) {
            m_primaryBuffer->vtbl->Release(m_primaryBuffer);
        }
        m_device->vtbl->Release(m_device);
    }
    m_initialized = 0;
}

// ---------------------------------------------------------------------------
// SoundDevice::CreateBuffer (__thiscall, /GX EH frame). Validate the
// PCM WAVEFORMATEX, build a DSBUFFERDESC and ask the IDirectSound device (+0x14)
// for a sound buffer, then RezAlloc + BaseInit a SampleVoice wrapping it, thread
// it on the +0x04 owned-buffer list, and seed its format/avg-bytes/byte-count +
// derived duration.
// @early-stop
// RezAlloc+placement-new EH-frame wall (docs/patterns/rezalloc-placement-new-no-eh-frame.md):
// the body (fmt copy, CreateSoundBuffer, RezAlloc, BaseInit, list insert,
// ComputeDuration) is byte-exact, but retail's `new`-with-RezAlloc-operator-new
// emits a /GX ctor-in-flight EH frame (push -1/fs:0 + trylevel) and a single
// shared `jmp` epilogue that MSVC5's placement-new (no placement operator delete)
// cannot reproduce. Same wall as the sibling SoundStream::CreateStreamBuffer
// (0x137780, 47%). Defer to the final sweep once the SampleVoice base ctor is
// modelled and a real `new T` allocator path emits the frame.
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

    i32 hr = m_device->vtbl->CreateSoundBuffer(m_device, &desc, &out, 0) != 0;
    if (hr) {
        DirectSoundMgr::GetErrorString(DSNDMGR_FILE, 0x422, hr);
        return 0;
    }
    if (out == 0) {
        return 0;
    }

    SampleVoice* voice = (SampleVoice*)RezAlloc(0x60);
    if (voice) {
        voice = new (voice) SampleVoice;
        voice->BaseInit(out, this);
    }
    voice->m_formatWord = *(u32*)&fmt->wFormatTag;
    ((SoundBufList*)&m_bufferHead)->Insert(voice ? &voice->m_link : 0);
    voice->m_avgBytesPerSec = fmt->nAvgBytesPerSec;
    voice->m_avgBytesPerSecDivisor = fmt->nAvgBytesPerSec;
    voice->m_byteCount = bytes;
    voice->ComputeDuration();
    return (DirectSoundMgr*)voice;
}

// The CRT file helpers AcquireFile drives: fopen (0x11f870, returns a FILE*),
// a file-size query (0x18c480, reads the FILE*'s +0x10 fd), fread (RezFRead,
// 0x18c220) and fclose (RezFClose, 0x11f780, from <Rez/RezMgr.h>). The whole-file
// buffer is allocated/freed through the global operator new/delete.
extern "C" void* Eng_fopen(const char* path, const char* mode); // 0x11f870
extern "C" u32 Eng_filelength(i32 fd);                          // 0x18c480
void* operator new(u32);                                        // 0x1b9b46
void operator delete(void*);                                    // 0x1b9b82

// The "rb" open mode string the loader passes fopen (.data constant @ 0x60b668).
DATA(0x0020b668)
extern const char s_rb[];

// ---------------------------------------------------------------------------
// SoundDevice::AcquireFile (__thiscall, ret 0xc => 3 args). Gated on
// init. fopen the path "rb", read the whole file into a freshly-new'd buffer (its
// length from the FILE* fd), Acquire that RIFF blob, then free the buffer and close
// the file. Returns the acquired buffer wrapper (0 on any I/O failure).
RVA(0x00136860, 0xa9)
DirectSoundMgr* SoundDevice::AcquireFile(char* path, u32 a2, u32 a3) {
    if (m_initialized == 0) {
        return 0;
    }
    void* fp = Eng_fopen(path, s_rb);
    if (fp == 0) {
        return 0;
    }
    u32 size = Eng_filelength(*(i32*)((char*)fp + 0x10));
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
// SoundDevice::Acquire (__thiscall). Parse a RIFF/WAVE blob for its PCM
// fmt + data extents, optionally downconvert a 16-bit PCM format to 8-bit (when
// the device forces it, m_force8Bit, or the parse flags request it), create a buffer for
// the format, then load the PCM data into it; on a load failure the buffer is
// removed. Only the first arg is live - the other two slots are scratch.
// @early-stop
// frame-homing-area-reuse wall (docs/patterns/stack-buffer-size-drives-frame.md):
// every code byte is IDENTICAL to retail - the only diff is the frame size, retail
// `sub esp,8` overlays the ParseFmt out-struct onto the dead incoming-arg homing
// slots ([esp+0x18..]) while MSVC5 here gives it fresh stack (`sub esp,0x18`),
// shifting the [esp+N] operands by 0x10. MSVC5's /O2 won't write a local above its
// own params into the caller frame, so the param-homing pack isn't steerable from
// C source. 99.8% - logic + every instruction complete, deferred to the final sweep.
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
    po.m_10 = 0;
    po.m_flag = 0;
    po.m_04 = 0;
    if (ParseWaveChunks(riff, &po, &data, &size) == 0) {
        return 0;
    }

    i32 cvt = 0;
    if (m_force8Bit != 0 || (po.m_flag & 1) == 1) {
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

    DirectSoundMgr* wrapper = CreateBuffer(po.m_fmt, size, po.m_flag);
    if (wrapper == 0) {
        return 0;
    }
    if (wrapper->LockConvert(data, size, cvt) == 0) {
        RemoveBuffer((SoundBuf*)wrapper);
        return 0;
    }
    return wrapper;
}

// ---------------------------------------------------------------------------
// SoundDevice::ValidateRestore (__thiscall, ret 0xc => 3 args). Gated on
// init. Validate the size + fmt (non-null) and the PCM format tag (wFormatTag==1),
// then Restore the buffer and return its normalized 0/1 success.
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
// SoundDevice::ReloadRiff (__thiscall, ret 0xc => 3 args). Re-load a RIFF
// blob into an EXISTING buffer wrapper (the Acquire sibling that reuses a buffer
// instead of creating one): gate on init + a non-null RIFF, only proceed when the
// buffer is currently looping, parse the chunks, optionally downconvert a 16-bit
// PCM format to 8-bit (forced by m_force8Bit or the parse flag), validate+Restore the
// buffer, then LockConvert the PCM data into it. Returns the LockConvert success.
// @early-stop
// frame-homing-area-reuse wall (docs/patterns/stack-buffer-size-drives-frame.md):
// same family as the sibling Acquire (0x136910, 99.8%) - the body is complete and
// the parse/downconvert/validate/lock shape matches, but MSVC5 gives the ParseFmt
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
    po.m_10 = 0;
    po.m_flag = 0;
    po.m_04 = 0;
    if (ParseWaveChunks(riff, &po, &data, &size) == 0) {
        return 0;
    }

    i32 cvt = 0;
    if (m_force8Bit != 0 || (po.m_flag & 1) == 1) {
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
// SoundDevice::RemoveBuffer (__thiscall, 1 arg). Reap the buffer's
// queued voices, release its IDirectSoundBuffer, unlink it from the owned-buffer
// list, then run its scalar-deleting destructor.
RVA(0x00136d80, 0x56)
void SoundDevice::RemoveBuffer(SoundBuf* node) {
    if (m_initialized) {
        ((SoundVoiceList*)&m_voiceHead)->Reap(node, 0xffff);
        if (node->m_buf0c) {
            node->m_buf0c->vtbl->Release(node->m_buf0c);
            node->m_buf0c = 0;
        }
        ((SoundBufList*)&m_bufferHead)->Unlink(node ? &node->m_link : 0);
        if (node) {
            ((DSoundCloneBase*)node)->ScalarDtor(1);
        }
    }
}

// ---------------------------------------------------------------------------
// SoundDevice::StopAll (__thiscall). Walk the owned-buffer list,
// StopAndRewind + StopAllClones on each.
RVA(0x00136de0, 0x3c)
void SoundDevice::StopAll() {
    if (m_initialized) {
        SoundBuf* node = m_bufferHead ? (SoundBuf*)((char*)m_bufferHead - 4) : 0;
        while (node) {
            node->StopAndRewind();
            node->StopAllClones();
            node = node->m_link ? (SoundBuf*)((char*)node->m_link - 4) : 0;
        }
    }
}

// ---------------------------------------------------------------------------
// SoundDevice::FreeSamples (__thiscall). Walk the cached-sample list
// (+0x0c, biased +4 links): for each, run its slot-1 "free" virtual, unlink it,
// restamp its vptr to the pure base, and RezFree it. Returns 1.
// @early-stop
// regalloc/early-out scheduling wall: retail reserves all 4 callee-saved regs
// (ebx/ebp/esi/edi) in the prologue and runs the `if(!m_initialized) return 0` early-out
// IN-frame (popping all 4), while MSVC here emits a leaner frameless early-out
// before the saves. The loop body (slot-1 Free, neg/sbb/and arg, vptr restamp,
// RezFree) is byte-exact; only the prologue/early-out register scheduling shifts.
// 77% on a documented regalloc wall - logic complete, deferred to the final sweep.
RVA(0x00136ed0, 0x72)
i32 SoundDevice::FreeSamples() {
    if (m_initialized == 0) {
        return 0;
    }
    SoundSample* node = m_voiceHead ? (SoundSample*)((char*)m_voiceHead - 4) : 0;
    while (node) {
        SoundSample* next = node->m_link ? (SoundSample*)((char*)node->m_link - 4) : 0;
        node->Free();
        ((SoundBufList*)&m_voiceHead)->Unlink(node ? &node->m_link : 0);
        if (node) {
            *(void**)node = (void*)g_PureVtbl;
            RezFree(node);
        }
        node = next;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// ParseWaveChunks (__cdecl). Scan a RIFF/WAVE blob in memory: verify
// the 'RIFF'/'WAVE' magic, then walk the chunk list (each {u32 id; u32 size;
// payload} even-aligned), recording the 'fmt ' chunk payload into out->m_fmt and,
// on the 'data' chunk, the payload pointer/length into *dataOut/*sizeOut. Returns
// nonzero only when a 'fmt ' chunk was already seen by the time 'data' is found.
// @early-stop
// add-fold scheduling wall: the ENTIRE function is byte-identical except the
// per-chunk cursor advance past the 8-byte {id,size} header - retail emits it as
// two `add $4,eax` (the source `p += 2` after reading p[0]/p[1]), while MSVC5 /O2
// strength-reduces consecutive increments into one `add $8,eax`. Not steerable
// from C source (the optimizer folds `p++;p++` and `p+=2` identically). 98.2% -
// logic + every other instruction complete, deferred to the final sweep.
RVA(0x00137110, 0x8d)
SYMBOL(_ParseWaveChunks)
extern "C" i32 ParseWaveChunks(void* riff, ParseFmt* out, void** dataOut, u32* sizeOut) {
    u32* p = (u32*)((char*)riff + 4);
    u32 riffSize = *p;
    p++;
    u32 waveTag = *p;
    p++;
    char* end = (char*)p + riffSize - 4;
    if (*(u32*)riff != 0x46464952) {
        return 0;
    }
    if (waveTag != 0x45564157) {
        return 0;
    }
    out->m_fmt = 0;
    *dataOut = 0;
    while ((char*)p < end) {
        u32 id = p[0];
        u32 size = p[1];
        p += 2;
        if (id == 0x20746d66) {
            out->m_fmt = (WaveFormatX*)p;
        } else if (id == 0x61746164) {
            *dataOut = p;
            *sizeOut = size;
            return out->m_fmt != 0;
        }
        p = (u32*)((char*)p + ((size + 1) & ~1));
    }
    return 0;
}

// ---------------------------------------------------------------------------
// SoundDevice::SetPrimaryFormat (__thiscall, 1 arg). Ensure the primary
// buffer exists, then set its WAVEFORMATEX; report a failing HRESULT and bail.
RVA(0x001371a0, 0x5a)
i32 SoundDevice::SetPrimaryFormat(void* fmt) {
    if (m_initialized == 0) {
        return 0;
    }
    if (CreatePrimaryBuffer() == 0) {
        return 0;
    }
    i32 hr = m_primaryBuffer->vtbl->SetFormat(m_primaryBuffer, fmt) != 0;
    if (hr) {
        DirectSoundMgr::GetErrorString(DSNDMGR_FILE, 0x678, hr);
        return 0;
    }
    return 1;
}
