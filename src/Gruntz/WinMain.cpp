#include <Mfc.h>
#include <stdio.h>  // sscanf (0x120900), the version parse
#include <string.h> // strstr (0x120090), cmd-line flag scan
#include <Wap32/Wap32.h>
#include <Gruntz/Enums.h>
#include <rva.h>

typedef enum GruntzHotKey {
    VK_DOLLAR = 0x24,
} GruntzHotKey;

// FindProcessByName (0x118ce0, HeapDiag.cpp): count running processes whose exe
// basename matches; reached via an incremental-link thunk here as the single-
// instance guard (path, wantCount, out-handle).
i32 FindProcessByName(const char* name, i32 wantCount, void** pHandleOut);

// StartUpPrompt (0x1f9b0): the resource/CD/launch validation prompt; reached via
// a thunk. Returns nonzero to proceed. (1 arg: the parent HWND, null here.)
i32 StartUpPrompt(HWND__* parent);

// ActiveWait (0x13dfe0) - the timeGetTime busy-wait, used here as a brief
// settle delay before the hot-key sample.
void ActiveWait(u32 milliseconds);

// VERSION.DLL imports (GetFileVersionInfoSizeA/GetFileVersionInfoA/VerQueryValueA)
// come from <windows.h> (winver, pulled by afx.h/MFC).

i32 CALLBACK AdvancedOptionsDialogProc(HWND, UINT, WPARAM, LPARAM);

#include <Gruntz/GruntzApp.h>

static i32 g_version0; // 1st %d
static i32 g_version1; // 2nd %d
static i32 g_version2; // 3rd %d
static i32 g_version3; // 4th %d
static CGruntzApp* g_pApp;
static HINSTANCE g_hInstance;

RVA(0x0011c860, 0x327)
i32 WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, i32 nShowCmd) {
    char szModulePath[0xFE]; // [esp+0x1c] - the GetModuleFileNameA buffer

    // 1. Module path + engine path-check. When the path-check SUCCEEDS this is
    //    a secondary / lobby launch: hand off to any existing instance and exit
    //    (this whole branch returns 0). Only when the path-check FAILS do we
    //    fall through to the normal startup below.
    if (GetModuleFileNameA(0, szModulePath, 0xFE) > 0
        && FindProcessByName(szModulePath, 2, 0) != 0) {
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
        VerQueryValueA(
            pInfo,
            const_cast<LPSTR>("\\StringFileInfo\\040904B0\\FileVersion"),
            &pValue,
            &uLen
        );
        sscanf(
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
    if (StartUpPrompt(0) == 0) {
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
    ActiveWait(0x64); // busy-wait, ~100ms
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
        i32 nDlgResult =
            DialogBoxParamA(g_hInstance, "CONFIG_ADVANCED", 0, &AdvancedOptionsDialogProc, 0);
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
