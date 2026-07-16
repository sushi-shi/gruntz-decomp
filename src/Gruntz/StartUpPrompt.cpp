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
// same ctors/dtors + the real MFC CWaitCursor scope guard: its ctor/dtor are
// AfxGetApp()->BeginWaitCursor()/EndWaitCursor() (afxwin2.inl), i.e. exactly
// AfxGetModuleState()->m_pCurrentWinApp->Begin/EndWaitCursor - byte-identical.

#include <Mfc.h>
// Real MFC CWinApp / CWaitCursor (BeginWaitCursor @0x1beafb). afxwin*.inl is skipped
// for the clang label step only (implicit-int CMenu::op==); wine cl keeps the inlines
// so `CWaitCursor wait;` inlines BeginWaitCursor exactly as retail does. See
// docs/patterns/afxwin-clang-label-step-skip-inl.md.
#ifdef __clang__
#undef _AFX_ENABLE_INLINES
#endif
#include <afxwin.h>
#include <string.h> // inline strcpy intrinsic (/O2 /Oi)

#include <rva.h>
#include <Globals.h>

// The app's cached resource HINSTANCE (LoadString source) + the global "CD found /
// spawn requested" result the launcher reads after the prompt.
DATA(0x00251618)
extern "C" HINSTANCE g_appResHandle; // 0x651618

// The CD-prompt result gate (0x6455ec, .bss). Owner-TU definition: this prompt is
// its writer; GruntzMgrCmd / VideoConfig / MenuState / MainMenuBuilder read it.
DATA(0x002455ec)
extern "C" {
    i32 g_cdPromptResult = 0;
}

// Engine launch helpers (reloc-masked __cdecl).
extern "C" i32 IsGruntzCDInAnyDrive();    // 0x402540
extern "C" i32 FileExists(const char* p); // 0x404282

// @early-stop
// ~98%: body byte-exact incl. the /GX frame, both CString temps + the CWaitCursor
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
                CWaitCursor wait;
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
