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
// <Mfc.h> brings <windows.h> USER32/KERNEL32 (GetModuleFileNameA / FindWindowA /
// IsIconic / SendMessageA / GetAsyncKeyState / DialogBoxParamA / PostMessageA;
// HINSTANCE / HWND / DWORD / WPARAM / LPARAM / LPSTR / LPCSTR / UINT / BOOL / WINAPI)
// and the WM_* / SC_RESTORE / CW_USEDEFAULT / VK_CONTROL / VK_SHIFT literals.
#include <Mfc.h>
#include <string.h> // strstr (0x120090), cmd-line flag scan
#include <Wap32/Wap32.h>
#include <Gruntz/Enums.h>
#include <rva.h>

// The developer hot-key's third key (no SDK VK_ name for '$'; == VK_HOME's code).
typedef enum GruntzHotKey {
    VK_DOLLAR = 0x24,
} GruntzHotKey;

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
    i32 CheckExePath(char* pszPath, i32 nCount, void* pReserved);

    // StartupGate (reached via a thunk). __cdecl 1 arg; runs the
    // resource/CD/launch validation, returns nonzero to proceed.
    i32 StartupGate(i32 nReserved);

    // SettleDelay - a GetTickCount busy-wait used as a brief settle delay
    // before the hot-key sample. __cdecl 1 arg (ms).
    i32 SettleDelay(i32 nMs);

    // VersionScan - an sscanf wrapper (parses "%d.%d.%d.%d" into the four
    // version ints). __cdecl variadic.
    i32 VersionScan(const char* pszVersion, const char* pszFormat, ...);

    // VERSION.DLL imports (GetFileVersionInfoSizeA/GetFileVersionInfoA/VerQueryValueA)
    // come from <windows.h> (winver, pulled by afx.h/MFC).
}

// The "Gruntz" app-name literal (0x60aac8, this TU's .data run - the "1.0" version
// literal follows it). Owner-TU definition; shared as the window title here and as
// the MessageBoxA caption in GruntzMgr.cpp (extern there as g_msgCaption). Length
// NULL-TERMINATOR-PROVEN ("Gruntz" + NUL = 7 B).
// (g_msgCaption @0x20aac8 was a FICTION -- an invented name for cl's folded
// `??_C@_06HPPL@Gruntz@` literal COMDAT, which all 7 referencing TUs emit for
// themselves. Spelled inline now.)

// The MFC global allocator / deallocator (NAFXCW); used as
// the explicit operator-function forms for the FileVersion query buffer (the
// `new CGruntzApp` further down uses the implicit new+ctor form).

// The Advanced Options modal dialog proc (matched, unit advancedoptions). Its
// address is taken for DialogBoxParamA (reloc-masked via a thunk).
i32 CALLBACK AdvancedOptionsDialogProc(HWND, UINT, WPARAM, LPARAM);

// CGruntzApp - the game application object `new`'d on the normal path, defined
// once in <Gruntz/GruntzApp.h>. WinMain touches only its vtable: slot 2
// (Init, the app "Init"), slot 6 (RunMessageLoop) and the
// scalar-deleting dtor (slot 0) - all virtual dispatches (`call [vtbl+N]`), so
// the call bytes are byte-exact regardless of method names.
#include <Gruntz/GruntzApp.h>

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
static i32 g_version0; // 1st %d
static i32 g_version1; // 2nd %d
static i32 g_version2; // 3rd %d
static i32 g_version3; // 4th %d
static CGruntzApp* g_pApp;
static HINSTANCE g_hInstance;

// ---------------------------------------------------------------------------
// WinMain - extern "C" int WINAPI WinMain(...) -> the linker symbol is
// `_WinMain@16` (NOT C++ mangled).
SYMBOL(_WinMain @16)
RVA(0x0011c860, 0x327)
extern "C" i32 WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, i32 nShowCmd) {
    char szModulePath[0xFE]; // [esp+0x1c] - the GetModuleFileNameA buffer

    // 1. Module path + engine path-check. When the path-check SUCCEEDS this is
    //    a secondary / lobby launch: hand off to any existing instance and exit
    //    (this whole branch returns 0). Only when the path-check FAILS do we
    //    fall through to the normal startup below.
    if (GetModuleFileNameA(0, szModulePath, 0xFE) > 0 && CheckExePath(szModulePath, 2, 0) != 0) {
        // 2. Single-instance guard: locate the prior window and, if present,
        //    restore it / forward a lobby-launch WM_COMMAND.
        HWND hPrev = FindWindowA("GruntzClass", "Gruntz");
        if (hPrev != 0) {
            if (IsIconic(hPrev)) {
                SendMessageA(hPrev, WM_SYSCOMMAND, SC_RESTORE, 0);
            }
            if (lpCmdLine != 0 && strstr(lpCmdLine, "LOBBYLAUNCH") != 0) {
                PostMessageA(hPrev, WM_COMMAND, LOBBYLAUNCH, 0);
            }
        }
        return 0;
    }

    // 3a. Read our own FileVersion ("\StringFileInfo\040904B0\FileVersion") and
    //     parse it into the four version ints via sscanf("%d.%d.%d.%d", ...).
    {
        DWORD dwSize = GetFileVersionInfoSizeA(szModulePath, 0);
        void* pInfo = operator new(dwSize);
        GetFileVersionInfoA(szModulePath, 0, dwSize, pInfo);
        void* pValue;
        UINT uLen;
        VerQueryValueA(pInfo, const_cast<LPSTR>("\\StringFileInfo\\040904B0\\FileVersion"), &pValue, &uLen);
        VersionScan(
            static_cast<const char*>(pValue),
            "%d, %d, %d, %d",
            &g_version0,
            &g_version1,
            &g_version2,
            &g_version3
        );
        operator delete(pInfo);
    }

    // 3b. The startup gate (resource/CD/launch check). On failure bail out.
    if (StartupGate(0) == 0) {
        return 0;
    }

    // 3c. Construct the application object (operator new + ctor under the EH
    //     frame). On allocation/ctor failure bail out.
    g_pApp = new CGruntzApp;
    if (g_pApp == 0) {
        return 0;
    }

    // 3d. Brief settle delay (a busy-wait), then decide whether to open the
    //     Advanced Options dialog: scan the dev hot-key (Ctrl + Shift + $) and
    //     the "advanced"/"optionz" cmd-line tokens. The hInstance store is
    //     scheduled here (the target interleaves it with the settle-delay call).
    g_hInstance = hInstance;
    i32 bAdvanced = 0;
    SettleDelay(0x64); // busy-wait, ~100ms
    if (static_cast<i16>(GetAsyncKeyState(VK_CONTROL)) & 0x80000000) {
        bAdvanced = 1;
    }
    if (static_cast<i16>(GetAsyncKeyState(VK_SHIFT)) & 0x80000000) {
        bAdvanced = 1;
    }
    if (static_cast<i16>(GetAsyncKeyState(VK_DOLLAR)) & 0x80000000) {
        bAdvanced = 1;
    }

    if (lpCmdLine != 0) {
        if (strstr(lpCmdLine, "advanced") != 0) {
            bAdvanced = 1;
        }
        if (strstr(lpCmdLine, "optionz") != 0) {
            bAdvanced = 1;
        }
        if (strstr(lpCmdLine, "ADVANCED") != 0) {
            bAdvanced = 1;
        }
        if (strstr(lpCmdLine, "OPTIONZ") != 0) {
            bAdvanced = 1;
        }
        if (strstr(lpCmdLine, "ADV") != 0) {
            bAdvanced = 1;
        }
        if (strstr(lpCmdLine, "adv") != 0) {
            bAdvanced = 1;
        }
    }

    // 3e. If requested, run the Advanced Options modal; on it returning 0 (the
    //     "do not launch the game" result) tear the app down and exit.
    if (bAdvanced != 0) {
        i32 nDlgResult = DialogBoxParamA(
            g_hInstance,
            "CONFIG_ADVANCED",
            0,
            reinterpret_cast<DLGPROC>(&AdvancedOptionsDialogProc),
            0
        );
        if (nDlgResult == 0) {
            if (g_pApp != 0) {
                delete g_pApp;
            }
            g_pApp = 0;
            return 0;
        }
    }

    // 3f. Init the app: Init(hInstance, "Gruntz", "Gruntz", cmdLine, 0,
    //     CW_USEDEFAULT, CW_USEDEFAULT). On failure tear down + return 0.
    if (g_pApp->Init(hInstance, "Gruntz", "Gruntz", lpCmdLine, 0, CW_USEDEFAULT, CW_USEDEFAULT)
        == 0) {
        if (g_pApp != 0) {
            delete g_pApp;
        }
        g_pApp = 0;
        return 0;
    }

    // 3g. Run the main message loop (vtable +0x18), then tear down.
    i32 rc = g_pApp->RunMessageLoop();
    if (g_pApp != 0) {
        delete g_pApp;
    }
    g_pApp = 0;
    return rc;
}

// (0x11e8dc - the ex-`CObjStamp11e8dc` @identity-TODO shell - is GONE, 2026-07-16:
// it IS the NAFXCW library ??1CException@@UAE@XZ. Proof: its sole caller, the ??_G
// scalar-deleting dtor @0x11ea46, is referenced ONLY from .rdata 0x1ed588 ==
// ??_7CException@@6B@+0x4 (the catalog-bound RTTI vtable @0x1ed584), and the whole
// 0x11e8xx band is the NAFXCW exception-family obj (??1CFileException @0x11e8ff,
// CArchive/CMemoryException rows around it; the FID "??_GCWinApp" rows there are
// AMBIG false positives). ~CException is _AFX_INLINE in AFX.INL, so no TU can
// re-emit it; the identity is recorded as a library label
// (config/library_labels.csv), where library code belongs - not hand-claimed here.)
