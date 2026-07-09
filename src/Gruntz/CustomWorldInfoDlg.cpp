// CustomWorldInfoDlg.cpp - the "CUSTOM_WORLDINFO" custom-world picker dialog helper
// (C:\Proj\Gruntz), re-homed from src/Stub/Backlog.cpp. Sibling of the CUSTOM_WORLD
// launcher in CustomWorldDialog.cpp (shares the CustomWorldInfoDlgProc + the
// g_pathStr/g_levelStr exchange globals). __cdecl, frameless; helpers are reloc-masked
// externals. Only offsets / code bytes are load-bearing.
#include <Mfc.h> // afx-first windows.h (WwdFile.h pulls MFC; superset of Win32.h dialog API)

#include <Gruntz/CustomWorldInfoDlg.h> // WwdWorldHolder / WwdLevelInfoSrc (the info-popup views)
#include <Gruntz/GameKeyStr.h>         // canonical GameKeyStr (g_pathStr/g_levelStr builders)
#include <Wwd/WwdFile.h>               // WwdHeader (the validated 0x5f4-byte .WWD header)
#include <rva.h>
#include <stdio.h>  // sprintf (inline CRT, reloc-masked)
#include <stdlib.h> // atoi (0x11ffb0, reloc-masked)

// The game world holder RunCustomWorldDialog stashes (g_dat62c268 @0x62c268, a DWORD
// pointer); its +0x24 level-info source validates the .WWD. Owner is CustomWorldDialog.cpp.
extern i32 g_dat62c268;

// The global key-string builders are the canonical GameKeyStr (<Gruntz/GameKeyStr.h>,
// included below); the same builder BuildPowerupIconKeys uses.
DATA(0x0022c25c)
extern GameKeyStr g_pathStr; // 0x62c25c  full path builder
DATA(0x0022c260)
extern GameKeyStr g_levelStr; // 0x62c260  level name

// FreeLevelStr @0x03ad30 - the atexit/teardown thunk that tears down the global
// g_levelStr key-string (GameKeyStr::Free1b9b93 == the inner CString ~ctor, 0x1b9b93,
// reloc-masked). Re-homed from src/Stub/BoundaryLowerThunks.cpp (was StrFree3ad30).
RVA(0x0003ad30, 0xa)
void FreeLevelStr() {
    g_levelStr.Free1b9b93();
}
DATA(0x0022c26c)
extern HWND g_customWorldParent; // 0x62c26c
DATA(0x0022c270)
extern HINSTANCE g_customWorldInst; // 0x62c270
// FUN_0011fc10 __cdecl: writes the gruntz game dir into buf (ret nonzero on ok).
extern i32 GetGameDir(char* buf, i32 cb);
// FUN_00004282 __cdecl: tests a path exists (OpenFile probe). ret nonzero on ok.
extern i32 PathFileExists(char* path);
// ===========================================================================
// CustomWorldInfoDlgProc @0x03b600 - the CUSTOM_WORLDINFO level-info popup proc.
// ===========================================================================
// WM_INITDIALOG validates the selected .WWD (the g_pathStr full path exists AND the
// world's level-info source parses its header); on success it fills the four info
// items - the level name (0x408 = g_levelStr), the header's author/paths sub-strings
// (0x428/0x429 = the +0x50/+0x90 slices of the name block), and a numeric field
// (0x40c) parsed out of the +0x10 version string; on any failure every item reads
// "Bad Level File". WM_COMMAND ends the dialog on OK (1).
RVA(0x0003b600, 0x15f)
INT_PTR CALLBACK CustomWorldInfoDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case 0x110: { // WM_INITDIALOG
            WwdHeader info;
            char num[0x20];
            i32 bad = 1;
            if (g_dat62c268 != 0 && PathFileExists(g_pathStr.m_str)
                && ((WwdWorldHolder*)g_dat62c268)->m_24->IsValidWwd(g_pathStr.m_str, &info)) {
                SetDlgItemTextA(hDlg, 0x408, g_levelStr.m_str);
                SetDlgItemTextA(hDlg, 0x428, info.levelName + 0x40);
                char* p = info.levelName;
                while (*p && (*p < '0' || *p > '9')) {
                    p++;
                }
                sprintf(num, "%d", atoi(p));
                SetDlgItemTextA(hDlg, 0x40c, num);
                SetDlgItemTextA(hDlg, 0x429, info.levelName + 0x80);
                bad = 0;
            }
            if (bad) {
                SetDlgItemTextA(hDlg, 0x408, "Bad Level File");
                SetDlgItemTextA(hDlg, 0x428, "Bad Level File");
                SetDlgItemTextA(hDlg, 0x40c, "Bad Level File");
                SetDlgItemTextA(hDlg, 0x429, "Bad Level File");
            }
            return 1;
        }
        case 0x111: // WM_COMMAND
            if (wParam == 1) {
                EndDialog(hDlg, 1);
                return 1;
            }
            break;
    }
    return 0;
}

// LoadCustomWorldInfo (0x3b7c0) - reads the level name from the dialog's listbox
// (id 0x3fc), builds "<gameDir>\Custom\<level>.WWD" through the global key builders,
// and if the file exists pops the CUSTOM_WORLDINFO dialog.
RVA(0x0003b7c0, 0x12c)
i32 LoadCustomWorldInfo(HWND hDlg) {
    char szLevel[0x100];
    char szDir[0x100];

    HWND hList = GetDlgItem(hDlg, 0x3fc);
    if (!hList) {
        return 0;
    }
    i32 sel = (i32)SendMessageA(hList, 0x188 /*LB_GETCURSEL*/, 0, 0);
    if (sel == -1) {
        return 0;
    }
    if ((i32)SendMessageA(hList, 0x189 /*LB_GETTEXT*/, sel, (LPARAM)szLevel) == -1) {
        return 0;
    }
    g_levelStr.Set(szLevel);
    if (!GetGameDir(szDir, 0xfe)) {
        return 0;
    }
    g_pathStr.Set(szDir);
    g_pathStr.Append("\\Custom\\");
    g_pathStr.Append(szLevel);
    g_pathStr.Append(".WWD");
    if (!PathFileExists(g_pathStr.m_str)) {
        g_pathStr.Reset();
        return 0;
    }
    DialogBoxParamA(
        g_customWorldInst,
        "CUSTOM_WORLDINFO",
        g_customWorldParent,
        (DLGPROC)CustomWorldInfoDlgProc,
        0
    );
    return 1;
}
