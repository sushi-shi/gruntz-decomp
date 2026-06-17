// DinMgr2.h - the engine DirectInputMgr2 - minimal reconstruction for
// DirectInputMgr2::GetErrorString. Same TU as DirectInputCreateA caller.
#ifndef DINMGR2_H
#define DINMGR2_H

typedef long           HRESULT;
typedef unsigned long  DWORD;
typedef int            BOOL;

extern "C" {
__declspec(dllimport) void __stdcall OutputDebugStringA(const char *lpOutputString);
__declspec(dllimport) int  __stdcall MessageBoxA(void *hWnd, const char *lpText,
                                                  const char *lpCaption, unsigned int uType);
__declspec(dllimport) BOOL __stdcall MessageBeep(unsigned int uType);
}

extern "C" int __cdecl EngFormat(char *dest, const char *fmt, ...);

// DirectInputMgr2 module-level flags
extern int  g_dinBeep;           // @0x653aac
extern int  g_dinDebug;          // @0x653aa4
extern int  g_dinMsgBox;         // @0x653aa8
extern int  g_dinOutputDbg;      // @0x653ab0
extern char g_szDinModule[];     // @0x6199d8  "DirectInputMgr2"

class DirectInputMgr2 {
public:
    void GetErrorString(const char *file, int line, HRESULT hr);
};

#endif // DINMGR2_H
