// DirectSoundMgr.h - the WAP32 DirectSound manager (Dsndmgr module,
// C:\Proj\Dsndmgr\). Surfaces the layout + COM interface shapes needed to
// byte-match DirectSoundMgr's DSNDMGR.CPP method run: the HRESULT->error-string
// diagnostic formatter GetErrorString (the DSound sibling of
// CDirectDrawMgr::GetErrorString) and the thin IDirectSound / IDirectSoundBuffer
// wrapper thunks that, on a nonzero HRESULT, route through GetErrorString.
//
// The wrappers come in two `this`-shapes that share one class here (the offsets
// never collide): the device-level methods (m_14 = IDirectSound, gated on the
// m_78 "initialized" flag) and the per-buffer methods (m_0c = IDirectSoundBuffer,
// m_10 = the owning manager, gated on m_10->m_78, caps in m_40). Every wrapper
// does `iface->vtbl->Method(iface, args...)` so the retail `call *off(reg)` COM
// dispatch falls out; only the called slots are pinned, the rest is padding.
#ifndef DSNDMGR_DIRECTSOUNDMGR_H
#define DSNDMGR_DIRECTSOUNDMGR_H

// DSBCAPS - the buffer-caps struct GetCaps fills (dwSize 0x14 in, dwFlags out).
// The ctor reads dwFlags into m_40 and ignores the rest.
struct DSBCAPS {
    unsigned long dwSize;
    unsigned long dwFlags;
    unsigned long dwBufferBytes;
    unsigned long dwUnlockTransferRate;
    unsigned long dwPlayCpuOverhead;
};

// DSBUFFERDESC - the 0x14-byte sound-buffer descriptor passed to
// IDirectSound::CreateSoundBuffer (dwSize, dwFlags, dwBufferBytes, dwReserved,
// lpwfxFormat). Only dwSize/dwFlags are stamped here; the rest is zeroed.
struct DSBUFFERDESC {
    unsigned long dwSize;
    unsigned long dwFlags;
    unsigned long dwBufferBytes;
    unsigned long dwReserved;
    void* lpwfxFormat;
};

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
        long(__stdcall* Release)(IDirectSoundZ*); // +0x08
        long(__stdcall* CreateSoundBuffer)(
            IDirectSoundZ*,
            void* desc,
            IDirectSoundZ** out,
            void* unk
        ); // +0x0c
        char m_pad10[0x18 - 0x10];
        long(__stdcall*
                 SetCooperativeLevel)(IDirectSoundZ*, void* hwnd, unsigned long level); // +0x18
    }* vtbl;
};

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
        long(__stdcall* Release)(IDirectSoundBufferZ*);             // +0x08
        long(__stdcall* GetCaps)(IDirectSoundBufferZ*, void* caps); // +0x0c
        long(__stdcall* GetCurrentPosition)(
            IDirectSoundBufferZ*,
            unsigned long* play,
            unsigned long* write
        ); // +0x10
        long(__stdcall* GetFormat)(
            IDirectSoundBufferZ*,
            void* fmt,
            unsigned long size,
            unsigned long* written
        );                                                                        // +0x14
        long(__stdcall* GetVolume)(IDirectSoundBufferZ*, long* vol);              // +0x18
        long(__stdcall* GetPan)(IDirectSoundBufferZ*, long* pan);                 // +0x1c
        long(__stdcall* GetFrequency)(IDirectSoundBufferZ*, unsigned long* freq); // +0x20
        long(__stdcall* GetStatus)(IDirectSoundBufferZ*, unsigned long* status);  // +0x24
        char m_pad28[0x2c - 0x28];
        long(__stdcall* Lock)(
            IDirectSoundBufferZ*,
            unsigned long off,
            unsigned long bytes,
            void** p1,
            unsigned long* n1,
            void** p2,
            unsigned long* n2,
            unsigned long fl
        ); // +0x2c
        char m_pad30[0x34 - 0x30];
        long(__stdcall* SetCurrentPosition)(IDirectSoundBufferZ*, unsigned long pos); // +0x34
        long(__stdcall* SetFormat)(IDirectSoundBufferZ*, void* fmt);                  // +0x38
        long(__stdcall* SetVolume)(IDirectSoundBufferZ*, long vol);                   // +0x3c
        long(__stdcall* SetPan)(IDirectSoundBufferZ*, long pan);                      // +0x40
        long(__stdcall* SetFrequency)(IDirectSoundBufferZ*, unsigned long freq);      // +0x44
        long(__stdcall* Stop)(IDirectSoundBufferZ*);                                  // +0x48
        long(__stdcall* Unlock)(
            IDirectSoundBufferZ*,
            void* p1,
            unsigned long n1,
            void* p2,
            unsigned long n2
        );                                              // +0x4c
        long(__stdcall* Restore)(IDirectSoundBufferZ*); // +0x50
    }* vtbl;
};

// ---------------------------------------------------------------------------
// DSoundCloneBase - a tiny polymorphic view used only to dispatch a clone's
// scalar-deleting destructor (vtable slot 0) as the exact `mov eax,[obj]; push 1;
// call [eax]` __thiscall call. Its virtuals are never defined here, so no ??_7
// vtable is emitted in this TU (same idiom as CInputDeviceBase).
// ---------------------------------------------------------------------------
class DSoundCloneBase {
public:
    virtual void* ScalarDtor(int flag); // +0x00 slot 0
};

// ---------------------------------------------------------------------------
// DirectSoundMgr. Single class spanning both wrapper `this`-shapes; only the
// touched offsets are pinned (the rest is opaque). Field names are placeholders;
// the offsets + the COM slot dispatch are the load-bearing facts.
// ---------------------------------------------------------------------------
class DirectSoundMgr {
public:
    // --- per-buffer wrappers (this = a buffer object, m_0c = the buffer) ------
    DirectSoundMgr(IDirectSoundBufferZ* buf, DirectSoundMgr* owner); // 0x1351d0 ctor
    int Restore();                                                   // 0x135310  m_0c->Restore()
    int StopAndRewind();      // 0x135380  Stop + SetCurrentPosition(0)
    int IsPlaying();          // 0x1353f0  GetStatus & 1
    int IsLooping();          // 0x135440  GetStatus & 2
    int SetVolume(long vol);  // 0x135560  SetVolume (caps 0x80)
    long GetVolume();         // 0x1355f0  GetVolume
    void SetField0(long idx); // 0x1355c0  SetVolume(g_volumeTable[idx]) (extern)
    long GetVolumePercent();  // 0x135640  GetVolume -> percent (0x135110)
    int CloneAndPlay(long key, long mode, long slot); // 0x135660  reap + spawn a voice
    int SetPan(long pan);                             // 0x135740  SetPan (caps 0x40)
    long GetPan();                                    // 0x1357f0  GetPan
    int SetFrequency(unsigned long freq);             // 0x135880  SetFrequency (caps 0x20)
    int Unlock(void* p1, unsigned long n1, void* p2, unsigned long n2);   // 0x1359c0
    int GetCurrentPosition(unsigned long* play, unsigned long* write);    // 0x135a20
    int SetCurrentPosition(unsigned long pos);                            // 0x135a70
    int GetFormat(void* fmt, unsigned long size, unsigned long* written); // 0x135ac0
    ~DirectSoundMgr();                       // 0x135bb0  destructor (frees clone list)
    void BaseDtor();                         // 0x136260  base-subobject dtor (extern)
    void RemoveClone(DirectSoundMgr* clone); // 0x135d20  release + unlink one clone
    int LockConvert(void* src, unsigned long lockBytes, unsigned long convert); // 0x135f40
    void StopAllClones();                                                       // 0x136150

    // --- device-level wrappers (this = the manager, m_14 = IDirectSound) ------
    int Create(
        void* hwnd,
        unsigned long level,
        unsigned long flags
    );                                                        // 0x136550  DirectSoundCreate + coop
    int SetCooperativeLevel(void* hwnd, unsigned long level); // 0x1365f0
    int CreatePrimaryBuffer();                                // 0x137260  m_14->CreateSoundBuffer

    static void GetErrorString(char* file, int line, long hr); // 0x138150

    // Engine-label backlog stubs (relocated from src/Stub/ - own this class here).
    int winapi_136e20_timeGetTime(int);
    int winapi_137ac0_timeGetTime(int);

    // --- layout ---------------------------------------------------------------
    char m_pad0[0x0c];
    IDirectSoundBufferZ* m_0c; // +0x0c  the held sound buffer (per-buffer this)
    DirectSoundMgr* m_10;      // +0x10  owning manager back-pointer (per-buffer this)
    IDirectSoundZ* m_14;       // +0x14  the DirectSound device (manager this)
    long m_18;                 // +0x18  cached frequency
    long m_1c;                 // +0x1c  cached pan
    long m_20;                 // +0x20  cached volume
    unsigned long m_24;        // +0x24  cached set-frequency value
    int m_28;                  // +0x28
    char m_pad2c[0x30 - 0x2c];
    int m_30;           // +0x30
    int m_34;           // +0x34
    int m_38;           // +0x38
    int m_3c;           // +0x3c
    unsigned long m_40; // +0x40  buffer capability flags (0x20/0x40/0x80)
    // +0x44  intrusive list-node by which a clone instance hangs in its parent's
    // clone list (next@+0x44, prev@+0x48, back-pointer to this clone @+0x4c). The
    // list-helper at 0x1391e0 unlinks the node given &m_node44; the parent walks
    // the list reading m_node44.m_inst (+0x4c) to reach each clone.
    struct CloneNode {
        CloneNode* m_next;      // +0x00 (clone +0x44)
        CloneNode* m_prev;      // +0x04 (clone +0x48)
        DirectSoundMgr* m_inst; // +0x08 (clone +0x4c) back-pointer to the clone
    } m_node44;
    char m_pad50[0x58 - 0x50];
    // +0x58  head of this instance's clone/child list (head@+0x58, tail@+0x5c);
    // each member is a cloned DirectSoundMgr chained through its m_node44.
    CloneNode* m_58_head;
    CloneNode* m_5c_tail;
    char m_pad60[0x78 - 0x60];
    int m_78; // +0x78  "initialized" flag (manager this)
    int m_7c; // +0x7c
    char m_pad80[0x84 - 0x80];
    IDirectSoundBufferZ* m_84; // +0x84  primary buffer
    int m_88;                  // +0x88  cooperative level / coop arg
    unsigned long m_8c;        // +0x8c  buffer-desc flags
};

#endif // DSNDMGR_DIRECTSOUNDMGR_H
