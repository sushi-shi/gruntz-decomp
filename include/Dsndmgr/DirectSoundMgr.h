// DirectSoundMgr.h - the WAP32 DirectSound per-buffer wrapper (Dsndmgr module,
// C:\Proj\Dsndmgr\). Surfaces the layout + COM interface shapes needed to
// byte-match DirectSoundMgr's DSNDMGR.CPP method run: the HRESULT->error-string
// diagnostic formatter GetErrorString (the DSound sibling of
// CDirectDrawMgr::GetErrorString) and the thin IDirectSound / IDirectSoundBuffer
// wrapper thunks that, on a nonzero HRESULT, route through GetErrorString.
//
// DirectSoundMgr is the per-buffer wrapper BASE (retail vtable 0x5ef6b8, size
// 0x44): it holds one IDirectSoundBuffer (m_buffer) plus its owning device
// (m_owner, a SoundDevice) and the cached caps/state. Two concrete leaves derive
// it (both in DirectSoundMgr.cpp): DSoundBaseSub (0x5ef6c0, 0x58 - the clone
// object Clone() news) and DSoundCloneInst (0x5ef6bc, 0x60 - the leaf that owns a
// clone list). The device-level bring-up (Create/SetCooperativeLevel/
// CreatePrimaryBuffer/ReacquireViaCallback) lives on SoundDevice.
//
// Each COM interface is a real abstract class (__stdcall virtuals), so a wrapper's
// `iface->Method(args...)` lowers to the retail `mov eax,[iface]; call [eax+slot]`
// COM dispatch; only the called slots carry meaningful signatures, the rest pad.
#ifndef DSNDMGR_DIRECTSOUNDMGR_H
#define DSNDMGR_DIRECTSOUNDMGR_H

#include <rva.h>
#include <ComDefs.h>                // STDMETHOD / HRESULT / REFIID - DirectSound COM macros
#include <Dsndmgr/SoundVoiceList.h> // DSoundLink / DSoundList intrusive list primitive

// DSBCAPS - the buffer-caps struct GetCaps fills (dwSize 0x14 in, dwFlags out).
// The ctor reads dwFlags into m_caps and ignores the rest.
struct DSBCAPS {
    u32 dwSize;
    u32 dwFlags;
    u32 dwBufferBytes;
    u32 dwUnlockTransferRate;
    u32 dwPlayCpuOverhead;
};
SIZE(DSBCAPS, 0x14); // 5-DWORD DirectSound buffer-caps struct

// DSBUFFERDESC - the 0x14-byte sound-buffer descriptor passed to
// IDirectSound::CreateSoundBuffer (dwSize, dwFlags, dwBufferBytes, dwReserved,
// lpwfxFormat). Only dwSize/dwFlags are stamped here; the rest is zeroed.
struct DSBUFFERDESC {
    u32 dwSize;
    u32 dwFlags;
    u32 dwBufferBytes;
    u32 dwReserved;
    void* lpwfxFormat;
};
SIZE(DSBUFFERDESC, 0x14); // 0x14-byte DirectSound buffer descriptor

struct IDirectSoundBufferZ; // forward-decl: CreateSoundBuffer's out-param type
class SoundDevice;          // owning device (m_owner); full def in SoundDevice.h
class DirectSoundMgr;       // a clone (CloneNode::m_inst back-points at it)

// A clone-list node: a self-referential doubly-linked node whose m_inst back-points
// at the buffer that owns it. Each buffer embeds one (m_cloneNode) by which it hangs in
// its parent's clone list; the list threads through these nodes directly, so
// iteration reads node->m_inst with no container-of arithmetic.
struct CloneNode {
    CloneNode* m_next;      // +0x00
    CloneNode* m_prev;      // +0x04
    DirectSoundMgr* m_inst; // +0x08  back-pointer to the owning buffer
};
SIZE(CloneNode, 0xc); // {next, prev, inst}

// ---------------------------------------------------------------------------
// IDirectSound (DSOUND) - the device interface DirectSoundCreate returns. Only
// the slots the manager calls are pinned. COM convention => __stdcall virtuals with
// the interface pointer as the hidden `this`; the manager invokes them as
// iface->Method(...).
//   +0x0c (slot 3)  CreateSoundBuffer  (LPCDSBUFFERDESC, LPDIRECTSOUNDBUFFER*, LPUNKNOWN)
//   +0x18 (slot 6)  SetCooperativeLevel(HWND, DWORD)
// ---------------------------------------------------------------------------
struct IDirectSoundZ {
    // Real COM interface (abstract), declared the dev-authentic SDK way with the
    // STDMETHOD / STDMETHOD_ macros (== `virtual HRESULT __stdcall` / `virtual u32
    // __stdcall`), so `iface->Method(args)` lowers to the same `mov eax,[iface];
    // call [eax+slot]` the manual vtbl-struct dispatch did. The IUnknown triad
    // (QueryInterface/AddRef/Release) heads the vtable.
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv) PURE; // slot 0
    STDMETHOD_(u32, AddRef)() PURE;                          // slot 1
    STDMETHOD_(u32, Release)() PURE;                         // slot 2  (+0x08)
    STDMETHOD(CreateSoundBuffer)(
        void* desc,
        IDirectSoundBufferZ** out,
        void* unk
    ) PURE;                              // slot 3  (+0x0c)
    STDMETHOD(GetCaps)(void* caps) PURE; // slot 4  (+0x10, unused)
    STDMETHOD(DuplicateSoundBuffer)(
        IDirectSoundBufferZ* original,
        IDirectSoundBufferZ** out
    ) PURE;                                                     // slot 5  (+0x14)
    STDMETHOD(SetCooperativeLevel)(void* hwnd, u32 level) PURE; // slot 6  (+0x18)
};
SIZE(IDirectSoundZ, 0x4); // COM interface: a single vptr (opaque SDK object, held by ptr)

// ---------------------------------------------------------------------------
// IDirectSoundBuffer (DSOUND) - the buffer interface the per-buffer wrappers
// drive. Slots pinned to their retail vtable offsets:
//   +0x08 (slot 2)  Release             ()
//   +0x10 (slot 4)  GetCurrentPosition  (LPDWORD, LPDWORD)
//   +0x14 (slot 5)  GetFormat           (LPWAVEFORMATEX, DWORD, LPDWORD)
//   +0x18 (slot 6)  GetVolume           (LPLONG)
//   +0x1c (slot 7)  GetPan              (LPLONG)
//   +0x24 (slot 9)  GetStatus           (LPDWORD)
//   +0x2c (slot 11) Lock                (DWORD, DWORD, LPVOID*, LPDWORD, LPVOID*, LPDWORD, DWORD)
//   +0x34 (slot 13) SetCurrentPosition  (DWORD)
//   +0x3c (slot 15) SetVolume           (LONG)
//   +0x40 (slot 16) SetPan              (LONG)
//   +0x44 (slot 17) SetFrequency        (DWORD)
//   +0x48 (slot 18) Stop                ()
//   +0x4c (slot 19) Unlock              (LPVOID, DWORD, LPVOID, DWORD)
//   +0x50 (slot 20) Restore             ()
// ---------------------------------------------------------------------------
struct IDirectSoundBufferZ {
    // Real COM interface (abstract), declared the dev-authentic SDK way with the
    // STDMETHOD / STDMETHOD_ macros (== `virtual HRESULT __stdcall` / `virtual u32
    // __stdcall`), so `buf->Method(args)` lowers to `mov eax,[buf]; call [eax+slot]`
    // (was the manual vtbl-struct dispatch). Only the touched slots carry meaningful
    // signatures; the IUnknown triad heads the vtable.
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv) PURE;      // slot 0
    STDMETHOD_(u32, AddRef)() PURE;                               // slot 1
    STDMETHOD_(u32, Release)() PURE;                              // slot 2  (+0x08)
    STDMETHOD(GetCaps)(void* caps) PURE;                          // slot 3  (+0x0c)
    STDMETHOD(GetCurrentPosition)(u32* play, u32* write) PURE;    // slot 4  (+0x10)
    STDMETHOD(GetFormat)(void* fmt, u32 size, u32* written) PURE; // slot 5  (+0x14)
    STDMETHOD(GetVolume)(i32* vol) PURE;                          // slot 6  (+0x18)
    STDMETHOD(GetPan)(i32* pan) PURE;                             // slot 7  (+0x1c)
    STDMETHOD(GetFrequency)(u32* freq) PURE;                      // slot 8  (+0x20)
    STDMETHOD(GetStatus)(u32* status) PURE;                       // slot 9  (+0x24)
    STDMETHOD(Initialize)(void* dsound, void* desc) PURE;         // slot 10 (+0x28, unused)
    STDMETHOD(Lock)(
        u32 off,
        u32 bytes,
        void** p1,
        u32* n1,
        void** p2,
        u32* n2,
        u32 fl
    ) PURE;                                                     // slot 11 (+0x2c)
    STDMETHOD(Play)(u32 r1, u32 r2, u32 flags) PURE;            // slot 12 (+0x30)
    STDMETHOD(SetCurrentPosition)(u32 pos) PURE;                // slot 13 (+0x34)
    STDMETHOD(SetFormat)(void* fmt) PURE;                       // slot 14 (+0x38)
    STDMETHOD(SetVolume)(i32 vol) PURE;                         // slot 15 (+0x3c)
    STDMETHOD(SetPan)(i32 pan) PURE;                            // slot 16 (+0x40)
    STDMETHOD(SetFrequency)(u32 freq) PURE;                     // slot 17 (+0x44)
    STDMETHOD(Stop)() PURE;                                     // slot 18 (+0x48)
    STDMETHOD(Unlock)(void* p1, u32 n1, void* p2, u32 n2) PURE; // slot 19 (+0x4c)
    STDMETHOD(Restore)() PURE;                                  // slot 20 (+0x50)
};
SIZE(IDirectSoundBufferZ, 0x4); // COM interface: a single vptr (opaque SDK object, held by ptr)

// ---------------------------------------------------------------------------
// DirectSoundMgr - the per-buffer sound-buffer wrapper BASE (vtable 0x5ef6b8,
// size 0x58). A concrete buffer is a DSoundCloneInst (0x60) whose base subobject
// is this class; a duplicated clone is a DSoundBaseSub (0x58, adds no fields). Every
// wrapper method lives here; only the clone-list ownership (Clone/RemoveClone/
// StopAllClones + its m_cloneList) belongs to the derived leaf. Field names are
// placeholders; the offsets + the COM slot dispatch are the load-bearing facts.
// ---------------------------------------------------------------------------
class DirectSoundMgr {
public:
    DirectSoundMgr(IDirectSoundBufferZ* buf, SoundDevice* owner); // 0x1351d0 ctor
    virtual ~DirectSoundMgr(); // 0x135300  base-subobject dtor (implicit vptr reset)

    i32 Restore();                 // 0x135310  m_buffer->Restore()
    i32 ReacquireBuffer();         // 0x135340  callback / owner reacquire (extern)
    i32 StopAndRewind();           // 0x135380  Stop + SetCurrentPosition(0)
    i32 IsPlaying();               // 0x1353f0  GetStatus & DSBSTATUS_PLAYING
    i32 IsLooping();               // 0x135440  GetStatus & DSBSTATUS_LOOPING
    void SetField3(i32 on);        // 0x135510  toggle the +0x14 play-flag bit 0 (looping)
    i32 SetVolume(i32 vol);        // 0x135560  SetVolume (caps DSBCAPS_CTRLVOLUME)
    i32 SetVolumeByIndex(i32 idx); // 0x1355c0  SetVolume(g_volumeTable[idx])
    i32 GetVolume();               // 0x1355f0  GetVolume
    i32 GetVolumePercent();        // 0x135640  GetVolume -> percent (0x135110)
    i32 CloneAndPlay(i32 key, i32 mode, i32 slot); // 0x135660  reap + spawn a voice
    i32 SetPan(i32 pan);                           // 0x135740  SetPan (caps DSBCAPS_CTRLPAN)
    i32 SetPanByIndex(i32 idx);                    // 0x1357a0  SetPan(+/-g_panTable[idx]) by sign
    i32 GetPan();                                  // 0x1357f0  GetPan
    i32 SetFrequency(u32 freq); // 0x135880  SetFrequency (caps DSBCAPS_CTRLFREQUENCY)
    i32 SetField2(i32 pct);     // 0x135920  freq-percent + duration recompute
    void ComputeDuration();     // 0x1359a0  m_durationMs = m_sampleCount*1000/m_sampleRate
    i32 Unlock(void* p1, u32 n1, void* p2, u32 n2);         // 0x1359c0
    i32 GetCurrentPosition(u32* play, u32* write);          // 0x135a20
    i32 SetCurrentPosition(u32 pos);                        // 0x135a70
    i32 GetFormat(void* fmt, u32 size, u32* written);       // 0x135ac0
    i32 LockConvert(void* src, u32 lockBytes, u32 convert); // 0x135f40
    i32 Play();                                             // 0x136270  Play + reacquire-retry
    i32 ApplyAndPlay(i32 vol, i32 pan, i32 freq, i32 d);    // 0x136300  apply params + play
    i32 Lock(
        u32 off,
        u32 bytes,
        void** p1,
        u32* n1,
        void** p2,
        u32* n2,
        u32 flags
    ); // 0x136370  Lock + reacquire-on-DSERR_BUFFERLOST retry

    static void GetErrorString(char* file, i32 line, i32 hr); // 0x138150

    // --- layout (per-buffer wrapper base, size 0x58) --------------------------
    // vptr @ +0x00 (implicit, from the virtual dtor); the first real field is +0x0c.
    char m_pad4[0x0c - 0x04];
    IDirectSoundBufferZ* m_buffer; // +0x0c  the held sound buffer
    SoundDevice* m_owner;          // +0x10  owning device back-pointer
    u32 m_playFlags;               // +0x14  Play/looping flags (bit 0 = loop)
    u32 m_freq;                    // +0x18  cached frequency (GetFrequency)
    i32 m_pan;                     // +0x1c  cached pan (GetPan)
    i32 m_volume;                  // +0x20  cached volume (GetVolume)
    u32 m_setFreq;                 // +0x24  cached SetFrequency value
    u32 m_durationMs;              // +0x28  duration (ComputeDuration)
    u32 m_sampleCount;             // +0x2c  sample count (set by the clone ctor)
    // +0x30  per-buffer reacquire callback (__cdecl fn-ptr taking (this, ctx)); a
    // pointer is 4 bytes, so this is layout-identical to the raw i32 slot.
    i32(__cdecl* m_reacquireCb)(DirectSoundMgr*, i32);
    i32 m_reacquireCtx;    // +0x34  per-buffer reacquire callback context
    i32 m_rateBase;        // +0x38  SetField2 percent base
    u32 m_sampleRate;      // +0x3c  sample rate (SetField2 sets it; ComputeDuration divides by it)
    u32 m_caps;            // +0x40  buffer capability flags (DSBCAPS_CTRLFREQUENCY/PAN/VOLUME)
    CloneNode m_cloneNode; // +0x44  clone-list node by which this buffer hangs in a parent's list
    i32 m_playKey;         // +0x50  clone play key
    DirectSoundMgr* m_reacquireOwner; // +0x54  device/owner used to reacquire a lost buffer
};
SIZE(DirectSoundMgr, 0x58);       // per-buffer wrapper base (fields end at +0x58)
VTBL(DirectSoundMgr, 0x001ef6b8); // cl-emitted ??_7DirectSoundMgr@@6B@ (base subobject dtor)

#endif // DSNDMGR_DIRECTSOUNDMGR_H
