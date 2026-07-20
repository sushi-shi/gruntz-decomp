#include <Mfc.h>
#ifdef __clang__
#undef _AFX_ENABLE_INLINES
#endif
#include <afxwin.h>
#include <string.h> // inline strcpy intrinsic (/O2 /Oi)

#include <rva.h>
#include <Globals.h>

DATA(0x00251618)
extern "C" HINSTANCE g_appResHandle; // 0x651618

DATA(0x002455ec)
extern "C" {
    i32 g_cdPromptResult = 0;
}

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
    strPath.Format("%s\\%s", szDir, static_cast<LPCTSTR>(strRez));

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
