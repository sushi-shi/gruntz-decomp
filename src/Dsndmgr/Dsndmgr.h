// Dsndmgr.h - DirectSoundMgr + CGMSound + ring-buffer classes.
// Matches the unnamed FUN_ functions in the 0x135000-0x13a000 region.
#ifndef DSNDMGR_H
#define DSNDMGR_H

// ---------------------------------------------------------------------------
// Minimal Win32 surface (no <windows.h>). Only types and imports touched by
// the matched methods are declared.
// ---------------------------------------------------------------------------
typedef long           HRESULT;
typedef unsigned long  DWORD;
typedef unsigned long  ULONG;
typedef int            BOOL;
typedef long           LONG;
typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef void *         HANDLE;
typedef void *         HRSRC;
typedef void *         HMODULE;
#define NULL 0
#define FALSE 0
#define TRUE 1
#define FAILED(hr) ((HRESULT)(hr) < 0)

extern "C" {
__declspec(dllimport) void __stdcall OutputDebugStringA(const char *lpOutputString);
__declspec(dllimport) int  __stdcall MessageBoxA(void *hWnd, const char *lpText,
                                                  const char *lpCaption, unsigned int uType);
__declspec(dllimport) BOOL __stdcall MessageBeep(unsigned int uType);
__declspec(dllimport) HRSRC __stdcall FindResourceA(HMODULE, const char *, const char *);
__declspec(dllimport) HRSRC __stdcall LoadResource(HMODULE, HRSRC);
__declspec(dllimport) void * __stdcall LockResource(HRSRC);
__declspec(dllimport) DWORD __stdcall timeGetTime();
}

// The engine's string formatting function (__cdecl: caller pops the args).
extern "C" int __cdecl EngFormat(char *dest, const char *fmt, ...);

// ---------------------------------------------------------------------------
// Module-level globals (output-mode flags + module name).
// ---------------------------------------------------------------------------
extern int  g_dsndBeep;          // @0x653c54
extern int  g_dsndDebug;         // @0x653c4c
extern int  g_dsndMsgBox;        // @0x653c50
extern int  g_dsndOutputDbg;     // @0x653c58
extern char g_szDsndModule[];    // @0x619f3c  "DirectSoundMgr"

// ---------------------------------------------------------------------------
// Minimal IDirectSoundBuffer vtable declaration (interface layout matches
// DirectX 5/6/7 IDirectSoundBuffer from <dsound.h>).
// ---------------------------------------------------------------------------
struct IDirectSoundBufferVtbl;
struct IDirectSoundBuffer {
    IDirectSoundBufferVtbl *lpVtbl;
};

struct IDirectSoundBufferVtbl {
    HRESULT (__stdcall *QueryInterface)(IDirectSoundBuffer*, const void*, void**);
    ULONG   (__stdcall *AddRef)(IDirectSoundBuffer*);
    ULONG   (__stdcall *Release)(IDirectSoundBuffer*);
    HRESULT (__stdcall *GetCaps)(IDirectSoundBuffer*, void*);
    HRESULT (__stdcall *GetCurrentPosition)(IDirectSoundBuffer*, DWORD*, DWORD*);
    HRESULT (__stdcall *GetFormat)(IDirectSoundBuffer*, void*, DWORD, DWORD*);
    HRESULT (__stdcall *GetVolume)(IDirectSoundBuffer*, LONG*);
    HRESULT (__stdcall *GetPan)(IDirectSoundBuffer*, LONG*);
    HRESULT (__stdcall *GetFrequency)(IDirectSoundBuffer*, DWORD*);
    HRESULT (__stdcall *GetStatus)(IDirectSoundBuffer*, DWORD*);
    HRESULT (__stdcall *Initialize)(IDirectSoundBuffer*, void*, void*);
    HRESULT (__stdcall *Lock)(IDirectSoundBuffer*, DWORD, DWORD, void**, DWORD*, void**, DWORD*, DWORD);
    HRESULT (__stdcall *Play)(IDirectSoundBuffer*, DWORD, DWORD, DWORD);
    HRESULT (__stdcall *SetCurrentPosition)(IDirectSoundBuffer*, DWORD);
    HRESULT (__stdcall *SetFormat)(IDirectSoundBuffer*, void*);
    HRESULT (__stdcall *SetVolume)(IDirectSoundBuffer*, LONG);
    HRESULT (__stdcall *SetPan)(IDirectSoundBuffer*, LONG);
    HRESULT (__stdcall *SetFrequency)(IDirectSoundBuffer*, DWORD);
    HRESULT (__stdcall *Stop)(IDirectSoundBuffer*);
    HRESULT (__stdcall *Unlock)(IDirectSoundBuffer*, void*, DWORD, void*, DWORD);
    HRESULT (__stdcall *Restore)(IDirectSoundBuffer*);
};

// ---------------------------------------------------------------------------
// Forward declarations for types referenced by DirectSoundMgr members.
// The DSound manager object layout is partially known from ErrorThunks.
// ---------------------------------------------------------------------------
struct DsManager {
    char pad_00[0x78];
    void* m_pSubBuffer;   // @ +0x78 (checked for NULL by ErrorThunks)
};

// ---------------------------------------------------------------------------
// DirectSoundMgr - DirectSound manager.
// Field names use m_<hexoffset> naming (only the offsets are load-bearing).
// +0x00-0x08: 3 DWORDs of unknown purpose (base class state or padding)
// +0x0c: IDirectSoundBuffer* m_pDSBuffer (primary sound buffer)
// +0x10: DsManager* m_pDSManager (manager state with callbacks at +0x78)
// ---------------------------------------------------------------------------
class DirectSoundMgr {
    void* m_pad_00;
    void* m_pad_04;
    void* m_pad_08;
public:
    IDirectSoundBuffer* m_pDSBuffer;  // @ +0x0c
    DsManager* m_pDSManager;           // @ +0x10

    // GetErrorString (does NOT use `this`)
    void GetErrorString(const char *file, int line, HRESULT hr);

    // ErrorThunks - IDirectSoundBuffer vtable wrappers
    BOOL ErrorThunk_135310();
    BOOL ErrorThunk_135340();
    void ErrorThunk_135380();
    BOOL ErrorThunk_1353f0();
    BOOL ErrorThunk_135440();
    void ErrorThunk_135510(int);
    BOOL ErrorThunk_135560(LONG);
    BOOL ErrorThunk_1355c0(int);
    BOOL ErrorThunk_1355f0();
    int  ErrorThunk_135640();
    void *ErrorThunk_135660(int, int, int, int);
    BOOL ErrorThunk_135740(LONG);
    BOOL ErrorThunk_1357a0(int);
    BOOL ErrorThunk_1357f0();
    BOOL ErrorThunk_135880(DWORD);
    BOOL ErrorThunk_135920(int);
    void ErrorThunk_1359a0();
    BOOL ErrorThunk_1359c0(void*, DWORD, void*, DWORD);
    BOOL ErrorThunk_135a20(DWORD*, DWORD*);
    BOOL ErrorThunk_135a70(DWORD);
    BOOL ErrorThunk_135ac0(void*, DWORD, DWORD*);
    // Complex ErrorThunks (constructor-like / multi-step)
    void ErrorThunk_1351d0(int, int);
    BOOL ErrorThunk_135f40(DWORD, DWORD, void**, DWORD*, void**, DWORD*, DWORD);
    BOOL ErrorThunk_1365f0(void*);
    HRESULT ErrorThunk_137260();
};

// ===========================================================================
// Sound format / WAV header utilities
// ===========================================================================
struct WAVEFORMATEX {
    WORD  wFormatTag;
    WORD  nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD  nBlockAlign;
    WORD  wBitsPerSample;
    WORD  cbSize;
};

// ===========================================================================
// External function declarations (other TUs)
// ===========================================================================
extern "C" int __cdecl UnknownSalazar_computeScaleFactor(int value);

// ===========================================================================
// Miles Sound System AIL import declarations
// ===========================================================================
extern "C" {
__declspec(dllimport) void __stdcall AIL_startup(void);
__declspec(dllimport) int  __stdcall AIL_midiOutOpen(int *handle);
__declspec(dllimport) void __stdcall AIL_shutdown(void);
__declspec(dllimport) void __stdcall AIL_set_XMIDI_master_volume(int, int);
__declspec(dllimport) int  __stdcall AIL_XMIDI_master_volume(int);
__declspec(dllimport) void * __stdcall AIL_allocate_sequence_handle(int);
__declspec(dllimport) void __stdcall AIL_release_sequence_handle(void *);
__declspec(dllimport) int  __stdcall AIL_init_sequence(void *, void *, int);
__declspec(dllimport) void __stdcall AIL_start_sequence(void *);
__declspec(dllimport) void __stdcall AIL_set_sequence_loop_count(void *, int);
__declspec(dllimport) void __stdcall AIL_end_sequence(void *);
__declspec(dllimport) void __stdcall AIL_stop_sequence(void *);
__declspec(dllimport) void __stdcall AIL_resume_sequence(void *);
__declspec(dllimport) int  __stdcall AIL_sequence_status(void *);
__declspec(dllimport) void __stdcall AIL_set_sequence_tempo(void *, int, int);
__declspec(dllimport) void __stdcall AIL_set_sequence_volume(void *, int, int);
}

#endif // DSNDMGR_H
