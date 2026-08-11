#include <rva.h>

#include <Gruntz/WinMain.h>

#include <Mfc.h>

#include <Enums.h>
#include <Gruntz/GameAssetNamespaces.h>
#include <Gruntz/GruntzApp.h>
#include <Gruntz/GruntzCommandId.h>
#include <Gruntz/MenuVersion.h>
#include <Gruntz/StartUpPrompt.h>
#include <Wap32/Wap32.h>

#include <stdio.h>
#include <string.h>

typedef enum GruntzHotKey {
    VK_DOLLAR = 0x24,
} GruntzHotKey;

DATA(0x00251600)
CGruntzApp* g_pApp;

RVA(0x0011c860, 0x327)
i32 WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, i32 nShowCmd) {
    char szModulePath[0xFE];

    if (GetModuleFileNameA(0, szModulePath, 0xFE) > 0
        && FindProcessByName(szModulePath, 2, 0) != 0) {

        HWND hPrev = FindWindowA("GruntzClass", "Gruntz");
        if (hPrev != NULL) {
            if (IsIconic(hPrev)) {
                SendMessageA(hPrev, WM_SYSCOMMAND, SC_RESTORE, 0);
            }
            if (lpCmdLine != NULL && strstr(lpCmdLine, "LOBBYLAUNCH") != NULL) {
                PostMessageA(hPrev, WM_COMMAND, IDX(CMD_LOBBY_RESET), 0);
            }
        }
        return 0;
    }

    {
        DWORD dwSize = GetFileVersionInfoSizeA(szModulePath, 0);
        u8* pInfo = new u8[dwSize];
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
            &g_versionMajor,
            &g_versionMid,
            &g_versionMinor,
            &g_buildNumber
        );
        delete[] pInfo;
    }

    if (StartUpPrompt(0) == 0) {
        return 0;
    }

    g_pApp = new CGruntzApp;
    if (g_pApp == NULL) {
        return 0;
    }

    g_appResHandle = hInstance;
    i32 bAdvanced = 0;
    ActiveWait(0x64);
    if (static_cast<i16>(GetAsyncKeyState(VK_CONTROL)) & 0x80000000) {
        bAdvanced = 1;
    }
    if (static_cast<i16>(GetAsyncKeyState(VK_SHIFT)) & 0x80000000) {
        bAdvanced = 1;
    }
    if (static_cast<i16>(GetAsyncKeyState(VK_DOLLAR)) & 0x80000000) {
        bAdvanced = 1;
    }

    if (lpCmdLine != NULL) {
        if (strstr(lpCmdLine, "advanced") != NULL) {
            bAdvanced = 1;
        }
        if (strstr(lpCmdLine, "optionz") != NULL) {
            bAdvanced = 1;
        }
        if (strstr(lpCmdLine, "ADVANCED") != NULL) {
            bAdvanced = 1;
        }
        if (strstr(lpCmdLine, "OPTIONZ") != NULL) {
            bAdvanced = 1;
        }
        if (strstr(lpCmdLine, "ADV") != NULL) {
            bAdvanced = 1;
        }
        if (strstr(lpCmdLine, "adv") != NULL) {
            bAdvanced = 1;
        }
    }

    if (bAdvanced != 0) {
        i32 nDlgResult =
            DialogBoxParamA(g_appResHandle, "CONFIG_ADVANCED", 0, &AdvancedOptionsDialogProc, 0);
        if (nDlgResult == 0) {
            if (g_pApp != NULL) {
                delete g_pApp;
            }
            g_pApp = NULL;
            return 0;
        }
    }

    if (g_pApp->Init(hInstance, "Gruntz", "Gruntz", lpCmdLine, 0, CW_USEDEFAULT, CW_USEDEFAULT)
        == 0) {
        if (g_pApp != NULL) {
            delete g_pApp;
        }
        g_pApp = NULL;
        return 0;
    }

    i32 rc = g_pApp->RunMessageLoop();
    if (g_pApp != NULL) {
        delete g_pApp;
    }
    g_pApp = NULL;
    return rc;
}
