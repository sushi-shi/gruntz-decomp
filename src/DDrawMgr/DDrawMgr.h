// DDrawMgr.h - the engine CDirectDrawMgr - minimal reconstruction for
// CDirectDrawMgr::GetErrorString. Same TU as DirectDrawCreate caller.
#ifndef DDRAWMGR_H
#define DDRAWMGR_H

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

// CDirectDrawMgr module-level flags
extern int  g_ddrawBeep;         // @0x683ec0
extern int  g_ddrawDebug;        // @0x683eb8
extern int  g_ddrawMsgBox;       // @0x683ebc
extern int  g_ddrawOutputDbg;    // @0x683ec4
extern char g_szDDrawModule[];   // @0x61a378  "DirectDrawMgr"

class CDirectDrawMgr {
public:
    void GetErrorString(const char *file, int line, HRESULT hr);
};

#endif // DDRAWMGR_H
