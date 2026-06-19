// WinMain.cpp - Gruntz program entry point (C:\Proj\Gruntz).
//
//   _WinMain@16 - the WINAPI entry. SEH-framed (push -1; push &handler;
//       mov fs:0). Allocates a 0x10c-byte frame and drives the whole startup:
//         1. GetModuleFileNameA -> engine path-check (CheckExePath);
//         2. single-instance guard (FindWindowA "GruntzClass"); if a prior
//            instance exists: restore it if iconic, forward a lobby-launch
//            WM_COMMAND if the cmd line contains "LOBBYLAUNCH", then return 0;
//         3. otherwise: read our own FileVersion (VERSION.DLL) into four
//            file-scope version ints, run a startup gate (resource/CD check),
//            `new CGruntzApp`, then scan the cmd line for "advanced"/"optionz"
//            (+ the ALL-CAPS variants) or the dev hot-key, optionally run the
//            Advanced Options dialog, Init the app, run its message loop
//            (app->Run, vtable +0x18), and tear it down.
//
// THE MAIN MESSAGE LOOP itself is NOT inline here: WinMain reaches it via the
// app's run virtual (vtable slot +0x18 -> CGameApp::RunMessageLoop,
// 159 B - a GetMessageA / TranslateAcceleratorA / TranslateMessage /
// DispatchMessageA pump with an idle vtable callback `[vtbl+0x20]`). That pump
// is the next dedicated target; here we only emit the dispatching
// `call [eax+0x18]`.
//
// Built /O2 /MT /GX: the normal path `new CGruntzApp`s under a C++ EH frame
// (the /GX `fs:0` setup matching the target's prolog). Only offsets / code
// bytes are load-bearing; class and field names are placeholders. Unmatched
// engine callees are modeled as external no-body functions so their `call` /
// `push &fn` relocs are masked in objdiff.
#include "../Wap32/Wap32.h"
#include "../rva.h"

// ---------------------------------------------------------------------------
// Minimal Win32 surface (USER32 / KERNEL32). We deliberately do NOT pull in
// <windows.h> - keep the visible symbol SET small (the compiler hashes it;
// entropy follows header churn - see docs/matching-patterns.md). This
// reproduces the FF15 [IAT] direct-call form for the imports. Win32 entry
// types (HINSTANCE / HWND / DWORD / WPARAM / LPARAM / LPCSTR / BOOL) come from
// Wap32.h above; PostMessageA is declared there too.
// ---------------------------------------------------------------------------
typedef char *  LPSTR;
typedef unsigned int UINT;
#ifndef WINAPI
#define WINAPI __stdcall
#endif

extern "C" {
__declspec(dllimport) DWORD   __stdcall GetModuleFileNameA(HINSTANCE hModule, LPSTR lpFilename, DWORD nSize);
__declspec(dllimport) HWND    __stdcall FindWindowA(LPCSTR lpClassName, LPCSTR lpWindowName);
__declspec(dllimport) BOOL    __stdcall IsIconic(HWND hWnd);
__declspec(dllimport) LRESULT __stdcall SendMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
__declspec(dllimport) short   __stdcall GetAsyncKeyState(int vKey);
__declspec(dllimport) int     __stdcall DialogBoxParamA(HINSTANCE hInstance, LPCSTR lpTemplateName,
                                                        HWND hWndParent, void *lpDialogFunc,
                                                        LPARAM dwInitParam);
}

// Win32 message / constant literals (kept local; not from <windows.h>).
#define WM_SYSCOMMAND 0x0112
#define WM_COMMAND    0x0111
#define SC_RESTORE    0xf120
#define CW_USEDEFAULT  ((int)0x80000000)

// Virtual-key codes scanned for the developer Advanced-Options hot-key:
// 0x11 (VK_CONTROL), 0x10 (VK_SHIFT), 0x24 (VK_HOME / numeric '$').
#define VK_CONTROL    0x11
#define VK_SHIFT      0x10
#define VK_DOLLAR     0x24

// ---------------------------------------------------------------------------
// Engine callees (unmatched; modeled as external no-body functions so the
// `call rel32` / `push &fn` reloc is masked - only the call/push bytes matter).
// The reloc-masked literal addresses (the "Gruntz" string, the cmd-line tokens)
// are passed as casted absolute pointers; objdiff masks the DIR32 operand.
// ---------------------------------------------------------------------------
extern "C" {
// CheckExePath (reached via an incremental-link thunk). Validates
// the module path; __cdecl 3 args (path, count, reserved); returns nonzero to
// proceed to the single-instance check.
int CheckExePath(char *pszPath, int nCount, void *pReserved);

// SubstringMatch (a strstr-class helper). Returns nonzero when
// `pszNeedle` occurs in `pszHaystack`. __cdecl 2 args (haystack first, then
// needle - the target's push order). Used for the LOBBYLAUNCH check and the
// "advanced"/"optionz" cmd-line scans.
int SubstringMatch(LPCSTR pszHaystack, LPCSTR pszNeedle);

// StartupGate (reached via a thunk). __cdecl 1 arg; runs the
// resource/CD/launch validation, returns nonzero to proceed.
int StartupGate(int nReserved);

// SettleDelay - a GetTickCount busy-wait used as a brief settle delay
// before the hot-key sample. __cdecl 1 arg (ms).
int SettleDelay(int nMs);

// VersionScan - an sscanf wrapper (parses "%d.%d.%d.%d" into the four
// version ints). __cdecl variadic.
int VersionScan(const char *pszVersion, const char *pszFormat, ...);

// VERSION.DLL imports (6-byte `jmp [IAT]` thunks); __stdcall (callee-cleaned, no
// `add esp` at the call site).
DWORD __stdcall GetFileVersionInfoSizeA(LPSTR lptstrFilename, DWORD *lpdwHandle);
int   __stdcall GetFileVersionInfoA(LPSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, void *lpData);
int   __stdcall VerQueryValueA(const void *pBlock, LPSTR lpSubBlock, void **lplpBuffer, UINT *puLen);
}

// The MFC global allocator / deallocator (NAFXCW); used as
// the explicit operator-function forms for the FileVersion query buffer (the
// `new CGruntzApp` further down uses the implicit new+ctor form).

// The Advanced Options modal dialog proc (matched, unit advancedoptions). Its
// address is taken for DialogBoxParamA (reloc-masked via a thunk).
int __stdcall AdvancedOptionsDialogProc(HWND, UINT, WPARAM, LPARAM);

// ---------------------------------------------------------------------------
// CGruntzApp - the game application object `new`'d on the normal path. The only
// members WinMain touches are the vtable: Init / RunMessageLoop / the
// scalar-deleting dtor are all virtual dispatches (`call [vtbl+N]`), so a class
// with an opaque 0x254-byte body + the used virtual slots suffices. The ctor
// (reached via a thunk) and the virtuals are UNMATCHED but their
// calls are reloc-masked, so the call bytes are still byte-exact. The vtable
// layout follows CGameApp (see src/Wap32/Wap32.h):
//   slot 0  (+0x00) ~CGruntzApp  -> `delete g_pApp` (scalar-deleting dtor)
//   slot 2  (+0x08) Init(hInst,name,ident,cmd,flags,w,h)
//   slot 6  (+0x18) RunMessageLoop()  -> the message pump
// ---------------------------------------------------------------------------
class CGruntzApp {
public:
    CGruntzApp();
    virtual ~CGruntzApp();                                              // slot 0 (+0x00)
    virtual int  Vfn1();                                               // slot 1 (+0x04)
    virtual int  Init(HINSTANCE hInstance, LPCSTR szWindowName, LPCSTR szIdent,
                      LPSTR szCmdLine, int flags, int w, int h);        // slot 2 (+0x08)
    virtual int  Vfn3();                                               // slot 3 (+0x0c)
    virtual int  Vfn4();                                               // slot 4 (+0x10)
    virtual int  Vfn5();                                               // slot 5 (+0x14)
    virtual int  RunMessageLoop();                                     // slot 6 (+0x18)
private:
    char m_pad[0x254 - 4];   // total 0x254 bytes (vptr @+0 + opaque body)
};

// ---------------------------------------------------------------------------
// File-scope globals (the relocs that name them are masked in objdiff; only the
// address-load bytes are load-bearing).
//   g_version0..3   - the parsed FileVersion components
//       (order matches the "%d.%d.%d.%d" out-params left-to-right).
//   g_pApp          - the CGruntzApp* (constructed on the normal path,
//       `delete`d on every exit of that path).
//   g_hInstance     - this module's HINSTANCE (shared with
//       AdvancedOptions.cpp, which reads it for LoadIconA).
// ---------------------------------------------------------------------------
static int        g_version0;     // 1st %d
static int        g_version1;     // 2nd %d
static int        g_version2;     // 3rd %d
static int        g_version3;     // 4th %d
static CGruntzApp *g_pApp;
static HINSTANCE   g_hInstance;

// ---------------------------------------------------------------------------
// WinMain - extern "C" int WINAPI WinMain(...) -> the linker symbol is
// `_WinMain@16` (NOT C++ mangled).
SYMBOL(_WinMain@16)
RVA(0x11c860, 0x327)
extern "C" int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                              LPSTR lpCmdLine, int nShowCmd)
{
    char szModulePath[0xFE];       // [esp+0x1c] - the GetModuleFileNameA buffer

    // 1. Module path + engine path-check. When the path-check SUCCEEDS this is
    //    a secondary / lobby launch: hand off to any existing instance and exit
    //    (this whole branch returns 0). Only when the path-check FAILS do we
    //    fall through to the normal startup below.
    if (GetModuleFileNameA(0, szModulePath, 0xFE) > 0 &&
        CheckExePath(szModulePath, 2, 0) != 0) {
        // 2. Single-instance guard: locate the prior window and, if present,
        //    restore it / forward a lobby-launch WM_COMMAND.
        HWND hPrev = FindWindowA("GruntzClass", "Gruntz");
        if (hPrev != 0) {
            if (IsIconic(hPrev))
                SendMessageA(hPrev, WM_SYSCOMMAND, SC_RESTORE, 0);
            if (lpCmdLine != 0 && SubstringMatch(lpCmdLine, "LOBBYLAUNCH") != 0)
                PostMessageA(hPrev, WM_COMMAND, 0x80b7, 0);
        }
        return 0;
    }

    // 3a. Read our own FileVersion ("\StringFileInfo\040904B0\FileVersion") and
    //     parse it into the four version ints via sscanf("%d.%d.%d.%d", ...).
    {
        DWORD dwSize = GetFileVersionInfoSizeA(szModulePath, 0);
        void *pInfo  = operator new(dwSize);
        GetFileVersionInfoA(szModulePath, 0, dwSize, pInfo);
        void *pValue;
        UINT  uLen;
        VerQueryValueA(pInfo, (LPSTR)"\\StringFileInfo\\040904B0\\FileVersion",
                       &pValue, &uLen);
        VersionScan((const char *)pValue, "%d, %d, %d, %d",
                    &g_version0, &g_version1, &g_version2, &g_version3);
        operator delete(pInfo);
    }

    // 3b. The startup gate (resource/CD/launch check). On failure bail out.
    if (StartupGate(0) == 0)
        return 0;

    // 3c. Construct the application object (operator new + ctor under the EH
    //     frame). On allocation/ctor failure bail out.
    g_pApp = new CGruntzApp;
    if (g_pApp == 0)
        return 0;

    // 3d. Brief settle delay (a busy-wait), then decide whether to open the
    //     Advanced Options dialog: scan the dev hot-key (Ctrl + Shift + $) and
    //     the "advanced"/"optionz" cmd-line tokens. The hInstance store is
    //     scheduled here (the target interleaves it with the settle-delay call).
    g_hInstance = hInstance;
    int bAdvanced = 0;
    SettleDelay(0x64);   // busy-wait, ~100ms
    if ((short)GetAsyncKeyState(VK_CONTROL) & 0x80000000)
        bAdvanced = 1;
    if ((short)GetAsyncKeyState(VK_SHIFT) & 0x80000000)
        bAdvanced = 1;
    if ((short)GetAsyncKeyState(VK_DOLLAR) & 0x80000000)
        bAdvanced = 1;

    if (lpCmdLine != 0) {
        if (SubstringMatch(lpCmdLine, "advanced") != 0) bAdvanced = 1;
        if (SubstringMatch(lpCmdLine, "optionz") != 0) bAdvanced = 1;
        if (SubstringMatch(lpCmdLine, "ADVANCED") != 0) bAdvanced = 1;
        if (SubstringMatch(lpCmdLine, "OPTIONZ") != 0) bAdvanced = 1;
        if (SubstringMatch(lpCmdLine, "ADV") != 0) bAdvanced = 1;
        if (SubstringMatch(lpCmdLine, "adv") != 0) bAdvanced = 1;
    }

    // 3e. If requested, run the Advanced Options modal; on it returning 0 (the
    //     "do not launch the game" result) tear the app down and exit.
    if (bAdvanced != 0) {
        int nDlgResult = DialogBoxParamA(g_hInstance, "CONFIG_ADVANCED",
                                         0, (void *)&AdvancedOptionsDialogProc, 0);
        if (nDlgResult == 0) {
            if (g_pApp != 0)
                delete g_pApp;
            g_pApp = 0;
            return 0;
        }
    }

    // 3f. Init the app: Init(hInstance, "Gruntz", "Gruntz", cmdLine, 0,
    //     CW_USEDEFAULT, CW_USEDEFAULT). On failure tear down + return 0.
    if (g_pApp->Init(hInstance, "Gruntz", "Gruntz", lpCmdLine,
                     0, CW_USEDEFAULT, CW_USEDEFAULT) == 0) {
        if (g_pApp != 0)
            delete g_pApp;
        g_pApp = 0;
        return 0;
    }

    // 3g. Run the main message loop (vtable +0x18), then tear down.
    int rc = g_pApp->RunMessageLoop();
    if (g_pApp != 0)
        delete g_pApp;
    g_pApp = 0;
    return rc;
}
