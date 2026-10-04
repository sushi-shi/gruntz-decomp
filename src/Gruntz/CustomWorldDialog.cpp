#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/CustomWorldDialog.h>

#include <Enums.h>
#include <Gruntz/CustomWorldInfoDlg.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/PortalPath.h>
#include <Gruntz/Utils.h>
#include <Gruntz/WaitCursorScope.h>
#include <Ints.h>
#include <MsgParam.h>
#include <Net/NetLobby.h>
#include <Wwd/WwdFile.h>

#include <direct.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windowsx.h>


std::string g_pathStr;

std::string g_levelStr;

std::string g_selectedCustomWorldName;

CDDrawSurfaceMgr* g_customWorldSurfaceMgr = NULL;

HWND g_customWorldParent = NULL;

HINSTANCE g_customWorldInst = NULL;

HWND g_customLevelList = NULL;

char g_dotDot[] = "..";

char g_customGlob[] = "*.WWD";

std::string RunCustomWorldDialog(HWND parent) {
    (g_pathStr).erase();
    HWND v = parent;
    if (parent == NULL) {
        v = g_gameReg->m_gameWnd->GetHwnd();
    }
    CDDrawSurfaceMgr* world = g_gameReg->World();
    g_customWorldParent = v;
    g_customWorldSurfaceMgr = world;

    g_customWorldInst = g_gameReg->m_owner->m_hInstance;
    i32 accepted = g_gameReg->RunModalDialog("CUSTOM_WORLD", CustomWorldDlgProc, false);
    if (accepted == 0) {
        (g_pathStr).erase();
    }
    g_customWorldSurfaceMgr = NULL;
    g_customWorldParent = NULL;
    g_customWorldInst = NULL;
    return g_pathStr;
}

BOOL CALLBACK CustomWorldDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    NetLobby::g_curDlg = hDlg;
    switch (msg) {
        case WM_INITDIALOG:
            g_customLevelList = GetDlgItem(hDlg, CTRL_CUSTOM_WORLD_LIST);
            if (g_customLevelList) {
                FillCustomLevelList(hDlg);
            }
            return true;
        case WM_COMMAND:
            if (wParam == IDCANCEL) {
                EndDialog(hDlg, 0);
                return true;
            }
            if (wParam == CTRL_CUSTOM_WORLD_INFO) {
                LoadCustomWorldInfo(hDlg);
                return true;
            }
            if (wParam == IDOK) {
                LoadCustomWorldSelection(hDlg);
                EndDialog(hDlg, 1);
                return true;
            }
            MsgParam listWnd;
            listWnd.m_hwnd = g_customLevelList;
            if (g_customLevelList != NULL && lParam == listWnd.m_lparam) {
                if (HIWORD(wParam) == LBN_SELCHANGE) {
                    FillLevelInfoDialog(hDlg);
                    return true;
                }
                if (HIWORD(wParam) == LBN_DBLCLK) {
                    PostMessageA(hDlg, WM_COMMAND, IDOK, 0);
                    return true;
                }
            }
            break;
    }
    return false;
}

i32 FillCustomLevelList(HWND hWnd) {
    HWND hList = GetDlgItem(hWnd, CTRL_CUSTOM_WORLD_LIST);
    if (!hList) {
        return 0;
    }
    ListBox_ResetContent(hList);
    if (_chdir("Custom")) {
        return 0;
    }
    char pattern[256];
    strcpy(pattern, g_customGlob);
    _finddata_t fd;
    i32 hFile = _findfirst(pattern, &fd);
    b32 bContinue = (hFile != -1);
    CWaitCursorScope wait;
    while (bContinue) {
        char disp[256];
        sprintf(disp, "%s", fd.name);
        if (!g_gameReg->IsBattlezMapFile(std::string(disp))) {
            i32 len = strlen(disp);
            if (len > 4) {
                disp[len - 4] = 0;
            }
            MsgParam name;
            name.m_str = disp;
            ListBox_AddString(hList, name.m_lparam);
        }
        if (_findnext(hFile, &fd) == -1) {
            bContinue = false;
        }
    }
    _chdir(g_dotDot);
    return 1;
}

i32 FillLevelInfoDialog(HWND hDlg) {
    if (!GetDlgItem(hDlg, 0x3fc)) {
        return 0;
    }
    if (!LoadCustomWorldSelection(hDlg)) {
        return 0;
    }
    char num[0x20];
    WwdHeader info;
    BOOL(WINAPI * setText)(HWND, int, LPCSTR) = SetDlgItemTextA;
    if (g_gameReg->World()->m_level->IsValidWwd((g_pathStr).c_str(), &info)) {
        char* p = info.m_levelName;
        while (*p && (*p < '0' || *p > '9')) {
            p++;
        }
        sprintf(num, "%i", atoi(p));
        setText(hDlg, 0x408, (g_selectedCustomWorldName).c_str());
        setText(hDlg, 0x428, info.m_author);
        setText(hDlg, 0x40c, num);
        setText(hDlg, 0x429, info.m_created);
    } else {
        setText(hDlg, 0x408, "Bad Level File");
        setText(hDlg, 0x428, "Bad Level File");
        setText(hDlg, 0x40c, "Bad Level File");
        setText(hDlg, 0x429, "Bad Level File");
    }
    return 1;
}

i32 LoadCustomWorldSelection(HWND hWnd) {
    char itemText[256];
    char dirBuf[256];
    HWND lb = GetDlgItem(hWnd, 0x3fc);
    if (!lb) {
        return 0;
    }
    i32 sel = ListBox_GetCurSel(lb);
    if (sel == LB_ERR) {
        return 0;
    }
    MsgParam out;
    out.m_str = itemText;
    if (ListBox_GetText(lb, sel, out.m_lparam) == LB_ERR) {
        return 0;
    }
    if (!_getcwd(dirBuf, 0xfe)) {
        return 0;
    }
    g_pathStr = dirBuf;
    g_pathStr += "\\Custom\\";
    g_pathStr += itemText;
    g_pathStr += ".WWD";
    if (!FileExists((g_pathStr).c_str())) {
        (g_pathStr).erase();
        return 0;
    }
    g_selectedCustomWorldName = itemText;
    return 1;
}

i32 WwdFile::ValidateMainBlock(const std::string& name) {
    if (name.empty()) {
        return -1;
    }

    CGameLevel* lvl = g_gameReg->World()->m_level;
    if (lvl == NULL) {
        return -1;
    }

    std::string headerName;
    if (!lvl->ReadWwdHeaderName(name, headerName)) {
        return -1;
    }

    const std::string::size_type digit = headerName.find_first_of("0123456789");
    return digit == std::string::npos ? 0 : atoi(headerName.c_str() + digit);
}

BOOL CALLBACK CustomWorldInfoDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG: {
            WwdHeader info;
            char num[0x20];
            i32 bad = 1;
            if (g_customWorldSurfaceMgr != NULL && FileExists((g_pathStr).c_str())
                && g_customWorldSurfaceMgr->m_level
                       ->IsValidWwd((g_pathStr).c_str(), &info)) {
                SetDlgItemTextA(hDlg, 0x408, (g_levelStr).c_str());
                SetDlgItemTextA(hDlg, 0x428, info.m_author);
                char* p = info.m_levelName;
                while (*p && (*p < '0' || *p > '9')) {
                    p++;
                }
                sprintf(num, "%i", atoi(p));
                SetDlgItemTextA(hDlg, 0x40c, num);
                SetDlgItemTextA(hDlg, 0x429, info.m_created);
                bad = 0;
            }
            if (bad) {
                SetDlgItemTextA(hDlg, 0x408, "Bad Level File");
                SetDlgItemTextA(hDlg, 0x428, "Bad Level File");
                SetDlgItemTextA(hDlg, 0x40c, "Bad Level File");
                SetDlgItemTextA(hDlg, 0x429, "Bad Level File");
            }
            return true;
        }
        case WM_COMMAND:
            if (wParam == IDOK) {
                EndDialog(hDlg, 1);
                return true;
            }
            break;
    }
    return false;
}

i32 LoadCustomWorldInfo(HWND hDlg) {
    char szLevel[0x100];
    char szDir[0x100];

    HWND hList = GetDlgItem(hDlg, 0x3fc);
    if (!hList) {
        return 0;
    }
    i32 sel = ListBox_GetCurSel(hList);
    if (sel == LB_ERR) {
        return 0;
    }
    MsgParam out;
    out.m_str = szLevel;
    if (ListBox_GetText(hList, sel, out.m_lparam) == LB_ERR) {
        return 0;
    }
    g_levelStr = szLevel;
    if (!_getcwd(szDir, 0xfe)) {
        return 0;
    }
    g_pathStr = szDir;
    g_pathStr += "\\Custom\\";
    g_pathStr += szLevel;
    g_pathStr += ".WWD";
    if (!FileExists((g_pathStr).c_str())) {
        (g_pathStr).erase();
        return 0;
    }
    DialogBoxA(g_customWorldInst, "CUSTOM_WORLDINFO", g_customWorldParent, CustomWorldInfoDlgProc);
    return 1;
}

std::string BuildCustomWwdPath(std::string name) {
    if ((name).empty()) {
        return name;
    }
    if (strstr((name).c_str(), "\\") != NULL) {
        return name;
    }
    char cwd[254];
    if (_getcwd(cwd, 254) == NULL) {
        return name;
    }
    std::string orig = name;
    name = cwd;
    name += "\\CUSTOM\\";
    name += orig;
    std::transform((name).begin(), (name).end(), (name).begin(), asciiUpper);
    if (strstr((name).c_str(), ".WWD") == NULL) {
        name += ".WWD";
    }
    return name;
}

std::string WwdFile::GetMapBaseName(const std::string& path) {
    if (path.size() <= 4) return path;
    const std::string stem = path.substr(0, path.size() - 4);
    const std::string::size_type separator = stem.find_last_of('\\');
    return separator == std::string::npos ? stem : stem.substr(separator + 1);
}
