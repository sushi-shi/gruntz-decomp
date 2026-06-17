// Dsndmgr.h - the engine DirectSoundMgr - minimal reconstruction for
// DirectSoundMgr::GetErrorString. This TU also contains the DirectSoundCreate
// import caller (external/no-body). Flags are module-level globals.
#ifndef DSNDMGR_H
#define DSNDMGR_H

// ---------------------------------------------------------------------------
// Minimal Win32 surface (no <windows.h>). Only types and imports touched by
// the matched methods are declared.
// ---------------------------------------------------------------------------
typedef long           HRESULT;
typedef unsigned long  DWORD;
typedef int            BOOL;

extern "C" {
__declspec(dllimport) void __stdcall OutputDebugStringA(const char *lpOutputString);
__declspec(dllimport) int  __stdcall MessageBoxA(void *hWnd, const char *lpText,
                                                  const char *lpCaption, unsigned int uType);
__declspec(dllimport) BOOL __stdcall MessageBeep(unsigned int uType);
}

// The engine's string formatting function (__cdecl: caller pops the args).
// Modeled as wsprintfA (output buffer first, then format string, then
// variadic args). The call-site `call rel32` displacement reloc-masks.
extern "C" int __cdecl EngFormat(char *dest, const char *fmt, ...);

// ---------------------------------------------------------------------------
// Module-level globals (output-mode flags + module name).
// The matched methods read these but never write them; the owning manager
// sets them during init. Declared as extern so the relocs name them.
// ---------------------------------------------------------------------------
// DirectSoundMgr flags
extern int  g_dsndBeep;          // @0x653c54  (MessageBeep if nonzero)
extern int  g_dsndDebug;         // @0x653c4c  (OutputDebugStringA if nonzero)
extern int  g_dsndMsgBox;        // @0x653c50  (MessageBoxA if nonzero)
extern int  g_dsndOutputDbg;     // @0x653c58  (another output gate)
extern char g_szDsndModule[];    // @0x619f3c  "DirectSoundMgr"

// ---------------------------------------------------------------------------
// DirectSoundMgr - manager for DirectSound (DsndMgr TU). The GetErrorString
// method is a self-contained HRESULT -> symbol mapper; it does NOT reference
// `this` (all state is module-level globals). Declared as a non-virtual method
// (no vtable entry in this TU; the engine calls it from its error-report path).
// ---------------------------------------------------------------------------
class DirectSoundMgr {
public:
    void GetErrorString(const char *file, int line, HRESULT hr);
};

#endif // DSNDMGR_H
