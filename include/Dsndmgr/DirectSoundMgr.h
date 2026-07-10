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
#include <stdio.h>                  // FILE (LoadFromFile stream arg)
#include <Dsndmgr/SoundVoiceList.h> // DSoundLink / DSoundList intrusive list primitive

// DSBCAPS - the buffer-caps struct GetCaps fills (dwSize 0x14 in, dwFlags out).
// The ctor reads dwFlags into m_caps and ignores the rest.

// DSBUFFERDESC - the 0x14-byte sound-buffer descriptor passed to
// IDirectSound::CreateSoundBuffer (dwSize, dwFlags, dwBufferBytes, dwReserved,
// lpwfxFormat). Only dwSize/dwFlags are stamped here; the rest is zeroed.

struct IDirectSound;       // forward-decl: real dsound.h interface (dispatched in the .cpp)
struct IDirectSoundBuffer; // forward-decl: CreateSoundBuffer's out-param type
class SoundDevice;         // owning device (m_owner); full def in SoundDevice.h
class DirectSoundMgr;      // a clone (CloneNode::m_inst back-points at it)

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
    DirectSoundMgr(IDirectSoundBuffer* buf, SoundDevice* owner); // 0x1351d0 ctor
    virtual ~DirectSoundMgr(); // 0x135300  base-subobject dtor (implicit vptr reset)

    i32 Restore();                 // 0x135310  m_buffer->Restore()
    i32 ReacquireBuffer();         // 0x135340  callback / owner reacquire (extern)
    i32 StopAndRewind();           // 0x135380  Stop + SetCurrentPosition(0)
    i32 IsPlaying();               // 0x1353f0  GetStatus & DSBSTATUS_PLAYING
    i32 IsLooping();               // 0x135440  GetStatus & DSBSTATUS_LOOPING
    i32 IsInHardware();            // 0x135490  GetCaps & DSBCAPS_LOCHARDWARE
    void SetField3(i32 on);        // 0x135510  toggle the +0x14 play-flag bit 0 (looping)
    i32 SetVolume(i32 vol);        // 0x135560  SetVolume (caps DSBCAPS_CTRLVOLUME)
    i32 SetVolumeByIndex(i32 idx); // 0x1355c0  SetVolume(g_volumeTable[idx])
    i32 GetVolume();               // 0x1355f0  GetVolume
    i32 GetVolumePercent();        // 0x135640  GetVolume -> percent (0x135110)
    i32 GetPanPercent();           // 0x135840  GetPan -> signed percent (0x135110)
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
    i32 LoadFromFile(FILE* fp, u32 bytes, i32 offset);      // 0x135e10  fseek+Lock+fread+Unlock
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
    // vptr @ +0x00 (implicit, from the virtual dtor); the first real field is +0x04.
    // +0x04 is the device buffer-list link: when the concrete leaf (DSoundCloneInst)
    // hangs in SoundDevice::m_bufferList, this DSoundLink is the biased element+4 the
    // list threads (SoundDevice::Shutdown/StopAll/CreateBuffer/RemoveBuffer). Set by
    // the list helpers, not the ctor.
    DSoundLink m_link;            // +0x04  device buffer-list link {next@+4, prev@+8}
    IDirectSoundBuffer* m_buffer; // +0x0c  the held sound buffer
    SoundDevice* m_owner;         // +0x10  owning device back-pointer
    u32 m_playFlags;              // +0x14  Play/looping flags (bit 0 = loop)
    u32 m_freq;                   // +0x18  cached frequency (GetFrequency)
    i32 m_pan;                    // +0x1c  cached pan (GetPan)
    i32 m_volume;                 // +0x20  cached volume (GetVolume)
    u32 m_setFreq;                // +0x24  cached SetFrequency value
    u32 m_durationMs;             // +0x28  duration (ComputeDuration)
    u32 m_sampleCount;            // +0x2c  sample count (set by the clone ctor)
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

// ---------------------------------------------------------------------------
// The DirectSoundMgr clone hierarchy (real 3-level polymorphic): the fields + methods
// live on the base above, the two leaves add only their own vtable/dtor (+ the clone
// list on the concrete leaf). Bodies live in DirectSoundMgr.cpp; the definitions live
// here so the device (SoundDevice.cpp) + the feeder (StreamFeeder.cpp) can name the
// concrete leaf DSoundCloneInst that its buffer list actually threads.

// Clone list {head,tail}; InsertHead/Unlink are shared engine helpers (0x1390e0/0x1391e0).
struct CloneList {
    CloneNode* m_head; // +0x00
    CloneNode* m_tail; // +0x04
};
SIZE(CloneList, 0x8); // {head, tail}

// DSoundBaseSub - clone/duplicate wrapper Clone() news (vtable 0x5ef6c0, 0x58B, no new
// fields). dtor 0x136260 resets the vptr + chains ~DirectSoundMgr.
class DSoundBaseSub : public DirectSoundMgr {
public:
    DSoundBaseSub(IDirectSoundBuffer* buf, SoundDevice* owner); // 0x136230
    // Clone/dup ctor 0x136180: 2-arg ctor + records source (m_reacquireOwner), copies
    // its sample/reacquire/rate block, recomputes duration.
    DSoundBaseSub(
        IDirectSoundBuffer* buf,
        SoundDevice* owner,
        DirectSoundMgr* original
    );                                 // 0x136180
    virtual ~DSoundBaseSub() OVERRIDE; // 0x136260  base-subobject dtor (vptr reset + chain)
};
SIZE(DSoundBaseSub, 0x58);       // clone alloc: Clone() news 0x58 (RezAlloc(0x58))
VTBL(DSoundBaseSub, 0x001ef6c0); // cl-emitted ??_7DSoundBaseSub@@6B@

// DSoundCloneInst - concrete per-buffer leaf owning a clone list (vtable 0x5ef6bc,
// 0x60B); dtor 0x135bb0 drains the clone list. This is the object SoundDevice mints in
// CreateBuffer (RezAlloc(0x60)), threads on its buffer list, and reaps in RemoveBuffer.
class DSoundCloneInst : public DSoundBaseSub {
public:
    DSoundCloneInst(IDirectSoundBuffer* buf, SoundDevice* owner); // 0x135b10
    virtual ~DSoundCloneInst() OVERRIDE;                          // 0x135bb0  clone-drain dtor

    DirectSoundMgr* Clone(i32 a);            // 0x135c20  new a clone, dup the buffer, link it
    void RemoveClone(DirectSoundMgr* clone); // 0x135d20  release + unlink + delete one clone
    void StopAllClones();                    // 0x136150  StopAndRewind each clone
    // BaseInit: the 2-arg ctor (0x135b10) reached as a method so CreateBuffer's
    // RezAlloc(0x60)+construct path lowers to a reloc-masked __thiscall instead of a
    // placement-new (which cl would frame differently). Same address as the ctor above;
    // declared-only here so the call reloc-masks.
    void BaseInit(IDirectSoundBuffer* buf, SoundDevice* owner); // 0x135b10 (== ctor)

    CloneList m_cloneList; // +0x58  clone/child list {head@+0x58, tail@+0x5c}
};
SIZE(DSoundCloneInst, 0x60);       // buffer leaf: CreateBuffer RezAlloc(0x60)
VTBL(DSoundCloneInst, 0x001ef6bc); // cl-emitted ??_7DSoundCloneInst@@6B@

#endif // DSNDMGR_DIRECTSOUNDMGR_H
