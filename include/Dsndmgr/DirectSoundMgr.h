// DirectSoundMgr.h - the WAP32 DirectSound manager (Dsndmgr module,
// C:\Proj\Dsndmgr\). Surfaces the layout + COM interface shapes needed to
// byte-match DirectSoundMgr's DSNDMGR.CPP method run: the HRESULT->error-string
// diagnostic formatter GetErrorString (the DSound sibling of
// CDirectDrawMgr::GetErrorString) and the thin IDirectSound / IDirectSoundBuffer
// wrapper thunks that, on a nonzero HRESULT, route through GetErrorString.
//
// The wrappers come in two `this`-shapes that share one class here (the offsets
// never collide): the device-level methods (m_device = IDirectSound, gated on the
// m_initialized flag) and the per-buffer methods (m_buffer = IDirectSoundBuffer,
// m_owner = the owning manager, gated on m_owner->m_initialized, caps in m_caps).
// Every wrapper does `iface->vtbl->Method(iface, args...)` so the retail
// `call *off(reg)` COM dispatch falls out; only the called slots are pinned, the
// rest is padding.
#ifndef DSNDMGR_DIRECTSOUNDMGR_H
#define DSNDMGR_DIRECTSOUNDMGR_H

#include <rva.h>

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

// ---------------------------------------------------------------------------
// IDirectSound (DSOUND) - the device interface DirectSoundCreate returns. Only
// the slots the manager calls are pinned. COM convention => __stdcall with the
// interface pointer as the hidden first ("this") argument; the manager always
// invokes them as iface->vtbl->Method(iface, ...).
//   +0x0c (slot 3)  CreateSoundBuffer  (LPCDSBUFFERDESC, LPDIRECTSOUNDBUFFER*, LPUNKNOWN)
//   +0x18 (slot 6)  SetCooperativeLevel(HWND, DWORD)
// ---------------------------------------------------------------------------
struct IDirectSoundZ {
    struct Vtbl {
        char m_pad0[0x08];
        i32(__stdcall* Release)(IDirectSoundZ*); // +0x08
        i32(__stdcall* CreateSoundBuffer)(
            IDirectSoundZ*,
            void* desc,
            IDirectSoundBufferZ** out,
            void* unk
        ); // +0x0c
        char m_pad10[0x14 - 0x10];
        i32(__stdcall* DuplicateSoundBuffer)(
            IDirectSoundZ*,
            IDirectSoundBufferZ* original,
            IDirectSoundBufferZ** out
        );                                                                          // +0x14
        i32(__stdcall* SetCooperativeLevel)(IDirectSoundZ*, void* hwnd, u32 level); // +0x18
    }* vtbl;
};
SIZE_UNKNOWN(IDirectSoundZ); // COM interface view (single vtbl pointer)

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
    struct Vtbl {
        char m_pad0[0x08];
        i32(__stdcall* Release)(IDirectSoundBufferZ*);             // +0x08
        i32(__stdcall* GetCaps)(IDirectSoundBufferZ*, void* caps); // +0x0c
        i32(__stdcall* GetCurrentPosition)(IDirectSoundBufferZ*, u32* play,
                                           u32* write); // +0x10
        i32(__stdcall* GetFormat)(IDirectSoundBufferZ*, void* fmt, u32 size,
                                  u32* written);                       // +0x14
        i32(__stdcall* GetVolume)(IDirectSoundBufferZ*, i32* vol);     // +0x18
        i32(__stdcall* GetPan)(IDirectSoundBufferZ*, i32* pan);        // +0x1c
        i32(__stdcall* GetFrequency)(IDirectSoundBufferZ*, u32* freq); // +0x20
        i32(__stdcall* GetStatus)(IDirectSoundBufferZ*, u32* status);  // +0x24
        char m_pad28[0x2c - 0x28];
        i32(__stdcall* Lock)(
            IDirectSoundBufferZ*,
            u32 off,
            u32 bytes,
            void** p1,
            u32* n1,
            void** p2,
            u32* n2,
            u32 fl
        ); // +0x2c
        i32(__stdcall* Play)(IDirectSoundBufferZ*, u32 r1, u32 r2,
                             u32 flags);                                   // +0x30
        i32(__stdcall* SetCurrentPosition)(IDirectSoundBufferZ*, u32 pos); // +0x34
        i32(__stdcall* SetFormat)(IDirectSoundBufferZ*, void* fmt);        // +0x38
        i32(__stdcall* SetVolume)(IDirectSoundBufferZ*, i32 vol);          // +0x3c
        i32(__stdcall* SetPan)(IDirectSoundBufferZ*, i32 pan);             // +0x40
        i32(__stdcall* SetFrequency)(IDirectSoundBufferZ*, u32 freq);      // +0x44
        i32(__stdcall* Stop)(IDirectSoundBufferZ*);                        // +0x48
        i32(__stdcall* Unlock)(IDirectSoundBufferZ*, void* p1, u32 n1, void* p2,
                               u32 n2);                // +0x4c
        i32(__stdcall* Restore)(IDirectSoundBufferZ*); // +0x50
    }* vtbl;
};
SIZE_UNKNOWN(IDirectSoundBufferZ); // COM interface view (single vtbl pointer)

// ---------------------------------------------------------------------------
// DSoundCloneBase - a tiny polymorphic view used only to dispatch a clone's
// scalar-deleting destructor (vtable slot 0) as the exact `mov eax,[obj]; push 1;
// call [eax]` __thiscall call. Its virtuals are never defined here, so no ??_7
// vtable is emitted in this TU (same idiom as CInputDeviceBase).
// ---------------------------------------------------------------------------
class DSoundCloneBase {
public:
    virtual void* ScalarDtor(i32 flag); // +0x00 slot 0
};
SIZE_UNKNOWN(DSoundCloneBase); // polymorphic slot-0 dispatch view

// ---------------------------------------------------------------------------
// DirectSoundMgr. Single class spanning both wrapper `this`-shapes; only the
// touched offsets are pinned (the rest is opaque). Field names are placeholders;
// the offsets + the COM slot dispatch are the load-bearing facts.
// ---------------------------------------------------------------------------
class DirectSoundMgr {
public:
    // --- per-buffer wrappers (this = a buffer object, m_buffer = the buffer) --
    DirectSoundMgr(IDirectSoundBufferZ* buf, DirectSoundMgr* owner); // 0x1351d0 ctor
    void RestampBufferVtbl();      // 0x135300  store the buffer vtbl into *this (base dtor tail)
    i32 Restore();                 // 0x135310  m_buffer->Restore()
    i32 StopAndRewind();           // 0x135380  Stop + SetCurrentPosition(0)
    i32 IsPlaying();               // 0x1353f0  GetStatus & DSBSTATUS_PLAYING
    i32 IsLooping();               // 0x135440  GetStatus & DSBSTATUS_LOOPING
    i32 SetVolume(i32 vol);        // 0x135560  SetVolume (caps DSBCAPS_CTRLVOLUME)
    i32 GetVolume();               // 0x1355f0  GetVolume
    i32 SetVolumeByIndex(i32 idx); // 0x1355c0  SetVolume(g_volumeTable[idx])
    i32 SetPanByIndex(i32 idx);    // 0x1357a0  SetPan(+/-g_panTable[idx]) by sign
    i32 SetField2(i32 pct);        // 0x135920  freq-percent + duration recompute
    void SetField3(i32 on);        // 0x135510  toggle the +0x14 play-flag bit 0 (looping)
    void ComputeDuration();        // 0x1359a0  m_durationMs = m_sampleCount*1000/m_sampleRate
    i32 Play();                    // 0x136270  Play(this looping flag) + reacquire-retry
    i32 GetVolumePercent();        // 0x135640  GetVolume -> percent (0x135110)
    i32 CloneAndPlay(i32 key, i32 mode, i32 slot); // 0x135660  reap + spawn a voice
    i32 SetPan(i32 pan);                           // 0x135740  SetPan (caps DSBCAPS_CTRLPAN)
    i32 GetPan();                                  // 0x1357f0  GetPan
    i32 SetFrequency(u32 freq); // 0x135880  SetFrequency (caps DSBCAPS_CTRLFREQUENCY)
    i32 Lock(
        u32 off,
        u32 bytes,
        void** p1,
        u32* n1,
        void** p2,
        u32* n2,
        u32 flags
    );                     // 0x136370  Lock + reacquire-on-DSERR_BUFFERLOST retry
    i32 ReacquireBuffer(); // 0x135340  restore via callback / m_owner reacquire (extern)
    i32 Unlock(void* p1, u32 n1, void* p2, u32 n2);   // 0x1359c0
    i32 GetCurrentPosition(u32* play, u32* write);    // 0x135a20
    i32 SetCurrentPosition(u32 pos);                  // 0x135a70
    i32 GetFormat(void* fmt, u32 size, u32* written); // 0x135ac0
    ~DirectSoundMgr();                                // 0x135bb0  destructor (frees clone list)
    DirectSoundMgr* Clone(i32 a); // 0x135c20  new a clone instance, dup the buffer, link it
    void BaseDtor(); // 0x136260  base-subobject dtor (stamps clone vtbl, tail 0x135300)
    i32 ApplyAndPlay(i32 vol, i32 pan, i32 freq, i32 d); // 0x136300  apply params + play
    i32 ReacquireViaCallback();              // 0x1365e0  tail-dispatch through m_reacquireMethod
    void RemoveClone(DirectSoundMgr* clone); // 0x135d20  release + unlink one clone
    i32 LockConvert(void* src, u32 lockBytes, u32 convert); // 0x135f40
    void StopAllClones();                                   // 0x136150

    // --- device-level wrappers (this = the manager, m_device = IDirectSound) --
    i32 Create(void* hwnd, u32 level,
               u32 flags);                          // 0x136550  DirectSoundCreate + coop
    i32 SetCooperativeLevel(void* hwnd, u32 level); // 0x1365f0
    i32 CreatePrimaryBuffer();                      // 0x137260  m_device->CreateSoundBuffer

    static void GetErrorString(char* file, i32 line, i32 hr); // 0x138150

    // Engine-label backlog stubs (relocated from src/Stub/ - own this class here).
    i32 winapi_136e20_timeGetTime(i32);
    i32 winapi_137ac0_timeGetTime(i32);

    // --- layout ---------------------------------------------------------------
    char m_pad0[0x0c];
    IDirectSoundBufferZ* m_buffer; // +0x0c  the held sound buffer (per-buffer this)
    DirectSoundMgr* m_owner;       // +0x10  owning manager back-pointer (per-buffer this)
    IDirectSoundZ* m_device;       // +0x14  the DirectSound device (manager this)
    u32 m_freq;                    // +0x18  cached frequency (GetFrequency)
    i32 m_pan;                     // +0x1c  cached pan (GetPan)
    i32 m_volume;                  // +0x20  cached volume (GetVolume)
    u32 m_setFreq;                 // +0x24  cached SetFrequency value
    u32 m_durationMs;   // +0x28  duration (ComputeDuration = m_sampleCount*1000/m_sampleRate)
    u32 m_sampleCount;  // +0x2c  sample count (set by the clone ctor)
    i32 m_reacquireCb;  // +0x30  per-buffer reacquire callback
    i32 m_reacquireCtx; // +0x34  per-buffer reacquire callback context
    i32 m_rateBase;     // +0x38  SetField2 percent base
    u32 m_sampleRate;   // +0x3c  sample rate (SetField2 sets it; ComputeDuration divides by it)
    u32 m_caps;         // +0x40  buffer capability flags (DSBCAPS_CTRLFREQUENCY/PAN/VOLUME)
    // +0x44  intrusive list-node by which a clone instance hangs in its parent's
    // clone list (next@+0x44, prev@+0x48, back-pointer to this clone @+0x4c). The
    // list-helper at 0x1391e0 unlinks the node given &m_node44; the parent walks
    // the list reading m_node44.m_inst (+0x4c) to reach each clone.
    struct CloneNode {
        CloneNode* m_next;      // +0x00 (clone +0x44)
        CloneNode* m_prev;      // +0x04 (clone +0x48)
        DirectSoundMgr* m_inst; // +0x08 (clone +0x4c) back-pointer to the clone
    } m_node44;
    char m_pad50[0x54 - 0x50];
    DirectSoundMgr* m_reacquireOwner; // +0x54  device/owner used to reacquire a lost buffer
    // +0x58  head of this instance's clone/child list (head@+0x58, tail@+0x5c);
    // each member is a cloned DirectSoundMgr chained through its m_node44.
    CloneNode* m_cloneHead;
    CloneNode* m_cloneTail;
    char m_pad60[0x78 - 0x60];
    i32 m_initialized; // +0x78  device-up flag (manager this)
    i32 m_7c;          // +0x7c  cleared by Create; role unproven
    // +0x80  reacquire callback fn-ptr (a __thiscall on the manager); the
    // ReacquireViaCallback dispatch (0x1365e0) tail-jumps through it. Stored as a
    // raw void* (MSVC5 cannot spell __thiscall on a fn-ptr) and re-typed at the
    // dispatch site to a member-fn-ptr (which is __thiscall by default).
    void* m_reacquireMethod;
    IDirectSoundBufferZ* m_primaryBuffer; // +0x84  primary buffer
    i32 m_coopLevel;                      // +0x88  cooperative level
    u32 m_bufferFlags;                    // +0x8c  buffer-desc flags
};
SIZE_UNKNOWN(DirectSoundMgr); // both wrapper this-shapes; opaque tail past +0x8c

#endif // DSNDMGR_DIRECTSOUNDMGR_H
