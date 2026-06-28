// StartUpPrompt.cpp - the start-up CD-ROM / Spawn-Mode prompt (graduated from
// src/Stub/Backlog.cpp). A free __cdecl helper run during launch: if the Gruntz
// CD is already in a drive it clears the result flag and returns; otherwise it
// builds "<curdir>\Gruntz.REZ" and, depending on whether that local copy exists,
// either asks the user to insert the CD (a retry loop with a wait-cursor scope)
// or offers to run in Spawn Mode. Two MFC CString temps + the wait-cursor scope
// give it the /GX exception frame.
//
// IDENTITY recovered by string-xref ("Gruntz.REZ", "%s\\%s", the two MessageBox
// prompts) + the IsGruntzCDInAnyDrive / FileExists engine helpers. Only offsets /
// code bytes are load-bearing; modeled with real <Mfc.h> CString so cl emits the
// same ctors/dtors + the AfxGetModuleState->app->BeginWaitCursor/EndWaitCursor
// frame (afxwin.h's CWinApp won't parse under the label-pass clang, so the app is
// reached through AfxGetModuleState()->m_pCurrentWinApp and the wait-cursor pair
// is modeled as two __thiscall slots).

#include <Mfc.h>
#include <string.h> // inline strcpy intrinsic (/O2 /Oi)

#include <rva.h>

// The app's cached resource HINSTANCE (LoadString source) + the global "CD found /
// spawn requested" result the launcher reads after the prompt.
DATA(0x00251618)
extern "C" HINSTANCE g_appResHandle; // 0x651618
DATA(0x002455ec)
extern "C" i32 g_cdPromptResult; // 0x6455ec

// Engine launch helpers (reloc-masked __cdecl).
extern "C" i32 IsGruntzCDInAnyDrive();    // 0x402540
extern "C" i32 FileExists(const char* p); // 0x404282

// The current app's wait-cursor slots (CCmdTarget::Begin/EndWaitCursor), reached
// via AfxGetModuleState()->m_pCurrentWinApp.
struct WaitApp {
    void BeginWaitCursor(); // 0x1beafb
    void EndWaitCursor();   // 0x1beb10
};
// A scoped hourglass: ctor begins the wait cursor, dtor ends it - the third
// destructible local that (with the two CStrings) shapes the /GX unwind states.
struct WaitScope {
    WaitScope() { ((WaitApp*)AfxGetModuleState()->m_pCurrentWinApp)->BeginWaitCursor(); }
    ~WaitScope() { ((WaitApp*)AfxGetModuleState()->m_pCurrentWinApp)->EndWaitCursor(); }
};

// @early-stop
// ~98%: body byte-exact incl. the /GX frame, both CString temps + the WaitScope
// dtor states. Residual is the return/EH-cleanup tail-merge wall
// (docs/patterns/identical-return-epilogue-tailmerge.md): retail emits the
// EndWaitCursor dtor inline at each loop break AND `mov eax,1` inline at the early
// CD-found return, where MSVC5 here tail-merges the duplicate exits into one
// shared block. Logic complete; the merge is a cl /O2 layout choice, not steerable.
RVA(0x0001f9b0, 0x2d2)
int StartUpPrompt(HWND hWnd) {
    if (IsGruntzCDInAnyDrive()) {
        g_cdPromptResult = 0;
        return 1;
    }

    char szDir[256];
    if (!GetCurrentDirectoryA(0xff, szDir)) {
        return 0;
    }

    CString strPath;
    CString strRez = "Gruntz.REZ";
    strPath.Format("%s\\%s", szDir, (LPCTSTR)strRez);

    char szText[128];
    char szCaption[62];

    if (!FileExists(strPath)) {
        g_cdPromptResult = 0;
        for (;;) {
            strcpy(szText, "Please insert the game CD-ROM into the drive.");
            if (!LoadStringA(g_appResHandle, 0x8003, szCaption, 0x3e)) {
                strcpy(szCaption, "Gruntz");
            }
            if (MessageBoxA(hWnd, szText, szCaption, 0x31) != IDOK) {
                return 0;
            }
            {
                WaitScope wait;
                if (IsGruntzCDInAnyDrive()) {
                    break;
                }
                Sleep(0x3e8);
                if (IsGruntzCDInAnyDrive()) {
                    break;
                }
            }
        }
        return 1;
    }

    if (!LoadStringA(g_appResHandle, 0x8021, szText, 0x7c)) {
        strcpy(szText, "Gruntz CD-ROM not found. Run in Spawn Mode?");
    }
    if (!LoadStringA(g_appResHandle, 0x8003, szCaption, 0x3e)) {
        strcpy(szCaption, "Gruntz");
    }
    if (MessageBoxA(hWnd, szText, szCaption, 0x34) == IDYES) {
        g_cdPromptResult = 1;
        return 1;
    }
    return 0;
}
