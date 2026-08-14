#include <rva.h>

#include <Gruntz/StartUpPrompt.h>

#include <Mfc.h>
#include <MfcWin.h>

#include <Gruntz/GruntzMgr.h>
#include <Gruntz/HeapDiag.h>
#include <Gruntz/WaitCursorScope.h>
#include <Utils/WinAPICdRom.h>

#include <string.h>

DATA(0x00251618)
HINSTANCE g_appResHandle;

DATA(0x002455ec)
i32 g_cdPromptResult = 0;

// @early-stop
// The two in-scope returns retain distinct CWaitCursorScope teardown tails.
// Primary code is exact; the fieldless guard's synthetic unwind receiver is the
// bounded C1 frame-colouring case documented for this class.
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
                CWaitCursorScope wait;
                if (IsGruntzCDInAnyDrive()) {
                    return 1;
                }
                Sleep(0x3e8);
                if (IsGruntzCDInAnyDrive()) {
                    return 1;
                }
            }
        }
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
