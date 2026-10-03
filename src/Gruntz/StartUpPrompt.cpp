#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/StartUpPrompt.h>

#include <Gruntz/GruntzMgr.h>
#include <Gruntz/PathBuffer.h>
#include <Gruntz/Utils.h>
#include <Gruntz/WaitCursorScope.h>
#include <Utils/WinAPICdRom.h>

#include <string.h>

HINSTANCE g_appResHandle;

b32 g_cdPromptResult = false;

int StartUpPrompt(HWND hWnd) {
    if (IsGruntzCDInAnyDrive()) {
        g_cdPromptResult = false;
        return 1;
    }

    char szDir[GRUNTZ_PATH_BUFFER_SIZE];
    if (!GetCurrentDirectoryA(GRUNTZ_PATH_BUFFER_MAX_CHARS, szDir)) {
        return 0;
    }

    CString strPath;
    CString strRez = "Gruntz.REZ";
    strPath.Format("%s\\%s", szDir, static_cast<LPCTSTR>(strRez));

    char szText[128];
    char szCaption[62];

    if (!FileExists(strPath)) {
        g_cdPromptResult = false;
        for (;;) {
            strcpy(szText, "Please insert the game CD-ROM into the drive.");
            if (!LoadStringA(g_appResHandle, 0x8003, szCaption, 0x3e)) {
                strcpy(szCaption, "Gruntz");
            }
            if (MessageBoxA(hWnd, szText, szCaption, MB_OKCANCEL | MB_ICONEXCLAMATION) != IDOK) {
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
    if (MessageBoxA(hWnd, szText, szCaption, MB_YESNO | MB_ICONEXCLAMATION) == IDYES) {
        g_cdPromptResult = true;
        return 1;
    }
    return 0;
}
