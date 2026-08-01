#include <Mfc.h>
#include <Utils/WinAPICdRom.h>
#include <Gruntz/HeapDiag.h>
#ifdef __clang__
#undef _AFX_ENABLE_INLINES
#endif
#include <afxwin.h>
#include <string.h>

#include <rva.h>
#include <Gruntz/StartUpPrompt.h>
#include <Gruntz/GruntzMgr.h>

DATA(0x00251618)
HINSTANCE g_appResHandle;

DATA(0x002455ec)
i32 g_cdPromptResult = 0;

// @early-stop
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

    if (!FileExists(const_cast<char*>(static_cast<const char*>(strPath)))) {
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
