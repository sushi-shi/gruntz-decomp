#include <rva.h>

#include <Gruntz/CustomWorldDialog.h>

#include <Mfc.h>
#include <MfcNoInline.h>
#include <MfcWin.h>

#include <Enums.h>
#include <Gruntz/CustomWorldInfoDlg.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/PortalPath.h>
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

DATA(0x0022cf68)
char g_mapNameBuf[0x200] = {0};
RVA_DYNINIT(0x0003ab30, 0xa, g_pathStr)
RVA_DYNINIT(0x0003ab50, 0xa, g_pathStr)
RVA_DYNINIT(0x0003ab70, 0xe, g_pathStr)
RVA_DYNINIT(0x0003ab90, 0xa, g_pathStr)
DATA(0x0022d1b4)
CString g_pathStr;
RVA_DYNINIT(0x0003ac30, 0xa, g_levelStr)
RVA_DYNINIT(0x0003ac50, 0xa, g_levelStr)
RVA_DYNINIT(0x0003ac70, 0xe, g_levelStr)
RVA_DYNINIT(0x0003ac90, 0xa, g_levelStr)
DATA(0x0022d1b8)
CString g_levelStr;
RVA_DYNINIT(0x0003abb0, 0xa, g_selectedCustomWorldName)
RVA_DYNINIT(0x0003abd0, 0xa, g_selectedCustomWorldName)
RVA_DYNINIT(0x0003abf0, 0xe, g_selectedCustomWorldName)
RVA_DYNINIT(0x0003ac10, 0xa, g_selectedCustomWorldName)
DATA(0x0022d1bc)
CString g_selectedCustomWorldName;
DATA(0x0022d1c0)
CDDrawSurfaceMgr* g_customWorldSurfaceMgr = NULL;
DATA(0x0022d1c4)
HWND g_customWorldParent = NULL;
DATA(0x0022d1c8)
HINSTANCE g_customWorldInst = NULL;
DATA(0x0022d1cc)
HWND g_customLevelList = NULL;

DATA(0x0020df38)
char g_dotDot[] = "..";
DATA(0x0020df3c)
char g_customGlob[] = "*.WWD";

// @early-stop
RVA(0x0003acb0, 0x97)
CString RunCustomWorldDialog(HWND parent, CString* outSource) {
    g_pathStr.Empty();
    HWND v = parent;
    if (parent == NULL) {
        v = g_gameReg->m_gameWnd->m_hwnd;
    }
    g_customWorldParent = v;
    g_customWorldSurfaceMgr = g_gameReg->m_world;

    g_customWorldInst = g_gameReg->m_owner->m_hInstance;
    if (g_gameReg->RunModalDialog("CUSTOM_WORLD", CustomWorldDlgProc, false) == 0) {
        g_pathStr.Empty();
    }
    g_customWorldSurfaceMgr = NULL;
    g_customWorldParent = NULL;
    g_customWorldInst = NULL;
    if (outSource != NULL) {
        *outSource = g_selectedCustomWorldName;
    }
    return g_pathStr;
}

RVA(0x0003ad80, 0xec)
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

RVA(0x0003aeb0, 0x194)
i32 FillCustomLevelList(HWND hWnd) {
    HWND lb = GetDlgItem(hWnd, CTRL_CUSTOM_WORLD_LIST);
    if (!lb) {
        return 0;
    }
    SendMessageA(lb, LB_RESETCONTENT, 0, 0);
    if (_chdir("Custom")) {
        return 0;
    }
    char pattern[256];
    strcpy(pattern, g_customGlob);
    _finddata_t fd;
    i32 h = _findfirst(pattern, &fd);
    b32 found = (h != -1);
    CWaitCursorScope wait;
    while (found) {
        char disp[256];
        sprintf(disp, "%s", fd.name);
        if (!g_gameReg->IsBattlezMapFile(CString(disp))) {
            i32 len = strlen(disp);
            if (len > 4) {
                disp[len - 4] = 0;
            }
            MsgParam name;
            name.m_str = disp;
            SendMessageA(lb, LB_ADDSTRING, 0, name.m_lparam);
        }
        if (_findnext(h, &fd) == -1) {
            found = false;
        }
    }
    _chdir(g_dotDot);
    return 1;
}

RVA(0x0003b0c0, 0x118)
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
    if (g_gameReg->m_world->m_level->IsValidWwd(static_cast<const char*>(g_pathStr), &info)) {
        char* p = info.levelName;
        while (*p && (*p < '0' || *p > '9')) {
            p++;
        }
        sprintf(num, "%i", atoi(p));
        setText(hDlg, 0x408, static_cast<const char*>(g_selectedCustomWorldName));
        setText(hDlg, 0x428, info.author);
        setText(hDlg, 0x40c, num);
        setText(hDlg, 0x429, info.created);
    } else {
        setText(hDlg, 0x408, "Bad Level File");
        setText(hDlg, 0x428, "Bad Level File");
        setText(hDlg, 0x40c, "Bad Level File");
        setText(hDlg, 0x429, "Bad Level File");
    }
    return 1;
}

RVA(0x0003b230, 0x10d)
i32 LoadCustomWorldSelection(HWND hWnd) {
    char itemText[256];
    char dirBuf[256];
    HWND lb = GetDlgItem(hWnd, 0x3fc);
    if (!lb) {
        return 0;
    }
    i32 sel = SendMessageA(lb, LB_GETCURSEL, 0, 0);
    if (sel == -1) {
        return 0;
    }
    MsgParam out;
    out.m_str = itemText;
    if (SendMessageA(lb, LB_GETTEXT, sel, out.m_lparam) == -1) {
        return 0;
    }
    if (!_getcwd(dirBuf, 0xfe)) {
        return 0;
    }
    g_pathStr = dirBuf;
    g_pathStr += "\\Custom\\";
    g_pathStr += itemText;
    g_pathStr += ".WWD";
    if (!FileExists(g_pathStr)) {
        g_pathStr.Empty();
        return 0;
    }
    g_selectedCustomWorldName = itemText;
    return 1;
}

RVA(0x0003b390, 0x13a)
i32 WwdFile::ValidateMainBlock(CString name) {
    char header[0x100];

    if (name.GetLength() == 0) {
        return -1;
    }

    CGameLevel* lvl = g_gameReg->m_world->m_level;
    if (lvl == NULL) {
        return -1;
    }

    if (!lvl->ReadWwdHeaderName(name, header)) {
        return -1;
    }

    char* p = header;
    char c = *p;
    while (c != 0 && (c < '0' || c > '9')) {
        c = *++p;
    }
    return atoi(p);
}

RVA(0x0003b520, 0x15f)
BOOL CALLBACK CustomWorldInfoDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG: {
            WwdHeader info;
            char num[0x20];
            i32 bad = 1;
            if (g_customWorldSurfaceMgr != NULL && FileExists(g_pathStr)
                && g_customWorldSurfaceMgr->m_level
                       ->IsValidWwd(static_cast<const char*>(g_pathStr), &info)) {
                SetDlgItemTextA(hDlg, 0x408, static_cast<const char*>(g_levelStr));
                SetDlgItemTextA(hDlg, 0x428, info.author);
                char* p = info.levelName;
                while (*p && (*p < '0' || *p > '9')) {
                    p++;
                }
                sprintf(num, "%i", atoi(p));
                SetDlgItemTextA(hDlg, 0x40c, num);
                SetDlgItemTextA(hDlg, 0x429, info.created);
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

RVA(0x0003b6e0, 0x12c)
i32 LoadCustomWorldInfo(HWND hDlg) {
    char szLevel[0x100];
    char szDir[0x100];

    HWND hList = GetDlgItem(hDlg, 0x3fc);
    if (!hList) {
        return 0;
    }
    i32 sel = static_cast<i32>(SendMessageA(hList, LB_GETCURSEL, 0, 0));
    if (sel == -1) {
        return 0;
    }
    MsgParam out;
    out.m_str = szLevel;
    if (static_cast<i32>(SendMessageA(hList, LB_GETTEXT, sel, out.m_lparam)) == -1) {
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
    if (!FileExists(g_pathStr)) {
        g_pathStr.Empty();
        return 0;
    }
    DialogBoxParamA(
        g_customWorldInst,
        "CUSTOM_WORLDINFO",
        g_customWorldParent,
        CustomWorldInfoDlgProc,
        0
    );
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0003b860, 0x19d)
CString BuildCustomWwdPath(CString name) {
    if (name.GetLength() == 0) {
        return name;
    }
    if (strstr(name, "\\") != NULL) {
        return name;
    }
    char cwd[254];
    if (_getcwd(cwd, 254) == NULL) {
        return name;
    }
    CString orig = name;
    name = cwd;
    name += "\\CUSTOM\\";
    name += orig;
    name.MakeUpper();
    if (strstr(name, ".WWD") == NULL) {
        name += ".WWD";
    }
    return name;
}

// @early-stop
RVA(0x0003ba70, 0x128)
CString WwdFile::GetMapBaseName(CString path) {
    CString result = path;
    i32 len = path.GetLength();
    if (len == 0) {
        return result;
    }
    if (len <= 4) {
        return result;
    }
    strcpy(g_mapNameBuf, path);
    i32 blen = strlen(g_mapNameBuf);
    if (blen >= 5) {
        g_mapNameBuf[blen - 4] = 0;
        i32 blen2 = strlen(g_mapNameBuf);
        if (blen2 >= 1) {
            i32 i = blen2 - 1;
            while (i >= 0) {
                if (g_mapNameBuf[i] == '\\') {
                    break;
                }
                i--;
            }
            result = &g_mapNameBuf[i + 1];
        }
    }
    return result;
}
